\version "1.7.18"

\header {

    texidoc = "Note grouping events are used to indicate where
brackets for analysis start and end.

@cindex bracket
@cindex note groups
@cindex music analysis
@cindex analysis

"
    
}



\score {
    \notes
    {
	c4-\startGroup-\startGroup-\startGroup
	c4-\stopGroup
	c4-\startGroup
	c4-\stopGroup-\stopGroup
	c4-\startGroup
	c4-\stopGroup-\stopGroup
    }

    \paper {
	\translator {
	    \StaffContext \consists "Horizontal_bracket_engraver"
	}
	raggedright = ##t
    }
}

