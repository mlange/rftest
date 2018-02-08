static char RCS_Id[] = "$Id: termid.c,v 1.21 1999/07/16 18:16:14 dmp_mjl Exp $";
/*
 **********************************************************************

 TERMID - Get an RF Terminal ID, given a particular vendor.

 **********************************************************************

 $MF_Copyright-Start$

	DISTRIBUTION MANAGER
	Copyright (c) 1994
	McHugh Freeman
	Waukesha, Wisconsin

 This software is furnished under a corporate license for use on a
 single computer system and can be copied (with inclusion of the
 above copyright) only for use on such a system.

 The information in this document is subject to change without notice
 and should not be construed as a commitment by McHugh Freeman.

 McHugh Freeman assumes no responsibility for the use of the software
 described in this document on equipment which has not been supplied
 or approved by McHugh Freeman.

 $MF_Copyright-End$

 Modification History

 $Log: termid.c,v $
 * Revision 1.21  1999/07/16  18:16:14  dmp_mjl
 * Added support for Symbol RF terminals.
 *
 * Revision 1.20  1999/05/21  22:27:43  dmp_sw
 * freeze for V5_2
 *
 * Revision 1.19  1998/11/16  22:26:35  dmp_sw
 * freeze for V5_1 (Stopped V5.1/V5.2 development)
 *
 * Revision 1.18  1998/11/09  14:38:50  dmp_ch
 * TR:44:RBUG
 * Added checks and retry of getting terminal id from Norand
 * equipment.
 *
 * Revision 1.17  1998/10/14  21:17:19  dmp_apl
 * freeze for V5_1
 *
 * Revision 1.16  1997/07/08  19:00:49  dmp_sw
 * freeze for V5_0
 *
 * Revision 1.15  1997/01/25  21:15:08  dmp_dp
 * conditionally used McHugh Freeman headers/library functions so that
 * this can be built more easily on an RF testing machine
 * introduced config.h (GNU configure) and other headers
 * renamed termid_main to termid and changed its arguments
 * moved getopts processing from termid function to main
 *
 * Revision 1.14  1997/01/23  00:47:17  dmp_dp
 * seperated main from termid function so that the function can be linked
 * into other executables
 * made local functions static
 * added getopt processing and -h and -t options
 * used tc(set|get)attr rather than system("stty"), USE_STTY test macro
 *
 * Revision 1.13  1997/01/20  16:36:52  dmp_dp
 * added Intermec support.  (changes were done by dmp_sw)
 *
 * Revision 1.12  1996/11/27  19:16:03  dmp_sw
 * freeze for V4_2
 *
 * Revision 1.11  1996/08/09  18:30:00  dmp_sw
 * freeze for V4_10
 *
 * Revision 1.10  1996/05/24  21:50:45  dmp_di
 * Added code to flush the input buffer before starting. Also changed
 * to be platform independent.
 *
 * Revision 1.9  1996/04/19  14:04:15  dmp_sw
 * freeze for V4_09
 *
 * Revision 1.8  1996/02/01  14:52:22  phx_sw
 * freeze for V4_08
 *
 * Revision 1.7  1995/11/09  20:36:42  phx_sw
 * freeze for V4_07
 *
 * Revision 1.6  1995/10/09  15:17:40  phx_dp
 * freeze for V4_06
 *
 * Revision 1.5  1995/09/07  23:41:49  phx_dp
 * freeze for V4.05
 *
 * Revision 1.4  1995/06/15  13:46:45  phx_di
 * Added Support for Norand's ENQ terminal ID response.
 *
 * Revision 1.3  1994/12/14  14:53:57  phx_di
 * Changed default timeout to 15 seconds
 *
 * Revision 1.2  1994/07/18  13:10:39  phx_di
 * Changed to handle LXE terminals, and also let the complete_respose()
 * function return a char *, rather than an int, because LXE's terminals
 * return an alphanumeric code. (It may be hex, but I see no documentation
 * to that effect.
 *
 * Revision 1.1  1994/07/13  23:02:22  phx_di
 * Initial revision
 *

 **********************************************************************

*/

#ifdef MF_SOURCE_DIR /* kludge to detect if building with vmake */
#  define MF_BUILD
#  define FEATURES ALL
#endif
#include "config.h"

