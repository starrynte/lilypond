/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2008--2015 Han-Wen Nienhuys <hanwen@lilypond.org>


  LilyPond is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  LilyPond is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "engraver.hh"

#include "axis-group-interface.hh"
#include "context.hh"
#include "directional-element-interface.hh"
#include "item.hh"
#include "side-position-interface.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "stream-event.hh"

#include <utility>

#include "translator.icc"

// Current approach to cross voice dynamics: dynamics immediately adjacent with
// the same spanner-id/spanner-share-context will be connected in a line
// The "other" field in the Spanner_engraver entries keeps track of the last
// unended dynamic spanner in each DynamicLineSpanner
// If it is null at the end of a step, that line has no more dynamics and ends
class Dynamic_align_engraver : public Spanner_engraver
{
  TRANSLATOR_DECLARATIONS (Dynamic_align_engraver);
  void acknowledge_rhythmic_head (Grob_info);
  void acknowledge_stem (Grob_info);
  void acknowledge_dynamic (Grob_info);
  void acknowledge_end_dynamic (Grob_info);

protected:
  virtual void process_acknowledged ();
  virtual void stop_translation_timestep ();

private:
  void set_spanner_bound (Spanner *span, Direction d, Grob_info info);
  // All dynamics acknowledged by acknowledge_dynamic: scripts then spanners
  vector<Grob_info> started_;
  // DynamicLineSpanners whose bounds will be set at the end of each step
  vector< pair<Spanner *, Grob_info> > left_bounds_;
  vector< pair<Spanner *, Grob_info> > right_bounds_;
  vector<Grob *> support_;
};

Dynamic_align_engraver::Dynamic_align_engraver ()
{
}

void
Dynamic_align_engraver::acknowledge_end_dynamic (Grob_info info)
{
  if (!has_interface<Spanner> (info.grob ()))
    return;

  Spanner *dynamic = info.spanner ();
  Stream_event *cause = info.event_cause ();
  assert (cause);
  // Look for DynamicLineSpanner corresponding to the ended dynamic
  SCM id = dynamic->get_property ("spanner-id");
  Context *share = get_share_context
                   (cause->get_property ("spanner-share-context"));
  SCM entry = get_cv_entry (share, id);
  if (scm_is_vector (entry))
    {
      // The other field should have been set in the start acknowledgement
      SCM other = get_cv_entry_other (entry);
      if (!scm_is_null (other) && unsmob<Spanner> (other) == dynamic)
        {
          right_bounds_.push_back
          (pair<Spanner *, Grob_info> (get_cv_entry_spanner (entry), info));
          if (to_boolean (dynamic->get_property ("spanner-broken")))
            {
              // Stop using this spanner if broken
              delete_cv_entry (share, id);
            }
          else
            {
              set_cv_entry_context (share, id, entry);
              // This spanner no longer has an unended dynamic
              set_cv_entry_other (share, id, entry, SCM_EOL);
            }
          return;
        }
    }
  programming_error ("lost track of this dynamic spanner");
}

void
Dynamic_align_engraver::acknowledge_rhythmic_head (Grob_info info)
{
  support_.push_back (info.grob ());
}

void
Dynamic_align_engraver::acknowledge_stem (Grob_info info)
{
  support_.push_back (info.grob ());
}

void
Dynamic_align_engraver::acknowledge_dynamic (Grob_info info)
{
  if (has_interface<Spanner> (info.grob ()))
    started_.push_back (info);
  else if (info.item ())
    started_.insert (started_.begin (), info);
  else
    info.grob ()->programming_error ("unknown dynamic grob");
}

