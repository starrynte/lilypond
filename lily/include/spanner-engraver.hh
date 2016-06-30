#include "context.hh"
#include <vector>

// Context property sharedSpanners is an alist:
// ((<spanner-id string> . (voice . spanner)) etc)

// List of spanners in this voice: avoid repeatedly looking up properties
#define CV_SPANNER_DECLARATIONS()                                        \
  private:                                                               \
  vector<Spanner *> my_cv_spanners_;                                     \
  Context *get_share_context (SCM s)                                     \
    {                                                                    \
      Context *share = find_context_above (context (), s);               \
      return (share == NULL) ? context ()->get_score_context () : share; \
    }

// Update the list of spanners currently part of this voice
// This should run at the beginning of every process_music ()
#define UPDATE_MY_CV_SPANNERS()                                    \
  my_cv_spanners_.clear ();                                        \
  {                                                                \
    SCM my_context = context ()->self_scm ();                      \
    for (Context *c = context ();                                  \
        c != NULL; c = c->get_parent_context ())                   \
      {                                                            \
        for (SCM s = c->get_property ("sharedSpanners");           \
            scm_is_pair (s); s = scm_cdr (s))                      \
          {                                                        \
            SCM entry = scm_cdar (s);                              \
            if (ly_is_equal (scm_car (entry), my_context))         \
              {                                                    \
                Spanner *span = unsmob<Spanner> (scm_cdr (entry)); \
                my_cv_spanners_.push_back (span);                  \
              }                                                    \
          }                                                        \
      }                                                            \
  }

// Get (voice . spanner) entry associated with an id
// Here and later: look in share_context's context property
#define GET_CV_ENTRY(share_context, spanner_id) \
  scm_assoc_ref (share_context->get_property ("sharedSpanners"), spanner_id)

// Get Spanner pointer from (voice . spanner) entry
#define GET_CV_ENTRY_SPANNER(entry) unsmob<Spanner> (scm_cdr (entry))

// Set the (voice . spanner) entry's context to this voice
#define SET_CV_ENTRY_CONTEXT(share_context, spanner_id, entry)       \
  scm_set_car_x (entry, context ()->self_scm ());                    \
  share_context->set_property ("sharedSpanners",                     \
    scm_assoc_set_x (share_context->get_property ("sharedSpanners"), \
      spanner_id, entry));

#define DELETE_CV_ENTRY(share_context, spanner_id)                      \
  share_context->set_property ("sharedSpanners",                        \
    scm_assoc_remove_x (share_context->get_property ("sharedSpanners"), \
      spanner_id));

#define CREATE_CV_ENTRY(share_context, spanner_id, spanner)             \
  SCM entry = scm_cons (context ()->self_scm (), spanner->self_scm ()); \
  share_context->set_property ("sharedSpanners",                        \
    scm_acons (spanner_id, entry,                                       \
      share_context->get_property ("sharedSpanners")));
