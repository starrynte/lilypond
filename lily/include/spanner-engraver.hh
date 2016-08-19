#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "context.hh"
#include "engraver.hh"
#include "engraver-group.hh"
#include "spanner.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include "stream-event.hh"
#include "translator-group.hh"
#include <utility>

// Context property spannerEngravers is an alist:
// ( #(engraver-class-symbol share-context spanner-id) . engraver-list )
// It is used to keep track of the spanner engraver instances that may exist
// within a voice.

// Context property sharedSpanners is an alist:
// (engraver-class-symbol . spanner-id) -> spanner-list
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
  Spanner_engraver ();

protected:
  Context *filter_share_;
  SCM filter_id_;
  // The initial engraver in each voice is responsible for creating other instances
  bool is_manager_;

protected:
  DECLARE_TRANSLATOR_CALLBACKS (Spanner_engraver);

  virtual void initialize ();

  template <void (T::*callback) (Grob_info)>
  void spanner_acknowledge (Grob_info info);

  template <void (T::*callback) (Stream_event *)>
  void spanner_listen (Stream_event *ev);

  template <void (T::*callback) (Stream_event *)>
  void spanner_single_listen (Stream_event *ev);

  template <class ParameterType>
  void call_spanner_filtered (Context *share, SCM spanner_id,
                              void (T::*callback) (ParameterType), ParameterType argument);

  T *create_instance (Context *share, SCM spanner_id, bool multiple = true);

protected:
  // When a spanner changes voices, this needs to be set to NULL
  // in the engraver originally containing the spanner
  // TODO double slurs
  Spanner *current_spanner_;

  #define make_multi_spanner(x, cause, share, id)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, share, id, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
                                        char const *file, int line, char const *fun);

  void end_spanner (Spanner *span, SCM cause, bool announce = true);

protected:
  Context *get_share_context (SCM s);

  // Get the spanner(s) in a context with an id
  // If spanner-list has more than one spanner, the first function warns
  // and returns the first spanner
  Spanner *get_shared_spanner (Context *share, SCM spanner_id);

  vector<Spanner *> get_shared_spanners (Context *share, SCM spanner_id);

  // Delete spanner(s) from share's sharedSpanners property
  void delete_shared_spanner (Context *share, SCM spanner_id);

  // Add spanner to share's sharedSpanners property
  void add_shared_spanner (Context *share, SCM spanner_id, Spanner *span);

private:
  inline SCM key (SCM spanner_id)
  { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }
};

template <class T>
Spanner_engraver<T>::Spanner_engraver ()
  : filter_share_ (NULL), filter_id_ (SCM_EOL), is_manager_ (true)
{
}

template <class T>
void Spanner_engraver<T>::initialize ()
{
  debug_output (string ("initializing se for ") + class_name () + ", is_manager? " + (is_manager_ ? "TRUE" : "FALSE"));
  if (is_manager_)
    {
      // Can't set this in constructor
      filter_share_ = context ();

      SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()), filter_share_->self_scm (), filter_id_));
      SCM spanner_engravers = context ()->get_property ("spannerEngravers");
      SCM instances = scm_assoc_ref (spanner_engravers, key);
      instances = scm_is_pair (instances)
                  ? scm_cons (self_scm (), instances)
                  : scm_list_1 (self_scm ());
      context ()->set_property ("spannerEngravers",
                                scm_assoc_set_x (spanner_engravers, key, instances));
    }
}

template <class T>
template <void (T::*callback) (Grob_info)>
void
Spanner_engraver<T>::spanner_acknowledge (Grob_info info)
{
  debug_output ("spanner_ack");
  Grob *grob = info.grob ();
  SCM id = grob->get_property ("spanner-id");
  Context *share = get_share_context (grob->get_property ("spanner-share-context"));

  if (is_manager_)
    {
      T *instance = create_instance (share, id, false);
      if (instance)
        {
          (instance->*callback) (info);
          return;
        }
    }

  if (ly_is_equal (id, filter_id_) && ly_is_equal (share->self_scm (), filter_share_->self_scm ()))
    (static_cast<T *> (this)->*callback) (info);
}

template <class T>
template <void (T::*callback) (Stream_event *)>
void
Spanner_engraver<T>::spanner_listen (Stream_event *ev)
{
  debug_output (class_name () + string (" spanner_listen"));
  SCM id = ev->get_property ("spanner-id");
  Context *share = get_share_context (ev->get_property ("spanner-share-context"));
  debug_output (ly_scm2string (scm_object_to_string (id, SCM_UNDEFINED)));

  if (is_manager_)
    {
      T *instance = create_instance (share, id, false);
      if (instance)
        {
          (instance->*callback) (ev);
          return;
        }
    }

  if (ly_is_equal (id, filter_id_) && ly_is_equal (share->self_scm (), filter_share_->self_scm ()))
    (static_cast<T *> (this)->*callback) (ev);
}

