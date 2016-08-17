#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "context.hh"
#include "engraver.hh"
#include "scm-hash.hh"
#include "spanner.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include "stream-event.hh"
#include "translator-group.hh"
#include <utility>

// Based on definitions in translator.icc
// Filtered callbacks are sent to each child instance, but first redirected
// through spanner_acknowledge/spanner_listen. If the id and share context
// match, the callback is then called on the child instance.
// TODO owners
#define ADD_FILTERED_ACKNOWLEDGER_FULL(CLASS, NAME, GROB, DIRECTION)       \
  add_acknowledger                                                         \
  (method_finder                                                           \
   <&Spanner_engraver::spanner_acknowledge<CLASS, &CLASS::acknowledge_ ## NAME> > \
   (), #GROB, acknowledge_static_array_drul_[DIRECTION])

#define ADD_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, START)

#define ADD_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, START)

#define ADD_END_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, STOP)

#define ADD_END_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, STOP)

#define ADD_FILTERED_LISTENER_FOR(CLASS, OWNER, NAME, EVENT)         \
  listener_list_ = scm_acons                                            \
    (event_class_symbol (#EVENT), method_finder                              \
     <&Spanner_engraver::spanner_listen<OWNER, &CLASS::listen_ ## NAME> > \
     (), listener_list_)

#define ADD_FILTERED_LISTENER(CLASS, OWNER, NAME) \
  ADD_FILTERED_LISTENER_FOR (CLASS, OWNER, NAME, NAME)

// Context property sharedSpanners is an alist:
// (engraver-class-name . spanner-id) -> spanner-list
// spanner-list: (spanner spanner etc)
//   If spanner-list has multiple elements, the spanner-id is associated with
//   multiple spanners. This is needed for, e.g., double slurs

// Any spanners in the context property may cross voices within that context.
// The current voice a spanner belongs to is stored in the spanner property
// current-engraver.

class Spanner_engraver : public Engraver
{
public:
  Spanner_engraver ()
    : instance_map_ (NULL), filter_id_ (SCM_EOL), is_manager_instance_ (true)
  {
  }
private:
  // spanner-id -> engraver-list
  Scheme_hash_table *instance_map_;
  SCM filter_id_;
  bool is_manager_instance_;

protected:
  DECLARE_TRANSLATOR_CALLBACKS (Spanner_engraver);

  template <class T, void (T::*callback) (Grob_info)>
  void spanner_acknowledge (Grob_info info)
  {
    Grob *grob = info.grob ();
    SCM id = grob->get_property ("spanner-id");
    if (ly_is_equal (id, filter_id_))
      (static_cast<T *> (this)->*callback) (info);
    if (is_manager_instance_)
      check_child_instance<T> (id);
  }

  template <class T, void (T::*callback) (Stream_event *)>
  void spanner_listen (Stream_event *ev)
  {
    SCM id = ev->get_property ("spanner-id");
    if (ly_is_equal (id, filter_id_))
      (static_cast<T *> (this)->*callback) (ev);
    if (is_manager_instance_)
      check_child_instance<T> (id);
  }

  template <class T, class ParameterType>
  void call_spanner_filtered (SCM spanner_id,
                              void (T::*callback)(ParameterType), ParameterType argument)
  {
    SCM instances = check_child_instance<T> (spanner_id);
    while (scm_is_pair (instances))
      {
        T *instance = unsmob<T> (scm_car (instances));
        (instance->*callback) (argument);
        instances = scm_cdr (instances);
      }
  }

/*
// TODO needed for double slurs
  template <class T>
  T *create_new_instance ()
  {
    T *instance = static_cast<T *> (clone ());
    instance->unprotect ();

    instance->daddy_context_ = daddy_context_;
    // Each instance shares same pointer to hash table
    instance->instance_map_ = instance_map_;

    return instance;
  }
  */

private:
  // Create new instance for the spanner id if one does not exist
  template <class T>
  SCM check_child_instance (SCM spanner_id)
  {
    if (!instance_map_)
      instance_map_ = unsmob<Scheme_hash_table> (Scheme_hash_table::make_smob ());

    SCM instances;
    if (instance_map_->try_retrieve (spanner_id, &instances))
      return instances;

    T *instance = static_cast<T *> (clone ());
    instance->unprotect ();
    instance->daddy_context_ = daddy_context_;
    instance->instance_map_ = instance_map_;
    instance->filter_id_ = spanner_id;
    instance->is_manager_instance_ = false;

    instance->connect_to_context (daddy_context_);
    SCM instance_scm = instance->self_scm ();

    Translator_group *group = get_daddy_translator ();
    group->simple_trans_list_ = scm_cons (instance_scm, group->simple_trans_list_);
    // TODO can speed up
    for (vsize i = 0; i < TRANSLATOR_METHOD_PRECOMPUTE_COUNT; i++)
      group->precomputed_method_bindings_[i].clear ();
    group->precompute_method_bindings ();

    instances = scm_list_1 (instance_scm);
    instance_map_->set (spanner_id, instances);
    return instances;
  }

// TODO
//  virtual void finalize ()
//  { scm_gc_unprotect_object (instance_map_->self_scm ()); }

// The following only have meaning in a child engraver instance
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
    Spanner_engraver *owner
      = unsmob<Spanner_engraver> (span->get_property ("current-engraver"));
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
  // TODO inline

private:
  inline SCM key (SCM spanner_id)
  { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }
};

#endif // SPANNER_ENGRAVER_HH
