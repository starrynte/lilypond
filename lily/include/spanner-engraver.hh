#include "engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"
#include <utility>

// Context property sharedSpanners is an alist:
// (((engraver-class-name . spanner-id) . entry) etc)
// entry: (voice spanner event name other)
// name: e.g. "crescendo"
// other: extra information formatted as an SCM in C++

typedef pair<SCM, Context *> cv_entry;

class Context;
class Stream_event;
class Spanner_engraver : public Engraver
{
protected:
  vector<cv_entry> my_cv_entries ();

  Context *get_share_context (SCM s);

  // Get entry associated with an id
  // Here and later: look in share_context's context property
  SCM get_cv_entry (Context *share_context, SCM spanner_id);

public:
  // Get Spanner pointer from entry
  static Spanner *get_cv_entry_spanner (SCM entry);

  // Get spanner event from entry
  static Stream_event *get_cv_entry_event (SCM entry);

  // Get spanner name from entry
  static string get_cv_entry_name (SCM entry);

  // Get/set other information from entry
  static SCM get_cv_entry_other (SCM entry);

protected:
  void set_cv_entry_other (Context *share_context, SCM spanner_id, SCM entry, SCM other);

  // Set entry's context to this voice
  void set_cv_entry_context (Context *share_context, SCM spanner_id, SCM entry);

  // Delete entry from share_context's sharedSpanners property
  void delete_cv_entry (Context *share_context, SCM spanner_id);

  // Create entry in share_context's sharedSpanners property
  void create_cv_entry (Context *share_context, SCM spanner_id, Spanner *spanner,
                        Stream_event *event, string name, SCM other = SCM_EOL);

private:
  void set_cv_entry (Context *share_context, SCM spanner_id, SCM entry);
};
