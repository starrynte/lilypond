\version "2.19.46"
\header{
  texidoc=" Dynamics appear below or above the staff.  If multiple
dynamics are linked with (de)crescendi, they should be on the same
line. Isolated dynamics may be forced up or down.
 "
}



\relative c''{
  a1^\sfz
  a1\fff\> c,,\!\pp a'' a\p

  %% We need this to test if we get two Dynamic line spanners
  a

  %% because do_removal_processing ()
  %% doesn't seem to post_process elements
  d\f

  a
}

<<
  \relative { a'1\=Staff.1\fff\=Staff.1\> a } \\
  \relative { c'1 c,\=Staff.1\!\=Staff.1\pp }
>>
