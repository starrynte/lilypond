\version "1.7.18"
% another regression.  -gp
\header { texidoc = "REGRESSION or DELETE. "}
%\header{
% should look the same
%title="symmetry"
%}
\score{
	\notes\relative c'{
		 g'8-[( e  c'-) g,]
		 d'-[( f'  a,-) a]
		 d-[( f  a,-) d']
		 g,-[( e,  c'-) c]
	}
	\paper{

		linewidth = 50.0\mm
	}
}

