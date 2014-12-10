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
/*  FILE   : QLGETOPT.H                                               */
/*  PURPOSE: Windows version of UNIX getopt code                      */
/**********************************************************************/
#ifdef GETOPT_DEFINES
  int    optind=1;               /* index into parent argv vector     */
  int    optopt;                 /* character checked for validity    */
  char   *optarg;                /* argument associated with option   */

  int getopt(int argc, char **argv, char *ostr);


#else
  extern int    optind;          /* index into parent argv vector     */
  extern int    optopt;          /* character checked for validity    */
  extern char   *optarg;         /* argument associated with option   */

  extern int getopt(int argc, char **argv, char *ostr);
#endif


