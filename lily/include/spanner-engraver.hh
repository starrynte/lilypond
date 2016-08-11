#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include <utility>

// Based on definitions in translator.icc
// Callbacks are first redirected through <a Spanner_engraver function>.
// The appropriate callbacks on the child instances are then called.
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

class Context;
class Stream_event;
template <class T>
class Spanner_engraver : public Engraver
{
private:
  Scheme_hash_table *child_instances;

protected:
  DECLARE_TRANSLATOR_CALLBACKS (Spanner_engraver);

  template <void (T::*callback)(Grob_info), bool filtered>
  void spanner_acknowledge (Grob_info info)
  {
    if (filtered)
      call_spanner_filtered<Grob_info> (NULL, SCM_EOL, callback, info);
    else
      {
        // call on each child instance
      }
  }

  template <void (T::*callback)(Stream_event *), bool filtered>
  void spanner_listen (Stream_event *ev)
  {
    if (filtered)
      call_spanner_filtered<Stream_event *> (ev);
  }

  template <class ParameterType>
  void call_spanner_filtered (SCM share_context, SCM spanner_id,
                              void (T::*callback)(ParameterType), ParameterType argument)
  {
    T *instance;
    SCM key = scm_cons (share_context, spanner_id);
    SCM instance_scm = child_instances->get (key);
    if (scm_is_null (instance_scm))
      {
        // instance = clone
      }
    else
        instance = unsmob<T> (instance_scm);
 
    (instance->*callback) (argument);
  }

// The following only have meaning in a child engraver instance
protected:
  // When a spanner changes voices, this needs to be set to NULL
  // in the engraver originally containing the spanner
  // TODO what about double slurs
  Spanner *current_spanner_;

  #define make_multi_spanner(x, cause, share, id)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, share, id, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, Context *share, SCM id,
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

#endif // SPANNER_ENGRAVER_HH
