#include <algorithm>
#include "context.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"

void
Spanner_engraver::derived_mark ()
{
  for (vsize i = start_events_.size (); i--;)
    {
      scm_gc_mark (start_events_[i].ev_->self_scm ());
      if (!scm_is_null (start_events_[i].info_))
        scm_gc_mark (start_events_[i].info_);
    }
  for (vsize i = stop_events_.size (); i--;)
    {
      scm_gc_mark (stop_events_[i].ev_->self_scm ());
      if (!scm_is_null (stop_events_[i].info_))
        scm_gc_mark (stop_events_[i].info_);
    }
}

void
Spanner_engraver::listen_spanner_event_once (Stream_event *ev, SCM info,
                                             bool warn_duplicate)
{
  Direction start_stop = to_dir (ev->get_property ("span-direction"));
  vector<Event_info> &events;
  if (start_stop == STOP)
    {
      events = stop_events_;
    }
  else
    {
      if (start_stop != START)
        ev->origin ()->warning (_f ("direction of %s invalid: %d",
                                    ev->name ().c_str (),
                                    int (start_stop)));
      events = start_events_;
    }

  SCM id = ev->get_property ("spanner-id");

  // Check for existing event with same id
  for (vsize i = 0; i < events.size (); i++)
    {
      Stream_event *existing = events[i].ev;
      if (ly_is_equal (id, existing->get_property ("spanner-id")))
        {
          // If existing has no direction but ev does, replace existing with ev
          if (!to_dir (existing->get_property ("direction"))
              && to_dir (ev->get_property ("direction")))
            events[i] = Event_info (ev, info);

          if (warn_duplicate)
            {
              ev->origin ()->warning ("Two simultaneous events, skipping this one");
              existing->origin ()->warning ("Previous event here");
            }
          return;
        }
    }

  events.push_back (Event_info (ev, info));
}

void
Spanner_engraver::process_stop_events (void (*callback)(Stream_event *, SCM, Spanner *))
{
  for (vsize i = 0; i < stop_events_.size (); i++)
    {
      Stream_event *ev = stop_events_[i].ev;
      SCM info = stop_events_[i].info;

      SCM id = ev->get_property ("spanner-id");
      Context *share
        = get_share_context (ev->get_property ("spanner-share-context"));
      vector<Spanner *> spanners = get_shared_spanners (share, id);

      for (vsize j = 0; j < spanners.size (); j++)
        callback (ev, info, spanners[j]);
    }
}

void
Spanner_engraver::process_start_events (void (*callback)(Stream_event *, SCM))
{
  for (vsize i = 0; i < start_events_.size (); i++)
    callback (start_events_[i].ev, start_events_[i].info);
}

Spanner *
Spanner_engraver::internal_make_multi_spanner (SCM x, SCM cause, Stream_event *event,
                                               char const *file, int line, char const *fun)
{
  SCM id = event->get_property ("spanner-id");
  Spanner *span = internal_make_spanner (x, cause, file, line, fun);
  span->set_property ("spanner-id", id);
  span->set_property ("current-engraver", self_scm ());

  current_spanners_.push_back (span);

  Context *share = get_share_context (event->get_property ("spanner-share-context"));
  create_shared_spanner (share, id, span);

  return span;
}

void
Spanner_engraver::end_spanner (Spanner *span, SCM cause,
                               Stream_event *event, bool announce)
{
  finished_spanners_.push_back (span);

  Spanner_engraver *owner
    = unsmob<Spanner_engraver> (span->get_property ("current-engraver"));
  vector<Spanner *> &current = owner->current_spanners_;
  SCM span_scm = span->self_scm ();
  for (vsize i = 0; i < current.size (); i++)
    if (to_boolean (scm_equal_p (current[i]->self_scm (), span_scm)))
      {
        current.erase (i);
        break;
      }

  if (announce)
    announce_end_grob (span, cause);

  SCM id = event->get_property ("spanner-id");
  Context *share = get_share_context (event->get_property ("spanner-share-context"));
  // TODO may be called multiple times: need to check?
  delete_shared_spanner (share, id);
}


Context *
Spanner_engraver::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context () : share;
}

Spanner *
Spanner_engraver::get_shared_spanner (Context *share, SCM spanner_id)
{
  SCM spanner_list = scm_assoc_ref (share->get_property ("sharedSpanners"),
                                    key (spanner_id));
  if (scm_is_false (spanner_list) || !scm_is_pair (spanner_list))
    return NULL;

  if (scm_is_pair (scm_cdr (spanner_list)))
    warning ("Requested one spanner when multiple present");

  return unsmob<Spanner> (scm_car (spanner_list));
}

vector<Spanner *>
Spanner_engraver::get_shared_spanners (Context *share, SCM spanner_id)
{
  SCM spanner_list = scm_assoc_ref (share->get_property ("sharedSpanners"),
                                    key (spanner_id));

  vector<Spanner *> spanners;
  while (scm_is_pair (spanner_list))
    {
      spanners.push_back (unsmob<Spanner> (scm_car (spanner_list)));
      spanner_list = scm_cdr (spanner_list);
    }

  return spanners;
}

void
Spanner_engraver::delete_shared_spanner (Context *share, SCM spanner_id)
{
  share->set_property ("sharedSpanners",
                       scm_assoc_remove_x (share->get_property ("sharedSpanners"),
                                           key (spanner_id)));
}

void
Spanner_engraver::add_shared_spanner (Context *share, SCM spanner_id, Spanner *span)
{
  SCM shared_spanners = share->get_property ("sharedSpanners");
  SCM spanner_list = scm_assoc_ref (shared_spanners, key (spanner_id));

  spanner_list = scm_is_false (spanner_list)
                 ? scm_list_1 (span)              // Create new list
                 : scm_cons (span, spanner_list); // Add spanner to existing list
                                     
  share->set_property ("sharedSpanners",
                       scm_assoc_set_x (shared_spanners,
                                        key (spanner_id), spanner_list));
}
