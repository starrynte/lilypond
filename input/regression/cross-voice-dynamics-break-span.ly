\version "2.19.47"
\header{
  texidoc="Cross-voice dynamic lines are broken by @code{\breakDynamicSpan}
and if adjacent dynamics have different directions (e.g. above and below)."
}

\new Staff {
  << { b4_\=Score.1\< \=Score.1\breakDynamicSpan a } >> << { g_\=Score.1\> f e } \\ { g d c^\=Score.1\p } >>
}