#ifdef MF_BUILD
#include <mf_head.h>
#endif
/* { POSIX stuff: */
#include <errno.h> /* errno */
#include <fcntl.h> /* fcntl, F_GETFL, F_SETFL, O_NONBLOCK */
#include <stddef.h> /* NULL */
#include <stdio.h> /* sscanf, sprintf, fprintf, stderr */
#include <stdlib.h> /* system, exit, EXIT_FAILURE */
#include <string.h> /* strstr, strpbrk, strchr, strerror, memset */
#include <termios.h> /* termios, tcgetattr, ISTRIP, IXON, IXOFF, BRKINT, IGNPAR, ICRNL, OPOST, PARENB, CS8, ECHO, ICANON, ISIG, VMIN, VTIME, VEOF, VEOL, tcsetattr, TCSADRAIN */
#include <time.h> /* time */
#include <unistd.h> /* read, STDERR_FILENO */
/* } */

/* { for select(2): */
#include <sys/time.h> /* for select */
#include <sys/types.h>
#ifdef TIME_WITH_SYS_TIME
#  include <time.h> /* for select under HP-UX */
#endif
#ifdef HAVE_SYS_SELECT_H
#  include <sys/select.h> /* for select under AIX (requires <sys/types.h>) */
#endif
/* } */
#ifdef MF_BUILD
#include <mf_tail.h>
#endif

#include "termid.h"

/*
 * Flush standard input so that we don't get bad IDs because users are
 * impatient and hit return a few times.
 */
static void flush_tty(void)
{
   int flags;
   char buffer[100];

   flags = fcntl(0, F_GETFL, 0);
   fcntl(0, F_SETFL, flags|O_NONBLOCK);
   while (read(0, buffer, sizeof buffer) > 0)
      ;
   fcntl(0, F_SETFL, flags);
}

/*
 * This is the function in which we determine if the terminal has given 
 * us a complete response, and if so, what it is... There's a lot of vendor
 * specific code here, but that's the way the vendors have decided to play it.
 */
static char *complete_response(const VENDORLIST *vendor, const char *buffer)
{
   char *pos;
   int dummy  = 0;
   int termid = 0;
   static char term_id[10]; /* Terminal ID in string mode */

   switch (vendor->vendor)
   {
   case RF_TEK:
      /* 
       * TEKLOGIX
       *
       * OK, we should get a string with the substring "tnum=" in it.
       * If we do, then we must mark that position, and check that the number
       * we've received is complete. The sequence ends with either a 
       * semicolon, (More attributes returned) an ESC character, or an ST 
       * character. (ST in 7- or 8-bit mode) If not, there's a chance we 
       * didn't get the whole string.
       */
      pos = strstr(buffer, "tnum=");
      if (pos && strpbrk(pos, ";\033\234"))
      {
	 sscanf(pos, "tnum=%d",&termid);
	 sprintf(term_id, "%03d", termid);
	 return term_id;
      }
      break;
   case RF_TEL:
      /*
       * TELXON
       *
       * Telxon returns a simple string: <Number><CR>. So all we have
       * to do is wait until the <CR> gets here. We'll be good and
       * check against both \r and \n.
       */
      if (strpbrk(buffer, "\r\n"))
      {
	 sscanf(buffer, "%d", &termid);
	 sprintf(term_id, "%03d", termid);
	 return term_id;
      }
      break;
   case RF_LXE:
      if (strpbrk(buffer, "\r\n"))
      {
	 int i;
	 const char *p;
	 for (i=0,p=buffer;i<4 && (p=strchr(p,'/')+1);i++)
#ifdef _4DIGIT_
	 sscanf(p, "%4s", term_id);
#else
	 sscanf(p, "%*c%3s", term_id);
#endif
	 return term_id;
      }

      break;
   case RF_NOR:
      /*
       * NORAND
       *
       * Norand, like telxon returns a simple string: <Number><CR>.
       * Let's just wait until the <CR> gets here.
       */
      if (strpbrk(buffer, "\r\n"))
      {
	 /*
	  * Check input string, this is based on the reporting from project.
	  * Norand equipment will return unpredicted terminal id when some
	  * sequence of keystrokes are hit during startup.
	  */
         if (strlen(buffer) > 4)
	 {
	    sprintf(term_id, "ERR");
         }
         else if (!isdigit(buffer[0]) ||
		  !isdigit(buffer[1]) ||
		  !isdigit(buffer[2]))
	 {
	    sprintf(term_id, "ERR");
         }
         else
	 {
	    sscanf(buffer, "%d", &termid);
	    sprintf(term_id, "%03d", termid);
         }
	 return term_id;
      }
      break;
   case RF_INT:
      /*
       * INTERMEC
       *
       * Intermec, like telxon returns a simple string: <Number><CR>.
       * Let's just wait until the <CR> gets here.
       */
      /* if (strlen(buffer)==3 ) */
      if (strpbrk(buffer, "\r\n"))
      {
	 sscanf(buffer, "%d", &termid);
	 sprintf(term_id, "%03d", termid);
	 return term_id;
      }
      break;
   case RF_SYM:
      /*
       * SYMBOL
       *
       * Symbol returns the last two octects of the terminal's IP
       * address followed by a <CR>.  We use the last octect as 
       * the terminal id.
       *
       * Example: 123.456<CR>
       */
      if (strpbrk(buffer, "\r\n"))
      {
	 sscanf(buffer, "%d.%d", &dummy, &termid);
	 sprintf(term_id, "%03d", termid);
	 return term_id;
      }
      break;
   default:
      /* Since we don't know what to do with it, might as well ignore it */
      sprintf(term_id, "%03d", 999);
      return term_id;
   }

   return NULL;
}

