/*
** LCLint - annotation-assisted static program checker
** Copyright (C) 1994-2001 University of Virginia,
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
** flagSpec.c
*/

# include "lclintMacros.nf"
# include "basic.h"

static /*@only@*/ flagSpecItem flagSpecItem_create (/*@only@*/ cstring fname)
{
  flagSpecItem res = (flagSpecItem) dmalloc (sizeof (*res));
  res->name = fname;
  res->code = identifyFlag (fname);
  /* Invalid flag okay for now... */
  return res;
}

static void flagSpecItem_free (/*@only@*/ flagSpecItem fitem)
{
  cstring_free (fitem->name);
  sfree (fitem);
}

static /*@only@*/ flagSpec flagSpec_create (/*@only@*/ flagSpecItem fitem,
					    /*@only@*/ flagSpec frest)
{
  flagSpec res = (flagSpec) dmalloc (sizeof (*res));
  res->tspec = fitem;
  res->trest = frest;
  return res;
}

flagSpec flagSpec_createPlain (cstring fname)
{
  flagSpecItem fitem = flagSpecItem_create (fname);
  flagSpec res = flagSpec_create (fitem, flagSpec_undefined);
  return res;
}

flagSpec flagSpec_createOr (cstring fname, flagSpec f)
{
  return flagSpec_create (flagSpecItem_create (fname), f);
}

void flagSpec_free (flagSpec f)
{
  if (flagSpec_isDefined (f))
    {
      flagSpecItem_free (f->tspec);
      if (flagSpec_isDefined (f->trest))
	{
	  flagSpec_free (f->trest);
	}

      sfree (f);
    }
}

cstring flagSpec_unparse (flagSpec f)
{
  if (flagSpec_isDefined (f))
    {
      if (flagSpec_isDefined (f->trest))
	{
	  return message ("%s | %q", f->tspec->name, flagSpec_unparse (f->trest));
	}
      else
	{
	  return cstring_copy (f->tspec->name);
	}
    }
  else
    {
      return cstring_makeLiteral ("<*** flagSpec undefined ***>");
    }
}

cstring flagSpec_dump (flagSpec f)
{
  llassert (flagSpec_isDefined (f));
  llassert (!cstring_containsChar (f->tspec->name, '|'));
  llassert (!cstring_containsChar (f->tspec->name, '#'));

  if (flagSpec_isDefined (f->trest))
    {
      return message ("%s|%q", f->tspec->name, flagSpec_dump (f->trest));
    }
  else
    {
      return cstring_copy (f->tspec->name);
    }
}  

flagSpec
flagSpec_undump (char **s)
{
  cstring flagname;
  flagname = reader_readUntilOne (s, "#|");

  if (reader_optCheckChar (s, '|'))
    {
      return flagSpec_createOr (flagname, flagSpec_undump (s));
    }
  else
    {
      return flagSpec_createPlain (flagname);
    }
}

flagcode 
flagSpec_getDominant (flagSpec fs)
{
  llassert (flagSpec_isDefined (fs));

  /* Invalid flags? */
  return fs->tspec->code;
}

bool
flagSpec_isOn (flagSpec fs, fileloc loc)
{
  llassert (flagSpec_isDefined (fs));

  if (context_flagOn (fs->tspec->code, loc))
    {
      return TRUE;
    }
  else if (flagSpec_isDefined (fs->trest))
    {
      return flagSpec_isOn (fs->trest, loc);
    }
  else
    {
      return FALSE;
    }
}

flagcode
flagSpec_getFirstOn (flagSpec fs, fileloc loc)
{
  llassert (flagSpec_isDefined (fs));

  if (context_flagOn (fs->tspec->code, loc))
    {
      return fs->tspec->code;
    }
  else if (flagSpec_isDefined (fs->trest))
    {
      return flagSpec_getFirstOn (fs->trest, loc);
    }
  else
    {
      BADBRANCH;
    }
}