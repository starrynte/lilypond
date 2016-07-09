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
#include "directional-element-interface.hh"
#include "item.hh"
#include "side-position-interface.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "stream-event.hh"

#include "translator.icc"

class Dynamic_align_engraver : public Spanner_engraver
{
  TRANSLATOR_DECLARATIONS (Dynamic_align_engraver);
  void acknowledge_rhythmic_head (Grob_info);
  void acknowledge_stem (Grob_info);
  void acknowledge_dynamic (Grob_info);
  void acknowledge_footnote_spanner (Grob_info);
  void acknowledge_end_dynamic (Grob_info);

protected:
  virtual void stop_translation_timestep ();

private:
  void set_spanner_bounds (Spanner *line, bool end);
  vector<Spanner *> started_spanners_;
  // Spanner manually broken, don't use it for new grobs
  vector<Spanner *> finished_spanners_;
  vector<Spanner *> ended_;
  vector<Spanner *> started_;
  vector<Grob *> scripts_;
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

  Spanner *dynamic_span = info.spanner ();
  ended_.push_back (dynamic_span);

  /* If the break flag is set, store the current spanner and let new dynamics
   * create a new spanner
   */
  if (to_boolean (dynamic_span->get_property ("spanner-broken")))
    {
      SCM id = dynamic_span->get_property ("spanner-id");
      Context *share = get_share_context (
        dynamic_span->get_property ("spanner-share-context"));
      SCM entry = get_cv_entry (share, id);

      if (scm_is_pair (entry))
        {
          // Verify the dynamic matches
          Spanner *start_dynamic = unsmob<Spanner> (get_cv_entry_other (entry));
          if (dynamic_span == start_dynamic)
            {
              debug_output ("ended_spanners; delete");
              Spanner *span = get_cv_entry_spanner (entry);
              finished_spanners_.push_back (span);
              delete_cv_entry (share, id);
            }
          else
            debug_output ("y u no match");
        }
    }
}

void
Dynamic_align_engraver::acknowledge_footnote_spanner (Grob_info info)
{
  Grob *grob = info.grob ();
  Grob *parent = grob->get_parent (Y_AXIS);

  if (parent
      && parent->internal_has_interface (ly_symbol2scm ("dynamic-interface")))
    {
      for (vsize i = 0; i < my_cv_spanners_.size (); i++)
        Axis_group_interface::add_element (my_cv_spanners_[i], grob);
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
  Grob *dynamic = info.grob ();
  SCM id = dynamic->get_property ("spanner-id");
  Context *share = get_share_context (
      dynamic->get_property ("spanner-share-context"));
  SCM entry = get_cv_entry (share, id);
  Spanner *span = 0;
  if (scm_is_pair (entry))
  {
    Spanner *start_dynamic = unsmob<Spanner> (get_cv_entry_other (entry));
    if (dynamic == start_dynamic)
      span = get_cv_entry_spanner (entry);
    else debug_output ("y u no match??");
  }

  Stream_event *cause = info.event_cause ();
  // Check whether an existing line spanner has the same direction
  if (cause && span)
    {
      Direction line_dir = get_grob_direction (span);
      Direction grob_dir = to_dir (cause->get_property ("direction"));

      // If we have an explicit direction for the new dynamic grob
      // that differs from the current line spanner, break the spanner
      if (grob_dir && (line_dir != grob_dir))
        {
          finished_spanners_.push_back (span);
          span = 0;
          delete_cv_entry (share, id);
        }
    }

  if (!span)
    span = make_spanner ("DynamicLineSpanner", dynamic->self_scm ());
  started_spanners_.push_back (span);

  if (has_interface<Spanner> (dynamic))
    {
      Spanner *dynamic_span = info.spanner ();
      started_.push_back (dynamic_span);
      create_cv_entry (share, id, span, cause, dynamic_span->self_scm ());
    }
  else if (info.item ())
    scripts_.push_back (info.item ());
  else
    dynamic->programming_error ("unknown dynamic grob");

  Axis_group_interface::add_element (span, dynamic);

  if (cause)
    {
      if (Direction d = to_dir (cause->get_property ("direction")))
        set_grob_direction (span, d);
    }
}

void
Dynamic_align_engraver::set_spanner_bounds (Spanner *line, bool end)
{
  if (!line)
    return;

  for (LEFT_and_RIGHT (d))
    {
      if ((d == LEFT && !line->get_bound (LEFT))
          || (end && d == RIGHT && !line->get_bound (RIGHT)))
        {
          vector<Spanner *> const &spanners
            = (d == LEFT) ? started_ : ended_;

          Grob *bound = 0;
          if (scripts_.size ())
            bound = scripts_[0];
          else if (spanners.size ())
            bound = spanners[0]->get_bound (d);
          else
            {
              bound = unsmob<Grob> (get_property ("currentMusicalColumn"));
              programming_error ("started DynamicLineSpanner but have no left bound");
            }

          line->set_bound (d, bound);
        }
    }
}

void
Dynamic_align_engraver::stop_translation_timestep ()
{
//  if (ended_)
//    {
//      set<Spanner *>::iterator it = running_.find (ended_);
//      if (it != running_.end ())
//        running_.erase (it);
//      else
//        started_->programming_error ("lost track of this dynamic spanner");
//    }

  // Set the proper bounds for the current spanner and for a spanner that
  // is ended now
  for (vsize i = 0; i < finished_spanners_.size (); i++)
    {
      set_spanner_bounds (finished_spanners_[i], true);
    }

  for (vsize i = 0; i < my_cv_spanners_.size (); i++)
    {
      bool end = true;
      for (vsize j = 0; j < started_spanners_.size (); j++)
        {
          if (started_spanners_[j] == my_cv_spanners_[i])
            {
              end = false;
              break;
            }
        }
      set_spanner_bounds (started_spanners_[i], end);

  // If the flag is set to break the spanner after the current child, don't
  // add any more support points (needed e.g. for style=none, where the
  // invisible spanner should NOT be shifted since we don't have a line).
  bool spanner_broken = false;
//                        && to_boolean (current_dynamic_spanner_->get_property ("spanner-broken"));
  for (vsize j = 0; !spanner_broken && j < support_.size (); j++)
    Side_position_interface::add_support (my_cv_spanners_[i], support_[j]);

  if (end)
    {
      SCM id = my_cv_spanners_[i]->get_property ("spanner-id");
      Context *share = get_share_context (my_cv_spanners_[i]->get_property ("spanner-share-context"));
      delete_cv_entry (share, id);
    }
    }

  started_spanners_.clear ();
  finished_spanners_.clear ();
  ended_.clear ();
  started_.clear ();
  scripts_.clear ();
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
