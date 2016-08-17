#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "engraver.hh"
#include "std-vector.hh"

// Based on definitions in translator.icc
// Callbacks are first redirected through spanner_acknowledge/spanner_listen
// The appropriate callbacks on the child instances are then called

// T should be derived from Spanner_engraver_instance
template <class T>
class Spanner_engraver : public Engraver
{
  TRANSLATOR_DECLARATIONS (Spanner_engraver<T>);
private:
  // alist: ((spanner-share-context . spanner-id) . engraver-list)
  SCM instance_map_;
  vector<T *> child_instances_;

  template <void (T::*callback)(Grob_info), bool filtered>
  void spanner_acknowledge (Grob_info info);

  template <void (T::*callback)(Stream_event *), bool filtered>
  void spanner_listen (Stream_event *ev);

  template <class ParameterType>
  void call_spanner_filtered (Context *share, SCM spanner_id,
                              void (T::*callback)(ParameterType), ParameterType argument);

  T *create_new_instance (Context *share, SCM spanner_id);

  friend T;
};

template <class T>
Spanner_engraver<T>::Spanner_engraver ()
  : instance_map_ (SCM_EOL)
{
}

template <class T>
void Spanner_engraver<T>::boot ()
{
  T::boot ();
}

template <class T>
template <void (T::*callback)(Grob_info), bool filtered>
void Spanner_engraver<T>::spanner_acknowledge (Grob_info info)
{
  if (filtered)
    {
      Grob *grob = info.grob ();
      // TODO can't directly use spanner-share-context, or need to default to Voice
      // get_share_context ??
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

template <class T>
template <class ParameterType>
void Spanner_engraver<T>::call_spanner_filtered (Context *share, SCM spanner_id,
                                                 void (T::*callback)(ParameterType),
                                                 ParameterType argument)
{
  SCM key = scm_cons (share->self_scm (), spanner_id);
  SCM instances = scm_assoc_ref (key, instance_map_);
  if (scm_is_false (instances))
    {
      T *instance = create_new_instance (share, spanner_id);
      (instance->*callback) (argument);
    }
  else
    {
      while (scm_is_pair (instances))
        {
          T *instance = unsmob<T> (scm_car (instances));
          (instance->*callback) (argument);
          instances = scm_cdr (instances);
        }
    }
}

template <class T>
T *Spanner_engraver<T>::create_new_instance (Context *share, SCM spanner_id)
{
  T *instance = static_cast<T *> (clone ());
  instance->unprotect ();
  instance->daddy_context_ = daddy_context_;

  SCM instance_scm = instance->self_scm ();
  SCM key = scm_cons (share->self_scm (), spanner_id);
  SCM instances = scm_assoc_ref (key, instance_map_);
  instances = scm_is_false (instances)
              ? scm_list_1 (instance_scm)
              : scm_cons (instance_scm, instances);

  child_instances_.push_back (instance);
  return instance;
}

#endif // SPANNER_ENGRAVER_HH
