//
// midi-score.hh -- declare midi_score
//
// copyright 1997 Jan Nieuwenhuizen <jan@digicash.com>

#ifndef MIDI_SCORE_HH
#define MIDI_SCORE_HH

/// (midi_score)
class Midi_score {
public:
	Midi_score( int format_i, int tracks_i, int tempo_i );
	~Midi_score();

	void add_track( Midi_track* midi_track_p );

private:
	int format_i_;
	int tracks_i_;
	int tempo_i_;
};

#endif // MIDI_SCORE_HH

