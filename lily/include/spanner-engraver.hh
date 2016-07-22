#ifndef SPANNER_ENGRAVER_HH
#define SPANNER_ENGRAVER_HH

#include "engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include <utility>

// Context property sharedSpanners is an alist: ( (key . entry) etc )
// key: (engraver-class-name . spanner-id)
// entry: #(voice spanner-list name other)
//   voice: Voice context that this spanner currently belongs to
//   name: e.g. "crescendo"
//   other: extra information formatted as an SCM in C++
// spanner-list: (spanner spanner etc)
//   Note that (within an engraver) a spanner-id may be associated with multiple
//   spanners: this is needed in, e.g., double slurs
typedef pair<SCM, Context *> cv_entry;

class Context;
class Stream_event;
class Spanner_engraver : public Engraver
{
protected:
  // Get spanner entries currently belonging to this voice
  vector<cv_entry> my_cv_entries ();

  Context *get_share_context (SCM s);

  // Get the entry associated with an id
  // Here and later: look in share_context's context property
  SCM get_cv_entry (Context *share_context, SCM spanner_id);

public:
  // Get first Spanner in spanner-list from entry
  static Spanner *get_cv_entry_spanner (SCM entry);
  // Get all Spanners in spanner-list
  static vector<Spanner *> get_cv_entry_spanners (SCM entry);

  // Get spanner name from entry
  static string get_cv_entry_name (SCM entry);

  // Get "other" from entry
  static SCM get_cv_entry_other (SCM entry);

protected:
  // Set entry's "other"
  void set_cv_entry_other (Context *share_context, SCM spanner_id, SCM entry, SCM other);

  // Set entry's context to this voice
  void set_cv_entry_context (Context *share_context, SCM spanner_id, SCM entry);

  // Delete entry from share_context's sharedSpanners property
  void delete_cv_entry (Context *share_context, SCM spanner_id);

  // Create entry in share_context's sharedSpanners property
  void create_cv_entry (Context *share_context, SCM spanner_id,
                        Spanner *spanner, string name, SCM other = SCM_EOL);
  void create_cv_entry (Context *share_context, SCM spanner_id,
                        vector<Spanner *> span_vector, string name, SCM other = SCM_EOL);

private:
  inline SCM key (SCM spanner_id)
    { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }
  void set_cv_entry (Context *share_context, SCM spanner_id, SCM entry);
};

#endif // SPANNER_ENGRAVER_HH
