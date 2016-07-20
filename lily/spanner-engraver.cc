#include "context.hh"
#include "spanner.hh"
#include "spanner-engraver.hh"
#include "std-string.hh"
#include "std-vector.hh"

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
          if (ly_is_equal (scm_list_ref (entry, scm_from_int (0)), my_context)
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

  SCM key = scm_cons (ly_symbol2scm (class_name ()), spanner_id);
  return scm_assoc_ref (s, key);
}

Spanner *
Spanner_engraver::get_cv_entry_spanner (SCM entry)
{
  return unsmob<Spanner> (scm_list_ref (entry, scm_from_int (1)));
}

Stream_event *
Spanner_engraver::get_cv_entry_event (SCM entry)
{
  return unsmob<Stream_event> (scm_list_ref (entry, scm_from_int (2)));
}

string
Spanner_engraver::get_cv_entry_name (SCM entry)
{
  return ly_scm2string (scm_list_ref (entry, scm_from_int (3)));
}

SCM
Spanner_engraver::get_cv_entry_other (SCM entry)
{
  return scm_list_ref (entry, scm_from_int (4));
}

void
Spanner_engraver::set_cv_entry_other (Context *share_context, SCM spanner_id,
                                      SCM entry, SCM other)
{
  scm_list_set_x (entry, scm_from_int (4), other);
  set_cv_entry (share_context, spanner_id, entry);
}

void
Spanner_engraver::set_cv_entry_context (Context *share_context,
                                        SCM spanner_id, SCM entry)
{
  scm_list_set_x (entry, scm_from_int (0), context ()->self_scm ());
  set_cv_entry (share_context, spanner_id, entry);
}

void
Spanner_engraver::delete_cv_entry (Context *share_context, SCM spanner_id)
{
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    return;

  SCM key = scm_cons (ly_symbol2scm (class_name ()), spanner_id);
  share_context->set_property ("sharedSpanners",
                               scm_assoc_remove_x (s, key));
}

void
Spanner_engraver::create_cv_entry (Context *share_context, SCM spanner_id,
                                   Spanner *spanner, Stream_event *event,
                                   string name, SCM other)
{
  SCM entry = scm_list_5 (context ()->self_scm (), spanner->self_scm (),
                          event->self_scm (), ly_string2scm (name), other);

  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    s = SCM_EOL;

  SCM key = scm_cons (ly_symbol2scm (class_name ()), spanner_id);
  share_context->set_property ("sharedSpanners",
                               scm_acons (key, entry, s));
}

void
Spanner_engraver::set_cv_entry (Context *share_context, SCM spanner_id,
                                SCM entry)
{
  SCM s;
  if (!share_context->here_defined (ly_symbol2scm ("sharedSpanners"), &s))
    s = SCM_EOL;

  SCM key = scm_cons (ly_symbol2scm (class_name ()), spanner_id);
  share_context->set_property ("sharedSpanners",
                               scm_assoc_set_x (s, key, entry));
}
