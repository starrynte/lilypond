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
      ...
      callback (ev, info, ...);
    }
}

void
Spanner_engraver::process_start_events (void (*callback)(Stream_event *, SCM))
{
  for (vsize i = 0; i < start_events_.size (); i++)
    callback (start_events_[i].ev, start_events_[i].info);
}

// Override
Spanner *
Spanner_engraver::internal_make_spanner (SCM x, SCM cause,
                                         char const *file, int line, char const *fun)
{
  Spanner *span = Engraver::internal_make_spanner (x, cause, file, line, fun);
  ...
  return span;
}

// announce_end_grob


vector<cv_entry>
Spanner_engraver::my_cv_entries ()
{
  vector<cv_entry> entries;
  SCM my_context = context ()->self_scm ();
  SCM my_class = ly_symbol2scm (class_name ());
  for (Context *c = context (); c != NULL; c = c->get_parent_context ())
    {
      SCM s;
      if (!c->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
        continue;

      for (; scm_is_pair (s); s = scm_cdr (s))
        {
          SCM clazz = scm_caaar (s);
          SCM entry = scm_cdar (s);
          if (ly_is_equal (scm_c_vector_ref (entry, 0), my_context)
              && ly_is_equal (clazz, my_class))
            {
              entries.push_back (cv_entry (entry, c));
            }
        }
    }
  return entries;
}

Context *
Spanner_engraver::get_share_context (SCM s)
{
  Context *share = find_context_above (context (), s);
  return (share == NULL) ? context () : share;
}

SCM
Spanner_engraver::get_cv_entry (Context *share_context, SCM spanner_id)
{
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    return SCM_EOL;

  return scm_assoc_ref (s, key (spanner_id));
}

Spanner *
Spanner_engraver::get_cv_entry_spanner (SCM entry)
{
  SCM spanner_or_list = scm_c_vector_ref (entry, 1);
  if (scm_is_pair (spanner_or_list))
    {
      warning ("Requested one spanner when multiple present");
      return unsmob<Spanner> (scm_car (spanner_or_list));
    }
  return unsmob<Spanner> (spanner_or_list);
}

vector<Spanner *>
Spanner_engraver::get_cv_entry_spanners (SCM entry)
{
  vector<Spanner *> spanners;
  SCM spanner_or_list = scm_c_vector_ref (entry, 1);
  if (scm_is_pair (spanner_or_list))
    {
      do
        {
          spanners.push_back (unsmob<Spanner> (scm_car (spanner_or_list)));
          spanner_or_list = scm_cdr (spanner_or_list);
        }
      while (scm_is_pair (spanner_or_list));
    }
  else
    spanners.push_back (unsmob<Spanner> (spanner_or_list));
  return spanners;
}

string
Spanner_engraver::get_cv_entry_name (SCM entry)
{
  return ly_scm2string (scm_c_vector_ref (entry, 2));
}

SCM
Spanner_engraver::get_cv_entry_other (SCM entry)
{
  return scm_c_vector_ref (entry, 3);
}

void
Spanner_engraver::set_cv_entry_other (Context *share_context, SCM spanner_id,
                                      SCM entry, SCM other)
{
  scm_c_vector_set_x (entry, 3, other);
  set_cv_entry (share_context, spanner_id, entry);
}

void
Spanner_engraver::set_cv_entry_context (Context *share_context,
                                        SCM spanner_id, SCM entry)
{
  scm_c_vector_set_x (entry, 0, context ()->self_scm ());
  set_cv_entry (share_context, spanner_id, entry);
}

void
Spanner_engraver::delete_cv_entry (Context *share_context, SCM spanner_id)
{
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    return;

  share_context->set_property ("sharedSpanners",
                               scm_assoc_remove_x (s, key (spanner_id)));
}

void
Spanner_engraver::create_cv_entry (Context *share_context, SCM spanner_id,
                                   Spanner *spanner, string name, SCM other)
{
  create_cv_entry (share_context, spanner_id, spanner->self_scm (), name, other);
}

void
Spanner_engraver::create_cv_entry (Context *share_context, SCM spanner_id,
                                   vector<Spanner *> spanners, string name, SCM other)
{
  SCM spanner_list = SCM_EOL;
  for (vsize i = spanners.size (); i--;)
    spanner_list = scm_cons (spanners[i]->self_scm (), spanner_list);
  create_cv_entry (share_context, spanner_id, spanner_list, name, other);
}

void
Spanner_engraver::create_cv_entry (Context *share_context, SCM spanner_id,
                                   SCM spanner_or_list, string name, SCM other)
{
  SCM entry = scm_vector (scm_list_4 (context ()->self_scm (),
                                      spanner_or_list,
                                      ly_string2scm (name),
                                      other));
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    s = SCM_EOL;

  share_context->set_property ("sharedSpanners",
                               scm_acons (key (spanner_id), entry, s));
}

void
Spanner_engraver::set_cv_entry (Context *share_context, SCM spanner_id,
                                SCM entry)
{
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    s = SCM_EOL;

  share_context->set_property ("sharedSpanners",
                               scm_assoc_set_x (s, key (spanner_id), entry));
}
