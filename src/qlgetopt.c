/*******************************************************************************
 * Copyright (c) 1995, 2014 IBM Corporation and other Contributors.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors:
 *   Paul Clarke - Initial Contribution
 */
/**********************************************************************/
/*  FILE   : QLGETOPT.C                                               */
/*  PURPOSE: Windows version of UNIX getopt code                      */
/**********************************************************************/
                                       /* Includes                   */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define GETOPT_DEFINES
#include "qlgetopt.h"
                                       /* Defines                    */
#define EMPTY_MSG  ""
#define BADCH ('?')
#define COLON (':')

static char *place = EMPTY_MSG;        /* option letter processing    */
/**********************************************************************/
/* Function: getopt                                                   */
/* Purpose : Simulate the UNIX getopt function                        */
/*           Each time this function is called it returns the next    */
/*           option letter and sets a static pointer pointing to      */
/*           any option argument.                                     */
/**********************************************************************/
int getopt(int argc, char **argv, char *ostr)
{
   char *oli = NULL;                   /* option letter list index    */
   unsigned int i;                     /* loop index                  */
   int   found = 0;                    /* found flag                  */

   if (!place || !*place)
   {                                   /* update scanning pointer     */
     place = argv[optind];
     /****************************************************************/
     /* Check for end of input                                       */
     /****************************************************************/

     if ((optind >= argc) ||
         ((*place != '-') && (*place != '/')) ||
         (!*++place))
     {
        return EOF;
     }
   }

   /******************************************************************/
   /* option letter okay?                                            */
   /******************************************************************/

   optopt = (int)*place++;             /* Current option             */

   for (i=0; (i < strlen(ostr)) && !found; i++)
   {                            /* check in option string for option */
     if (optopt == ostr[i])
     {
       found = 1;                      /* option is present          */
       oli = &ostr[i];                 /* set oli to point to it     */
     }
   } /* End check in option string for option */

   if (optopt == (int)COLON || !oli)

   {                                   /* Current option not valid   */
      if (!*place)
      {
        ++optind;
      }
      return BADCH;
   } /* End current option not valid */

   /******************************************************************/
   /* Check if this option requires an argument                      */
   /******************************************************************/

   if (*++oli != COLON)
   {                                   /* this option doesn't require*/
                                       /* an arg                     */
     optarg = NULL;
     if (!*place)
     {
        ++optind;
     }
   }
   else
   {                                   /* this option needs an arg   */
     if (*place)
     {
        optarg = place;                /* parm follows arg directly  */
        if (*optarg == COLON)
          optarg++;                    /* Remove colon if present    */
     }
     else
     {
       if (argc <= ++optind)
       {                               /* No argument found          */
         place = EMPTY_MSG;
         return BADCH;
       }
       else
       {
         optarg = argv[optind];        /* white space                */
         if (*optarg == COLON)
           optarg++;                   /* Remove colon if present    */
       }
     }

     place = EMPTY_MSG;
     ++optind;
   }

   return optopt;                      /* return option letter       */
}

