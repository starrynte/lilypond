/*   
  text-item.cc -- implement Text_item

  source file of the GNU LilyPond music typesetter
  
  (c) 1998--2004 Han-Wen Nienhuys <hanwen@cs.uu.nl>
                 Jan Nieuwenhuizen <janneke@gnu.org>
*/

#include <math.h>

#include "warn.hh"
#include "grob.hh"
#include "text-item.hh"
#include "font-interface.hh"
#include "virtual-font-metric.hh"
#include "output-def.hh"
#include "modified-font-metric.hh"
#include "ly-module.hh"

MAKE_SCHEME_CALLBACK (Text_item, interpret_string, 4)
SCM
Text_item::interpret_string (SCM paper_smob,
			     SCM props, SCM input_encoding, SCM markup)
{
  Output_def *paper = unsmob_output_def (paper_smob);
  
  SCM_ASSERT_TYPE (paper, paper_smob, SCM_ARG1,
		   __FUNCTION__, "Paper definition");
  SCM_ASSERT_TYPE (ly_c_string_p (markup), markup, SCM_ARG3,
		   __FUNCTION__, "string");
  SCM_ASSERT_TYPE (input_encoding == SCM_EOL || ly_c_symbol_p (input_encoding),
		   input_encoding, SCM_ARG2, __FUNCTION__, "symbol");
  
  String str = ly_scm2string (markup);
  if (!ly_c_symbol_p (input_encoding))
    {
      SCM enc = paper->lookup_variable (ly_symbol2scm ("inputencoding"));
      if (ly_c_string_p (enc))
	input_encoding = scm_string_to_symbol (enc);
      else if (ly_c_symbol_p (enc))
	input_encoding = enc;
    }
  
  Font_metric *fm = select_encoded_font (paper, props, input_encoding);

  SCM lst = SCM_EOL;      
  Box b;
  if (Modified_font_metric* mf = dynamic_cast<Modified_font_metric*> (fm))
    {
      lst = scm_list_3 (ly_symbol2scm ("text"),
			mf->self_scm (),
			markup);
	
      b = mf->text_dimension (str);
    }
  else
    {
      /* ARGH. */
      programming_error ("Must have Modified_font_metric for text.");
      scm_display (fm->description_, scm_current_error_port ());
    }
      
  return Stencil (b, lst).smobbed_copy ();
}


MAKE_SCHEME_CALLBACK (Text_item, interpret_markup, 3)
SCM
Text_item::interpret_markup (SCM paper_smob, SCM props, SCM markup)
{
  if (ly_c_string_p (markup))
    return interpret_string (paper_smob, props, SCM_EOL, markup);
  else if (ly_c_pair_p (markup))
    {
      SCM func = ly_car (markup);
      SCM args = ly_cdr (markup);
      if (!markup_p (markup))
	programming_error ("Markup head has no markup signature.");
      
      return scm_apply_2 (func, paper_smob, props, args);
    }
  return SCM_EOL;
}

MAKE_SCHEME_CALLBACK (Text_item,print,1);
SCM
Text_item::print (SCM grob)
{
  Grob *me = unsmob_grob (grob);
  
  SCM t = me->get_property ("text");
  SCM chain = Font_interface::text_font_alist_chain (me);
  return interpret_markup (me->get_paper ()->self_scm (), chain, t);
}

/* Ugh. Duplicated from Scheme.  */
bool
Text_item::markup_p (SCM x)
{
  return (ly_c_string_p (x)
	  || (ly_c_pair_p (x)
	      && SCM_BOOL_F
	      != scm_object_property (ly_car (x),
				      ly_symbol2scm ("markup-signature"))));
}

ADD_INTERFACE (Text_item,"text-interface",
	       "A scheme markup text, see @usermanref{Text-markup}.",
	       "text baseline-skip word-space");




