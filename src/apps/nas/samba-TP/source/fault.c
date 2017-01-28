/* 
   Unix SMB/Netbios implementation.
   Version based 3.0.
   Critical Fault handling
   Copyright (C) Andrew Tridgell 1992-1997
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"
extern int DEBUGLEVEL;


static void (*cont_fn)();


/*******************************************************************
report a fault
********************************************************************/
static void fault_report(int sig)
{
#ifdef _DEBUG
  DEBUG(0,("===============================================================\n"));
  DEBUG(0,("INTERNAL ERROR: Signal %d in pid %d (%s)",sig,(int)sys_getpid(),VERSION));
  DEBUG(0,("\nPlease read the file BUGS.txt in the distribution\n"));
  DEBUG(0,("===============================================================\n"));
  
#if AJT
  ajt_panic();
#endif  

  if (cont_fn)
    {
      fault_setup(cont_fn);
      cont_fn(NULL);
#ifdef SIGSEGV
      signal(SIGSEGV,SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGBUS
      signal(SIGBUS,SIGNAL_CAST SIG_DFL);
#endif
      return; /* this should cause a core dump */
    }
  exit(1);
#else	/*_DEBUG*/
  exit(1);
#endif	/*no _DEBUG*/
}

/****************************************************************************
catch serious errors
****************************************************************************/
static void sig_fault(int sig)
{
  fault_report(sig);
}

/*******************************************************************
setup our fault handlers
********************************************************************/
void fault_setup(void (*fn)())
{
  cont_fn = fn;

#ifdef SIGSEGV
  signal(SIGSEGV,SIGNAL_CAST sig_fault);
#endif
#ifdef SIGBUS
  signal(SIGBUS,SIGNAL_CAST sig_fault);
#endif
}



