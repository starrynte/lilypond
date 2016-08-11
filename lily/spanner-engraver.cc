#include <algorithm>
#include "context.hh"
#include "international.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"

Spanner *
Spanner_engraver<T>::internal_make_multi_spanner (SCM x, SCM cause, Stream_event *event,
                                                  char const *file, int line, char const *fun)
{
  SCM id = event->get_property ("spanner-id");
  Spanner *span = internal_make_spanner (x, cause, file, line, fun);
  span->set_property ("spanner-id", id);
  span->set_property ("current-engraver", self_scm ());

  current_spanner_ = span;

  Context *share = get_share_context (event->get_property ("spanner-share-context"));
  add_shared_spanner (share, id, span);

  return span;
}

void
Spanner_engraver<T>::end_spanner (Spanner *span, SCM cause,
                                  Stream_event *event, bool announce)
{
  Spanner_engraver *owner
    = unsmob<Spanner_engraver> (span->get_property ("current-engraver"));
  owner->current_spanner_ = NULL;

  if (announce)
    announce_end_grob (span, cause);

  SCM id = event->get_property ("spanner-id");
  Context *share = get_share_context (event->get_property ("spanner-share-context"));
  // TODO may be called multiple times: need to check?
  delete_shared_spanner (share, id);
}


Context *
Spanner_engraver<T>::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context () : share;
}

Spanner *
Spanner_engraver<T>::get_shared_spanner (Context *share, SCM spanner_id)
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
Spanner_engraver<T>::get_shared_spanners (Context *share, SCM spanner_id)
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
Spanner_engraver<T>::delete_shared_spanner (Context *share, SCM spanner_id)
{
  SCM shared_spanners;
  if (share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    share->set_property ("sharedSpanners",
                         scm_assoc_remove_x (shared_spanners, key (spanner_id)));
                                           
}

void
Spanner_engraver<T>::add_shared_spanner (Context *share, SCM spanner_id, Spanner *span)
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
