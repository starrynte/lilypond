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
#include "stream-event.hh"
#include "text-interface.hh"

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
  Spanner *current_spanner_;
  Spanner *finished_spanner_;

  Item *script_;
  Stream_event *script_event_;
  Stream_event *current_span_event_;
  bool end_new_spanner_;

  // alist: ((<spanner-id string> . (('current-voice . voice) ('spanner . spanner))) () ())
  static SCM cvspanners_;
};
SCM Dynamic_engraver::cvspanners_ = SCM_EOL;

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
}

void
Dynamic_engraver::listen_span_dynamic (Stream_event *ev)
{
  Direction d = to_dir (ev->get_property ("span-direction"));
  string id = robust_scm2string (ev->get_property ("spanner-id"), "");
  debug_output (string("received: ") + (d == START ? "START" : "STOP") + ", id " + id);

  ASSIGN_EVENT_ONCE (accepted_spanevents_drul_[d], ev);
}

void
Dynamic_engraver::listen_break_span (Stream_event *event)
{
  if (event->in_event_class ("break-dynamic-span-event"))
    {
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

      finished_spanner_ = current_spanner_;
      announce_end_grob (finished_spanner_, ender->self_scm ());
      current_spanner_ = 0;
      current_span_event_ = 0;
    }
  else if (accepted_spanevents_drul_[STOP])
    {
      debug_output ("checking cross voice");
      Stream_event *ender = accepted_spanevents_drul_[STOP];
      SCM ender_id = ender->get_property ("spanner-id");
      assert (scm_is_string (ender_id));
      for (SCM s = cvspanners_; scm_is_pair (s); s = scm_cdr (s))
        {
	  SCM entry = scm_car (s);
	  SCM entry_id = scm_car (entry);
	  bool id_equal = scm_string_eq (entry_id, ender_id, SCM_UNDEFINED, SCM_UNDEFINED, SCM_UNDEFINED, SCM_UNDEFINED);
	  debug_output ("checking spanner with id " + ly_scm2string(entry_id));
	  SCM entry_props = scm_cdr (entry);
//	  SCM entry_voice = scm_assoc_ref (entry_props, ly_symbol2scm ("current-voice"));
//	  bool voice_equal = ly_is_equal (context ()->self_scm (), entry_voice);
	  if (id_equal)
	    {
	      // For now, just change the voice when we see the stop event
	      entry_props = scm_assoc_set_x (entry_props, ly_symbol2scm ("current-voice"), context ()->self_scm ());
	      SCM span = scm_assoc_ref (entry_props, ly_symbol2scm ("spanner"));
	      // We got a STOP event for the spanner, so end it
	      // TODO
	    }
	}
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
            = make_spanner ("DynamicTextSpanner",
                            accepted_spanevents_drul_[START]->self_scm ());

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
          current_spanner_ = make_spanner ("Hairpin",
                                           current_span_event_->self_scm ());
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

      // Add spanner to cvspanners_ in case it ends in another voice
      SCM spanner_id = current_span_event_->get_property ("spanner-id");
      string spanner_id_str = robust_scm2string (spanner_id, "");
      if (spanner_id_str.size() > 0)
        {
	  SCM cvprops = SCM_EOL;
	  cvprops = scm_acons (ly_symbol2scm ("current-voice"), context ()->self_scm (), cvprops);
	  cvprops = scm_acons (ly_symbol2scm ("spanner"), current_spanner_->self_scm (), cvprops);
	  // check if exists?
	  cvspanners_ = scm_acons (spanner_id, cvprops, cvspanners_);
	}
    }

  if (script_event_)
    {
      script_ = make_item ("DynamicText", script_event_->self_scm ());
      script_->set_property ("text",
                             script_event_->get_property ("text"));

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
