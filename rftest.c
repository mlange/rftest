static char RCS_Id[] = "$Id: rftest.c,v 1.6 1997/01/25 20:15:09 dmp_dp Exp $";

/*
 **********************************************************************

 rftest.c - Main source code for the RF test utility.

 **********************************************************************

 $MF_Copyright-Start$

	DISTRIBUTION MANAGER
	Copyright (c) 1996-1998
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

 $Log: rftest.c,v $
 *
 * Revision 1.9  2001/11/06  15:40:42  Matt Wagner
 * Expanded Revision 1.7 so program would not dump core when run
 * with the random character flag.
 * Added bell character at end of test to indicate when terminal has
 * completed the test.
 *
 * Revision 1.8  2001/09/05  14:25:43  Matt Wagner
 * Added 3-character rolling digit.
 * Condensed Revision 1.7 to make it easier to read.
 * Added version number to usage.
 *
 * Revision 1.7  1999/12/27  15:12:32  Damon Gross
 * Added -A option to randomize the characters in the buffer.
 *
 * Revision 1.6  1997/01/25  20:15:09  dmp_dp
 * removed dependencies on McHugh libraries so that it could be
 * built on test machines more easily using config.h.
 *
 * Revision 1.5  1997/01/23  00:54:11  dmp_dp
 * renamed from speed.c to rftest.c
 * used tc(set|get)attr rather than system("stty"), USE_STTY test macro
 * linked termid in (TERMID_BUILTIN) rather than using popen("termid")
 * added -h, -f, -L, -l, -R, -t options and usage information
 *
 * Revision 1.4  1997/01/09  22:28:19  dmp_dp
 * Derek's changes
 *
 * Revision 1.3  96/08/08  20:16:22  20:16:22  dmp_di
 * Added many options.
 * 
 * Revision 1.2  1996/04/22  21:09:05  derek
 * *** empty log message ***
 *
 * Revision 1.1  1996/02/22  15:07:55  derek
 * Initial revision 
 *
 **********************************************************************
 */

/*
 *  Preprocessor directives.
 */

#define MAIN

/* For POSIX stuff. */
#include <errno.h>
#include <fcntl.h>
#include <stddef.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/types.h>
#include <time.h> 
#include <unistd.h>

/* For select(2) stuff. */
#include <sys/time.h> 		/* for select */

#ifdef TIME_WITH_SYS_TIME
#  include <time.h> 		/* for select under HP-UX */
#endif

#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h> 	/* for select under AIX */
#endif

/* For local stuff. */
#include "config.h"
#include "termid.h"

/* For i18n stuff. */
#include <locale.h>
#include <libintl.h>

#define PACKAGE 	"rftest"
#define LOCALEDIR 	"."

#ifndef MAXPATHLEN
#  define MAXPATHLEN 1024
#endif

#define NULL_CHAR 	0
#define SLEEP_MIN 	0
#define SLEEP_MAX 	20

#define DELIMS 		", \t"

#define DA_SEQUENCE 	"\033[c"
#define CRLF 		DA_SEQUENCE "\r\n"
#define NOCRLF 		DA_SEQUENCE


/*
 *  Global variables.
 */
static int MAJOR_VERSION = 1;
static int MINOR_VERSION = 9;
static int BUILD_NUMBER = 11;

static FILE *detail;

static int sleepmax = SLEEP_MAX;
static int sleepmin = SLEEP_MIN;

static struct termios saved;

static struct
{
   char *args;
   unsigned int h; 	/* Help */
   unsigned int p; 	/* Prime-the-pump mode */
   unsigned int q; 	/* Quiet Mode */
   unsigned int r; 	/* Run ID */
   unsigned int f; 	/* output filename printf such as "rf_detail.%d%s" */
   unsigned int L; 	/* loop "forever", maintain runnum in file specified */
   unsigned int l; 	/* loop count */
   unsigned int R; 	/* Run number file/lock file */
   unsigned int t; 	/* Termid timeout */
   char *r_arg;
   unsigned int s; 	/* Sleep Mode */
   unsigned int v; 	/* Query for the terminal id using built-in code */
   unsigned int V; 	/* Query for the terminal id using external termid command */
   unsigned int i; 	/* Sleep Interval */
   char *v_arg;
   unsigned int c; 	/* Number of sends */
   size_t c_arg;
   unsigned int n; 	/* Send n characters */
   size_t n_arg;
   unsigned int d; 	/* Randomize the send n characters based on percentages */
   size_t d_arg[100];
   char *d_string[100];
   unsigned int D; 	/* Prefix buffer with rolling digit ('0' -> '9') */
   unsigned int usage;
   char *x;
   unsigned int A;      /* Randomize the buffer */

} opt = { "hpqi:r:f:sv:LR:l:t:c:n:d:Dx:V:Ax:" };

