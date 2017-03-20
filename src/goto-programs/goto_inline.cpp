/*******************************************************************\

Module: Function Inlining

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include <cassert>

#include <util/prefix.h>
#include <util/cprover_prefix.h>
#include <util/base_type.h>
#include <util/std_code.h>
#include <util/std_expr.h>

#include "remove_skip.h"
#include "goto_inline.h"
#include "goto_inline_class.h"

/*******************************************************************\

Function: goto_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_inline(
  goto_modelt &goto_model,
  message_handlert &message_handler,
  bool adjust_function)
{
  const namespacet ns(goto_model.symbol_table);
  goto_inline(
    goto_model.goto_functions,
    ns,
    message_handler,
    adjust_function);
}

/*******************************************************************\

Function: goto_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_inline(
  goto_functionst &goto_functions,
  const namespacet &ns,
  message_handlert &message_handler,
  bool adjust_function)
{
  goto_inlinet goto_inline(
    goto_functions,
    ns,
    message_handler,
    adjust_function);

  typedef goto_functionst::goto_functiont goto_functiont;

    // find entry point
    goto_functionst::function_mapt::iterator it=
      goto_functions.function_map.find(goto_functionst::entry_point());

    if(it==goto_functions.function_map.end())
      return;

    goto_functiont &goto_function=it->second;
    assert(goto_function.body_available());

    // gather all calls
    // we use non-transitive inlining to avoid the goto program
    // copying that goto_inlinet would do otherwise
    goto_inlinet::inline_mapt inline_map;

    Forall_goto_functions(f_it, goto_functions)
    {
      goto_functiont &goto_function=f_it->second;

      if(!goto_function.body_available())
        continue;

      goto_inlinet::call_listt &call_list=inline_map[f_it->first];

      goto_programt &goto_program=goto_function.body;

      Forall_goto_program_instructions(i_it, goto_program)
      {
        if(!goto_inlinet::is_call(i_it))
          continue;

        call_list.push_back(goto_inlinet::callt(i_it, false));
      }
    }

  goto_inline.goto_inline(
    goto_functionst::entry_point(), goto_function, inline_map, true);

  // clean up
  Forall_goto_functions(f_it, goto_functions)
  {
    if(f_it->first!=goto_functionst::entry_point())
    {
      goto_functiont &goto_function=f_it->second;
      goto_function.body.clear();
    }
  }
}

/*******************************************************************\

Function: goto_partial_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_partial_inline(
  goto_modelt &goto_model,
  message_handlert &message_handler,
  unsigned smallfunc_limit,
  bool adjust_function)
{
  const namespacet ns(goto_model.symbol_table);
  goto_partial_inline(
    goto_model.goto_functions,
    ns,
    message_handler,
    smallfunc_limit,
    adjust_function);
}

/*******************************************************************\

Function: goto_partial_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_partial_inline(
  goto_functionst &goto_functions,
  const namespacet &ns,
  message_handlert &message_handler,
  unsigned smallfunc_limit,
  bool adjust_function)
{
  goto_inlinet goto_inline(
    goto_functions,
    ns,
    message_handler,
    adjust_function);

  typedef goto_functionst::goto_functiont goto_functiont;

  // gather all calls
  goto_inlinet::inline_mapt inline_map;

  Forall_goto_functions(f_it, goto_functions)
  {
    goto_functiont &goto_function=f_it->second;

    if(!goto_function.body_available())
      continue;

    goto_programt &goto_program=goto_function.body;

    goto_inlinet::call_listt &call_list=inline_map[f_it->first];

    Forall_goto_program_instructions(i_it, goto_program)
    {
      if(!goto_inlinet::is_call(i_it))
        continue;

      exprt lhs;
      exprt function_expr;
      exprt::operandst arguments;
      exprt constrain;

      goto_inlinet::get_call(i_it, lhs, function_expr, arguments, constrain);

      if(function_expr.id()!=ID_symbol)
        continue;

      const symbol_exprt &symbol_expr=to_symbol_expr(function_expr);
      const irep_idt id=symbol_expr.get_identifier();

      goto_functionst::function_mapt::const_iterator f_it=
        goto_functions.function_map.find(id);

      if(f_it==goto_functions.function_map.end())
        continue;

      // called function
      const goto_functiont &goto_function=f_it->second;

      // We can't take functions without bodies to find functions
      // inside them to be inlined.
      // We also don't allow for the _start function to have any of its
      // function calls to be inlined
      if(!goto_function.body_available() ||
         f_it->first==ID__start)
        continue;

      const goto_programt &goto_program=goto_function.body;

      if(goto_function.is_inlined() ||
         goto_program.instructions.size()<=smallfunc_limit)
      {
        assert(goto_inlinet::is_call(i_it));
        call_list.push_back(goto_inlinet::callt(i_it, false));
      }
    }
  }

  goto_inline.goto_inline(inline_map, false);
}

/*******************************************************************\

Function: goto_function_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_function_inline(
  goto_modelt &goto_model,
  const irep_idt function,
  message_handlert &message_handler,
  bool adjust_function)
{
  const namespacet ns(goto_model.symbol_table);
  goto_function_inline(
    goto_model.goto_functions,
    function,
    ns,
    message_handler);
}

/*******************************************************************\

Function: goto_function_inline

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_function_inline(
  goto_functionst &goto_functions,
  const irep_idt function,
  const namespacet &ns,
  message_handlert &message_handler,
  bool adjust_function,
  bool caching)
{
  goto_inlinet goto_inline(
    goto_functions,
    ns,
    message_handler,
    adjust_function,
    caching);

  goto_functionst::function_mapt::iterator f_it=
    goto_functions.function_map.find(function);

  if(f_it==goto_functions.function_map.end())
    return;

  goto_functionst::goto_functiont &goto_function=f_it->second;

  if(!goto_function.body_available())
    return;

  // gather all calls
  goto_inlinet::inline_mapt inline_map;

  goto_inlinet::call_listt &call_list=inline_map[f_it->first];

  goto_programt &goto_program=goto_function.body;

  Forall_goto_program_instructions(i_it, goto_program)
  {
    if(!goto_inlinet::is_call(i_it))
      continue;

    call_list.push_back(goto_inlinet::callt(i_it, true));
  }

  goto_inline.goto_inline(function, goto_function, inline_map, true);
}

/*******************************************************************\

Function: goto_function_inline_and_log

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

jsont goto_function_inline_and_log(
  goto_functionst &goto_functions,
  const irep_idt function,
  const namespacet &ns,
  message_handlert &message_handler,
  bool adjust_function,
  bool caching)
{
  goto_inlinet goto_inline(
    goto_functions,
    ns,
    message_handler,
    adjust_function,
    caching);

  goto_functionst::function_mapt::iterator f_it=
    goto_functions.function_map.find(function);

  if(f_it==goto_functions.function_map.end())
    return jsont();

  goto_functionst::goto_functiont &goto_function=f_it->second;

  if(!goto_function.body_available())
    return jsont();

  // gather all calls
  goto_inlinet::inline_mapt inline_map;

  // create empty call list
  goto_inlinet::call_listt &call_list=inline_map[f_it->first];

  goto_programt &goto_program=goto_function.body;

  Forall_goto_program_instructions(i_it, goto_program)
  {
    if(!goto_inlinet::is_call(i_it))
      continue;

    call_list.push_back(goto_inlinet::callt(i_it, true));
  }

  goto_inline.goto_inline(function, goto_function, inline_map, true);
  goto_functions.update();
  goto_functions.compute_loop_numbers();

  return goto_inline.output_inline_log_json();
}
