/* Supporting functions for C exception handling.
   Copyright (C) 2002-2025 Free Software Foundation, Inc.
   Contributed by Aldy Hernandez <aldy@quesejoda.com>.
   Shamelessly stolen from the Java front end.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

Under Section 7 of GPL version 3, you are granted additional
permissions described in the GCC Runtime Library Exception, version
3.1, as published by the Free Software Foundation.

You should have received a copy of the GNU General Public License and
a copy of the GCC Runtime Library Exception along with this program;
see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
<http://www.gnu.org/licenses/>.  */

#include "tconfig.h"
#include "tsystem.h"
#include "auto-target.h"
#include "unwind.h"
#define NO_SIZE_OF_ENCODED_VALUE
#include "unwind-pe.h"

typedef struct
{
  _Unwind_Ptr Start;
  _Unwind_Ptr LPStart;
  _Unwind_Ptr ttype_base;
  const unsigned char *TType;
  const unsigned char *action_table;
  unsigned char ttype_encoding;
  unsigned char call_site_encoding;
} lsda_header_info;

static const unsigned char *
parse_lsda_header (struct _Unwind_Context *context, const unsigned char *p,
		   lsda_header_info *info)
{
  _uleb128_t tmp;
  unsigned char lpstart_encoding;

  info->Start = (context ? _Unwind_GetRegionStart (context) : 0);

  /* Find @LPStart, the base to which landing pad offsets are relative.  */
  lpstart_encoding = *p++;
  if (lpstart_encoding != DW_EH_PE_omit)
    p = read_encoded_value (context, lpstart_encoding, p, &info->LPStart);
  else
    info->LPStart = info->Start;

  /* Find @TType, the base of the handler and exception spec type data.  */
  info->ttype_encoding = *p++;
  if (info->ttype_encoding != DW_EH_PE_omit)
    {
      p = read_uleb128 (p, &tmp);
      info->TType = p + tmp;
    }
  else
    info->TType = 0;

  /* The encoding and length of the call-site table; the action table
     immediately follows.  */
  info->call_site_encoding = *p++;
  p = read_uleb128 (p, &tmp);
  info->action_table = p + tmp;

  return p;
}

#ifdef __ARM_EABI_UNWINDER__
/* ARM EABI personality routines must also unwind the stack.  */
#define CONTINUE_UNWINDING \
  do								\
    {								\
      if (__gnu_unwind_frame (ue_header, context) != _URC_OK)	\
	return _URC_FAILURE;					\
      return _URC_CONTINUE_UNWIND;				\
    }								\
  while (0)
#else
#define CONTINUE_UNWINDING return _URC_CONTINUE_UNWIND
#endif

#ifdef __USING_SJLJ_EXCEPTIONS__
#define PERSONALITY_FUNCTION    __gcc_personality_sj0
#define __builtin_eh_return_data_regno(x) x
#elif defined(__SEH__)
#define PERSONALITY_FUNCTION	__gcc_personality_imp
#else
#define PERSONALITY_FUNCTION    __gcc_personality_v0
#endif

#ifdef __ARM_EABI_UNWINDER__
_Unwind_Reason_Code
PERSONALITY_FUNCTION (_Unwind_State, struct _Unwind_Exception *,
		      struct _Unwind_Context *);

_Unwind_Reason_Code
__attribute__((target ("general-regs-only")))
PERSONALITY_FUNCTION (_Unwind_State state,
		      struct _Unwind_Exception * ue_header,
		      struct _Unwind_Context * context)
#else
#if defined (__SEH__) && !defined (__USING_SJLJ_EXCEPTIONS__)
static
#endif
_Unwind_Reason_Code
PERSONALITY_FUNCTION (int, _Unwind_Action, _Unwind_Exception_Class,
		      struct _Unwind_Exception *, struct _Unwind_Context *);

_Unwind_Reason_Code
PERSONALITY_FUNCTION (int version,
		      _Unwind_Action actions,
		      _Unwind_Exception_Class exception_class ATTRIBUTE_UNUSED,
		      struct _Unwind_Exception *ue_header,
		      struct _Unwind_Context *context)
