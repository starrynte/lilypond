#include "context.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"

Spanner_engraver::Spanner_engraver ()
  : filter_share_ (NULL), filter_id_ (SCM_EOL), is_manager_ (true), current_spanner_ (NULL)
{
}

void Spanner_engraver::initialize ()
{
  if (is_manager_)
    {
      // Can't set this in constructor
      filter_share_ = context ();

      SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()),
                                        filter_share_->self_scm (), filter_id_));
      SCM spanner_engravers = context ()->get_property ("spannerEngravers");
      SCM instances = scm_assoc_ref (spanner_engravers, key);
      instances = scm_is_pair (instances)
                  ? scm_cons (self_scm (), instances)
                  : scm_list_1 (self_scm ());
      context ()->set_property ("spannerEngravers",
                                scm_assoc_set_x (spanner_engravers, key, instances));
    }
}

Spanner *
Spanner_engraver::internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
                                               char const *file, int line, char const *fun)
{
  Spanner *span = internal_make_spanner (x, cause, file, line, fun);
  span->set_property ("spanner-id", id);
  span->set_property ("spanner-share-context", share);
  span->set_property ("current-engraver", self_scm ());

  current_spanner_ = span;

  add_shared_spanner (get_share_context (share), id, span);
  return span;
}

Spanner_engraver *
Spanner_engraver::take_spanner (SCM share_context, SCM id)
{
  Spanner *span = get_shared_spanner (get_share_context (share_context), id);
  if (!span)
    {
      current_spanner_ = NULL;
      return NULL;
    }

  Spanner_engraver *owner
    = unsmob<Spanner_engraver> (span->get_property ("current-engraver"));
  owner->current_spanner_ = NULL;
  current_spanner_ = span;
  span->set_property ("current-engraver", self_scm ());

  return owner;
}

void
Spanner_engraver::end_spanner (Spanner *span, SCM cause)
{
  if (!scm_is_null (cause))
    announce_end_grob (span, cause);

  SCM id = span->get_property ("spanner-id");
  Context *share = get_share_context (span->get_property ("spanner-share-context"));
  // TODO may be called multiple times: need to check?
  delete_shared_spanner (share, id);
}

Context *
Spanner_engraver::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context () : share;
}

Spanner *
Spanner_engraver::get_shared_spanner (Context *share, SCM spanner_id)
{
  SCM shared_spanners;
  if (!share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    return NULL;

  SCM spanner_list = scm_assoc_ref (shared_spanners, key (spanner_id));
  if (scm_is_false (spanner_list) || !scm_is_pair (spanner_list))
    return NULL;

  if (scm_is_pair (scm_cdr (spanner_list)))
    warning ("Requested one spanner when multiple present");

  return unsmob<Spanner> (scm_car (spanner_list));
}

vector<Spanner *>
Spanner_engraver::get_shared_spanners (Context *share, SCM spanner_id)
{
  vector<Spanner *> spanners;
  SCM shared_spanners;
  if (share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    {
      SCM spanner_list = scm_assoc_ref (shared_spanners, key (spanner_id));
      while (scm_is_pair (spanner_list))
        {
          spanners.push_back (unsmob<Spanner> (scm_car (spanner_list)));
          spanner_list = scm_cdr (spanner_list);
        }
    }

  return spanners;
}

void
Spanner_engraver::delete_shared_spanner (Context *share, SCM spanner_id)
{
  SCM shared_spanners;
  if (share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    share->set_property ("sharedSpanners",
                         scm_assoc_remove_x (shared_spanners, key (spanner_id)));
}

void
Spanner_engraver::add_shared_spanner (Context *share, SCM spanner_id, Spanner *span)
{
  SCM shared_spanners;
  if (!share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    shared_spanners = SCM_EOL;

  SCM spanner_list = scm_assoc_ref (shared_spanners, key (spanner_id));
  spanner_list = scm_is_false (spanner_list)
                 ? scm_list_1 (span->self_scm ())              // Create new list
                 : scm_cons (span->self_scm (), spanner_list); // Add spanner to existing list

  share->set_property ("sharedSpanners",
                       scm_assoc_set_x (shared_spanners,
                                        key (spanner_id), spanner_list));
}
