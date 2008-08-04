%% Do not edit this file; it is auto-generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.11.55"

\header {
  lsrtags = "vocal-music, template"

  texidoc = "
This small template demonstrates a simple melody with lyrics. Cut and
paste, add notes, then words for the lyrics. This example turns off
automatic beaming, which is common for vocal parts. To use automatic
beaming, change or comment out the relevant line.

"
  doctitle = "Single staff template with notes and lyrics"
} % begin verbatim
melody = \relative c' {
  \clef treble
  \key c \major
  \time 4/4
  
  a4 b c d
}

text = \lyricmode {
  Aaa Bee Cee Dee
}

\score{
  <<
    \new Voice = "one" {
      \autoBeamOff
      \melody
    }
    \new Lyrics \lyricsto "one" \text
  >>
  \layout { }
  \midi { }
}
