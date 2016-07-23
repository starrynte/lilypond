\version "2.19.46"

#(ly:set-option 'warning-as-error #f)
#(ly:expect-warning (ly:translate-cpp-warning-scheme "unterminated %s") "crescendo")
#(ly:expect-warning (ly:translate-cpp-warning-scheme "unterminated %s") "decrescendo")
#(ly:expect-warning (ly:translate-cpp-warning-scheme "unterminated %s") "crescendo")

\header {
  texidoc = "A warning is printed if a dynamic spanner is
unterminated."
}

<<
  \new Staff {
    % warning expected: unterminated crescendo
    c'1\<
  }
  \new Staff {
    % warning expected: unterminated decrescendo
    c'1\>
  }
  \new Staff {
    % warning expected: unterminated crescendo
    << { c'1\=Staff.1\< c } \\ { a a\=1\! } >>
  }
>>

