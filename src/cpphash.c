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
** cpphash.c
**
** Pre-processor hash table.  Derived from gnu cpp.
*/

/* Part of CPP library.  (Macro hash table support.)
   Copyright (C) 1986, 87, 89, 92-95, 1996 Free Software Foundation, Inc.
   Written by Per Bothner, 1994.
   Based on CCCP program by by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987

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
# include <string.h>
# include "cpp.h"
# include "cpplib.h"
# include "cpphash.h"

typedef /*@only@*/ HASHNODE *o_HASHNODE;

static o_HASHNODE hashtab[CPP_HASHSIZE]; 
static o_HASHNODE ohashtab[CPP_HASHSIZE];

static void HashNode_delete (/*@null@*/ /*@only@*/ HASHNODE *);

/* p_prev need not be defined, but isn't defined by HashNode_copy */

/*@function static unsigned int hashStep (unsigned, char) modifies nothing ; @*/
# define hashStep(old, c) (((old) << 2) + (unsigned int) (c))

/*@function static unsigned int makePositive (unsigned int) modifies nothing ; @*/
# define makePositive(v) ((v) & 0x7fffffff) /* make number positive */

static /*@null@*/ HASHNODE *
   HashNode_copy (/*@null@*/ HASHNODE *, 
		  /*@dependent@*/ HASHNODE **p_hdr, 
		  /*@dependent@*/ /*@null@*/ /*@special@*/ HASHNODE *p_prev) 
     /*@*/ ;

void cppReader_saveHashtab ()
{
  int i;

  for (i = 0; i < CPP_HASHSIZE; i++) 
    {
      ohashtab[i] = HashNode_copy (hashtab[i], &ohashtab[i], NULL);
    }
}

void cppReader_restoreHashtab ()
{
  int i;

  for (i = 0; i < CPP_HASHSIZE; i++) {
    /* HashNode_delete (hashtab[i]); */
    hashtab[i] = HashNode_copy (ohashtab[i], &hashtab[i], NULL);
  }  
}

static void HashNode_delete (/*@only@*/ /*@null@*/ HASHNODE *node) 
{
  if (node == NULL) 
    {
      ;
    } 
  else 
    {
      HashNode_delete (node->next);
      
      if (node->type == T_MACRO)
	{
	  DEFINITION *d = node->value.defn;
	  struct reflist *ap, *nextap;
	  
	  for (ap = d->pattern; ap != NULL; ap = nextap)
	    {
	      nextap = ap->next;
	      sfree (ap);
	    }
	  
	  if (d->nargs >= 0) 
	    {
	      sfree (d->args.argnames);
	    }
	  
	  sfree (d);
	}
      
      cstring_free (node->name);
      sfree (node); 
    }
}

