#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "context.hh"
#include "direction.hh"
#include "engraver.hh"
#include "engraver-group.hh"
#include "spanner.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include "stream-event.hh"
#include "translator-dispatch-list.hh"
#include "translator-group.hh"
#include <sstream>
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

class Spanner_engraver : public Engraver
{
public:
  Spanner_engraver ();

private:
  // This instance will only listen/acknowledge for this share context and spanner id
  Context *filter_share_;
  SCM filter_id_;
  // The initial engraver in each voice is responsible for creating other instances
  bool is_manager_;

protected:
  virtual void initialize ();

  // Filtered listen/acknowledge callbacks are first passed through these functions
  template <class T, void (T::*callback) (Grob_info)>
  void spanner_acknowledge (Grob_info info);

  template <class T, void (T::*callback) (Stream_event *)>
  void spanner_listen (Stream_event *ev);

  template <class T, void (T::*callback) (Stream_event *)>
  void spanner_single_listen (Stream_event *ev);

  // Call a function on a particular instance (with some share context and spanner id)
  template <class T, class ParameterType>
  void call_spanner_filtered (SCM share_context, SCM spanner_id,
                              void (T::*callback) (ParameterType), ParameterType argument);

  // Create a new instance for this share context and spanner id
  // If an instance already exists for that combination, only create another one
  // if multiple is true
  template <class T>
  T *create_instance (SCM share_context, SCM spanner_id, bool multiple = true);

protected:
  // When a spanner changes voices, this needs to be set to NULL in the
  // engraver originally containing the spanner (see take_spanner)
  Spanner *current_spanner_;

  #define make_multi_spanner(x, cause, share, id)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, share, id, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
                                        char const *file, int line, char const *fun);

  // Get the spanner with some share context and id, and move it to this context
  Spanner_engraver *take_spanner (SCM share_context, SCM id);

  // If cause is SCM_EOL, don't announce
  void end_spanner (Spanner *span, SCM cause);

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

template <class T, void (T::*callback) (Grob_info)>
void
Spanner_engraver::spanner_acknowledge (Grob_info info)
{
  debug_output ("spanner_ack");
  Grob *grob = info.grob ();
  SCM id = grob->get_property ("spanner-id");
  SCM share_context = grob->get_property ("spanner-share-context");
  Context *share = get_share_context (share_context);
  debug_output (ly_scm2string (scm_object_to_string (id, SCM_UNDEFINED)));
  debug_output ("self " + ly_scm2string (scm_object_to_string (filter_id_, SCM_UNDEFINED)));
  debug_output (ly_scm2string (scm_object_to_string (context ()->get_property ("sharedSpanners"), SCM_UNDEFINED)));
  debug_output (ly_scm2string (scm_object_to_string (context ()->get_property ("spannerEngravers"), SCM_UNDEFINED)));

  if (is_manager_)
    {
      T *instance = create_instance<T> (share_context, id, false);
      if (instance)
        {
          (instance->*callback) (info);
          return;
        }
    }

  if (ly_is_equal (id, filter_id_) && ly_is_equal (share->self_scm (), filter_share_->self_scm ()))
    (static_cast<T *> (this)->*callback) (info);
}

template <class T, void (T::*callback) (Stream_event *)>
void
Spanner_engraver::spanner_listen (Stream_event *ev)
{
  debug_output (class_name () + string (" spanner_listen"));
  SCM id = ev->get_property ("spanner-id");
  SCM share_context = ev->get_property ("spanner-share-context");
  Context *share = get_share_context (share_context);
  debug_output (ly_scm2string (scm_object_to_string (id, SCM_UNDEFINED)));
  debug_output ("self " + ly_scm2string (scm_object_to_string (filter_id_, SCM_UNDEFINED)));

  if (is_manager_)
    {
      T *instance = create_instance<T> (share_context, id, false);
      if (instance)
        {
          (instance->*callback) (ev);
          return;
        }
    }

  if (ly_is_equal (id, filter_id_) && ly_is_equal (share->self_scm (), filter_share_->self_scm ()))
    (static_cast<T *> (this)->*callback) (ev);
}

template <class T, void (T::*callback) (Stream_event *)>
void
Spanner_engraver::spanner_single_listen (Stream_event *ev)
{
  debug_output (class_name () + string (" spanner_single_listen"));
  if (is_manager_)
    (static_cast<T *> (this)->*callback) (ev);
}

