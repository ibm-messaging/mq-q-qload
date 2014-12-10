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
/*********************************************************************/
/**********************************************************************/
/*  FILE   : Q.H                                                      */
/*  PURPOSE: MQSeries Queueing Program                                */
/**********************************************************************/
                                       /* Verbose Levels              */
#define VL_MESSAGE   1
#define VL_MQAPI     2
#define VL_MQDETAILS 3
                                       /* Message Options             */
#define MO_DETAIL_2        '2'
#define MO_DETAIL_3        '3'
#define MO_MQMD_BEFORE     'D'
#define MO_PROP_FORCE_RFH2 'F'
#define MO_MQGMO_BEFORE    'G'
#define MO_OUTPUT_NOMSG    'N'
#define MO_MQOD_BEFORE     'O'
#define MO_MQPMO_BEFORE    'P'
#define MO_MQSRO_BEFORE    'R'
#define MO_MQSD_BEFORE     'S'
#define MO_OUTPUT_NOXML    'X'
#define MO_PROP_ALL        'a'
#define MO_IGNORE_CRLF     'c'
#define MO_MQMD_AFTER      'd'
#define MO_OUTPUT_MQMD     'd'
#define MO_OUTPUT_FORMAT   'f'
#define MO_MQGMO_AFTER     'g'
#define MO_OUTPUT_HEX      'h'
#define MO_OUTPUT_MSGLEN   'l'
#define MO_PROP_NONE       'n'
#define MO_MQOD_AFTER      'o'
#define MO_MQPMO_AFTER     'p'
#define MO_MQSRO_AFTER     'r'
#define MO_MQSD_AFTER      's'
#define MO_OUTPUT_OFFSET   't'
#define MO_OUTPUT_XML      'x'

                                       /* Typedefs                    */
typedef struct _MQ
{
  MQAPI    Api;
  MQHCONN  hQm;
  char     Qm[50];
  void   * pQArray;
} MQ;
                                       /* Global variables            */
#ifdef Q_GLOBALS
MQ       Mq;
int      verbose;
#else
extern MQ       Mq;
extern int      verbose;
#endif

                                       /* MVS defines                 */
#ifdef MVS
  #define QueryQMName QuQMName
  #define OpenQueue   OpenQ
  #pragma linkage(QuQMName, OS)
  #pragma linkage(OpenQ,    OS)
#endif

#ifndef MQRR_DEFAULT          /* Strangly missing from cmqc.h for MVS */
typedef struct tagMQRR MQRR;
typedef MQRR MQPOINTER PMQRR;

struct tagMQRR {
  MQLONG  CompCode;  /* Completion code for queue */
  MQLONG  Reason;    /* Reason code for queue */
};

#define MQRR_DEFAULT MQCC_OK,\
                     MQRC_NONE
#endif
                                       /* Prototypes                  */
MQLONG QueryQMName  (MQ        * pMQ,
                     MQCHAR48    QM);

MQLONG OpenQueue    (MQ        * Mq,
                     char      * Q,
                     MQLONG      OpenFlags,
                     MQLONG      Version,
                     MQHOBJ    * hObJ);

void   Usage        (int         AskedFor,char * Topic);
