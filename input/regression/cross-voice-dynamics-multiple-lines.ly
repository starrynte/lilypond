\version "2.19.47"
\header{
  texidoc="Multiple, concurrent DynamicLineSpanners are properly created for
overlapping dynamics."
}

{ c\=1\< d\=2\< e\=3\< f\=1\> g\=2\> a\=3\> b\=1\p \=2\breakDynamicSpan b\=2\mp b\=3\mf }