template <class T, class ParameterType>
void
Spanner_engraver::call_spanner_filtered (SCM share_context, SCM spanner_id,
                                         void (T::*callback)(ParameterType), ParameterType argument)
{
  T *instance = create_instance<T> (share_context, spanner_id, false);
  if (instance)
    {
      (instance->*callback) (argument);
      return;
    }

  SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()),
                                    get_share_context (share_context)->self_scm (),
                                    spanner_id));
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
Spanner_engraver::create_instance (SCM share_context, SCM id, bool multiple)
{
  Context *share = get_share_context (share_context);
  SCM key = scm_vector (scm_list_3 (ly_symbol2scm (class_name ()), share->self_scm (), id));
  SCM spanner_engravers = context ()->get_property ("spannerEngravers");
  SCM instances = scm_assoc_ref (spanner_engravers, key);
  debug_output (ly_scm2string (scm_object_to_string (instances, SCM_UNDEFINED)));
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

  // Add to Translator_group::precomputed_method_bindings
  Translator_group *group = get_daddy_translator ();
  group->simple_trans_list_ = scm_cons (instance_scm, group->simple_trans_list_);
  SCM ptrs[TRANSLATOR_METHOD_PRECOMPUTE_COUNT];
  fetch_precomputable_methods (ptrs);
  for (vsize i = 0; i < TRANSLATOR_METHOD_PRECOMPUTE_COUNT; i++)
    {
      if (!SCM_UNBNDP (ptrs[i]))
        group->precomputed_method_bindings_[i].push_back (Method_instance (ptrs[i], instance));
    }

  // Add to Engraver_group::acknowledge_hash_table_drul
  Engraver_group *egroup = static_cast<Engraver_group *> (group);
  for (LEFT_and_RIGHT (d))
    {
      Scheme_hash_table *table = unsmob<Scheme_hash_table> (T::acknowledge_static_array_drul_[d]);
      // Skip if there are no acknowledgers in this direction
      if (!table)
        continue;
      SCM interfaces = table->to_alist ();
      while (scm_is_pair (interfaces))
        {
          SCM name = scm_caar (interfaces);
          SCM ptr = scm_cdar (interfaces);
          SCM acklist_scm = scm_hashq_ref (egroup->acknowledge_hash_table_drul_[d], name, SCM_BOOL_F);
          if (!scm_is_false (acklist_scm))
            {
              Engraver_dispatch_list *acklist = unsmob<Engraver_dispatch_list> (acklist_scm);
              acklist->dispatch_entries_.push_back (Method_instance (ptr, this));
            }

          interfaces = scm_cdr (interfaces);
        }
    }

  instances = scm_is_pair (instances)
    ? scm_cons (instance_scm, instances)
    : scm_list_1 (instance_scm);
  context ()->set_property ("spannerEngravers", scm_assoc_set_x (spanner_engravers, key, instances));

  return instance;
}

// Based on definitions in translator.icc
// Filtered callbacks are sent to each child instance, but first redirected
// through spanner_acknowledge/spanner_listen. If the id and share context
// match, the callback is then called on the child instance.
// TODO add option to filter out grobs from sibling engraver instances
// TODO need to handle listeners/acknowledgers with inheritance
// TODO should take_spanner be automatically called for filtered listener/acknowledgers?
#define ADD_FILTERED_ACKNOWLEDGER_FULL(CLASS, NAME, GROB, DIRECTION)                               \
  add_acknowledger                                                                                 \
  (Callback2_wrapper::make_smob                                                                    \
   <trampoline<Spanner_engraver,                                                                   \
               &Spanner_engraver::spanner_acknowledge<CLASS, &CLASS::acknowledge_ ## NAME> > > (), \
   #GROB, acknowledge_static_array_drul_[DIRECTION])

#define ADD_FILTERED_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, NAME, START)

#define ADD_FILTERED_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, START)

#define ADD_FILTERED_END_ACKNOWLEDGER(CLASS, NAME) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, end_ ## NAME, NAME, STOP)

#define ADD_FILTERED_END_ACKNOWLEDGER_FOR(CLASS, NAME, GROB) \
  ADD_FILTERED_ACKNOWLEDGER_FULL (CLASS, NAME, GROB, STOP)

#define ADD_FILTERED_LISTENER_FOR(CLASS, NAME, EVENT)                                      \
  listener_list_ = scm_acons                                                               \
    (event_class_symbol (#EVENT),                                                          \
     Callback_wrapper::make_smob                                                           \
     <trampoline<Spanner_engraver,                                                         \
                 &Spanner_engraver::spanner_listen<CLASS, &CLASS::listen_ ## NAME> > > (), \
     listener_list_)

#define ADD_FILTERED_LISTENER(CLASS, NAME) \
  ADD_FILTERED_LISTENER_FOR (CLASS, NAME, NAME)

#define ADD_SINGLE_LISTENER(CLASS, NAME)                                                          \
  listener_list_ = scm_acons                                                                      \
    (event_class_symbol (#NAME),                                                                  \
     Callback_wrapper::make_smob                                                                  \
     <trampoline<Spanner_engraver,                                                                \
                 &Spanner_engraver::spanner_single_listen<CLASS, &CLASS::listen_ ## NAME> > > (), \
     listener_list_)

#endif // SPANNER_ENGRAVER_HH
