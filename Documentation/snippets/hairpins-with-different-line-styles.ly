%% Do not edit this file; it is automatically
%% generated from LSR http://lsr.dsi.unimi.it
%% This file is in the public domain.
\version "2.13.16"

\header {
  lsrtags = "expressive-marks, tweaks-and-overrides"

  texidoc = "
Hairpins can take any style from @code{line-interface} - dashed-line,
dotted-line, line, trill or zigzag.

"
  doctitle = "Hairpins with different line styles"
} % begin verbatim

\relative c' {
  c2\< c\!
  \override Hairpin #'style = #'dashed-line
  c2\< c\!
  \override Hairpin #'style = #'dotted-line
  c2\< c\!
  \override Hairpin #'style = #'line
  c2\< c\!
  \override Hairpin #'style = #'trill
  c2\< c\!
  \override Hairpin #'style = #'zigzag
  c2\< c\!
  \revert Hairpin #'style
  c2\< c\!
}
