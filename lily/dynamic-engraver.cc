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

#include "context.hh"
#include "engraver.hh"
#include "hairpin.hh"
#include "international.hh"
#include "item.hh"
#include "note-column.hh"
#include "pointer-group-interface.hh"
#include "self-alignment-interface.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "stream-event.hh"
#include "text-interface.hh"

#include <map>
#include <vector>

#include "translator.icc"

class Dynamic_engraver : public Engraver
{
  TRANSLATOR_DECLARATIONS (Dynamic_engraver);
  void acknowledge_note_column (Grob_info);
  void listen_absolute_dynamic (Stream_event *);
  void listen_span_dynamic (Stream_event *);
  void listen_break_span (Stream_event *);

protected:
  virtual void process_music ();
  virtual void stop_translation_timestep ();
  virtual void finalize ();

private:
  SCM get_property_setting (Stream_event *evt, char const *evprop,
                            char const *ctxprop);
  string get_spanner_type (Stream_event *ev);

  Drul_array<Stream_event *> accepted_spanevents_drul_;
  vector<Stream_event *> named_start_events_;
  vector<Stream_event *> named_stop_events_;
  Spanner *current_spanner_;
  vector<Spanner *> finished_spanners_;

  Item *script_;
  Stream_event *script_event_;
  Stream_event *current_span_event_;
  // Map from spanner-id to value
  map<string, bool> end_new_spanner_;

  CV_SPANNER_DECLARATIONS ();
};

Dynamic_engraver::Dynamic_engraver ()
{
  script_event_ = 0;
  current_span_event_ = 0;
  script_ = 0;
  current_spanner_ = 0;
  accepted_spanevents_drul_.set (0, 0);
}

void
Dynamic_engraver::listen_absolute_dynamic (Stream_event *ev)
{
  ASSIGN_EVENT_ONCE (script_event_, ev);
}

void
Dynamic_engraver::listen_span_dynamic (Stream_event *ev)
{
  Direction d = to_dir (ev->get_property ("span-direction"));
  SCM id = ev->get_property ("spanner-id");
  debug_output (string("received: ") + (d == START ? "START" : "STOP") + ", id " + robust_scm2string (id, ""));

  if (scm_is_string (id))
    {
      if (d == START)
        named_start_events_.push_back (ev);
      else if (d == STOP)
        named_stop_events_.push_back (ev);
    }
  else
    {
      ASSIGN_EVENT_ONCE (accepted_spanevents_drul_[d], ev);
    }
}

void
Dynamic_engraver::listen_break_span (Stream_event *event)
{
  if (event->in_event_class ("break-dynamic-span-event"))
    {
      // Case 1: Already have a start dynamic event -> break applies to new
      //         spanner (created later) -> set a flag
      // Case 2: no new spanner, but spanner already active -> break it now
      // Only break cross voice spanners if event has matching id
      SCM id = event->get_property ("spanner-id");
      end_new_spanner_[robust_scm2string (id, "")] = true;

      if (scm_is_string (id))
        {
          SCM entry = GET_CV_ENTRY (id);
          if (scm_is_pair (entry))
            {
              SET_CV_ENTRY_CONTEXT (id, entry, context ());
              Spanner *span = GET_CV_ENTRY_SPANNER (entry);
              span->set_property ("spanner-broken", SCM_BOOL_T);
            }
        }
      else if (current_spanner_)
        {
          current_spanner_->set_property ("spanner-broken", SCM_BOOL_T);
        }
    }
}

SCM
Dynamic_engraver::get_property_setting (Stream_event *evt,
                                        char const *evprop,
                                        char const *ctxprop)
{
  SCM spanner_type = evt->get_property (evprop);
  if (scm_is_null (spanner_type))
    spanner_type = get_property (ctxprop);
  return spanner_type;
}

