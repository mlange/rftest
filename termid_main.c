static char RCS_Id[] = "$Id: main.c,v 1.2 1997/01/25 21:15:08 dmp_dp Exp $";
/*
 **********************************************************************

 termid_main - Main source code to retreive a terminal id.

 **********************************************************************

 $MF_Copyright-Start$

	DISTRIBUTION MANAGER
	Copyright (c) 1997
	McHugh Software International
	Waukesha, Wisconsin

 This software is furnished under a corporate license for use on a
 single computer system and can be copied (with inclusion of the
 above copyright) only for use on such a system.

 The information in this document is subject to change without notice
 and should not be construed as a commitment by McHugh Software 
 International.

 McHugh Software International assumes no responsibility for the use of
 the software described in this document on equipment which has not been
 supplied or approved by McHugh Software International.

 $MF_Copyright-End$

 Modification History

 * $Log: main.c,v $
 * Revision 1.2  1997/01/25  21:15:08  dmp_dp
 * conditionally used McHugh headers/library functions so that
 * this can be built more easily on an RF testing machine
 * introduced config.h (GNU configure) and other headers
 * renamed termid_main to termid and changed its arguments
 * moved getopts processing from termid function to main
 *
 * Revision 1.1  1997/01/23  00:46:17  dmp_dp
 * Initial revision
 *

 **********************************************************************
 */


/*
 *  Preprocessor directives.
 */

#define MAIN

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

#include "termid.h"


/*
 *  Global variables.
 */

static struct
{

   char *args;
   unsigned int h; /* help */
   unsigned int t; /* timeout */
   unsigned int usage;

} opt = { "ht:" };

static char *program_name;


/*
 *  Function prototypes.
 */

static void usage(int verbose);


/*
 * FUNCTION: main
 * 
 * PURPOSE:  See above.
 */

int main(int argc, char *argv[])
{
   char *s;
   int ii;
   int c;
   unsigned long timeout = 15;
   extern char *optarg;
   extern int optind;

   if ((char *)0 != (s = strrchr(argv[0], '/')))
      program_name = s + sizeof *"/";
   else program_name = argv[0];

   while (EOF != (c = getopt(argc, argv, opt.args)))
   {
      switch (c)
      {
	 case 'h':		/* help */
	    opt.h++;
	    break;

	 case 't':		/* termid timeout */
	    opt.t++;
	    { /* for scope */
	       char *unconv;
	       timeout = strtoul(optarg, &unconv, 10);
	       if ('\0' != *unconv) opt.usage++;
	    }
	    break;

	 case '?':
	 default:
	    opt.usage++;
      }

      if (opt.usage)
	 break;
   }

   argv+=(optind-1);
   argc-=(optind-1);

   if (opt.h)
   {
      usage(1);
      exit(EXIT_SUCCESS);
   }

   /*
    * Did they pass in a vendor code at all?
    */
   if (opt.usage || 2 != argc)
   {
      usage(0);
      exit(EXIT_FAILURE);
   }

   /*
    * Did they pass in a valid vendor code?
    */
   for (ii=0;vendors[ii].vendorstr;ii++)
   {
      if (!strcmp(vendors[ii].vendorstr, argv[1])) break;
   }

   if (!vendors[ii].vendorstr)
   {
      fprintf(stderr, "Invalid RF vendor - \n"
		      "Possible vendors are:");
      for (ii=0;vendors[ii].vendorstr;ii++)
      {
	 fprintf(stderr, " %s", vendors[ii].vendorstr);
      }
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
   }

   if ((char *)0 == (s = termid(vendors+ii, timeout)))
   {
      fprintf(stderr, "Could not get terminal ID.\n");
      exit(EXIT_FAILURE);
   }

   printf("%s\n", s);

   exit(EXIT_SUCCESS);
}


/*
 * FUNCTION: usage
 *
 * PURPOSE:  Display command line usage to stderr.
 *
 * RETURNS:  void
 */

static void usage(int verbose) {
   size_t ii;
   fprintf(stderr, "usage: %s [-h] [-ttimeout] vendor\n", ((char *)0 == program_name)? "speed" : program_name);
   if (verbose) {
      fprintf(stderr, "             -h\t- help (displays this information)\n");
      fprintf(stderr, "             -ttimeout\t- in seconds (default is 15, 0 means don't timeout)\n");
      fprintf(stderr, "             vendor\t- specify one of:");
      for (ii=0; (char *)0 != vendors[ii].vendorstr; ii++)
      {
	 fprintf(stderr, " %s", vendors[ii].vendorstr);
      }
      fprintf(stderr, "\n");
   }
}
