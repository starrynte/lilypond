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
  vector<Grob_info> started_;
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
  debug_output ("dynamicline ended " + ly_scm2string (scm_object_to_string (id, SCM_UNDEFINED)) + " " + share->context_name ());
  if (scm_is_pair (entry))
    {
      debug_output ("entry exists");
      SCM other = get_cv_entry_other (entry);
      if (!scm_is_null (other) && unsmob<Spanner> (other) == dynamic)
        {
          debug_output ("other matches");
          set_cv_entry_context (share, id, entry);
          set_cv_entry_other (share, id, entry, SCM_EOL);
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
  debug_output ("something happening?");
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
      debug_output ("dynamicline started " + ly_scm2string (scm_object_to_string (id, SCM_UNDEFINED)) + " " + share->context_name ());
      // ...
      bool reuse = false;
      SCM entry = get_cv_entry (share, id);
      if (scm_is_pair (entry))
        {
          SCM other = get_cv_entry_other (entry);
          if (scm_is_null (other))
            {
              // We can reuse this DynamicLineSpanner unless the direction conflicts
              Direction line_dir = get_grob_direction (my_cv_spanners_[i]);
              Direction grob_dir = to_dir (
                cause->get_property ("direction"));

              // If we have an explicit direction for the new dynamic grob
              // that differs from the current line spanner, break it
              reuse = !grob_dir || line_dir == grob_dir;
            }
          else
            programming_error ("Simultaneous dynamics with same spanner-id");
        }

      SCM dynamic_scm;
      Item *left_bound, *right_bound;
      if (has_interface<Spanner> (dynamic))
        {
          Spanner *dynamic_span = started_[i].spanner ();
          dynamic_scm = dynamic_span->self_scm ();
          left_bound = dynamic_span->get_bound (LEFT);
          right_bound = dynamic_span->get_bound (RIGHT);
        }
      else
        {
          dynamic_scm = SCM_EOL;
          left_bound = right_bound = started_[i].item ();
        }

      Spanner *span;
      if (reuse)
        {
          span = get_cv_entry_spanner (entry);
          set_cv_entry_other (share, id, entry, dynamic_scm);
        }
      else
        {
          span = make_spanner ("DynamicLineSpanner", cause->self_scm ());
          create_cv_entry (share, id, span, cause, dynamic_scm);
        }
      debug_output (string ("reuse is ") + (reuse ? "TRUE" : "FALSE"));

      span->set_bound (LEFT, left_bound);
      span->set_bound (RIGHT, right_bound);

      Axis_group_interface::add_element (span, dynamic);
      if (Direction d = to_dir (cause->get_property ("direction")))
        set_grob_direction (span, d);
    }
}

void
Dynamic_align_engraver::stop_translation_timestep ()
{
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

  started_.clear ();
  support_.clear ();
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
