#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "context.hh"
#include "engraver.hh"
#include "std-vector.hh"

// Based on definitions in translator.icc
// Callbacks are first redirected through spanner_acknowledge/spanner_listen
// The appropriate callbacks on the child instances are then called

template <class T>
class Spanner_engraver : public Engraver
{
private:
  class Instance;

  // alist: ((spanner-share-context . spanner-id) . instance-list)
  SCM instance_map_;
  vector<Instance *> child_instances_;

  template <void (Instance::*callback)(Grob_info), bool filtered>
  void spanner_acknowledge (Grob_info info);

  template <void (Instance::*callback)(Stream_event *), bool filtered>
  void spanner_listen (Stream_event *ev);

  template <class ParameterType>
  void call_spanner_filtered (Context *share, SCM spanner_id,
                              void (Instance::*callback)(ParameterType),
                              ParameterType argument);

  Instance *create_new_instance (Context *share, SCM spanner_id);

  Context *get_share_context (SCM s)
  {
    Context *share = find_context_above (context (), s);
    return (share == NULL) ? context () : share;
  }
};

template <class T>
Spanner_engraver<T>::Spanner_engraver ()
  : instance_map_ (SCM_EOL)
{
}

template <class T>
void
Spanner_engraver<T>::boot ()
{
  T::boot ();
}

template <class T>
template <void (Instance::*callback)(Grob_info), bool filtered>
void
Spanner_engraver<T>::spanner_acknowledge (Grob_info info)
{
  if (filtered)
    {
      Grob *grob = info.grob ();
      // TODO can't directly use spanner-share-context, or need to default to Voice
      call_spanner_filtered<Grob_info> (get_share_context (grob->get_property ("spanner-share-context")),
                                        grob->get_property ("spanner-id"),
                                        callback, info);
    }
  else
    {
      for (vsize i = 0; i < child_instances_.size (); i++)
        (child_instances_[i]->*callback) (info);
    }
}

template <class T>
template <void (Instance::*callback)(Stream_event *), bool filtered>
void
Spanner_engraver<T>::spanner_listen (Stream_event *ev)
{
  if (filtered)
    {
      call_spanner_filtered<Stream_event *> (get_share_context (ev->get_property ("spanner-share-context")),
                                        ev->get_property ("spanner-id"),
                                        callback, ev);
    }
  else
    {
      for (vsize i = 0; i < child_instances_.size (); i++)
        (child_instances_[i]->*callback) (ev);
    }
}

template <class T>
template <class ParameterType>
void
Spanner_engraver<T>::call_spanner_filtered (Context *share, SCM spanner_id,
                                            void (Instance::*callback)(ParameterType),
                                            ParameterType argument)
{
  SCM key = scm_cons (share->self_scm (), spanner_id);
  SCM instances = scm_assoc_ref (key, instance_map_);
  if (scm_is_false (instances))
    {
      Instance *instance = create_new_instance (share, spanner_id);
      (instance->*callback) (argument);
    }
  else
    {
      while (scm_is_pair (instances))
        {
          Instance *instance = unsmob<Instance> (scm_car (instances));
          (instance->*callback) (argument);
          instances = scm_cdr (instances);
        }
    }
}

template <class T>
Instance *
Spanner_engraver<T>::create_new_instance (Context *share, SCM spanner_id)
{
  Instance *instance = new Instance;
  instance->unprotect ();
  instance->manager_ = this;

  SCM instance_scm = instance->self_scm ();
  SCM key = scm_cons (share->self_scm (), spanner_id);
  SCM instances = scm_assoc_ref (key, instance_map_);
  instances = scm_is_false (instances)
              ? scm_list_1 (instance_scm)
              : scm_cons (instance_scm, instances);

  child_instances_.push_back (instance);
  return instance;
}

// TODO process_music/etc
// Templated version of most of ADD_TRANSLATOR from translator.icc
// IMPLEMENT_FETCH_PRECOMPUTABLE_METHODS
template <class T>
void
Spanner_engraver<T>::fetch_precomputable_methods (SCM ptrs[])
{
  ptrs[START_TRANSLATION_TIMESTEP] =
    method_finder <&Spanner_engraver<T>::start_translation_timestep> ();

  ptrs[STOP_TRANSLATION_TIMESTEP] =
    method_finder <&Spanner_engraver<T>::stop_translation_timestep> ();

  ptrs[PROCESS_MUSIC] =
    method_finder <&Spanner_engraver<T>::process_music> ();

  ptrs[PROCESS_ACKNOWLEDGED] =
    method_finder <&Spanner_engraver<T>::process_acknowledged> ();
}

// DEFINE_ACKNOWLEDGERS
template <class T>
Drul_array<Protected_scm> Spanner_engraver<T>::acknowledge_static_array_drul_;

template <class T>
SCM
Spanner_engraver<T>::static_get_acknowledger (SCM sym, Direction start_end)
{
  return generic_get_acknowledger
    (sym, acknowledge_static_array_drul_[start_end]);
}

// ADD_THIS_TRANSLATOR (most)
template <class T>
SCM Spanner_engraver<T>::static_description_ = SCM_EOL;

template <class T>
static void
Spanner_engraver_adder ()
{
  Spanner_engraver<T>::boot ();
  Spanner_engraver<T> *t = new Spanner_engraver<T>;
  Spanner_engraver<T>::static_description_
    = scm_permanent_object (t->static_translator_description ());
  add_translator (t, ly_symbol2scm ("Slur_engraver"));
}

template <class T>
SCM
Spanner_engraver<T>::translator_description () const
{
  return static_description_;
}

// DEFINE_TRANSLATOR_LISTENER_LIST
template <class T>
Protected_scm Spanner_engraver<T>::listener_list_ (SCM_EOL);

#endif // SPANNER_ENGRAVER_HH
