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

#include <set>

#include "engraver.hh"

#include "axis-group-interface.hh"
#include "directional-element-interface.hh"
#include "item.hh"
#include "side-position-interface.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "stream-event.hh"

#include "translator.icc"
#include <sstream>

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
  void create_line_spanner (Grob *cause);
  void take_spanner (Grob *ack_dynamic);
  void set_spanner_bounds (Spanner *line, bool end);
  Spanner *ended_line_; // Spanner manually broken, don't use it for new grobs
  // Dynamic that we're waiting to end
  Spanner *current_dynamic_spanner_;
  // Dynamic spanners/items that ended/started this step
  Spanner *ended_;
  Spanner *started_;
  Grob *script_;
  vector<Grob *> support_;
};

Dynamic_align_engraver::Dynamic_align_engraver ()
{
  ended_line_ = 0;
  current_dynamic_spanner_ = 0;
  ended_ = 0;
  started_ = 0;
  script_ = 0;
}

void
Dynamic_align_engraver::create_line_spanner (Grob *cause)
{
  if (!current_spanner_)
    {
      current_spanner_ = make_multi_spanner ("DynamicLineSpanner", cause->self_scm (),
                                             cause->get_property ("spanner-share-context"),
                                             cause->get_property ("spanner-id"));
      current_spanner_->set_property ("warn-unterminated", SCM_BOOL_F);
    }
}

void
Dynamic_align_engraver::acknowledge_end_dynamic (Grob_info info)
{
  debug_output ("ack_end");
  assert (has_interface<Spanner> (info.grob ()));
  take_spanner (info.grob ());

  Spanner *dynamic = info.spanner ();
  if (!ended_)
    ended_ = dynamic;
  else
    programming_error ("multiple dynamics ended on same timestep");

  // The current line could have been broken already by a dynamic with a
  // conflicting direction in acknowledge_dynamic
  if (!current_spanner_)
    return;

  debug_output (current_dynamic_spanner_ ? "cds wao" : "D:<");
  if (current_dynamic_spanner_ == dynamic)
    {
      /* If the break flag is set, store the current spanner and let new dynamics
       * create a new spanner
       */
      if (to_boolean (current_dynamic_spanner_->get_property ("spanner-broken")))
        {
          if (ended_line_)
            programming_error ("already have a force-ended DynamicLineSpanner.");
          ended_line_ = current_spanner_;
          debug_output ("break");
          end_spanner (current_spanner_, SCM_EOL);
          current_spanner_ = 0;
        }
      current_dynamic_spanner_ = 0;
    }
  else
    programming_error ("lost track of this dynamic spanner");
}

void
Dynamic_align_engraver::acknowledge_footnote_spanner (Grob_info info)
{
  Grob *parent = info.grob ()->get_parent (Y_AXIS);
  if (current_spanner_ && parent
      && parent->internal_has_interface (ly_symbol2scm ("dynamic-interface")))
    Axis_group_interface::add_element (current_spanner_, info.grob ());
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
  debug_output ("ack_dyn");
  Grob *dynamic = info.grob ();
  take_spanner (dynamic);

  Stream_event *cause = info.event_cause ();
  // Check whether an existing line spanner has the same direction
  if (current_spanner_ && cause)
    {
      Direction line_dir = get_grob_direction (current_spanner_);
      Direction grob_dir = to_dir (cause->get_property ("direction"));

      // If we have an explicit direction for the new dynamic grob
      // that differs from the current line spanner, break the spanner
      if (grob_dir && (line_dir != grob_dir))
        {
          if (!ended_line_)
            ended_line_ = current_spanner_;
          debug_output ("dir break");
          end_spanner (current_spanner_, SCM_EOL);
          current_spanner_ = 0;
          current_dynamic_spanner_ = 0;
        }
    }

  create_line_spanner (dynamic);
  if (has_interface<Spanner> (dynamic))
    {
      assert (!started_);
      started_ = info.spanner ();
      ostringstream oss;
      oss << "setting started " << this;
      debug_output (oss.str ());
      current_dynamic_spanner_ = info.spanner ();
    }
  else if (info.item ())
    {
      assert (!script_);
      script_ = info.item ();
    }
  else
    dynamic->programming_error ("unknown dynamic grob");

  Axis_group_interface::add_element (current_spanner_, dynamic);

  if (cause)
    {
      if (Direction d = to_dir (cause->get_property ("direction")))
        set_grob_direction (current_spanner_, d);
    }
}

void
Dynamic_align_engraver::take_spanner (Grob *dynamic)
{
  Dynamic_align_engraver *owner
    = static_cast<Dynamic_align_engraver *> (Spanner_engraver::take_spanner (dynamic->get_property ("spanner-share-context"), dynamic->get_property ("spanner-id")));
  if (owner && owner != this)
    {
      // TODO check
      current_dynamic_spanner_ = owner->current_dynamic_spanner_;
      owner->current_dynamic_spanner_ = 0;
      ended_ = owner->ended_;
      owner->ended_ = 0;
      script_ = owner->script_;
      owner->script_ = 0;
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
          Spanner *spanner
            = (d == LEFT) ? started_ : ended_;

          Grob *bound = 0;
          if (script_)
            bound = script_;
          else if (spanner)
            bound = spanner->get_bound (d);
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
  debug_output ("stop timestep");
  bool end = current_spanner_ && !current_dynamic_spanner_;

  // Set the proper bounds for the current spanner and for a spanner that
  // is ended now
  set_spanner_bounds (ended_line_, true);
  set_spanner_bounds (current_spanner_, end);
  // If the flag is set to break the spanner after the current child, don't
  // add any more support points (needed e.g. for style=none, where the
  // invisible spanner should NOT be shifted since we don't have a line).
  bool spanner_broken = current_dynamic_spanner_
                        && to_boolean (current_dynamic_spanner_->get_property ("spanner-broken"));
  for (vsize i = 0; current_spanner_ && !spanner_broken && i < support_.size (); i++)
    Side_position_interface::add_support (current_spanner_, support_[i]);

  if (end)
    {
      debug_output ("end = true");
      end_spanner (current_spanner_, SCM_EOL);
      current_spanner_ = 0;
    }

  ended_line_ = 0;
  ended_ = 0;
  started_ = 0;
  script_ = 0;
  support_.clear ();
}

void
Dynamic_align_engraver::boot ()
{
  ADD_FILTERED_ACKNOWLEDGER (Dynamic_align_engraver, dynamic);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, rhythmic_head);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, stem);
  ADD_ACKNOWLEDGER (Dynamic_align_engraver, footnote_spanner);
  ADD_FILTERED_END_ACKNOWLEDGER (Dynamic_align_engraver, dynamic);
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
