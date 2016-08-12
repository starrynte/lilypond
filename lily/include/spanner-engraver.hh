#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "context.hh"
#include "engraver.hh"
#include "scm-hash.hh"
#include "spanner.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include "stream-event.hh"
#include <utility>

// Based on definitions in translator.icc
// Callbacks are first redirected through spanner_acknowledge/spanner_listen
// The appropriate callbacks on the child instances are then called
#define ADD_SPANNER_ACKNOWLEDGER_FULL(CLASS, NAME, GROB, DIRECTION, FILTERED)        \
  add_acknowledger                                                                   \
  (method_finder                                                                     \
   <&Spanner_engraver::spanner_acknowledge<&CLASS::acknowledge_ ## NAME, FILTERED> > \
   (), #GROB, CLASS::acknowledge_static_array_drul_[DIRECTION])

#define ADD_SPANNER_ACKNOWLEDGER(CLASS, NAME) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, START, false)

#define ADD_SPANNER_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, START, false)

#define ADD_SPANNER_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, START, true)

#define ADD_SPANNER_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, START, true)

#define ADD_SPANNER_END_ACKNOWLEDGER(CLASS, NAME) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, STOP, false)

#define ADD_SPANNER_END_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, STOP, false)

#define ADD_SPANNER_END_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, STOP, true)

#define ADD_SPANNER_END_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_SPANNER_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, STOP, true)

#define ADD_SPANNER_LISTENER_FULL(CLASS, NAME, EVENT, FILTERED)         \
  listener_list_ = scm_acons                                            \
    (event_class_symbol (#EVENT), method_finder                              \
     <&Spanner_engraver::spanner_listen<&CLASS::listen_ ## NAME, FILTERED> > \
     (), listener_list_)

#define ADD_SPANNER_LISTENER(CLASS, NAME) \
  ADD_SPANNER_LISTENER_FULL (CLASS, NAME, NAME, false);

#define ADD_SPANNER_LISTENER_FOR(CLASS, NAME, EVENT) \
  ADD_SPANNER_LISTENER_FULL (CLASS, NAME, EVENT, false);

#define ADD_SPANNER_FILTERED_LISTENER(CLASS, NAME) \
  ADD_SPANNER_LISTENER_FULL (CLASS, NAME, NAME, true);

#define ADD_SPANNER_FILTERED_LISTENER_FOR(CLASS, NAME, EVENT) \
  ADD_SPANNER_LISTENER_FULL (CLASS, NAME, EVENT, true);

// Context property sharedSpanners is an alist:
// (engraver-class-name . spanner-id) -> spanner-list
// spanner-list: (spanner spanner etc)
//   If spanner-list has multiple elements, the spanner-id is associated with
//   multiple spanners. This is needed for, e.g., double slurs

// Any spanners in the context property may cross voices within that context.
// The current voice a spanner belongs to is stored in the spanner property
// current-engraver.

template <class T>
class Spanner_engraver : public Engraver
{
public:
  Spanner_engraver<T> ()
  { instance_map_ = 0; }
private:
  // (spanner-share-context . spanner-id) -> engraver-list
  Scheme_hash_table *instance_map_;
  vector<T *> child_instances_;

protected:
  DECLARE_TRANSLATOR_CALLBACKS (Spanner_engraver);

  template <void (T::*callback)(Grob_info), bool filtered>
  void spanner_acknowledge (Grob_info info)
  {
    if (filtered)
      {
        Grob *grob = info.grob ();
        call_spanner_filtered<Grob_info> (grob->get_property ("spanner-share-context"),
                                          grob->get_property ("spanner-id"),
                                          callback, info);
      }
    else
      {
        for (vsize i = 0; i < child_instances_.size (); i++)
          (child_instances_[i]->*callback) (info);
      }
  }

  template <void (T::*callback)(Stream_event *), bool filtered>
  void spanner_listen (Stream_event *ev)
  {
    if (filtered)
      {
        // TODO can't directly use spanner-share-context, or need to default to Voice
        call_spanner_filtered<Stream_event *> (ev->get_property ("spanner-share-context"),
                                               ev->get_property ("spanner-id"),
                                               callback, ev);
      }
    else
      {
        for (vsize i = 0; i < child_instances_.size (); i++)
          (child_instances_[i]->*callback) (ev);
      }
  }

  template <class ParameterType>
  void call_spanner_filtered (SCM share_context, SCM spanner_id,
                              void (T::*callback)(ParameterType), ParameterType argument)
  {
    // TODO use alist instead? since key has to be symbol for hashtable
//    SCM key = scm_cons (share_context, spanner_id);
    // TODO initialize somewhere better?
    if (!instance_map_)
      instance_map_ = unsmob<Scheme_hash_table> (Scheme_hash_table::make_smob ());
    SCM instances;

    if (instance_map_->try_retrieve (spanner_id, &instances))
      {
        while (scm_is_pair (instances))
          {
            T *instance = unsmob<T> (scm_car (instances));
            (instance->*callback) (argument);

            instances = scm_cdr (instances);
          }
      }
    else
      {
        T *instance = create_new_instance ();
        instance_map_->set (spanner_id, scm_list_1 (instance->self_scm ()));
        child_instances_.push_back (instance);
        (instance->*callback) (argument);
      }
  }

  T *create_new_instance ()
  {
    T *instance = static_cast<T *> (clone ());
    instance->unprotect ();

    instance->daddy_context_ = daddy_context_;
    // Each instance shares same pointer to hash table
    instance->instance_map_ = instance_map_;
    // TODO make child_instances_ a pointer?

    return instance;
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