/*@null@*/ HASHNODE *HashNode_copy (HASHNODE *node, HASHNODE **hdr, 
				    /*@dependent@*/ HASHNODE *prev)
{
  if (node == NULL) 
    {
      return NULL;
    } 
  else 
    {
      HASHNODE *res = dmalloc (sizeof (*res));
      
      res->next = HashNode_copy (node->next, hdr, res);
      res->prev = prev;
      
      res->bucket_hdr = hdr;
      res->type = node->type;
      res->length = node->length;
      res->name = cstring_copy (node->name);
      
      if (node->type == T_MACRO)
	{
	  DEFINITION *d = node->value.defn;
	  DEFINITION *nd = dmalloc (sizeof (*nd));
	  
	  res->value.defn = nd;
	  nd->nargs = d->nargs;
	  
	  nd->length = d->length;
	  nd->predefined = d->predefined;
	  nd->expansion = d->expansion; 
	  nd->line = d->line;
	  nd->file = d->file; 
	  
	  if (d->pattern != NULL) 
	    {
	      struct reflist *ap, *nextap;
	      struct reflist **last = &nd->pattern;
	      
	      for (ap = d->pattern; ap != NULL; ap = nextap) 
		{
		  struct reflist *npattern = dmalloc (sizeof (*(nd->pattern)));
		  
		  nextap = ap->next;
		  
		  if (ap == d->pattern) 
		    {
		      *last = npattern;
		      /*@-branchstate@*/ 
		    } /*@=branchstate@*/ /* npattern is propagated through loop */
		  
		  last = &(npattern->next);
		  npattern->next = NULL; /* will get filled in */
		  npattern->stringify = d->pattern->stringify;
		  npattern->raw_before = d->pattern->raw_before;
		  npattern->raw_after = d->pattern->raw_after;
		  npattern->rest_args = d->pattern->rest_args;
		  npattern->argno = d->pattern->argno;
		  /*@-mustfree@*/ 
		}
	      /*@=mustfree@*/
	    } 
	  else 
	    {
	      nd->pattern = NULL;
	    }
	  
	  if (d->nargs >= 0) 
	    {
	      llassert (d->args.argnames != NULL);
	      
	      nd->args.argnames = mstring_copy (d->args.argnames);
	    } 
	  else 
	    {
	      /*
	      ** This fix found by:
	      **
	      **    Date: Mon, 31 May 1999 15:10:50 +0900 (JST)
	      **    From: "N.Komazaki" <koma@focs.sei.co.jp>
	      */

	      /*! why doesn't lclint report an error for this? */
	      nd->args.argnames = mstring_createEmpty ();
	    }
	} 
      else 
	{
	  if (node->type == T_CONST) 
	    {
	      res->value.ival = node->value.ival;
	    } 
	  else if (node->type == T_PCSTRING) 
	    {
	      res->value.cpval = mstring_copy (node->value.cpval);
		  llassert (res->value.cpval != NULL);
	    } 
	  else 
	    {
	      res->value = node->value;
	    }
	}
      
      /*@-uniondef@*/ /*@-compdef@*/ /* res->prev is not defined */
      return res;
      /*@=uniondef@*/ /*@=compdef@*/
    }
}

/* Return hash function on name.  must be compatible with the one
   computed a step at a time, elsewhere  */

int
hashf (const char *name, int len, int hashsize)
{
  unsigned int r = 0;

  while (len-- != 0)
    {
      r = hashStep (r, *name++);
    }

  return (int) (makePositive (r) % hashsize);
}

/*
** Find the most recent hash node for name name (ending with first
** non-identifier char) cppReader_installed by install
**
** If LEN is >= 0, it is the length of the name.
** Otherwise, compute the length by scanning the entire name.
**
** If HASH is >= 0, it is the precomputed hash code.
** Otherwise, compute the hash code.  
*/

/*@null@*/ HASHNODE *cppReader_lookup (char *name, int len, int hash)
{
  const char *bp;
  HASHNODE *bucket;

  if (len < 0)
    {
      for (bp = name; isIdentifierChar (*bp); bp++) 
	{
	  ;
	}

      len = bp - name;
    }

  if (hash < 0)
    {
      hash = hashf (name, len, CPP_HASHSIZE);
    }

  bucket = hashtab[hash];

  while (bucket != NULL) 
    {
      if (bucket->length == len && 
	  cstring_equalLen (bucket->name, cstring_fromChars (name), len)) 
	{
	  return bucket;
	}
      
      bucket = bucket->next;
    }

  return NULL;
}

/*@null@*/ HASHNODE *cppReader_lookupExpand (char *name, int len, int hash)
{
  HASHNODE *node = cppReader_lookup (name, len, hash);

  DPRINTF (("Lookup expand: %s", name));

  if (node != NULL) 
    {
      if (node->type == T_MACRO)
	{
	  DEFINITION *defn = (DEFINITION *) node->value.defn;
	
	  DPRINTF (("Check macro..."));

	  if (defn->noExpand) {
	    DPRINTF (("No expand!"));
	    return NULL;
	  }
	}
    }
  
  return node;
}

/*
 * Delete a hash node.  Some weirdness to free junk from macros.
 * More such weirdness will have to be added if you define more hash
 * types that need it.
 */

