\version "2.19.47"
\header{
  texidoc="Basic cross-voice dynamics can be written, between two voices
that may or may not exist simultaneously. Dynamic spanners (e.g. Hairpins,
DynamicLineSpanners) are correctly created between the two events, using
\= to indicate the @code{spanner-id} and @code{spanner-share-context}."
}

\new Staff {
  << { b4\=Score.1\< a } >> << { g\=Score.1\> f e } \\ { g\< d c\f\=Score.1\! } >>
}