void
Dynamic_align_engraver::process_acknowledged ()
{
  for (vsize i = 0; i < started_.size (); i++)
    {
      Grob *dynamic = started_[i].grob ();
      Stream_event *cause = started_[i].event_cause ();
      assert (cause);

      // Get properties from the event, since dynamic script grobs won't have
      // spanner-id or spanner-share-context properties
      SCM id = cause->get_property ("spanner-id");
      Context *share = get_share_context
                       (cause->get_property ("spanner-share-context"));
      // Whether or not an existing DynamicLineSpanner with matching
      // id/share context can be extended with this dynamic
      bool reuse = false;
      // Check if such a line spanner already exists
      SCM entry = get_cv_entry (share, id);
      if (scm_is_vector (entry))
        {
          // Check if the line spanner isn't waiting for a dynamic to end
          SCM other = get_cv_entry_other (entry);
          if (scm_is_null (other))
            {
              // We can reuse this line spanner unless the direction conflicts
              Direction line_dir
                = get_grob_direction (get_cv_entry_spanner (entry));
              Direction grob_dir
                = to_dir (cause->get_property ("direction"));

              // If we have an explicit direction for the new dynamic grob
              // that differs from this line spanner, break it
              if (grob_dir && line_dir != grob_dir)
                delete_cv_entry (share, id);
              else
                reuse = true;
            }
          else
            programming_error ("Simultaneous dynamics with same spanner-id");
        }

      // The other field should be set to the new dynamic if it is a spanner
      // Otherwise, there is nothing to wait for, so it should be set to null
      SCM dynamic_scm = has_interface<Spanner> (dynamic)
                        ? started_[i].spanner ()->self_scm ()
                        : SCM_EOL;

      Spanner *span;
      if (reuse)
        {
          // Extend this line spanner to include the dynamic
          span = get_cv_entry_spanner (entry);
          set_cv_entry_other (share, id, entry, dynamic_scm);
        }
      else
        {
          // Make a new line spanner
          span = make_spanner ("DynamicLineSpanner", dynamic->self_scm ());
          span->set_property ("spanner-id", id);
          left_bounds_.push_back
          (pair<Spanner *, Grob_info> (span, started_[i]));
          create_cv_entry (share, id, span, "", dynamic_scm);
        }
      // If dynamic is a script, set the right bound
      if (!has_interface<Spanner> (dynamic))
        right_bounds_.push_back
        (pair<Spanner *, Grob_info> (span, started_[i]));

      Axis_group_interface::add_element (span, dynamic);
      if (Direction d = to_dir (cause->get_property ("direction")))
        set_grob_direction (span, d);
    }

  started_.clear ();
}

void
Dynamic_align_engraver::stop_translation_timestep ()
{
  for (vsize i = 0; i < left_bounds_.size (); i++)
    set_spanner_bound (left_bounds_[i].first, LEFT, left_bounds_[i].second);
  for (vsize i = 0; i < right_bounds_.size (); i++)
    set_spanner_bound (right_bounds_[i].first, RIGHT, right_bounds_[i].second);

  vector<cv_entry> entries = my_cv_entries ();
  for (vsize i = 0; i < entries.size (); i++)
    {
      bool spanner_broken = false;
      SCM entry = entries[i].first;
      Spanner *span = get_cv_entry_spanner (entry);
      SCM other = get_cv_entry_other (entry);
      if (!scm_is_null (other))
        {
          Spanner *dynamic = unsmob<Spanner> (other);
          spanner_broken = to_boolean
                           (dynamic->get_property ("spanner-broken"));
        }
      // If the flag is set to break the spanner after the current child, don't
      // add any more support points (needed e.g. for style=none, where the
      // invisible spanner should NOT be shifted since we don't have a line).
      for (vsize j = 0; !spanner_broken && j < support_.size (); j++)
        Side_position_interface::add_support (span, support_[j]);

      // If this spanner is not in the middle of a dynamic, end it
      if (scm_is_null (other))
        delete_cv_entry (entries[i].second, span->get_property ("spanner-id"));
    }

  support_.clear ();
  left_bounds_.clear ();
  right_bounds_.clear ();
}

void
Dynamic_align_engraver::set_spanner_bound (Spanner *span, Direction d,
                                           Grob_info info)
{
  Item *bound = has_interface<Spanner> (info.grob ())
                ? info.spanner ()->get_bound (d)
                : info.item ();
  span->set_bound (d, bound);
}

void
Dynamic_align_engraver::boot ()
{
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, dynamic);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, rhythmic_head);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, stem);
  ADD_END_ACKNOWLEDGER (Dynamic_align_engraver, dynamic);
}

ADD_TRANSLATOR (Dynamic_align_engraver,
                /* doc */
                "Align hairpins and dynamic texts on a horizontal line.",

                /* create */
                "DynamicLineSpanner ",

                /* read */
                "currentMusicalColumn ",

                /* write */
                ""
               );
