#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include <utility>

// Context property sharedSpanners is an alist: ( (key . entry) etc )
// key: (engraver-class-name . spanner-id)
// entry: #(engraver spanner-or-list name other)
//   engraver: engraver/voice that this spanner currently belongs to
//   name: e.g. "crescendo"
//   other: extra information formatted as an SCM in C++
// spanner-or-list: spanner OR (spanner spanner etc)
//   If spanner-or-list is a list, the spanner-id is associated with multiple
//   spanners. This is needed for, e.g., double slurs
typedef pair<SCM, Context *> cv_entry;

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
  }
  vector<Event_info> start_events_;
  vector<Event_info> stop_events_;

  vector<Spanner *> current_spanners_;
  vector<Spanner *> finished_spanners_;

  #define CURRENT_SPANNERS (s)                    \
    int i = 0, Spanner *s = current_spanners_[0]; \
    s != NULL;                                    \
    i++, s = (i < current_spanners_.size () ? current_spanners_[i] : NULL)

  #define FINISHED_SPANNERS (s)                    \
    int i = 0, Spanner *s = finished_spanners_[0]; \
    s != NULL;                                     \
    i++, s = (i < finished_spanners_.size () ? finished_spanners_[i] : NULL)

  virtual void derived_mark () const;



protected:
  // Get spanner entries currently belonging to this voice
  vector<cv_entry> my_cv_entries ();

  Context *get_share_context (SCM s);

  // Get the entry associated with an id
  // Here and later: look in share_context's context property
  SCM get_cv_entry (Context *share_context, SCM spanner_id);

public:
  // Get Spanner from entry
  // If spanner-or-list is a list, warn and return the first Spanner
  static Spanner *get_cv_entry_spanner (SCM entry);
  // Get all Spanners in spanner-or-list
  static vector<Spanner *> get_cv_entry_spanners (SCM entry);

  // Get spanner name from entry
  static string get_cv_entry_name (SCM entry);

  // Get "other" from entry
  static SCM get_cv_entry_other (SCM entry);

protected:
  // Set entry's "other"
  void set_cv_entry_other (Context *share_context, SCM spanner_id, SCM entry,
                           SCM other);

  // Set entry's context to this voice
  void set_cv_entry_context (Context *share_context, SCM spanner_id, SCM entry);

  // Delete entry from share_context's sharedSpanners property
  void delete_cv_entry (Context *share_context, SCM spanner_id);

  // Create entry in share_context's sharedSpanners property
  void create_cv_entry (Context *share_context, SCM spanner_id,
                        Spanner *spanner, string name, SCM other = SCM_EOL);
  void create_cv_entry (Context *share_context, SCM spanner_id,
                        vector<Spanner *> spanners, string name,
                        SCM other = SCM_EOL);

private:
  inline SCM key (SCM spanner_id)
  { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }

  void set_cv_entry (Context *share_context, SCM spanner_id, SCM entry);

  void create_cv_entry (Context *share_context, SCM spanner_id,
                        SCM spanner_or_list, string name, SCM other = SCM_EOL);
};

#endif // SPANNER_ENGRAVER_HH
