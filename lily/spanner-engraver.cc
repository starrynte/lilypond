#include "context.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "std-vector.hh"

void Spanner_engraver::update_my_cv_spanners ()
{
  my_cv_spanners_.clear ();
  SCM my_context = context ()->self_scm ();
  for (Context *c = context (); c != NULL; c = c->get_parent_context ())
    {
      for (SCM s = c->get_property ("sharedSpanners");
          scm_is_pair (s); s = scm_cdr (s))
        {
          SCM entry = scm_cdar (s);
          if (ly_is_equal (scm_car (entry), my_context))
            {
              Spanner *span = unsmob<Spanner> (scm_cdr (entry));
              my_cv_spanners_.push_back (span);
            }
        }
    }
}

Context *Spanner_engraver::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context ()->get_score_context () : share;
}

SCM Spanner_engraver::get_cv_entry (Context *share_context, SCM spanner_id)
{
  return scm_assoc_ref (share_context->get_property ("sharedSpanners"), spanner_id);
}

Spanner *Spanner_engraver::get_cv_entry_spanner (SCM entry)
{
  return unsmob<Spanner> (scm_cdr (entry));
}

void Spanner_engraver::set_cv_entry_context(Context *share_context, SCM spanner_id, SCM entry)
{
  scm_set_car_x (entry, context ()->self_scm ());
  share_context->set_property ("sharedSpanners",
    scm_assoc_set_x (share_context->get_property ("sharedSpanners"),
      spanner_id, entry));
}

void Spanner_engraver::delete_cv_entry(Context *share_context, SCM spanner_id)
{
  share_context->set_property ("sharedSpanners",
    scm_assoc_remove_x (share_context->get_property ("sharedSpanners"),
      spanner_id));
}

void Spanner_engraver::create_cv_entry(Context *share_context, SCM spanner_id, Spanner *spanner)
{
  SCM entry = scm_cons (context ()->self_scm (), spanner->self_scm ());
  share_context->set_property ("sharedSpanners",
    scm_acons (spanner_id, entry,
      share_context->get_property ("sharedSpanners")));
}
