/*
  span-bar-grav.cc -- implement Span_bar_engraver

  source file of the GNU LilyPond music typesetter

  (c)  1997--2001 Han-Wen Nienhuys <hanwen@cs.uu.nl>
*/


#include "lily-guile.hh"
#include "bar.hh"
#include "item.hh"
#include "span-bar.hh"
#include "engraver.hh"


/** 

  Make bars that span multiple "staffs". Catch bars, and span a
  Span_bar over them if we find more than 2 bars.  Vertical alignment
  of staffs changes the appearance of spanbars.  It is up to the
  aligner (Vertical_align_engraver, in this case, to add extra
  dependencies to the spanbars.

  */
class Span_bar_engraver : public Engraver
{
  Item * spanbar_p_;
  Link_array<Item> bar_l_arr_;

public:
  VIRTUAL_COPY_CONS (Translator);
  Span_bar_engraver ();
protected:
  virtual void acknowledge_grob (Grob_info);
  virtual void stop_translation_timestep ();

};


Span_bar_engraver::Span_bar_engraver ()
{
  spanbar_p_ =0;
}



void
Span_bar_engraver::acknowledge_grob (Grob_info i)
{
  int depth = i.origin_trans_l_arr (this).size ();
  if (depth > 1
      && Bar::has_interface (i.elem_l_))
    {
      Item * it = dynamic_cast<Item*> (i.elem_l_);
      bar_l_arr_.push (it);

      if (bar_l_arr_.size () >= 2 && !spanbar_p_) 
	{
	  spanbar_p_ = new Item (get_property ("SpanBar"));
	  Span_bar::set_interface (spanbar_p_);
		
	  spanbar_p_->set_parent (bar_l_arr_[0], Y_AXIS);
	  spanbar_p_->set_parent (bar_l_arr_[0], X_AXIS);

	  announce_grob (spanbar_p_,0);
	}
    }
}
void
Span_bar_engraver::stop_translation_timestep ()
{
  if (spanbar_p_) 
    {
      for (int i=0; i < bar_l_arr_.size () ; i++)
	Span_bar::add_bar (spanbar_p_,bar_l_arr_[i]);

      SCM vissym =ly_symbol2scm ("visibility-lambda");
      SCM vis = bar_l_arr_[0]->get_grob_property (vissym);	  
      if (scm_equal_p (spanbar_p_->get_grob_property (vissym), vis) != SCM_BOOL_T)
	spanbar_p_->set_grob_property (vissym, vis);

      typeset_grob (spanbar_p_);
      spanbar_p_ =0;
    }
  bar_l_arr_.set_size (0);
}



ADD_THIS_TRANSLATOR (Span_bar_engraver);



