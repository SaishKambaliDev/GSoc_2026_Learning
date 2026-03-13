/* A simple state machine for detecting calls to exit().
   This is a minimal GCC analyzer checker example.  */

#include "analyzer/common.h"

#include "diagnostics/event-id.h"
#include "stringpool.h"
#include "attribs.h"
#include "xml-printer.h"
#include "target.h"

#include "analyzer/analyzer-logging.h"
#include "analyzer/sm.h"
#include "analyzer/pending-diagnostic.h"
#if ENABLE_ANALYZER

namespace ana {

namespace {

/* Diagnostic emitted when exit() is called.  */

class exit_diagnostic : public pending_diagnostic
{
public:
  exit_diagnostic () {}

  const char *get_kind () const final override
  {
    return "exit-call";
  }

  int get_controlling_option () const final override
  {
    return 0;
  }

  bool subclass_equal_p (const pending_diagnostic &) const final override
  {
    return true;
  }

  bool emit (diagnostic_emission_context &ctxt) final override
  {
    return ctxt.warn ("call to %<exit%>");
  }
};


/* Simple state machine detecting exit().  */

class exit_state_machine : public state_machine
{
public:

  exit_state_machine (logger *logger)
  : state_machine ("exit", logger)
  {
  }

  bool inherited_state_p () const final override
  {
    return false;
  }

  bool can_purge_p (state_t) const final override
  {
    return true;
  }

  bool on_stmt (sm_context &sm_ctxt,
                const gimple *stmt) const final override
  {
    if (const gcall *call = dyn_cast<const gcall *>(stmt))
      if (tree fndecl = sm_ctxt.get_fndecl_for_call (*call))
        if (is_named_call_p (fndecl, "exit"))
          {
            sm_ctxt.warn (NULL_TREE,
                          std::make_unique<exit_diagnostic> ());
            return true;
          }

    return false;
  }
};

} // anonymous namespace


/* Factory function called from sm.cc */

std::unique_ptr<state_machine>
make_exit_state_machine (logger *logger)
{
  return std::make_unique<exit_state_machine> (logger);
}

} // namespace ana

#endif