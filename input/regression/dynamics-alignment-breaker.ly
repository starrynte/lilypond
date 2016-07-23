\version "2.19.46"

\header {
  texidoc = "Hairpins, DynamicTextSpanners and dynamics can be
positioned independently using @code{\\breakDynamicSpan}, which
causes the alignment spanner to end prematurely.
"
}

\relative {
  c'1^\<
  \dimTextDim
  c1_\>
  f,1\p

  c'1^\<
  \breakDynamicSpan
  c1_\>
  \breakDynamicSpan
  f,1\p
}

<<
  \relative {
    c'1\=Staff.1^\<
    \=Staff.1\breakDynamicSpan
    f1
    f,1\=Staff.1\p
  } \\
  \relative {
    f1
    c'1\=Staff.1_\>
    \=Staff.1\breakDynamicSpan
    c,1
  }
>>
