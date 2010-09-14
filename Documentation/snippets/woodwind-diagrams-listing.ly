% Do not edit this file; it is automatically
% generated from Documentation/snippets/new
% This file is in the public domain.
%% Note: this file works from version 2.13.31
\version "2.13.31"

\header {
%%%    Translation of GIT committish: ab9e3136d78bfaf15cc6d77ed1975d252c3fe506


  texidocde="
Folgende Noten zeige alle Holzbläserdiagramme, die für LilyPond
definiert sind.

"
  doctitlede = "Liste der Holzbläserdiagramme"


  lsrtags="winds"
  texidoc="
The following music shows all of the woodwind diagrams currently
defined in LilyPond.
"
  doctitle = "Woodwind diagrams listing"

} % begin verbatim


\relative c' {
  \textLengthOn
  c1^
  \markup {
    \center-column {
      'piccolo
      " "
       \woodwind-diagram
                  #'piccolo
                  #'()
    }
  }

  c1^
  \markup {
    \center-column {
       'flute
       " "
       \woodwind-diagram
          #'flute
          #'()
    }
  }
  c1^\markup {
    \center-column {
      'oboe
      " "
      \woodwind-diagram
        #'oboe
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'clarinet
      " "
      \woodwind-diagram
        #'clarinet
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'bass-clarinet
      " "
      \woodwind-diagram
        #'bass-clarinet
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'saxophone
      " "
      \woodwind-diagram
        #'saxophone
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'bassoon
      " "
      \woodwind-diagram
        #'bassoon
        #'()
    }
  }

  c1^\markup {
    \center-column {
      'contrabassoon
      " "
      \woodwind-diagram
        #'contrabassoon
        #'()
    }
  }
}
