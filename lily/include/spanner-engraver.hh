#include "engraver.hh"
#include "std-vector.hh"

// Context property sharedSpanners is an alist:
// ((<spanner-id string> . (voice . spanner)) etc)

class Context;
class Stream_event;
class Spanner_engraver : public Engraver
{
protected:
  // List of spanners in this voice: avoid repeatedly looking up properties
  vector<Spanner *> my_cv_spanners_;

  // Update the list of spanners currently part of this voice
  // This should run at the beginning of every process_music ()
  void update_my_cv_spanners ();

  Context *get_share_context (SCM s);

  // Get (voice . spanner) entry associated with an id
  // Here and later: look in share_context's context property
  SCM get_cv_entry (Context *share_context, SCM spanner_id);

  // Get Spanner pointer from (voice . spanner) entry
  Spanner *get_cv_entry_spanner (SCM entry);

  // Set the (voice . spanner) entry's context to this voice
  void set_cv_entry_context(Context *share_context, SCM spanner_id, SCM entry);

  // Delete entry from share_context's sharedSpanners property
  void delete_cv_entry(Context *share_context, SCM spanner_id);

  // Create entry in share_context's sharedSpanners property
  // Also adds spanner to my_cv_spanners_
  void create_cv_entry(Context *share_context, SCM spanner_id, Spanner *spanner);

  // (subject to change implementation) used to set spanner-id to ""
  // and spanner-share-context to 'Voice if no id is given
  void check_event_properties(Stream_event *ev);
};
