/*
 **********************************************************************

 VENDORS -

 **********************************************************************

 $MF_Copyright-Start$

	DISTRIBUTION MANAGER
	Copyright (c) 1997
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

 * $Log: vendors.h,v $
 * Revision 1.5  1999/07/16  18:16:29  dmp_mjl
 * Added support for Symbol RF terminals.
 *
 * Revision 1.4  1999/05/21  22:27:44  dmp_sw
 * freeze for V5_2
 *
 * Revision 1.3  1998/10/14  21:17:23  dmp_apl
 * freeze for V5_1
 *
 * Revision 1.2  1997/07/08  19:00:50  dmp_sw
 * freeze for V5_0
 *
 * Revision 1.1  1997/01/25  21:18:48  dmp_dp
 * Initial revision
 *

 **********************************************************************
 */


/* Symbolic vendor names for use in switch() below */
#define RF_TEK 0
#define RF_TEL 1
#define RF_LXE 2
#define RF_NOR 3
#define RF_SYM 4
#define RF_INT 5

typedef struct 
{
   char *vendorstr;
   char *sendstr;
   int  vendor;
} VENDORLIST;

/*
 * Create a list of vendors and corresponding strings. Note the last
 * one must be NULL in order to define the size of our array.
 */
#ifndef MAIN
extern
#endif
VENDORLIST vendors[]
#ifdef MAIN
= 
{
   {"TEK", "\033P?tnum\033\\", RF_TEK},
   {"TEL", "\033R08\033\\", RF_TEL},
   {"LXE", "\005", RF_LXE},
   {"NOR", "\005", RF_NOR},
   {"SYM", "\005", RF_SYM},
   {"INT", "\005", RF_INT},
   {NULL, NULL, 0}
}
#endif
;
