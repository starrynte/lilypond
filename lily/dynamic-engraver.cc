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

#include "translator.icc"
#include <sstream>

class Dynamic_engraver : public Spanner_engraver
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
  Spanner *finished_spanner_;

  Item *script_;
  Stream_event *script_event_;
  Stream_event *current_span_event_;
  bool end_new_spanner_;
};

Dynamic_engraver::Dynamic_engraver ()
{
  script_event_ = 0;
  current_span_event_ = 0;
  script_ = 0;
  finished_spanner_ = 0;
  current_spanner_ = 0;
  accepted_spanevents_drul_.set (0, 0);
  end_new_spanner_ = false;
}

void
Dynamic_engraver::listen_absolute_dynamic (Stream_event *ev)
{
  ASSIGN_EVENT_ONCE (script_event_, ev);
  take_spanner (ev->get_property ("spanner-share-context"), ev->get_property ("spanner-id"));
}

void
Dynamic_engraver::listen_span_dynamic (Stream_event *ev)
{
  Direction d = to_dir (ev->get_property ("span-direction"));

  ASSIGN_EVENT_ONCE (accepted_spanevents_drul_[d], ev);
  take_spanner (ev->get_property ("spanner-share-context"), ev->get_property ("spanner-id"));
}

void
Dynamic_engraver::listen_break_span (Stream_event *ev)
{
  if (ev->in_event_class ("break-dynamic-span-event"))
    {
      take_spanner (ev->get_property ("spanner-share-context"), ev->get_property ("spanner-id"));
      // Case 1: Already have a start dynamic event -> break applies to new
      //         spanner (created later) -> set a flag
      // Case 2: no new spanner, but spanner already active -> break it now
      if (accepted_spanevents_drul_[START])
        end_new_spanner_ = true;
      else if (current_spanner_)
        current_spanner_->set_property ("spanner-broken", SCM_BOOL_T);
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
  debug_output (string ("current span? ") + (current_spanner_ ? "YES" : "NO"));
  debug_output (string ("start event? ") + (accepted_spanevents_drul_[START] ? "YES" : "NO"));
  debug_output (string ("stop event? ") + (accepted_spanevents_drul_[STOP] ? "YES" : "NO"));
  debug_output (string ("script event? ") + (script_event_ ? "YES" : "NO"));
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

      take_spanner (ender->get_property ("spanner-share-context"), ender->get_property ("spanner-id"));
      finished_spanner_ = current_spanner_;
      end_spanner (current_spanner_, ender->self_scm ());
      current_spanner_ = 0;
      current_span_event_ = 0;
    }

  if (accepted_spanevents_drul_[START])
    {
      current_span_event_ = accepted_spanevents_drul_[START];

      string start_type = get_spanner_type (current_span_event_);
      SCM cresc_type = get_property_setting (current_span_event_, "span-type",
                                             (start_type + "Spanner").c_str ());

      if (scm_is_eq (cresc_type, ly_symbol2scm ("text")))
        {
          current_spanner_
            = make_multi_spanner ("DynamicTextSpanner",
                                  accepted_spanevents_drul_[START]->self_scm (),
                                  accepted_spanevents_drul_[START]->get_property ("spanner-share-context"),
                                  accepted_spanevents_drul_[START]->get_property ("spanner-id"));

          SCM text = get_property_setting (current_span_event_, "span-text",
                                           (start_type + "Text").c_str ());
          if (Text_interface::is_markup (text))
            current_spanner_->set_property ("text", text);
          /*
            If the line of a text spanner is hidden, end the alignment spanner
            early: this allows dynamics to be spaced individually instead of
            being linked together.
          */
          if (scm_is_eq (current_spanner_->get_property ("style"),
                         ly_symbol2scm ("none")))
            current_spanner_->set_property ("spanner-broken", SCM_BOOL_T);
        }
      else
        {
          if (!scm_is_eq (cresc_type, ly_symbol2scm ("hairpin")))
            {
              string as_string = ly_scm_write_string (cresc_type);
              current_span_event_
              ->origin ()->warning (_f ("unknown crescendo style: %s\ndefaulting to hairpin.", as_string.c_str ()));
            }
          current_spanner_
            = make_multi_spanner ("Hairpin",
                                  accepted_spanevents_drul_[START]->self_scm (),
                                  accepted_spanevents_drul_[START]->get_property ("spanner-share-context"),
                                  accepted_spanevents_drul_[START]->get_property ("spanner-id"));
          ostringstream oss;
          oss << "made new spanner " << current_spanner_;
          debug_output (oss.str ());
        }
      // if we have a break-dynamic-span event right after the start dynamic, break the new spanner immediately
      if (end_new_spanner_)
        {
          current_spanner_->set_property ("spanner-broken", SCM_BOOL_T);
          end_new_spanner_ = false;
        }
      if (finished_spanner_)
        {
          if (has_interface<Hairpin> (finished_spanner_))
            Pointer_group_interface::add_grob (finished_spanner_,
                                               ly_symbol2scm ("adjacent-spanners"),
                                               current_spanner_);
          if (has_interface<Hairpin> (current_spanner_))
            Pointer_group_interface::add_grob (current_spanner_,
                                               ly_symbol2scm ("adjacent-spanners"),
                                               finished_spanner_);
        }
    }

  if (script_event_)
    {
      script_ = make_item ("DynamicText", script_event_->self_scm ());
      script_->set_property ("text",
                             script_event_->get_property ("text"));
      script_->set_property ("spanner-id", script_event_->get_property ("spanner-id"));
      script_->set_property ("spanner-share-context", script_event_->get_property ("spanner-share-context"));

      if (finished_spanner_)
        finished_spanner_->set_bound (RIGHT, script_);
      if (current_spanner_)
        current_spanner_->set_bound (LEFT, script_);
    }
}

void
Dynamic_engraver::stop_translation_timestep ()
{
  if (finished_spanner_ && !finished_spanner_->get_bound (RIGHT))
    finished_spanner_
    ->set_bound (RIGHT,
                 unsmob<Grob> (get_property ("currentMusicalColumn")));

  if (current_spanner_ && !current_spanner_->get_bound (LEFT))
    current_spanner_
    ->set_bound (LEFT,
                 unsmob<Grob> (get_property ("currentMusicalColumn")));
  script_ = 0;
  script_event_ = 0;
  accepted_spanevents_drul_.set (0, 0);
  finished_spanner_ = 0;
  end_new_spanner_ = false;
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
  if (finished_spanner_ && !finished_spanner_->get_bound (RIGHT))
    finished_spanner_->set_bound (RIGHT, info.grob ());
}

void
Dynamic_engraver::boot ()
{
  ADD_FILTERED_LISTENER (Dynamic_engraver, absolute_dynamic);
  ADD_FILTERED_LISTENER (Dynamic_engraver, span_dynamic);
  ADD_FILTERED_LISTENER (Dynamic_engraver, break_span);
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
