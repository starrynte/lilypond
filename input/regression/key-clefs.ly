\version "1.7.18"
\header { texidoc = "Tests placement of accidentals in every clef. " }

\score { \notes
  \relative cis' {

% \clef french % same as octaviated bass
\clef violin
\key cis \major cis1  \key ces \major ces
\clef soprano
\key cis \major cis \key ces \major ces
\clef mezzosoprano
\key cis \major cis \key ces \major ces
\clef alto
\key cis \major cis \key ces \major ces
\clef tenor
\key cis \major cis \key ces \major ces
\clef baritone
\key cis \major cis \key ces \major ces
\clef bass
\key cis \major cis \key ces \major  ces
}

	\paper{\paperSixteen}

}