void
Dynamic_engraver::process_music ()
{
  UPDATE_MY_CV_SPANNERS ();

  if (current_spanner_
      && (accepted_spanevents_drul_[STOP]
          || script_event_
          || accepted_spanevents_drul_[START]))
    {
      Stream_event *ender = accepted_spanevents_drul_[STOP];
      if (!ender)
        ender = script_event_;

      if (!ender)
        ender = accepted_spanevents_drul_[START];

      finished_spanners_.push_back (current_spanner_);
      announce_end_grob (current_spanner_, ender->self_scm ());
      current_spanner_ = 0;
      current_span_event_ = 0;
    }
  for (size_t i = 0; i < named_stop_events_.size (); i++)
    {
      Stream_event *ender = named_stop_events_[i];
      SCM ender_id = ender->get_property ("spanner-id");
      SCM entry = GET_CV_ENTRY (ender_id);
      if (scm_is_string (ender_id) && scm_is_pair (entry))
        {
          // We got a STOP event for the spanner, so end it
          Spanner *spanner = GET_CV_ENTRY_SPANNER (entry);
          finished_spanners_.push_back (spanner);
          debug_output ("announcing cv spanner");
          announce_end_grob (spanner, ender->self_scm ());
          DELETE_CV_ENTRY (ender_id);
        }
    }

  vector<Stream_event *> start_events = named_start_events_;
  if (accepted_spanevents_drul_[START])
    start_events.push_back (accepted_spanevents_drul_[START]);
  vector<Spanner *> start_spanners;
  for (size_t i = 0; i < start_events.size(); i++)
    {
      Stream_event *ev = start_events[i];
      SCM id = ev->get_property ("spanner-id");
      string id_string = robust_scm2string (id, "");
      // If this is the within-voice spanner
      if (!scm_is_string (id))
        current_span_event_ = accepted_spanevents_drul_[START];
      Spanner *spanner;

      string start_type = get_spanner_type (ev);
      SCM cresc_type = get_property_setting (ev, "span-type",
                                             (start_type + "Spanner").c_str ());

      if (scm_is_eq (cresc_type, ly_symbol2scm ("text")))
        {
          spanner
            = make_spanner ("DynamicTextSpanner",
                            accepted_spanevents_drul_[START]->self_scm ());

          SCM text = get_property_setting (ev, "span-text",
                                           (start_type + "Text").c_str ());
          if (Text_interface::is_markup (text))
            spanner->set_property ("text", text);
          /*
            If the line of a text spanner is hidden, end the alignment spanner
            early: this allows dynamics to be spaced individually instead of
            being linked together.
          */
          if (scm_is_eq (spanner->get_property ("style"),
                         ly_symbol2scm ("none")))
            spanner->set_property ("spanner-broken", SCM_BOOL_T);
        }
      else
        {
          if (!scm_is_eq (cresc_type, ly_symbol2scm ("hairpin")))
            {
              string as_string = ly_scm_write_string (cresc_type);
              ev
              ->origin ()->warning (_f ("unknown crescendo style: %s\ndefaulting to hairpin.", as_string.c_str ()));
            }
          spanner = make_spanner ("Hairpin", ev->self_scm ());
        }
      // if we have a break-dynamic-span event right after the start dynamic, break the new spanner immediately
      if (end_new_spanner_[id_string])
        {
          spanner->set_property ("spanner-broken", SCM_BOOL_T);
          end_new_spanner_[id_string] = false;
        }
      for (size_t j = 0; j < finished_spanners_.size (); j++)
        {
          Spanner *finished = finished_spanners_[j];
          if (has_interface<Hairpin> (finished))
            Pointer_group_interface::add_grob (finished,
                                               ly_symbol2scm ("adjacent-spanners"),
                                               spanner);
          if (has_interface<Hairpin> (spanner))
            Pointer_group_interface::add_grob (spanner,
                                               ly_symbol2scm ("adjacent-spanners"),
                                               finished);
        }

      start_spanners.push_back (spanner);
      if (!scm_is_string (id))
        {
          current_spanner_ = spanner;
        }
      else
        {
          // Add spanner to sharedSpanners
          if (scm_is_pair (GET_CV_ENTRY (id)))
            {
              // spanner with this id already exists, warning?
            }
          CREATE_CV_ENTRY (id, context (), spanner);
        }
    }

  if (script_event_)
    {
      script_ = make_item ("DynamicText", script_event_->self_scm ());
      script_->set_property ("text",
                             script_event_->get_property ("text"));

      for (size_t i = 0; i < finished_spanners_.size (); i++)
        finished_spanners_[i]->set_bound (RIGHT, script_);
      for (size_t i = 0; i < start_spanners.size (); i++)
        start_spanners[i]->set_bound (LEFT, script_);
    }
}

