#ifndef SPANNER_ENGRAVER_INSTANCE_HH
#define SPANNER_ENGRAVER_INSTANCE_HH

#include "engraver.hh"
#include "spanner.hh"
#include "std-vector.hh"
#include "virtual-methods.hh"

#define SPANNER_ENGRAVER_INSTANCE_DECLARATIONS(NAME) \
public:                                              \
  NAME ();                                           \
  static void boot ();                               \
  DECLARE_CLASSNAME (NAME);

// Context property sharedSpanners is an alist:
// (engraver-class-name . spanner-id) -> spanner-list
// spanner-list: (spanner spanner etc)
//   If spanner-list has multiple elements, the spanner-id is associated with
//   multiple spanners. This is needed for, e.g., double slurs

// Any spanners in the context property may cross voices within that context.
// The current voice a spanner belongs to is stored in the spanner property
// current-engraver.

class Context;
class Spanner;
class Spanner_engraver_instance : public Smob<Spanner_engraver_instance>
{
public:
  DECLARE_CLASSNAME (Spanner_engraver_instance);

  template <class T>
  static T *create_instance (Context *dad)
  {
    Spanner_engraver_instance *instance = new T;
//    instance->unprotect ();
//    instance->daddy_context_ = dad;
    return static_cast<T *> (instance);
  }

  virtual ~Spanner_engraver_instance ()
  { }

protected:
  // When a spanner changes voices, this needs to be set to NULL
  // in the engraver originally containing the spanner
  // TODO double slurs
  Spanner *current_spanner_;

  Engraver *manager_;

  inline Context *context () const
  { return manager_->context (); }

  inline SCM internal_get_property (SCM symbol) const
  { return manager_->internal_get_property (symbol); }

  #define make_multi_spanner(x, cause, share, id)                     \
    internal_make_multi_spanner (ly_symbol2scm (x), cause, share, id, \
                                 __FILE__, __LINE__, __FUNCTION__)
  Spanner *internal_make_multi_spanner (SCM x, SCM cause, SCM share, SCM id,
                                        char const *file, int line, char const *fun);

  void end_spanner (Spanner *span, SCM cause, bool announce = true);

protected:
  Context *get_share_context (SCM s);

  // Get the spanner(s) in a context with an id
  // If spanner-list has more than one spanner, the first function warns
  // and returns the first spanner
  Spanner *get_shared_spanner (Context *share, SCM spanner_id);
  vector<Spanner *> get_shared_spanners (Context *share, SCM spanner_id);

  // Delete spanner(s) from share's sharedSpanners property
  void delete_shared_spanner (Context *share, SCM spanner_id);

  // Add spanner to share's sharedSpanners property
  void add_shared_spanner (Context *share, SCM spanner_id, Spanner *span);

private:
  inline SCM key (SCM spanner_id)
  { return scm_cons (ly_symbol2scm (class_name ()), spanner_id); }
};

#endif /* SPANNER_ENGRAVER_INSTANCE_HH */
