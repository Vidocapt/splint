/*
** LCLint - annotation-assisted static program checker
** Copyright (C) 1994-2000 University of Virginia,
**         Massachusetts Institute of Technology
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of the GNU General Public License as published by the
** Free Software Foundation; either version 2 of the License, or (at your
** option) any later version.
** 
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** The GNU General Public License is available from http://www.gnu.org/ or
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
** MA 02111-1307, USA.
**
** For information on lclint: lclint-request@cs.virginia.edu
** To report a bug: lclint-bug@cs.virginia.edu
** For more information: http://lclint.cs.virginia.edu
*/
/*
** cppmain.c
*/
/* CPP main program, using CPP Library.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Written by Per Bothner, 1994-95.

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 In other words, you are welcome to use, share and improve this program.
 You are forbidden to forbid anyone else to use, share and improve
 what you give them.   Help stamp out software-hoarding!  */

# include "lclintMacros.nf"
# include "llbasic.h"
# include "cpp.h"
# include "cpplib.h"
# include "cpphash.h"
# include "cpperror.h"
# include "llmain.h"

# include <stdio.h>

# ifdef WIN32
/*@-ansireserved@*/
extern /*@external@*/ /*@observer@*/ char *getenv (const char *);
/*@=ansireserved@*/
# endif

/* char *progname; */

cppReader g_cppState;

#ifdef abort
/* More 'friendly' abort that prints the line and file.
   config.h can #define abort fancy_abort if you like that sort of thing.  */

void
fancy_abort ()
{
  fatal ("Internal gcc abort.");
}
#endif

void cppReader_initMod ()
{
  struct cppOptions *opts = (struct cppOptions *) dmalloc (sizeof (*opts));

  cppReader_init (&g_cppState);
  llassert (g_cppState.opts == NULL);
  g_cppState.opts = opts;

  cppOptions_init (opts);
  /*@-compdef@*/ /* g_cppState is not yet innitialized */
} /*@=compdef@*/

void cppReader_initialize ()
{
  cppReader_initializeReader (&g_cppState);
}

int cppProcess (/*@dependent@*/ cstring infile, 
		/*@dependent@*/ cstring outfile) 
{
  FILE *ofile;
  struct cppOptions *opts = CPPOPTIONS (&g_cppState);
  
  opts->out_fname = outfile;
  opts->in_fname = infile;
  opts->out_fname = outfile;
  
  if (cppFatalErrors (&g_cppState))
    {
      llexit (LLFAILURE);
    }
  
  g_cppState.show_column = TRUE;

  if (cppReader_startProcess (&g_cppState, opts->in_fname) == 0) 
    {
      llexit (LLFAILURE);
    }

  ofile = fopen (cstring_toCharsSafe (outfile), "w");
  
  if (ofile == NULL) 
    {
      fileTable_noDelete (context_fileTable (), outfile);
      llfatalerror (message ("Cannot create temporary file for "
			     "pre-processor output.  Trying to "
			     "open: %s.  Use -tmpdir to change "
			     "the directory for temporary files.",
			     outfile));
    }
  
  for (;;)
    {
      enum cpp_token kind;
      
      llassert (g_cppState.token_buffer != NULL);

      if (!opts->no_output)
	{
	  (void) fwrite (g_cppState.token_buffer, (size_t) 1,
			 cppReader_getWritten (&g_cppState), ofile);
	}
      
      cppReader_setWritten (&g_cppState, 0);
      kind = cppGetToken (&g_cppState);
      
      if (kind == CPP_EOF)
	break;
    }

  cppReader_finish (&g_cppState);
  check (fclose (ofile) == 0);

  /* Resotre the original definition table. */

  if (!context_getFlag (FLG_SINGLEINCLUDE))
    {
      cppReader_restoreHashtab ();  
    }

  
  /* Undefine everything from this file! */

  if (g_cppState.errors != 0) {
    return -1;
  }

  return 0;
}

void cppAddIncludeDir (cstring dir) 
{
  /* -I option (Add directory to include path) */
  struct file_name_list *dirtmp = (struct file_name_list *) dmalloc (sizeof (*dirtmp));

  DPRINTF (("Add include: %s", dir));

  dirtmp->next = 0;		/* New one goes on the end */
  dirtmp->control_macro = 0;
  dirtmp->c_system_include_path = FALSE;
  
  /* This copy is necessary...but shouldn't be? */
  /*@-onlytrans@*/
  dirtmp->fname = cstring_copy (dir);
  /*@=onlytrans@*/
  
  dirtmp->got_name_map = FALSE;
  cppReader_addIncludeChain (&g_cppState, dirtmp);
}

void cppDoDefine (cstring str)
{
  cppBuffer *tbuf = g_cppState.buffer;

  g_cppState.buffer = NULL;
  cppReader_define (&g_cppState, cstring_toCharsSafe (str));
  g_cppState.buffer = tbuf;
}

void cppDoUndefine (cstring str)
{
  int sym_length;
  HASHNODE *hp;
  char *buf = cstring_toCharsSafe (str);

  sym_length = cppReader_checkMacroName (&g_cppState, buf,
				 cstring_makeLiteralTemp ("macro"));
  
  while ((hp = cppReader_lookup (buf, sym_length, -1)) != NULL)
    {
      /*@-exposetrans@*/ /*@-dependenttrans@*/
      cppReader_deleteMacro (hp);
      /*@=exposetrans@*/ /*@=dependenttrans@*/
    }
}

void cppReader_saveDefinitions ()
{
  if (!context_getFlag (FLG_SINGLEINCLUDE))
    {
      cppReader_saveHashtab ();
    }
}