#endif
{
  lsda_header_info info;
  const unsigned char *language_specific_data, *p;
  _Unwind_Ptr landing_pad, ip;
  int ip_before_insn = 0;

#ifdef __ARM_EABI_UNWINDER__
  if ((state & _US_ACTION_MASK) != _US_UNWIND_FRAME_STARTING)
    CONTINUE_UNWINDING;

  /* The dwarf unwinder assumes the context structure holds things like the
     function and LSDA pointers.  The ARM implementation caches these in
     the exception header (UCB).  To avoid rewriting everything we make a
     virtual scratch register point at the UCB.  */
  ip = (_Unwind_Ptr) ue_header;
  _Unwind_SetGR (context, UNWIND_POINTER_REG, ip);
#else
  if (version != 1)
    return _URC_FATAL_PHASE1_ERROR;

  /* Currently we only support cleanups for C.  */
  if ((actions & _UA_CLEANUP_PHASE) == 0)
    CONTINUE_UNWINDING;
#endif

  language_specific_data = (const unsigned char *)
    _Unwind_GetLanguageSpecificData (context);

  /* If no LSDA, then there are no handlers or cleanups.  */
  if (! language_specific_data)
    CONTINUE_UNWINDING;

  /* Parse the LSDA header.  */
  p = parse_lsda_header (context, language_specific_data, &info);
#ifdef HAVE_GETIPINFO
  ip = _Unwind_GetIPInfo (context, &ip_before_insn);
#else
  ip = _Unwind_GetIP (context);
#endif
  if (! ip_before_insn)
    --ip;
  landing_pad = 0;

#ifdef __USING_SJLJ_EXCEPTIONS__
  /* The given "IP" is an index into the call-site table, with two
     exceptions -- -1 means no-action, and 0 means terminate.  But
     since we're using uleb128 values, we've not got random access
     to the array.  */
  if ((int) ip <= 0)
    return _URC_CONTINUE_UNWIND;
  else
    {
      _uleb128_t cs_lp, cs_action;
      do
	{
	  p = read_uleb128 (p, &cs_lp);
	  p = read_uleb128 (p, &cs_action);
	}
      while (--ip);

      /* Can never have null landing pad for sjlj -- that would have
	 been indicated by a -1 call site index.  */
      landing_pad = (_Unwind_Ptr)cs_lp + 1;
      goto found_something;
    }
#else
  /* Search the call-site table for the action associated with this IP.  */
  while (p < info.action_table)
    {
      _Unwind_Ptr cs_start, cs_len, cs_lp;
      _uleb128_t cs_action;

      /* Note that all call-site encodings are "absolute" displacements.  */
      p = read_encoded_value (0, info.call_site_encoding, p, &cs_start);
      p = read_encoded_value (0, info.call_site_encoding, p, &cs_len);
      p = read_encoded_value (0, info.call_site_encoding, p, &cs_lp);
      p = read_uleb128 (p, &cs_action);

      /* The table is sorted, so if we've passed the ip, stop.  */
      if (ip < info.Start + cs_start)
	p = info.action_table;
      else if (ip < info.Start + cs_start + cs_len)
	{
	  if (cs_lp)
	    landing_pad = info.LPStart + cs_lp;
	  goto found_something;
	}
    }
#endif

  /* IP is not in table.  No associated cleanups.  */
  /* ??? This is where C++ calls std::terminate to catch throw
     from a destructor.  */
  CONTINUE_UNWINDING;

 found_something:
  if (landing_pad == 0)
    {
      /* IP is present, but has a null landing pad.
	 No handler to be run.  */
      CONTINUE_UNWINDING;
    }

  _Unwind_SetGR (context, __builtin_eh_return_data_regno (0),
		 (_Unwind_Ptr) ue_header);
  _Unwind_SetGR (context, __builtin_eh_return_data_regno (1), 0);
  _Unwind_SetIP (context, landing_pad);
  return _URC_INSTALL_CONTEXT;
}

#if defined (__SEH__) && !defined (__USING_SJLJ_EXCEPTIONS__)
EXCEPTION_DISPOSITION
__gcc_personality_seh0 (PEXCEPTION_RECORD ms_exc, void *this_frame,
			PCONTEXT ms_orig_context, PDISPATCHER_CONTEXT ms_disp)
{
  return _GCC_specific_handler (ms_exc, this_frame, ms_orig_context,
				ms_disp, __gcc_personality_imp);
}
#endif /* SEH */
