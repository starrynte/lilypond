#ifndef SPANNER_ENGRAVER_INSTANCE_HH
#define SPANNER_ENGRAVER_INSTANCE_HH

#include "engraver.hh"
#include "spanner.hh"
#include "std-vector.hh"

#define SPANNER_ENGRAVER_INSTANCE_DECLARATIONS(NAME) \
public:                                              \
  NAME ();                                           \
  static void boot ();
// TODO move definitions

// Context property sharedSpanners is an alist:
// (engraver-class-name . spanner-id) -> spanner-list
// spanner-list: (spanner spanner etc)
//   If spanner-list has multiple elements, the spanner-id is associated with
//   multiple spanners. This is needed for, e.g., double slurs

// Any spanners in the context property may cross voices within that context.
// The current voice a spanner belongs to is stored in the spanner property
// current-engraver.
class Spanner_engraver_instance : public Engraver
{
protected:
  // When a spanner changes voices, this needs to be set to NULL
  // in the engraver originally containing the spanner
  // TODO double slurs
  Spanner *current_spanner_;

  #define make_multi_spanner(x, cause, share, id)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, share, id, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
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

  void end_spanner (Spanner *span, SCM cause, bool announce = true)
  {
    Spanner_engraver_instance *owner
      = unsmob<Spanner_engraver_instance> (span->get_property ("current-engraver"));
    owner->current_spanner_ = NULL;

    if (announce)
      announce_end_grob (span, cause);

    SCM id = span->get_property ("spanner-id");
    Context *share = get_share_context (span->get_property ("spanner-share-context"));
    // TODO may be called multiple times: need to check?
    delete_shared_spanner (share, id);
  }

protected:
  Context *get_share_context (SCM s)
  {
    Context *share = find_context_above (context (), s);
    return (share == NULL) ? context () : share;
  }

  // Get the spanner(s) in a context with an id
  // If spanner-list has more than one spanner, the first function warns
  // and returns the first spanner
  Spanner *get_shared_spanner (Context *share, SCM spanner_id)
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

  vector<Spanner *> get_shared_spanners (Context *share, SCM spanner_id)
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

  // Delete spanner(s) from share's sharedSpanners property
  void delete_shared_spanner (Context *share, SCM spanner_id)
  {
    SCM shared_spanners;
    if (share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
      share->set_property ("sharedSpanners",
                           scm_assoc_remove_x (shared_spanners, key (spanner_id)));
  }

  // Add spanner to share's sharedSpanners property
  void add_shared_spanner (Context *share, SCM spanner_id, Spanner *span)
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

private:
  inline SCM key (SCM spanner_id)
  { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }
};

#endif /* SPANNER_ENGRAVER_INSTANCE_HH */
