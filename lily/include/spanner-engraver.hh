#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include <utility>

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
class Spanner_engraver : public Engraver
{
protected:
  struct Event_info
  {
    Stream_event *ev_;
    // Additional information
    SCM info_;
    Event_info (Stream_event *ev, SCM info)
      : ev_ (ev), info_ (info)
    { }
  };
  vector<Event_info> start_events_;
  vector<Event_info> stop_events_;

  vector<Spanner *> current_spanners_;
  vector<Spanner *> finished_spanners_;

//  #define LISTEN_SPANNER_EVENT_ONCE(...)
  void listen_spanner_event_once (Stream_event *ev, SCM info, bool warn_duplicate = true);

  void process_stop_events ();
  void process_start_events ();
  virtual void stop_event_callback (Stream_event *ev, SCM info, Spanner *span);
  virtual void start_event_callback (Stream_event *ev, SCM info);

  #define make_multi_spanner(x, cause, event)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, event, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, Stream_event *event,
                                        char const *file, int line, char const *fun);

  void end_spanner (Spanner *span, SCM cause, Stream_event *event, bool announce = true);

  virtual void derived_mark () const;

  void stop_timestep_clear ();

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
