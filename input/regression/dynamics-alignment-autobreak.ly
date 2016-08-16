\version "2.19.46"

\header {
  texidoc = "If a dynamic has an explicit direction that differs from the 
dynamic line spanner's direction, automatically break the dynamic line spanner.
"
}

\relative {
  c'1^\<
  c1_\>
  f,1\p

  c'1^\<
  c1_\p^\>
  c1\!
}

<<
  \relative {
    c'1\=Staff.1^\<
    f1
    f,1\=Staff.1\p
  } \\
  \relative {
    f1
    c'1\=Staff.1_\>
    c,1
  }
>>
