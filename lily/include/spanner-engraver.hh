#include "engraver.hh"
#include "std-vector.hh"
#include <utility>

// Context property sharedSpanners is an alist:
// (((engraver-class-name . spanner-id) . entry) etc)
// entry: (voice spanner event other)
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

  // Get Spanner pointer from entry
  Spanner *get_cv_entry_spanner (SCM entry);

  // Get/set other information from entry
  SCM get_cv_entry_other (SCM entry);
  void set_cv_entry_other (Context *share_context, SCM spanner_id, SCM entry, SCM other);

  // Set entry's context to this voice
  void set_cv_entry_context (Context *share_context, SCM spanner_id, SCM entry);

  // Delete entry from share_context's sharedSpanners property
  void delete_cv_entry (Context *share_context, SCM spanner_id);

  // Create entry in share_context's sharedSpanners property
  void create_cv_entry (Context *share_context, SCM spanner_id, Spanner *spanner,
                        Stream_event *event, SCM other = SCM_EOL);

private:
  void set_cv_entry (Context *share_context, SCM spanner_id, SCM entry);
};