static char *program_name;


/*
 *  Function prototypes.
 */

static void usage(int verbose);
static size_t mb_pattern(char *addr, char *pattern, size_t pat_bytes, size_t chars, int seed);
static void cook(void);
static void msleep(int msec);
static void speedtest(int repeat, char *pattern, int buffersize, char sleeping);



/*
 * FUNCTION: main
 *
 * PURPOSE:  See above.
 */

int main(int argc, char *argv[])
{
   static char filename[MAXPATHLEN];
   static char file_printf[MAXPATHLEN] = "rf_detail.%d%s";
   int repeat, buffersize;
   char *separator = "";
   char *runid = "";
   char *term_id = "";
   char *vendor = (char *)0;
   int c;
   size_t loop_count = 1; /* default */
   size_t timeout = 15; /* default */
   size_t run_num = 1;
   char *s;
   char *run_file = (char *)0;
   static struct termios t;
   time_t prev_mtime = 0; /* the epoch */
   extern int optind;
   extern char *optarg;
   char *string = (char *)0;

   setlocale(LC_ALL, "");
   {
      char *home = getenv("HOME");
      if ((char *)0 == home) {
	 fprintf(stderr, "HOME not set!\n");
	 exit(EXIT_FAILURE);
      }
      bindtextdomain(PACKAGE, home);
   }
   textdomain(PACKAGE);

   setbuf(stdout, NULL); /* make standard output unbuffered */

   if ((char *)0 != (s = strrchr(argv[0], '/')))
      program_name = s + sizeof *"/";
   else 
	program_name = argv[0];

   opt.n_arg = -1;
   while (EOF != (c = getopt(argc, argv, opt.args)))
   {
      switch (c)
      {
	 case 'h':		/* Help */
	    opt.h++;
	    break;

	 case 's':		/* Sleep Mode */
	    opt.s++;
	    break;

	 case 'q':		/* Quiet Mode */
	    opt.q++;
	    break;

	 case 'p': 		/* Prime-the-pump Mode */
	    opt.p++;
	    break;

	 case 'v':		/* Auto-vendor ID */
	    opt.v++;
	    separator=".";
	    vendor = optarg;
	    break;

	 case 'V': 		/* Auto-vendor ID, force use of termid program */
	    opt.V++;
	    separator=".";
	    { /* For scope */
	       char cmdline[100];
	       static char buf[100];
	       FILE *fp;

	       sprintf(cmdline, "termid -t%lu %s", (unsigned long)timeout, optarg);
	       if ((FILE *)0 == (fp=(FILE *)popen(cmdline, "r")))
	       {
		  fprintf(stderr, "Error from termid: %d - %s\n", errno, strerror(errno));
		  opt.usage++;
	       }
	       if (1 == fscanf(fp, "%s", buf))
		  term_id = buf;
	       pclose(fp);
	    }
	    break;

	 case 'r':		/* Run ID */
	    opt.r++;
	    separator=".";
	    runid = optarg;
	    break;

	 case 'l':		/* Run loop count */
	    opt.l++;
	    separator=".";
	    { /* for scope */
	       char *unconv;
	       loop_count = strtoul(optarg, &unconv, 10);
	       if ('\0' != *unconv || 0 == loop_count) opt.usage++;
	    }
	    break;

	 case 'L':		/* loop "forever" */
	    opt.L++;
	    separator=".";
	    break;

	 case 'R':		/* Run number/lock file name */
	    opt.R++;
	    run_file = optarg;
	    break;

	 case 'f':		/* Filename printf format */
	    opt.f++;
	    strcpy(file_printf, optarg);
	    break;

	 case 't':		/* Termid timeout */
	    opt.t++;
	    { /* for scope */
	       char *unconv;
	       timeout = strtoul(optarg, &unconv, 10);
	       if ('\0' != *unconv) opt.usage++;
	    }
	    break;

	 case 'i':		/* Sleep Interval */
            opt.i++;
            sleepmax=atoi(optarg);
            break;
 
	 case 'c': /* count */
	    opt.c++;
	    { /* for scope */
	       char *unconv;
               opt.c_arg = strtoul(optarg, &unconv, 10);
               if ('\0' != *unconv) opt.usage++;
	    }
	    break;

	 case 'n': 		/* Send n characters */
	    opt.n++;
	    { /* for scope */
	       char *unconv;
               opt.n_arg = strtoul(optarg, &unconv, 10);
               if ('\0' != *unconv) opt.usage++;
	    }
	    break;

	 case 'd': 		/* Do the specified distribution (by percentages) */
	    opt.d++;
	    { /* parse an option like "-d60%20,25%60,5%200,10%400" */
	       size_t index = 0;
	       char *arg, *token;
	       size_t p, n;
	       arg = malloc(sizeof "" + strlen(optarg));
	       strcpy(arg, optarg);
	       token = strtok(arg, DELIMS);
	       do {
		  {
		     char *s = token, *unconv;
		     p = strtoul(s, &unconv, 10);
		     if ('\0' == *unconv || '%' != *unconv) {
		        /* looking for '%' ... */
		        opt.usage++;
			break;
		     }

		     s = unconv+sizeof *"%";
		     n = strtoul(s, &unconv, 10);
		     if ('\0' == *unconv) {
		        string = "X";
		     } else {
		        string = unconv;
		     }
		     string = gettext(string);
		  }
		  for (; p > 0; p--) {
		     if (100 == index) { /* we've exceeded 100% */
		        opt.usage++;
			goto opt_d_label;
		     }
		     opt.d_arg[index] = n;
		     opt.d_string[index] = string;
		     index++;
		  }
	       } while (token = strtok((char *)0, DELIMS));
	       if (100 != index) { /* percentages must add to 100 */
	          opt.usage++;
	       }
	    opt_d_label: ;
	    }
	    break;

	 case 'D':
	    opt.D++;
	    break;

         case 'A':
            opt.A++;
            break; 

         case 'x':		/* This is just a dummy really */
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

   if (3 == argc) /* for backward compatibility... */
   { /* argv[0], count, and size should be left */
      char *unconv;
      opt.c++;
      opt.c_arg = strtoul(argv[1], &unconv, 10);
      if ('\0' != *unconv) opt.usage++;
      opt.n++;
      if (0 == strcmp("dist", argv[2])) {
         opt.n_arg = 0;  /* this magic # means to do a default distribution */
      } else {
         opt.n_arg = strtoul(argv[2], &unconv, 10);
         if ('\0' != *unconv) opt.usage++;
      }
      argv+=2;
      argc-=2;
   }

   if (!(opt.n || opt.d) || !opt.c) 
	opt.usage++;
   if (1 < opt.d + opt.n) 
	opt.usage++;
   if (1 != argc) 
	opt.usage++;

   /* opt.L and opt.l (loops) exclude the use of opt.r (fixed run number) */
   if ((opt.L + opt.l > 1) && opt.r) opt.usage++;

   /* opt.L and opt.l are mutually exclusive */
   if (opt.L + opt.l > 1) opt.usage++;

   /* opt.f requires (opt.L or opt.l) because we need a run number */
   if (opt.f && 1 != (opt.L + opt.l)) opt.usage++;

   /* opt.L and opt.l require opt.R and vice-versa */
   if ((opt.L + opt.l > 0) && 0 == opt.R) opt.usage++;
   if (opt.R && 0 == (opt.L + opt.l)) opt.usage++;

   /* opt.R and opt.r are mutually exclusive */
   if ((opt.R + opt.r) > 1) opt.usage++;

   /* opt.f requires opt.v (must have terminal ID) */
   if (opt.f && !opt.v) opt.usage++;

   if (opt.usage)
   {
      usage(opt.h);
      exit(opt.h? EXIT_SUCCESS : EXIT_FAILURE);
   }

   if (opt.v)
   {
      size_t ii;

      /*
       * Did they pass in a valid vendor code?
       */

      for (ii=0; vendors[ii].vendorstr; ii++)
      {
         if (0 == strcmp(vendors[ii].vendorstr, vendor)) break;
      }

      if ((char *)0 == vendors[ii].vendorstr)
      {
         fprintf(stderr, "Invalid RF vendor - \n"
		         "Possible vendors are:");
         for (ii=0; vendors[ii].vendorstr; ii++)
         {
	    fprintf(stderr, " %s", vendors[ii].vendorstr);
         }
         fprintf(stderr, "\n");
         exit(EXIT_FAILURE); }

      term_id = termid(vendors+ii, timeout);
      if ((char *)0 == term_id)
      {
         fprintf(stderr, "Could not get terminal ID.\n");
         exit(EXIT_FAILURE);
      }
   }

   /* Save our original tty settings. */
   if (tcgetattr(STDERR_FILENO, &saved))
   {
      fprintf(stderr, "tcgetattr failed - %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* Set up the parameters for our new tty settings. */
   t = saved;
   t.c_iflag &= ~(ISTRIP | IXON | IXOFF | BRKINT | IGNPAR | ICRNL);
   t.c_oflag &= ~(OPOST);
   t.c_cflag &= ~(PARENB);
   t.c_cflag |= (CS8);
   t.c_lflag &= ~(ECHO | ICANON | ISIG);
   t.c_cc[VMIN] = 1;
   t.c_cc[VTIME] = 1;
   t.c_cc[VEOF] = 1;
   t.c_cc[VEOL] = 1;
   /* Set our new tty settings. */
   if (tcsetattr(STDERR_FILENO, TCSADRAIN, &t))
   {
      fprintf(stderr, "tcsetattr failed - %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   /* Make sure our original tty settings are put back on exit. */
   atexit(cook);
   /* Print Copyright Information */
   fprintf(stderr, "Copyright (c) 2001-2002 RedPrairie.  All rights reserved.\r\n");
   do {

      if (opt.L || opt.l) { /* loop, starting a test whenever a file appears  */
         static struct stat sb;

         printf("Waiting for modification of %s...\r\n", run_file);
	 do {
            if (0 == stat(run_file, &sb)) {
	       if (0 == prev_mtime) {
		  /* Save this modification time so we can tell if the file has been modified */
	          prev_mtime = sb.st_mtime;
	       } else {
	          if (sb.st_mtime > prev_mtime) { /* the file has been touched */
	             prev_mtime = sb.st_mtime;
		     break;
	          }
	       }
	    } else {

               if (0 != prev_mtime) {
		
		  /* The file must have been unlink(2)ed */
                  fputs("Aborted...\r\n", stderr);
                  exit(EXIT_FAILURE);

               }

	    }
            sleep(1);

         } while (1);
      }

      if (opt.L) {

	 /* Open the run_file and set run_num accordingly */
	 FILE *runfp = fopen(run_file, "r");
	 if ((FILE *)0 == runfp) {
	    fprintf(stderr, "Error opening \"%s\" for read: %d - %s\r\n", run_file, errno, strerror(errno));
	    exit(EXIT_FAILURE);
	 }
	 if (1 != fscanf(runfp, "%d", &run_num)) {
	    fprintf(stderr, "Error reading integer from file \"%s\"\r\n", run_file);
	    exit(EXIT_FAILURE);
	 }
	 fclose(runfp);
      }

      /* Open the detail log file */
      if (opt.L || opt.l || opt.f) {
         sprintf(filename, file_printf, run_num, term_id);
      } else {
	 sprintf(filename, "./rf_detail%s%s%s", separator, runid, term_id);
      }
      detail = fopen(filename, "a");
   
      /* How many times to repeat the test */
      repeat = opt.c_arg;
   
      /* How big a test to perform */
      if (0 == opt.n_arg) /* for backward compatibility */
      { /* do a default distribution */
         struct timeval tmp_tm;
         /* struct timezone tz; */
	 unsigned long tz;
         int which;
   
         gettimeofday(&tmp_tm,&tz);
         srand(tmp_tm.tv_sec | tmp_tm.tv_usec);
         which = rand()%100;
         if (which < 60) buffersize = 20;
         else if (which < 85) buffersize = 60;
         else if (which < 90) buffersize = 200;
         else buffersize = 400;
      } else if (opt.d) { /* do the specified distribution (by percentages) */
         struct timeval tmp_tm;
         /* struct timezone tz; */
	 unsigned long tz;

         gettimeofday(&tmp_tm,&tz);
         srand(tmp_tm.tv_sec | tmp_tm.tv_usec);
	 {
	    size_t index = rand()%100;
	    buffersize = opt.d_arg[index];
	    string = gettext(opt.d_string[index]);
	 }
      } else {
         buffersize = opt.n_arg;
	 string = gettext("Message 1");
      }

      speedtest(repeat, string, buffersize, opt.s);

      fclose(detail);

      if (opt.l) {
	 run_num++;
	 loop_count -= 1;
      }
      if (0 == (opt.L + opt.l)) break; /* we're not looping */

   } while (loop_count);

   exit (EXIT_SUCCESS);
}


/* 
 * FUNCTION: usage
 *
 * PURPOSE:  Display command line usage to stderr.
 *
 * RETURNS:  void
 */

static void usage(int verbose) {
   fprintf(stderr, "RF Terminal test program, version %d.%d.%d\n", MAJOR_VERSION, MINOR_VERSION, BUILD_NUMBER);
   fprintf(stderr, "Copyright (c) 2001-2002 RedPrairie.  All rights reserved.\n");
   fprintf(stderr, "usage: %s [-hpqs] [[-ttimeout] -vvendor] [[-L | -lrun_loop_count] -Rrun_file] [-rrun_name] [-ffile_printf] [-nchars | -dp%%n,p%%n,... ] -ccount [-A]\n", ((char *)0 == program_name)? "speed" : program_name);

   if (verbose) {
      fprintf(stderr, "             -h\t- help (displays this information)\n");
      fprintf(stderr, "             -p\t- \"Prime the pump\" mode (do a priming send to \"wake-up\" the RF terminal)\n");
      fprintf(stderr, "             -q\t- Quiet mode (use '\\0' rather than 'X' characters)\n");
      fprintf(stderr, "             -s\t- Sleep mode (sleep for a random time between sending strings)\n");
      fprintf(stderr, "             -ttimeout\t- Timeout to read terminal ID (in seconds)\n");
      fprintf(stderr, "             -vvendor\t- RF terminal Vendor (as passed to termid command)\n");
      fprintf(stderr, "             -L\t- loop indefinitely\n");
      fprintf(stderr, "             -lrun_loop_count\t- Loop for this many \"runs\"\n");
      fprintf(stderr, "             -Rrun_file\t- file containing run number (reqiures -L or -l)\n");
      fprintf(stderr, "             -rrun_name\t- name of single Run\n");
      fprintf(stderr, "             -ffile_printf\t- a printf format containing %%d followed by %%s\n");
      fprintf(stderr, "             -ccount\t- number of strings to send\n");
      fprintf(stderr, "             -nchars\t- number of characters in strings\n");
      fprintf(stderr, "             -dpercent%%nchars,percent%%nchars,...\t- randomly select nchars according to percentages\n");
      fprintf(stderr, "             -D - overlay 1st byte of string with a rolling char ('0' -> '9')\n");
      fprintf(stderr, "             -A - randomize the string \n");
      fprintf(stderr, "\nExample -\n$ rftest -vINT -L -Rrunnum -frf_detail.%%02d%%s -c100 -d60%%20,25%%60,5%%200,10%%400 -D -A\n");
  }
}


/* 
 * FUNCTION: mb_pattern
 *
 * PURPOSE:  Populate a buffer using a pattern of multi-byte characters.
 *
 * RETURNS:  The number of multi-byte charactes copied.
 *
 * BUGS:     This function assumes that the buffer at addr is large enough
 *           to hold however many bytes chars multi-byte chars will occupy.
 */

static size_t mb_pattern(char *addr,		/* pointer destination buffer */
                         char *pattern,		/* pointer to pattern of multi-byte characters */
	                 size_t pat_bytes,	/* length, in bytes, of pattern */
	                 size_t chars,  	/* number of multi-byte charactes to put at addr */      int seed               /* seed for random number generation */)
{
   register char *s;
   register char *p;
   register size_t pbytes;
   register size_t mbl;
   int alpha;

   srand (seed + time(NULL)); /* Seeds the Random Generator to seed*/
   
   for (pbytes = pat_bytes, s = addr, p = pattern;
        0 < chars;
	s += mbl, p += mbl, chars--, pbytes -= mbl)
   {

      if (0 == pbytes)
      {
         pbytes = pat_bytes;
         p = pattern;
      }
 
	/* by: M.Wagner - Revision 1.9
	   * Added case statement.  Terminals core dumped when the
           * case statement was in the code.
           * by: M.Wagner - Revision 1.8
	   * Removed case statement to make code smaller
	   * by: Damon Gross - Revision 1.7 
	   * The following was added to provide more realistic terminal results 
	   */
      if (opt.A)
      {
        alpha = 1 + rand() % 26;
        /* The following code causes a core dump when run
         *
	alpha += 64;				/* Convert to upper case ASCII char.*/
	/* *p = (char)alpha;
         *
         */

	 /* Revision 1.9 */
         switch (alpha) 
         {
	  case 1:
	   p = (char *) "A";
	   break;
	  case 2:
	   p = (char *) "B";
	   break;
	  case 3:
	   p = (char *) "C";
	   break;
	  case 4:
	   p = (char *) "D";
	   break;
	  case 5:
	   p = (char *) "E";
	   break;
	 case 6:
		p = (char *)"F";
		break;
	 case 7:
		p = (char *)"G";
		break;
	 case 8:
		p = (char *)"H";
		break;
	 case 9:
		p = (char *)"I";
		break;
	 case 10:
		p = (char *)"J";
		break;
	 case 11:
		p = (char *)"K";
		break;
	 case 12:
		p = (char *)"L";
		break;
	 case 13:
		p = (char *)"M";
		break;
	 case 14:
		p = (char *)"N";
		break;
	 case 15:
		p = (char *)"O";
		break;
	 case 16:
		p = (char *)"P";
		break;
	 case 17:
		p = (char *)"Q";
		break;
	 case 18:
		p = (char *)"R";
		break;
	 case 19:
		p = (char *)"S";
		break;
	 case 20:
		p = (char *)"T";
		break;
	 case 21:
		p = (char *)"U";
		break;
	 case 22:
		p = (char *)"V";
		break;
	 case 23:
		p = (char *)"W";
		break;
	 case 24:
		p = (char *)"X";
		break;
	 case 25:
		p = (char *)"Y";
		break;
	 case 26:
		p = (char *)"Z";
		break;
	 } 
      }
      /* End Revision 1.7 */
      /* End Revision 1.8 */
      /* End Revision 1.9 */

      else 
        p = pattern;

      mbl = mblen(p, pat_bytes - (p - pattern));
      memcpy(s, p, mbl);
   }
   return(s - addr);
}


/* 
 * FUNCTION: cook
 *
 * PURPOSE:  Return the tty back to its original settings.
 *
 * RETURNS:  void
 */

static void cook(void) {

   if (tcsetattr(STDERR_FILENO, TCSADRAIN, &saved))
   {
      fprintf(stderr, "tcsetattr failed - %s\n", strerror(errno));
      exit(EXIT_FAILURE);
   }

   return;
}


/*
 * FUNCTION: msleep
 *
 * PURPOSE:  Provide a high-granularity sleep utility via select.
 *
 * RETURNS:  void
 */
static void msleep(int msec)
{
   struct timeval tv;

   tv.tv_sec  = msec/1000;
   tv.tv_usec = (msec%1000)*1000;
   select(0, 0, 0, 0, &tv);

   return;
}

/*
 * FUNCTION: speedtest
 *
 * PURPOSE:  Perform the actual RF speed tests.
 *
 * RETURNS:  void
 */

static void speedtest(int repeat, char *pattern, int buffersize, char sleeping)
{
   static char buffer[2048];
   char *primebuffer;
   int primelen;
   struct tm tm1, tm2;
   time_t time1, time2;
   struct timeval *start, *end, total;
   /* struct timezone tz; */
   unsigned long tz;
   int seqsize;
   int i;

   if (0 == buffersize) {
      buffersize = strlen(pattern);
      if (buffersize < 6) {
         buffersize = 6; /* FIXME - this is quietly enforced - probably should
	                    generate usage instead.
	                    the length must be at least this big cuz we will
			    subtract for the CRLF, etc. */
      }
   }

   /* Allocate timevals for all tests */
   start = malloc((sizeof *start) * repeat);
   end   = malloc((sizeof *end) * repeat);

   /* Set up "priming" buffer */
   primebuffer = NOCRLF;
   primelen = strlen(primebuffer);

   if (opt.q)
      seqsize = sizeof(NOCRLF)-sizeof("");
   else
      seqsize = sizeof(CRLF)-sizeof("");

   /* If buffer <= our sequence size, that many chars anyway */
   if (buffersize > seqsize)
   {
      if (opt.q) {
         memset(buffer, NULL_CHAR, buffersize-seqsize);
         strcpy(buffer+buffersize-seqsize, NOCRLF);
      } else {
	 size_t bytes;
         bytes = mb_pattern(buffer, pattern, strlen(pattern), buffersize-seqsize,0);
         strcpy(buffer+bytes, CRLF);
      }
   }
   else
   {
      strcpy(buffer, NOCRLF);
      buffersize = sizeof(NOCRLF) - sizeof("");
   }

   /* Seed the random number generator with a semi-random seed (the time) */
   gettimeofday(&start[0],&tz);
   srand(start[0].tv_sec | start[0].tv_usec);

   time(&time1);

   for (i=0;i<repeat;i++)
   {
      char text;

      /* Figure out a random sleep interval.  */
      if (sleeping)
	 msleep((rand()%((sleepmax-sleepmin)*1000))+sleepmin*1000);

      /* If we're "priming", send the primebuffer */
      if (opt.p)
      {
	 /* Prime the pump and ignore the response time. */
	 write(1, primebuffer, primelen);
	 while(read(0, &text,1)) if (text == 'c') break;
      }

      if (opt.A) 
       { 
         size_t bytes_n;
         bytes_n = mb_pattern (buffer, pattern, strlen(pattern), buffersize-seqsize, i);
         strcpy (buffer + bytes_n, CRLF);
       } 

      if (opt.D) { /* Make the first three characters rolling digit */
	buffer[0] = '0' + (((i-(i%100))/100)%10);  /* Added by M.Wagner */
	buffer[1] = '0' + (((i-(i%10))/10)%10);	   /* Added by M.Wagner */
        buffer[2] = '0' + (i%10);		   /* Modified by M.Wagner */
      }
 
      /* Send the buffer */
      write(1, buffer, buffersize);
      gettimeofday(&start[i], &tz);
      while(read(0, &text,1)) if (text == 'c') break;
      gettimeofday(&end[i], &tz);

   }

   time(&time2);

   tm1 = *(localtime(&time1));
   tm2 = *(localtime(&time2));

   fprintf(detail, "-----\nStarted %02d:%02d:%02d\n",
	   tm1.tm_hour, tm1.tm_min, tm1.tm_sec);

   total.tv_sec = 0;
   total.tv_usec = 0;

   for (i=0;i<repeat;i++)
   {
      unsigned long sec;
      long usec;
      sec = end[i].tv_sec - start[i].tv_sec;
      usec = end[i].tv_usec - start[i].tv_usec;
      if (usec < 0) { usec += 1000000; sec -= 1; }
      fprintf(detail, " +%d.%03d %d\n", sec, usec/1000, buffersize);
      total.tv_sec += sec;
      total.tv_usec += usec;
      if (total.tv_usec > 1000000) {total.tv_usec -= 1000000;total.tv_sec++;}
   }

   fprintf(detail, "%d - %d: (%d seconds) (tx: %d.%03d)\n",
	   repeat,buffersize,time2-time1, total.tv_sec, total.tv_usec/1000);
   system("clear");
   /* Add a bell character so we know when the terminal is done. */
   fprintf(stderr, "");			/* Added by M.Wagner */
   fprintf(stderr, "Test Complete\r\n-------------\r\n%d Transactions\r\n%d Bytes\r\n%d Seconds\r\nTX: %d.%03d\r\n",
	   repeat,buffersize,time2-time1, total.tv_sec, total.tv_usec/1000);
   
   free(end);
   free(start);

   return;
}
