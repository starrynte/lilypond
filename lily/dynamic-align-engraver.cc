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

#include <map>

#include "translator.icc"

// Current approach: DynamicLineSpanner only cares about
// dynamics with matching id
class Dynamic_align_engraver : public Spanner_engraver
{
  TRANSLATOR_DECLARATIONS (Dynamic_align_engraver);
  void acknowledge_rhythmic_head (Grob_info);
  void acknowledge_stem (Grob_info);
  void acknowledge_dynamic (Grob_info);
  void acknowledge_footnote_spanner (Grob_info);
  void acknowledge_end_dynamic (Grob_info);

protected:
  virtual void process_acknowledged ();
  virtual void stop_translation_timestep ();

private:
  void set_spanner_bound (Spanner *span, Direction d, Grob_info info);
  vector<Grob_info> started_;
  map<Spanner *, Grob_info> left_bounds_;
  map<Spanner *, Grob_info> right_bounds_;
  vector<Grob *> footnotes_;
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
  SCM id = dynamic->get_property ("spanner-id");
  Context *share = get_share_context (
    dynamic->get_property ("spanner-share-context"));
  SCM entry = get_cv_entry (share, id);
  if (scm_is_pair (entry))
    {
      SCM other = get_cv_entry_other (entry);
      if (!scm_is_null (other) && unsmob<Spanner> (other) == dynamic)
        {
          set_cv_entry_context (share, id, entry);
          set_cv_entry_other (share, id, entry, SCM_EOL);
          right_bounds_[get_cv_entry_spanner (entry)] = info;
          return;
        }
    }
  programming_error ("lost track of this dynamic spanner");
}

void
Dynamic_align_engraver::acknowledge_footnote_spanner (Grob_info info)
{
  Grob *grob = info.grob ();
  Grob *parent = grob->get_parent (Y_AXIS);

  if (parent
      && parent->internal_has_interface (ly_symbol2scm ("dynamic-interface")))
    {
      // TODO
      footnotes_.push_back (grob);
    }
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
  update_my_cv_spanners ();

  for (vsize i = 0; i < started_.size (); i++)
    {
      Grob *dynamic = started_[i].grob ();
      Stream_event *cause = started_[i].event_cause ();
      assert (cause);

      SCM id = dynamic->get_property ("spanner-id");
      Context *share = get_share_context (dynamic->get_property ("spanner-share-context"));
      // ...
      bool reuse = false;
      SCM entry = get_cv_entry (share, id);
      if (scm_is_pair (entry))
        {
          SCM other = get_cv_entry_other (entry);
          if (scm_is_null (other))
            {
              // We can reuse this DynamicLineSpanner unless the direction conflicts
              Direction line_dir = get_grob_direction (get_cv_entry_spanner (entry));
              Direction grob_dir = to_dir (
                cause->get_property ("direction"));

              // If we have an explicit direction for the new dynamic grob
              // that differs from the current line spanner, break it
              reuse = !grob_dir || line_dir == grob_dir;
            }
          else
            programming_error ("Simultaneous dynamics with same spanner-id");
        }

      SCM dynamic_scm = has_interface<Spanner> (dynamic) ?
        started_[i].spanner ()->self_scm () :
        SCM_EOL;

      Spanner *span;
      if (reuse)
        {
          span = get_cv_entry_spanner (entry);
          set_cv_entry_other (share, id, entry, dynamic_scm);
        }
      else
        {
          span = make_spanner ("DynamicLineSpanner", cause->self_scm ());
          span->set_property ("spanner-id", id);
          span->set_property ("spanner-share-context", share->context_name_symbol ());
          left_bounds_[span] = started_[i];
          create_cv_entry (share, id, span, cause, dynamic_scm);
        }

      Axis_group_interface::add_element (span, dynamic);
      if (Direction d = to_dir (cause->get_property ("direction")))
        set_grob_direction (span, d);
    }

  started_.clear ();
}

void
Dynamic_align_engraver::stop_translation_timestep ()
{
  for (map<Spanner *, Grob_info>::iterator it = left_bounds_.begin ();
      it != left_bounds_.end (); it++)
    set_spanner_bound (it->first, LEFT, it->second);

  for (map<Spanner *, Grob_info>::iterator it = right_bounds_.begin ();
      it != right_bounds_.end (); it++)
    set_spanner_bound (it->first, RIGHT, it->second);

  for (vsize i = 0; i < my_cv_spanners_.size (); i++)
    {
  // If the flag is set to break the spanner after the current child, don't
  // add any more support points (needed e.g. for style=none, where the
  // invisible spanner should NOT be shifted since we don't have a line).
      bool spanner_broken = false;
      SCM other = my_cv_spanners_other_[i];
      if (!scm_is_null (other))
        {
          Spanner *dynamic = unsmob<Spanner> (other);
          spanner_broken = to_boolean (dynamic->get_property ("spanner-broken"));
        }
      for (vsize j = 0; !spanner_broken && j < support_.size (); j++)
        Side_position_interface::add_support (my_cv_spanners_[i], support_[j]);

      if (scm_is_null (other))
        {
          SCM id = my_cv_spanners_[i]->get_property ("spanner-id");
          Context *share = get_share_context (my_cv_spanners_[i]->get_property ("spanner-share-context"));
          delete_cv_entry (share, id);
        }
    }

  support_.clear ();
  left_bounds_.clear ();
  right_bounds_.clear ();
}

void
Dynamic_align_engraver::set_spanner_bound (Spanner *span, Direction d,
                                           Grob_info info)
{
  Item *bound = has_interface<Spanner> (info.grob ()) ?
    info.spanner ()->get_bound (d) :
    info.item ();
  span->set_bound (d, bound);
}

void
Dynamic_align_engraver::boot ()
{
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, dynamic);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, rhythmic_head);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, stem);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, footnote_spanner);
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