template <class T>
template <void (T::*callback) (Stream_event *)>
void
Spanner_engraver<T>::spanner_single_listen (Stream_event *ev)
{
  debug_output (class_name () + string (" spanner_single_listen"));
  if (is_manager_)
    (static_cast<T *> (this)->*callback) (ev);
}

template <class T>
template <class ParameterType>
void
Spanner_engraver<T>::call_spanner_filtered (Context *share, SCM spanner_id,
                                         void (T::*callback)(ParameterType), ParameterType argument)
{
  T *instance = create_instance (share, spanner_id, false);
  if (instance)
    {
      (instance->*callback) (argument);
      return;
    }

  SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()), share->self_scm (), spanner_id));
  SCM instances = scm_assoc_ref (context ()->get_property ("spannerEngravers"), key);
  while (scm_is_pair (instances))
    {
      T *instance = unsmob<T> (scm_car (instances));
      (instance->*callback) (argument);
      instances = scm_cdr (instances);
    }
}


template <class T>
T *
Spanner_engraver<T>::create_instance (Context *share, SCM id, bool multiple)
{
  SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()), share->self_scm (), id));
  SCM spanner_engravers = context ()->get_property ("spannerEngravers");
  SCM instances = scm_assoc_ref (spanner_engravers, key);
  if (!multiple && scm_is_pair (instances))
    return NULL;

  T *instance = new T;
  instance->filter_share_ = share;
  instance->filter_id_ = id;
  instance->is_manager_ = false;
  instance->initialize ();
  instance->unprotect ();

  instance->daddy_context_ = daddy_context_;
  instance->connect_to_context (daddy_context_);

  SCM instance_scm = instance->self_scm ();
  Translator_group *group = get_daddy_translator ();
  group->simple_trans_list_ = scm_cons (instance_scm, group->simple_trans_list_);
  // TODO can speed up
  for (vsize i = 0; i < TRANSLATOR_METHOD_PRECOMPUTE_COUNT; i++)
    group->precomputed_method_bindings_[i].clear ();
  group->precompute_method_bindings ();
  Engraver_group *egroup = static_cast<Engraver_group *> (group);
  egroup->acknowledge_hash_table_drul_[LEFT] = scm_c_make_hash_table (61);
  egroup->acknowledge_hash_table_drul_[RIGHT] = scm_c_make_hash_table (61);

  instances = scm_is_pair (instances)
    ? scm_cons (instance_scm, instances)
    : scm_list_1 (instance_scm);
  context ()->set_property ("spannerEngravers", scm_assoc_set_x (spanner_engravers, key, instances));

  return instance;
}

template <class T>
Spanner *
Spanner_engraver<T>::internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
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

template <class T>
void
Spanner_engraver<T>::end_spanner (Spanner *span, SCM cause, bool announce)
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

template <class T>
Context *
Spanner_engraver<T>::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context () : share;
}

template <class T>
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

template <class T>
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

template <class T>
void
Spanner_engraver<T>::delete_shared_spanner (Context *share, SCM spanner_id)
{
  SCM shared_spanners;
  if (share->here_defined (ly_symbol2scm ("sharedSpanners"), &shared_spanners))
    share->set_property ("sharedSpanners",
                         scm_assoc_remove_x (shared_spanners, key (spanner_id)));
}

template <class T>
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





// Based on definitions in translator.icc
// Filtered callbacks are sent to each child instance, but first redirected
// through spanner_acknowledge/spanner_listen. If the id and share context
// match, the callback is then called on the child instance.
#define ADD_FILTERED_ACKNOWLEDGER_FULL(CLASS, NAME, GROB, DIRECTION)       \
  add_acknowledger                                                         \
  (method_finder                                                           \
   <&Spanner_engraver::spanner_acknowledge<&CLASS::acknowledge_ ## NAME> > \
   (), #GROB, acknowledge_static_array_drul_[DIRECTION])

#define ADD_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, START)

#define ADD_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, START)

#define ADD_END_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, STOP)

#define ADD_END_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, STOP)

#define ADD_FILTERED_LISTENER_FOR(CLASS, NAME, EVENT)         \
  listener_list_ = scm_acons                                            \
    (event_class_symbol (#EVENT), method_finder                              \
     <&Spanner_engraver::spanner_listen<&CLASS::listen_ ## NAME> > \
     (), listener_list_)

#define ADD_FILTERED_LISTENER(CLASS, NAME) \
  ADD_FILTERED_LISTENER_FOR (CLASS, NAME, NAME)

#define ADD_SINGLE_LISTENER(CLASS, NAME) \
  listener_list_ = scm_acons                                            \
    (event_class_symbol (#NAME), method_finder                              \
     <&Spanner_engraver::spanner_single_listen<&CLASS::listen_ ## NAME> > \
     (), listener_list_)

#endif // SPANNER_ENGRAVER_HH
