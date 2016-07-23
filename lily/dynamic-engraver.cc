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
#include "std-vector.hh"
#include "stream-event.hh"
#include "text-interface.hh"

#include "translator.icc"

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

private:
  SCM get_property_setting (Stream_event *evt, char const *evprop,
                            char const *ctxprop);
  string get_spanner_type (Stream_event *ev);

  vector<Stream_event *> start_events_;
  vector<Stream_event *> stop_events_;
  vector<Spanner *> finished_spanners_;

  Item *script_;
  Stream_event *script_event_;
  // alist: (spanner-id . boolean)
  SCM end_new_spanner_;
};

Dynamic_engraver::Dynamic_engraver ()
{
  script_event_ = 0;
  script_ = 0;
  end_new_spanner_ = SCM_EOL;
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

  vector<Stream_event *> &events = (d == STOP) ? stop_events_ : start_events_;
  for (vsize i = 0; i < events.size (); i++)
    {
      if (ly_is_equal (events[i]->get_property ("spanner-id"), id))
        {
          ev->origin ()->warning ("Ignoring duplicate span dynamic event");
          return;
        }
    }
  events.push_back (ev);
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
      for (vsize i = 0; i < start_events_.size (); i++)
        {
          if (ly_is_equal (start_events_[i]->get_property ("spanner-id"), id))
            {
              end_new_spanner_ = scm_assoc_set_x
                                 (end_new_spanner_, id, SCM_BOOL_T);
              return;
            }
        }

      Context *share = get_share_context
                       (event->get_property ("spanner-share-context"));
      SCM entry = get_cv_entry (share, id);
      if (scm_is_vector (entry))
        {
          set_cv_entry_context (share, id, entry);
          Spanner *span = get_cv_entry_spanner (entry);
          span->set_property ("spanner-broken", SCM_BOOL_T);
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
  // Check for ended spanners
  // All events that end: stop events, start events, script event
  vector<Stream_event *> enders = stop_events_;
  enders.insert (enders.end (), start_events_.begin (), start_events_.end ());
  if (script_event_)
    enders.push_back (script_event_);

  for (vsize i = 0; i < enders.size (); i++)
    {
      Stream_event *ender = enders[i];

      SCM ender_id = ender->get_property ("spanner-id");
      Context *share = get_share_context
                       (ender->get_property ("spanner-share-context"));
      SCM entry = get_cv_entry (share, ender_id);
      if (scm_is_vector (entry))
        {
          Spanner *spanner = get_cv_entry_spanner (entry);
          finished_spanners_.push_back (spanner);
          announce_end_grob (spanner, ender->self_scm ());
          delete_cv_entry (share, ender_id);
        }
    }

  // Create new spanners
  vector<Spanner *> start_spanners;
  for (vsize i = 0; i < start_events_.size (); i++)
    {
      Stream_event *ev = start_events_[i];
      SCM id = ev->get_property ("spanner-id");
      Spanner *spanner;

      string start_type = get_spanner_type (ev);
      SCM cresc_type = get_property_setting (ev, "span-type",
                                             (start_type + "Spanner").c_str ());

      if (scm_is_eq (cresc_type, ly_symbol2scm ("text")))
        {
          spanner = make_spanner ("DynamicTextSpanner", ev->self_scm ());

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

      spanner->set_property ("spanner-id", id);

      // if we have a break-dynamic-span event right after the start dynamic,
      // break the new spanner immediately
      if (to_boolean (scm_assoc_ref (end_new_spanner_, id)))
        {
          spanner->set_property ("spanner-broken", SCM_BOOL_T);
          end_new_spanner_ = scm_assoc_remove_x (end_new_spanner_, id);
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
      Context *share = get_share_context
                       (ev->get_property ("spanner-share-context"));
      // Add spanner to sharedSpanners
      create_cv_entry (share, id, spanner, get_spanner_type (ev));
    }

  if (script_event_)
    {
      script_ = make_item ("DynamicText", script_event_->self_scm ());
      script_->set_property ("text",
                             script_event_->get_property ("text"));

      for (vsize i = 0; i < finished_spanners_.size (); i++)
        finished_spanners_[i]->set_bound (RIGHT, script_);
      for (vsize i = 0; i < start_spanners.size (); i++)
        start_spanners[i]->set_bound (LEFT, script_);
    }
}

void
Dynamic_engraver::stop_translation_timestep ()
{
  for (vsize i = 0; i < finished_spanners_.size (); i++)
    {
      if (!finished_spanners_[i]->get_bound (RIGHT))
        finished_spanners_[i]
        ->set_bound (RIGHT,
                     unsmob<Grob> (get_property ("currentMusicalColumn")));
    }

  vector<cv_entry> entries = my_cv_entries ();
  for (vsize i = 0; i < entries.size (); i++)
    {
      Spanner *span = get_cv_entry_spanner (entries[i].first);
      if (!span->get_bound (LEFT))
        span
        ->set_bound (LEFT,
                     unsmob<Grob> (get_property ("currentMusicalColumn")));
    }

  script_ = 0;
  script_event_ = 0;
  start_events_.clear ();
  stop_events_.clear ();
  finished_spanners_.clear ();
  end_new_spanner_ = SCM_EOL;
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

  vector<cv_entry> entries = my_cv_entries ();
  for (vsize i = 0; i < entries.size (); i++)
    {
      Spanner *span = get_cv_entry_spanner (entries[i].first);
      if (!span->get_bound (LEFT))
        span->set_bound (LEFT, info.grob ());
    }
  for (vsize i = 0; i < finished_spanners_.size (); i++)
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