void
Dynamic_engraver::stop_translation_timestep ()
{
  for (size_t i = 0; i < finished_spanners_.size (); i++)
    {
      if (!finished_spanners_[i]->get_bound (RIGHT))
        finished_spanners_[i]
        ->set_bound (RIGHT,
                     unsmob<Grob> (get_property ("currentMusicalColumn")));
    }

  if (current_spanner_ && !current_spanner_->get_bound (LEFT))
    current_spanner_
    ->set_bound (LEFT,
                 unsmob<Grob> (get_property ("currentMusicalColumn")));

  for (size_t i = 0; i < my_cv_spanners_.size(); i++)
    {
        if (!my_cv_spanners_[i]->get_bound (LEFT))
          my_cv_spanners_[i]->set_bound (LEFT, unsmob<Grob> (get_property ("currentMusicalColumn")));
    }

  script_ = 0;
  script_event_ = 0;
  accepted_spanevents_drul_.set (0, 0);
  named_start_events_.clear ();
  named_stop_events_.clear ();
  finished_spanners_.clear ();
  end_new_spanner_.clear ();
}

void
Dynamic_engraver::finalize ()
{
  if (current_spanner_
      && !current_spanner_->is_live ())
    current_spanner_ = 0;
  if (current_spanner_)
    {
      current_span_event_
      ->origin ()->warning (_f ("unterminated %s",
                                get_spanner_type (current_span_event_)
                                .c_str ()));
      current_spanner_->suicide ();
      current_spanner_ = 0;
    }
}

string
Dynamic_engraver::get_spanner_type (Stream_event *ev)
{
  string type;
  SCM start_sym = scm_car (ev->get_property ("class"));

  if (scm_is_eq (start_sym, ly_symbol2scm ("decrescendo-event")))
    type = "decrescendo";
  else if (scm_is_eq (start_sym, ly_symbol2scm ("crescendo-event")))
    type = "crescendo";
  else
    programming_error ("unknown dynamic spanner type");

  return type;
}

void
Dynamic_engraver::acknowledge_note_column (Grob_info info)
{
  if (script_ && !script_->get_parent (X_AXIS))
    {
      extract_grob_set (info.grob (), "note-heads", heads);
      /*
        Spacing constraints may require dynamics to be attached to rests,
        so check for a rest if this note column has no note heads.
      */
      Grob *x_parent = (heads.size ()
                        ? info.grob ()
                        : unsmob<Grob> (info.grob ()->get_object ("rest")));
      if (x_parent)
        script_->set_parent (x_parent, X_AXIS);
    }

  if (current_spanner_ && !current_spanner_->get_bound (LEFT))
    current_spanner_->set_bound (LEFT, info.grob ());
  for (size_t i = 0; i < my_cv_spanners_.size (); i++)
    {
      if (!my_cv_spanners_[i]->get_bound (LEFT))
        my_cv_spanners_[i]->set_bound (LEFT, info.grob ());
    }
  for (size_t i = 0; i < finished_spanners_.size (); i++)
    {
      if (!finished_spanners_[i]->get_bound (RIGHT))
        finished_spanners_[i]->set_bound (RIGHT, info.grob ());
    }
}

void
Dynamic_engraver::boot ()
{
  ADD_LISTENER (Dynamic_engraver, absolute_dynamic);
  ADD_LISTENER (Dynamic_engraver, span_dynamic);
  ADD_LISTENER (Dynamic_engraver, break_span);
  ADD_ACKNOWLEDGER (Dynamic_engraver, note_column);
}

ADD_TRANSLATOR (Dynamic_engraver,
                /* doc */
                "Create hairpins, dynamic texts and dynamic text spanners.",

                /* create */
                "DynamicTextSpanner "
                "DynamicText "
                "Hairpin ",

                /* read */
                "crescendoSpanner "
                "crescendoText "
                "currentMusicalColumn "
                "decrescendoSpanner "
                "decrescendoText ",

                /* write */
                ""
               );
