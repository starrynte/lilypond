/*
 lily-proto.hh -- declare class names.

 source file of the GNU LilyPond music typesetter

  (c) 1997--2001 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/

#ifndef LILY_PROTO_HH
#define LILY_PROTO_HH
#include "flower-proto.hh"




class Adobe_font_metric;
class All_font_metrics;
class Articulation_req;
class Audio_column;
class Audio_dynamic;
class Audio_element;
class Audio_instrument;
class Audio_item;
class Audio_key;
class Audio_note;
class Audio_piano_pedal;
class Audio_staff;
class Audio_tempo;
class Audio_text;
class Audio_tie;
class Audio_time_signature;
class Auto_change_iterator;
class Auto_change_music;
class Axis_group_engraver;
class Bar_engraver;
class Bar_req_collect_engraver;
class Barcheck_req;
class Base_span_bar_engraver;
class Beaming_info_list;
class Bezier;
class Bezier_bow;
class Break_algorithm;
class Break_req;
class Breathing_sign_req;
class Busy_playing_req;
class Change_iterator;
class Change_translator;
class Chord_tremolo_iterator;

class Column_x_positions;
class Context_specced_music;
class Engraver;
class Engraver;
class Engraver_group_engraver;
class Extender_req;
class Folded_repeat_iterator;
class Font_metric;
class Font_size_engraver;
class Global_translator;
class Glissando_req;
class Gourlay_breaking;
class Grace_engraver_group;
class Grace_iterator;
class Grace_music;
class Grace_performer_group;
class Hara_kiri_engraver;
class Hara_kiri_line_group_engraver;
class Hyphen_req;

class Includable_lexer;
class Input;
class Item;
class Key_change_req;
class Key_performer;
class Keyword_ent;
class Keyword_table;
class Line_group_engraver_group;
class Line_of_score;
class Local_key_item;
class Lookup;
class Lyric_combine_music;
class Lyric_combine_music_iterator;
class Lyric_engraver;
class Lyric_performer;
class Lyric_phrasing_engraver;
class Lyric_req;
class Mark_req;
class Melisma_playing_req;
class Melisma_req;
class Melodic_req;
class Midi_chunk;
class Midi_def;
class Midi_duration;
class Midi_dynamic;
class Midi_header;
class Midi_instrument;
class Midi_item;
class Midi_key;
class Midi_note;
class Midi_note_event;
class Midi_note_off;
class Midi_piano_pedal;
class Midi_stream;
class Midi_tempo;
class Midi_text;
class Midi_time_signature;
class Midi_track;
class Molecule;
class Moment;
class Music;
class Music_iterator;
class Music_list;
class Music_output;
class Music_output_def;
class Music_sequence;
class Music_wrapper;
class Music_wrapper_iterator;
class Pitch;
class Musical_req;
class My_lily_lexer;
class Note_performer;
class Note_req;
class Output_property;
class Paper_column;
class Paper_def;
class Paper_outputter;
class Paper_score;
class Paper_stream;
class Performance;
class Performer;
class Performer_group_performer;
class Piano_bar_engraver;

class Pitch_squash_engraver;
class Property_iterator;
class Rational;
class Relative_octave_music;
class Repeated_music;
class Request;
class Request_chord;
class Request_chord_iterator;
class Rest_req;
class Rhythmic_req;
class Scaled_font_metric;
class Scheme_hash_table;
class Scope;
class Score;
class Grob;
class Score_engraver;
class Score_performer;
class Script_req;
class Sequential_music;
class Sequential_music_iterator;
class Simple_music_iterator;
class Simple_spacer;
class Simultaneous_music;
class Simultaneous_music_iterator;
class Skip_req;
class Slur_bezier_bow;
class Span_req;
class Span_score_bar_engraver;
class Spanner;
class Staff_group_bar_engraver;
class Staff_performer;
class Swallow_engraver;
class Swallow_performer;
class Tempo_performer;
class Tempo_req;
class Tex_font_metric;
class Text_script_req;
class Tie;
class Tie_performer;
class Tie_req;
class Time_scaled_music;
class Time_scaled_music_iterator;
class Time_signature_performer;
class Timing_engraver;
class Timing_translator;
class Translation_property;
class Translator;
class Translator_change;
class Translator_group;
class Transposed_music;
class Tremolo_req;
class Type_swallow_translator;
class Unfolded_repeat_iterator;
class yyFlexLexer;
#endif // LILY_PROTO_HH;
