#include "context.hh"
#include <vector>

// alist:
// ((<spanner-id string> . (('current-voice . voice) ('spanner . spanner))) etc)
#define CV_SPANNER_DECLARATIONS() \
  private:                                                         \
  static SCM cvspanners_;                                          \
  vector<Spanner *> get_cv_spanners_in_voice (Context *context)    \
  {                                                                \
    vector<Spanner *> spanners;                                    \
    for (SCM s = cvspanners_; scm_is_pair (s); s = scm_cdr (s))    \
      {                                                            \
        SCM entry = scm_cdar (s);                                  \
        if (scm_assoc_ref (entry, ly_symbol2scm ("current-voice")) \
            == context->self_scm ())                               \
          {                                                        \
            Spanner *span = unsmob<Spanner> (scm_assoc_ref (       \
              entry, ly_symbol2scm ("spanner")));                  \
            spanners.push_back (span);                             \
          }                                                        \
      }                                                            \
    return spanners;                                               \
  }

#define GET_CV_ENTRY(spanner_id) scm_assoc_ref (cvspanners_, spanner_id)

#define GET_CV_ENTRY_SPANNER(entry) \
  unsmob<Spanner> (scm_assoc_ref (entry, ly_symbol2scm ("spanner")))

#define SET_CV_ENTRY_CONTEXT(spanner_id, entry, context) \
  entry = scm_assoc_set_x (entry, ly_symbol2scm ("current-voice"), \
    context->self_scm ()); \
  cvspanners_ = scm_assoc_set_x (cvspanners_, spanner_id, entry);

#define DELETE_CV_ENTRY(spanner_id) scm_assoc_remove_x (cvspanners_, spanner_id);

#define CREATE_CV_ENTRY(spanner_id, context, spanner) \
  SCM entry = scm_acons (ly_symbol2scm ("current-voice"), context->self_scm (), SCM_EOL); \
  entry = scm_acons (ly_symbol2scm ("spanner"), spanner->self_scm (), entry); \
  cvspanners_ = scm_acons (spanner_id, entry, cvspanners_);

