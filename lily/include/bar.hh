/*
  bar.hh -- part of GNU LilyPond

  (c) 1996,97 Han-Wen Nienhuys
*/

#ifndef BAR_HH
#define BAR_HH
#include "item.hh"
/**
  A vertical bar.
  
  TODO: connections with pre and postbreak
 */
class Bar:public Item {
public:
  String type_str_;

  DECLARE_MY_RUNTIME_TYPEINFO;
  SCORE_ELEM_CLONE(Bar);
  Bar();
private:
  void do_print() const;
protected:
  virtual void do_pre_processing();
  Molecule*brew_molecule_p() const;
};
#endif // BAR_HH