/* Note that the DEFINITION of a macro is removed from the hash table
   but its storage is not freed.  This would be a storage leak
   except that it is not reasonable to keep undefining and redefining
   large numbers of macros many times.
   In any case, this is necessary, because a macro can be #undef'd
   in the middle of reading the arguments to a call to it.
   If #undef freed the DEFINITION, that would crash.  */

void
cppReader_deleteMacro (HASHNODE *hp)
{
  if (hp->prev != NULL) 
    {
      /*@-mustfree@*/
      hp->prev->next = hp->next;
      /*@=mustfree@*/
      /*@-branchstate@*/ 
    } /*@=branchstate@*/ 
  
  if (hp->next != NULL) 
    {
      hp->next->prev = hp->prev;
    }
  
  /* make sure that the bucket chain header that
     the deleted guy was on points to the right thing afterwards.  */
  if (hp == *hp->bucket_hdr) {
    *hp->bucket_hdr = hp->next;
  }
  
  if (hp->type == T_MACRO)
    {
      DEFINITION *d = hp->value.defn;
      struct reflist *ap, *nextap;

      for (ap = d->pattern; ap != NULL; ap = nextap)
	{
	  nextap = ap->next;
	  sfree (ap);
	}

      if (d->nargs >= 0)
	{
	  sfree (d->args.argnames);
	}
    }

  /*@-dependenttrans@*/ /*@-exposetrans@*/ /*@-compdestroy@*/ 
  sfree (hp); 
  /*@=dependenttrans@*/ /*@=exposetrans@*/ /*@=compdestroy@*/
}

/* Install a name in the main hash table, even if it is already there.
     name stops with first non alphanumeric, except leading '#'.
   caller must check against redefinition if that is desired.
   cppReader_deleteMacro () removes things installed by install () in fifo order.
   this is important because of the `defined' special symbol used
   in #if, and also if pushdef/popdef directives are ever implemented.

   If LEN is >= 0, it is the length of the name.
   Otherwise, compute the length by scanning the entire name.

   If HASH is >= 0, it is the precomputed hash code.
   Otherwise, compute the hash code.  */

HASHNODE *cppReader_install (char *name, int len, enum node_type type, 
			     int ivalue, char *value, int hash)
{
  HASHNODE *hp;
  int i, bucket;
  char *p, *q;

  if (len < 0) {
    p = name;

    while (isIdentifierChar (*p))
      {
	p++;
      }

    len = p - name;
  }

  if (hash < 0) 
    {
      hash = hashf (name, len, CPP_HASHSIZE);
    }

  i = sizeof (*hp) + len + 1;

  
  hp = (HASHNODE *) dmalloc (size_fromInt (i));
  bucket = hash;
  hp->bucket_hdr = &hashtab[bucket];

  hp->next = hashtab[bucket];
  hp->prev = NULL;

  if (hp->next != NULL) 
    {
      hp->next->prev = hp;
    }

  hashtab[bucket] = hp;

  hp->type = type;
  hp->length = len;

  if (hp->type == T_CONST)
    {
      hp->value.ival = ivalue;
      llassert (value == NULL);
    }
  else
    {
      hp->value.cpval = value;
    }
  
  {
    char *tmp = ((char *) hp) + sizeof (*hp);
    p = tmp;
    q = name;

    for (i = 0; i < len; i++)
      {
	*p++ = *q++;
      }
    
    tmp[len] = '\0';
    hp->name = cstring_fromChars (tmp);
  }

  /*@-mustfree@*/ /*@-uniondef@*/ /*@-compdef@*/
  return hp;
  /*@=mustfree@*/ /*@=uniondef@*/ /*@=compdef@*/
}

HASHNODE *cppReader_installMacro (char *name, int len, 
				  struct definition *defn, int hash)
{
  return cppReader_install (name, len, T_MACRO, 0, (char  *) defn, hash);
}

void
cppReader_hashCleanup (void)
{
  int i;

  for (i = CPP_HASHSIZE; --i >= 0; )
    {
      while (hashtab[i] != NULL)
	{
	  cppReader_deleteMacro (hashtab[i]);
	}
    }
}