/*   
   translator-property.cc --  implement manipulation of

   immutable Grob property lists. 

   source file of the GNU LilyPond music typesetter

   (c) 2004 Han-Wen Nienhuys <hanwen@xs4all.nl>
 */

#include "main.hh"
#include "context.hh"
#include "warn.hh"
#include "item.hh"
#include "spanner.hh"

/*
  Grob descriptions (ie. alists with layout properties) are
  represented as a (ALIST . BASED-ON) pair, where BASED-ON is the
  alist defined in a parent context. BASED-ON should always be a tail
  of ALIST.
  
  */

/*
  Push or pop (depending on value of VAL) a single entry (ELTPROP . VAL)
  entry from a translator property list by name of PROP
*/


void
execute_pushpop_property (Context * trg,
			  SCM prop, SCM eltprop, SCM val)
{
  if (is_symbol (prop) && is_symbol (eltprop))
    {
      if (val != SCM_UNDEFINED)
	{
	  SCM prev = SCM_EOL;
	  Context * where = trg->where_defined (prop);

	  /*
	    Don't mess with MIDI.
	   */
	  if (!where)
	    return ;
	  
	  if (where != trg)
	    {
	      SCM base = updated_grob_properties (trg, prop);
	      prev = scm_cons (base, base); 
	      trg->internal_set_property (prop, prev);
	    }
	  else
	    prev = trg->internal_get_property (prop);
	  
	  if (!is_pair (prev))
	    {
	      programming_error ("Grob definition should be cons.");
	      return ;
	    }

	  SCM prev_alist = ly_car (prev);
	  
	  if (is_pair (prev_alist) || prev_alist == SCM_EOL)
	    {
	      bool ok = type_check_assignment (eltprop, val, ly_symbol2scm ("backend-type?"));

	      /*
	       tack onto alist:
	      */
	      if (ok)
		scm_set_car_x (prev, scm_acons (eltprop, val, prev_alist));
	    }
	  else
	    {
	      // warning here.
	    }
	}
      else if (trg->where_defined (prop) == trg)
	{
	  SCM prev = trg->internal_get_property (prop);
	  SCM prev_alist = ly_car (prev);
	  SCM daddy = ly_cdr (prev);
	  
	  SCM new_alist = SCM_EOL;
	  SCM *tail = &new_alist;

	  while (prev_alist != daddy)
	    {
	      if (is_equal (ly_caar (prev_alist), eltprop))
		{
		  prev_alist = ly_cdr (prev_alist);
		  break ;
		}

	      
	      *tail = scm_cons (ly_car (prev_alist), SCM_EOL);
	      tail = SCM_CDRLOC (*tail);
	      prev_alist = ly_cdr (prev_alist);
	    }

	  if (new_alist == SCM_EOL && prev_alist == daddy)
	    trg->unset_property (prop);
	  else
	    {
	      *tail = prev_alist;
	      trg->internal_set_property (prop, scm_cons (new_alist, daddy));
	    }
	}
    }
  else
    {
      warning ("Need symbol arguments for \\override and \\revert");
      if (internal_type_checking_global_b)
	assert (false);
    }
}

/*
  PRE_INIT_OPS is in the order specified, and hence must be reversed.
 */
void
apply_property_operations (Context *tg, SCM pre_init_ops)
{
  SCM correct_order = scm_reverse (pre_init_ops);
  for (SCM s = correct_order; is_pair (s); s = ly_cdr (s))
    {
      SCM entry = ly_car (s);
      SCM type = ly_car (entry);
      entry = ly_cdr (entry); 
      
      if (type == ly_symbol2scm ("push") || type == ly_symbol2scm ("poppush"))
	{
	  SCM val = ly_cddr (entry);
	  val = is_pair (val) ? ly_car (val) : SCM_UNDEFINED;

	  execute_pushpop_property (tg, ly_car (entry), ly_cadr (entry), val);
	}
      else if (type == ly_symbol2scm ("assign"))
	{
	  tg->internal_set_property (ly_car (entry), ly_cadr (entry));
	}
    }
}

/*
  Return the object alist for SYM, checking if its base in enclosing
  contexts has changed. The alist is updated if necessary. 
   */
SCM
updated_grob_properties (Context * tg, SCM sym)
{
  assert (is_symbol (sym));
  
  tg = tg->where_defined (sym);
  if (!tg)
    return SCM_EOL;
  
  SCM daddy_props
    = (tg->daddy_context_)
    ? updated_grob_properties (tg->daddy_context_, sym)
    : SCM_EOL;
  
  SCM props  = tg->internal_get_property (sym);

  if (!is_pair (props))
    {
      programming_error ("grob props not a pair?");
      return SCM_EOL;
    }

  SCM based_on = ly_cdr (props);
  if (based_on == daddy_props)
    {
      return ly_car (props);
    }
  else
    {
      SCM copy = daddy_props;
      SCM * tail = &copy;
      SCM p = ly_car (props);
      while  (p != based_on)
	{
	  *tail = scm_cons (ly_car (p), daddy_props);
	  tail = SCM_CDRLOC (*tail);
	  p = SCM_CDR (p);
	}
      
      scm_set_car_x (props, copy);
      scm_set_cdr_x (props, daddy_props);

      return copy;
    }
}

Item*
make_item_from_properties (Context * tg, SCM x)
{
  SCM props = updated_grob_properties (tg, x);
  return new Item (props);
}

Spanner*
make_spanner_from_properties (Context *tg, SCM x)
{
  SCM props = updated_grob_properties (tg, x);
  return new Spanner (props);
}

