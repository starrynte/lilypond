%% Do not edit this file; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.16"

\header {
  lsrtags = "rhythms, simultaneous-notes, tweaks-and-overrides"

  texidoc = "
When a dotted note in the upper voice is moved to avoid a collision
with a note in another voice, the default is to move the upper note to
the right.  This behaviour can be over-ridden by using the
@code{prefer-dotted-right} property of @code{NoteCollision}.

"
  doctitle = "Moving dotted notes in polyphony"
} % begin verbatim

\new Staff \relative c' <<
  { f2. f4
    \override Staff.NoteCollision #'prefer-dotted-right = ##f
    f2. f4
    \override Staff.NoteCollision #'prefer-dotted-right = ##t
    f2. f4
  }
  \\
  { e4 e e e e e e e e e e e}
>>
