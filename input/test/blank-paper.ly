\version "2.3.0"
\header {
  texidoc="@cindex Blank Paper

A blank music paper can be produced also by using invisible notes, and removing
@code{Bar_number_engraver}.

" }


\score \with {
    \override TimeSignature #'transparent = ##t
    \override NoteHead #'transparent = ##t
    defaultBarType = #""
    \remove Bar_number_engraver
}

{
    %% \clef treble
    \clef bass 
    \repeat unfold 3 { c1 \break }
}


