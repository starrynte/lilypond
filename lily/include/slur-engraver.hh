/*
  This file is part of LilyPond, the GNU music typesetter.

  Copyright (C) 2013--2015 Mike Solomon <mike@mikesolomon.org>
  Copyright (C) 2016 David Kastrup <dak@gnu.org>

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

#ifndef SLUR_ENGRAVER_HH
#define SLUR_ENGRAVER_HH

#include "engraver.hh"
#include "spanner-engraver.hh"
#include <map>

class Slur_engraver : public Spanner_engraver<Slur_engraver>
{
protected:
  struct Event_info {
    Stream_event *slur_, *note_;

    Event_info ()
      : slur_ (NULL), note_ (NULL)
    { }

    Event_info (Stream_event *slur, Stream_event *note)
      : slur_ (slur), note_ (note)
    { }

    Event_info (const Event_info& evi)
      : slur_ (evi.slur_), note_ (evi.note_)
    { }
  };
  // TODO default initialize
  Drul_array<Event_info> spanner_events_;

  typedef std::multimap<Stream_event *, Spanner *> Note_slurs;
  Drul_array<Note_slurs> note_slurs_;
  Spanner *finished_spanner_;
  vector<Grob_info> objects_to_acknowledge_;

  virtual SCM event_symbol () const;
  virtual bool double_property () const;
  virtual SCM grob_symbol () const;
  virtual const char *object_name () const;

  void acknowledge_note_column (Grob_info);
  void acknowledge_script (Grob_info);

  void listen_note (Stream_event *ev);
  // A slur on an in-chord note is not actually announced as an event
  // but rather produced by the note listener.
  void listen_note_slur (Event_info evi);
  void listen_slur (Stream_event *ev) { listen_note_slur (Event_info (ev, 0)); }
  void acknowledge_extra_object (Grob_info);
  void stop_translation_timestep ();
  void process_music ();

  void create_slur (Event_info evi, Direction dir);

  virtual void set_melisma (bool);
  virtual void derived_mark () const;

public:
  TRANSLATOR_DECLARATIONS (Slur_engraver);
  TRANSLATOR_INHERIT (Spanner_engraver<Slur_engraver>);
};

#endif // SLUR_ENGRAVER_HH
