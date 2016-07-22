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
  SCM spanner_list = scm_c_vector_ref (entry, 1);
  return unsmob<Spanner> (scm_car (spanner_list));
}

vector<Spanner *>
Spanner_engraver::get_cv_entry_spanners (SCM entry)
{
  vector<Spanner *> spanners;
  for (SCM spanner_list = scm_c_vector_ref (entry, 1);
       scm_is_pair (spanner_list); spanner_list = scm_cdr (spanner_list))
    {
      spanners.push_back (unsmob<Spanner> (scm_car (spanner_list)));
    }
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
  vector<Spanner *> span_vector;
  span_vector.push_back (spanner);
  create_cv_entry (share_context, spanner_id, span_vector, name, other);
}

void
Spanner_engraver::create_cv_entry (Context *share_context, SCM spanner_id,
                                   vector<Spanner *> span_vector, string name, SCM other)
{
  SCM spanners = SCM_EOL;
  for (vsize i = span_vector.size (); i--;)
    spanners = scm_cons (span_vector[i]->self_scm (), spanners);
  SCM entry = scm_vector (scm_list_4
    (context ()->self_scm (), spanners, ly_string2scm (name), other));

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
