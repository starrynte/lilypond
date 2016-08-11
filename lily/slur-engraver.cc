/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 1997--2015 Han-Wen Nienhuys <hanwen@xs4all.nl>

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

#include "context.hh"
#include "directional-element-interface.hh"
#include "note-column.hh"
#include "pointer-group-interface.hh"
#include "slur-engraver.hh"
#include "slur.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "std-vector.hh"
#include "warn.hh"

#include "translator.icc"

SCM
Slur_engraver::event_symbol () const
{
  return ly_symbol2scm ("slur-event");
}

bool
Slur_engraver::double_property () const
{
  return to_boolean (get_property ("doubleSlurs"));
}

SCM
Slur_engraver::grob_symbol () const
{
  return ly_symbol2scm ("Slur");
}

const char *
Slur_engraver::object_name () const
{
  return "slur";
}

Slur_engraver::Slur_engraver ()
{
}

void
Slur_engraver::set_melisma (bool m)
{
  context ()->set_property ("slurMelismaBusy", ly_bool2scm (m));
}

void
Slur_engraver::boot ()
{
  ADD_SPANNER_FILTERED_LISTENER (Slur_engraver, slur);
  ADD_SPANNER_LISTENER (Slur_engraver, note);
  ADD_SPANNER_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, inline_accidental);
  ADD_SPANNER_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, fingering);
  ADD_SPANNER_ACKNOWLEDGER (Slur_engraver, note_column);
  ADD_SPANNER_ACKNOWLEDGER (Slur_engraver, script);
  ADD_SPANNER_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, text_script);
  ADD_SPANNER_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, dots);
  ADD_SPANNER_END_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, tie);
  ADD_SPANNER_ACKNOWLEDGER_FOR (Slur_engraver, extra_object, tuplet_number);
}

ADD_TRANSLATOR (Slur_engraver,
                /* doc */
                "Build slur grobs from slur events.",

                /* create */
                "Slur ",

                /* read */
                "slurMelismaBusy "
                "doubleSlurs ",

                /* write */
                ""
               );

void
Slur_engraver::listen_note_slur (pair<Stream_event *, Stream_event *>)
{
//  listen_spanner_event_once (ev, note ? note->self_scm () : SCM_EOL, false);
}

void
Slur_engraver::listen_note (Stream_event *ev)
{
  for (SCM arts = ev->get_property ("articulations");
       scm_is_pair (arts); arts = scm_cdr (arts))
    {
      Stream_event *art = unsmob<Stream_event> (scm_car (arts));
      if (art->in_event_class (event_symbol ()))
        call_spanner_filtered<pair<Stream_event *, Stream_event *> >
        (get_share_context (ev->get_property ("spanner-share-context")),
         ev->get_property ("spanner-id"),
         &Slur_engraver::listen_note_slur, pair<Stream_event *, Stream_event *> (art, ev));
    }
}

void
Slur_engraver::acknowledge_note_column (Grob_info info)
{
  Grob *e = info.grob ();
  if (current_spanner_)
    Slur::add_column (current_spanner_, e);
  if (finished_spanner_)
    Slur::add_column (finished_spanner_, e);
  // Now cater for slurs starting/ending at a notehead: those override
  // the column bounds
  if (note_slurs_[START].empty () && note_slurs_[STOP].empty ())
    return;
  extract_grob_set (e, "note-heads", heads);
  for (vsize i = heads.size (); i--;)
    {
      if (Stream_event *ev =
          unsmob<Stream_event> (heads[i]->get_property ("cause")))
        for (LEFT_and_RIGHT (d))
          {
            std::pair<Note_slurs::const_iterator, Note_slurs::const_iterator> its
              = note_slurs_[d].equal_range (ev);
            for (Note_slurs::const_iterator it = its.first;
                 it != its.second;
                 ++it)
              it->second->set_bound (d, heads[i]);
          }
    }
}

void
Slur_engraver::acknowledge_extra_object (Grob_info info)
{
  objects_to_acknowledge_.push_back (info);
}

void
Slur_engraver::acknowledge_script (Grob_info info)
{
  if (!info.grob ()->internal_has_interface (ly_symbol2scm ("dynamic-interface")))
    acknowledge_extra_object (info);
}

void
Slur_engraver::stop_event_callback (Stream_event *ev, SCM note, Spanner *slur)
{
  end_spanner (slur, ev->self_scm (), ev);
  if (!scm_is_null (note))
    note_slurs_[STOP].insert
    (Note_slurs::value_type (unsmob<Stream_event> (note), slur));
}

void
Slur_engraver::create_slur (Stream_event *ev, SCM note, Direction dir)
{
  Context *share = get_share_context (ev->get_property ("spanner-share-context"));
  SCM id = ev->get_property ("spanner-id");
  Spanner *slur = make_multi_spanner (grob_symbol (), ev->self_scm (), share, id);
  if (dir)
    set_grob_direction (slur, dir);
  if (!scm_is_null (note))
    note_slurs_[START].insert
    (Note_slurs::value_type (unsmob<Stream_event> (note), slur));
}

void
Slur_engraver::start_event_callback (Stream_event *ev, SCM note)
{
  if (double_property ())
    {
      create_slur (ev, note, UP);
      create_slur (ev, note, DOWN);
    }
  else
    create_slur (ev, note, to_dir (ev->get_property ("direction")));
}

void
Slur_engraver::process_music ()
{
  process_stop_events ();

  process_start_events ();

  set_melisma (current_spanners_.size ());
}

void
Slur_engraver::stop_translation_timestep ()
{
  if (Grob *g = unsmob<Grob> (get_property ("currentCommandColumn")))
    {
      if (finished_spanner_)
        Slur::add_extra_encompass (finished_spanner_, g);

      if (!start_events_.size ())
        {
          if (current_spanner_)
            Slur::add_extra_encompass (current_spanner_, g);
        }
    }

  if (finished_spanner_)
    {
      if (!finished_spanner_->get_bound (RIGHT))
        finished_spanner_
          ->set_bound (RIGHT, unsmob<Grob> (get_property ("currentMusicalColumn")));
    }

  for (vsize i = 0; i < objects_to_acknowledge_.size (); i++)
    Slur::auxiliary_acknowledge_extra_object (objects_to_acknowledge_[i],
                                              current_spanner_,
                                              finished_spanner_);

  note_slurs_[LEFT].clear ();
  note_slurs_[RIGHT].clear ();
  objects_to_acknowledge_.clear ();
}