char *termid(const VENDORLIST *vendor, const unsigned long timeout)
{
   fd_set fds;
   struct timeval tv;
   int nbytes;
   int attempts=0;
   char *response;
   static char buffer[100];
   char *s;
#ifndef USE_STTY
   static struct termios saved, t;
#endif

#define MAX_ATTEMPTS  10

   /* Loop while the response is ERR (Retry) (For Norand Only) */
   do
   {
      /*
       * Did they pass in a valid vendor code?
       */
      if ((VENDORLIST *)0 == vendor) return (char *)0;

      /*
       * OK, we know what vendor we're trying to identify. Now, all that's
       * left to do is to send the appropriate string and wait for the 
       * appropriate response.
       */

      /*
       * Now we flush the TTY so that no extra <RETURN>s can screw us up.
       */
      flush_tty();

      /* 
       * Let's go into raw/noecho mode, so we don't screw up the tty driver
       * with bad characters. The 8-bit DEC character ST, in particular, is
       * seen in the 7-bit telnet pty driver as a ctrl-\, which by default
       * invokes the "Quit" signal. This might cause the program to dump core.
       */

#ifdef USE_STTY
      system("stty raw -echo");
#else
      if (tcgetattr(STDERR_FILENO, &saved))
      {
         fprintf(stderr, "tcgetattr failed - %s\n", strerror(errno));
         exit(EXIT_FAILURE);
      }
      t = saved;
      /* FYI, I determined these flag values simply by mimicking the HP-UX
       * "stty raw" (which derek was using at the cost of fork(2)ing and
       * exec(2)ing stty(1)). There's no other rhyme or reason.
       * Since it sets things relative to their previous values,
       * it is expected to be run from a cooked mode... - DP
       * { */
      t.c_iflag &= ~(ISTRIP | IXON | IXOFF | BRKINT | IGNPAR | ICRNL);
      t.c_oflag &= ~(OPOST);
      t.c_cflag &= ~(PARENB);
      t.c_cflag |= (CS8);
      t.c_lflag &= ~(ECHO | ICANON | ISIG);
      t.c_cc[VMIN] = 1;
      t.c_cc[VTIME] = 1;
      t.c_cc[VEOF] = 1;
      t.c_cc[VEOL] = 1;
      if (tcsetattr(STDERR_FILENO, TCSADRAIN, &t))
      {
         fprintf(stderr, "tcsetattr failed - %s\n", strerror(errno));
         exit(EXIT_FAILURE);
      }
   /* } */
#endif

      fprintf(stderr, "%s", vendor->sendstr);

      FD_ZERO(&fds);
      FD_SET(0, &fds);
      tv.tv_sec  = timeout;
      tv.tv_usec = 0;

      memset(buffer, 0, sizeof buffer);
      nbytes = 0;

      /* If we haven't received a response, wait for one */
      if (NULL == (response = complete_response(vendor, buffer)))
      {
         while (0 == timeout || (select(1, (void *)&fds, (void *)0, (void *)0, &tv) > 0))
         {
	    nbytes += read(0, buffer + nbytes, sizeof buffer - nbytes);
	    if (NULL != (response = complete_response(vendor, buffer))) break;
         }
      }

      /* restory the tty modes */
#ifdef USE_STTY
      system("stty -raw echo");
#else
   /* { */
      if (tcsetattr(STDERR_FILENO, TCSADRAIN, &saved))
      {
         fprintf(stderr, "tcsetattr failed - %s\n", strerror(errno));
         exit(EXIT_FAILURE);
      }
   /* } */
#endif
   } while ((strcmp (response, "ERR") == 0) && (++attempts < MAX_ATTEMPTS));
   return response;
}
