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
/*  FILE   : QLVALUES.H                                               */
/*  PURPOSE: Manipulation routines for MQSeries constants             */
/**********************************************************************/

                                       /* Datatypes                   */
#define PMQENTRY  MQENTRY *

typedef struct _MQVALUE
{
  MQLONG  Const;
  char  * Name;
  char  * Desc;
} MQVALUE;

void   MQError          (char   * Api,
                         char   * Obj,
                         int      reason);

char * MQReason         (int      reason);
char * MQCommand        (int      command);
char * MQCommandConst   (int      command);
int    MQCommandStr     (char   * command);
char * MQParameter      (int      parameter);
char * MQParameterConst (int      parameter);
char * MQParameterValue (int      parameter,
                         int      value);
int    MQParameterStr   (char   * parameter);
char * MQConst          (int  parameter);
int    MQConstStr       (char   * name,
                         MQLONG * Const);

