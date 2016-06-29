#include "context.hh"
#include <vector>

// context property sharedSpanners is an alist:
// ((<spanner-id string> . (('current-voice . voice) ('spanner . spanner))) etc)
#define CV_SPANNER_DECLARATIONS()    \
  private:                           \
  vector<Spanner *> my_cv_spanners_; \

#define UPDATE_MY_CV_SPANNERS()                                         \
  my_cv_spanners_.clear ();                                             \
  {                                                                     \
    SCM my_context = context ()->self_scm ();                           \
    for (Context *c = context ();                                       \
        c != NULL; c = c->get_parent_context ())                        \
      {                                                                 \
        for (SCM s = c->get_property ("sharedSpanners");                \
            scm_is_pair (s); s = scm_cdr (s))                           \
          {                                                             \
            SCM entry = scm_cdar (s);                                   \
            if (ly_is_equal (                                           \
                scm_assoc_ref (entry, ly_symbol2scm ("current-voice")), \
                my_context))                                            \
              {                                                         \
                Spanner *span = unsmob<Spanner> (scm_assoc_ref (        \
                  entry, ly_symbol2scm ("spanner")));                   \
                my_cv_spanners_.push_back (span);                       \
              }                                                         \
          }                                                             \
      }                                                                 \
  }

// The remaining macros, for now, only use the score context's property and are WIP
#define GET_CV_ENTRY(spanner_id) scm_assoc_ref (context ()->get_score_context ()->get_property ("sharedSpanners"), spanner_id)

#define GET_CV_ENTRY_SPANNER(entry) \
  unsmob<Spanner> (scm_assoc_ref (entry, ly_symbol2scm ("spanner")))

#define SET_CV_ENTRY_CONTEXT(spanner_id, entry, context) \
  entry = scm_assoc_set_x (entry, ly_symbol2scm ("current-voice"), \
    context->self_scm ()); \
  context->get_score_context ()->set_property ("sharedSpanners", scm_assoc_set_x (context->get_score_context ()->get_property ("sharedSpanners"), spanner_id, entry));

#define DELETE_CV_ENTRY(spanner_id) context ()->get_score_context ()->set_property ("sharedSpanners", scm_assoc_remove_x (context ()->get_score_context ()->get_property ("sharedSpanners"), spanner_id));

#define CREATE_CV_ENTRY(spanner_id, context, spanner) \
  SCM entry = scm_acons (ly_symbol2scm ("current-voice"), context->self_scm (), SCM_EOL); \
  entry = scm_acons (ly_symbol2scm ("spanner"), spanner->self_scm (), entry); \
  context->get_score_context ()->set_property ("sharedSpanners", scm_acons (spanner_id, entry, context->get_score_context ()->get_property ("sharedSpanners")));
