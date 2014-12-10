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
/*  FILE   : QLOAD.H                                                  */
/*  PURPOSE: MQSeries Queue Unload/Reload Utility                     */
/**********************************************************************/
/**********************************************************************/
/* Constants                                                          */
/**********************************************************************/

#define QLOAD_VERSION          "8.0"  /* To be same version as MQ */
#define MAX_FILE_NAME           256
#define MAX_SEARCH_SIZE          99
#define MAX_SEARCH                3

#define MAX_SIZE_BUCKETS          6
#define MAX_AGE_BUCKETS           6
                                       /* Option constants            */
#define qoQUIET                 0x00000001
#define qoMOVE                  0x00000002
#define qoFORCE_FILE            0x00000004
#define qoUNUSED                0x00000008
#define qoDSP_ASCII_FILE        0x00000010
#define qoDSP_ASCII_COLS_FILE   0x00000020
#define qoCONVERT               0x00000040
#define qoDEFAULT_CONTEXT       0x00000080
#define qoPASS_ALL_CONTEXT      0x00000100
#define qoPASS_IDENTITY_CONTEXT 0x00000200
#define qoSET_ALL_CONTEXT       0x00000400
#define qoSET_IDENTITY_CONTEXT  0x00000800
#define qoNO_CONTEXT            0x00001800
#define qoCONTEXT_MASK          0x00001F80
#define qoAMQSBCG_INPUT         0x00010000
#define qoWRITE_INDEX           0x00020000
#define qoFILTER                0x00040000
#define qoPURGE                 0x00080000  /* Source queue is emptied*/
#define qoUNIQUE_FILES_IN       0x00100000
#define qoUNIQUE_FILES_OUT      0x00200000
#define qoUNIQUE_FILES          0x00300000
#define qoWRITE_AGE             0x00400000
#define qoRELATIVE_DELAY        0x00800000
#define qoSTRIP_HEADERS         0x01000000
#define qoHEXCHAR_CHAR          0x02000000
#define qoSUMMARY               0x04000000
#define qoSUMMARY_MSGID         0x08000000
#define qoSUMMARY_CORRELID      0x10000000
#define qoMATCH_MSGID           0x20000000
#define qoMATCH_CORRELID        0x40000000
#define qoMATCH_GROUPID         0x80000000
                                       /* Second set of options       */
#define qo2DSP_PRINTABLE        0x00000001
#define qo2DSP_TEXT             0x00000002
#define qo2NO_WRITE_MQMD        0x00000004
#define qo2NO_WRITE_HEADER      0x00000008

#define vbDEBUG_MEMORY          0x00000001


                                       /* Macros                      */
#define SAME8(a,b) (*(((PULONG)(a))  ) == *(((PULONG)(b))  )  &&  \
                    *(((PULONG)(a))+1) == *(((PULONG)(b))+1) )

#define SAME4(a,b) (*(((PULONG)(a))  ) == *(((PULONG)(b))  ))

typedef struct _SEARCH
{
  int  Len;
  char Str[MAX_SEARCH_SIZE];
} SEARCH;

typedef struct
{
  char * name;
  UINT32 val;
} SIFT;

/**********************************************************************/
/* Structures                                                         */
/**********************************************************************/
#ifdef HP_NSK

  #define MQMD2         MQMD
  #define MQMD2_DEFAULT MQMD_DEFAULT

#endif

typedef struct _QLOPT
{
  /********************************************************************/
  /* Control state                                                    */
  /********************************************************************/
  long       Options;
  long       Options2;
  long       Verbose;
  char       Qm[50];
  MQHCONN    hQm;
  char       InputQueue[50];
  MQHOBJ     hInput;
  char       OutputQueue[50];
  MQHOBJ     hOutput;
  char       InFileName[MAX_FILE_NAME];
  int        InFileIndex;
  int        OutFileIndex;
  char       InputFileName[MAX_FILE_NAME];
  char       OutFileName[MAX_FILE_NAME];
  char       ActOutFileName[MAX_FILE_NAME];
  FILE     * hInFile;
  FILE     * hOutFile;
  int        WaitInterval;
  int        DataWidth;
  SEARCH     Ascii[MAX_SEARCH];
  SEARCH     NegAscii[MAX_SEARCH];
  SEARCH     Ebcdic[MAX_SEARCH];
  SEARCH     NegEbcdic[MAX_SEARCH];
  SEARCH     Hex[MAX_SEARCH];
  SEARCH     NegHex[MAX_SEARCH];
  MQLONG     RequiredCCSID;
  MQLONG     RequiredEncoding;
  char     * pLex;
  char     * pLexEnd;
  int        FirstIndex;
  int        LastIndex;
  int        InIndex;
  int        OutIndex;
  int        TransactionLimit;         /* Messages in transaction     */
  int        TransactionSize;
  BOOL       UseTransactions;
  BOOL       ClientMode;
  char     * AccessMode;
  time_t     Now;
  int        OlderThanAge;             /* In seconds                  */
  int        YoungerThanAge;           /* In seconds                  */
  int        Delay;
  int        StartMsgArea;
  MQBYTE24   MsgId;
  MQBYTE24   CorrelId;
  MQBYTE24   GroupId;
  /********************************************************************/
  /* Current Message                                                  */
  /********************************************************************/
  char       ErrorMessage[MAX_FILE_NAME + 200];
  char     * pMsg;
  char     * pMsgStart;
  MQLONG     MsgSize;
  size_t     MsgLen;
  MQMD2      mqmd;
  /********************************************************************/
  /* Current Line                                                     */
  /********************************************************************/
  char     * pLine;
  char     * pLineEnd;
  int        LineSize;
  size_t     LineLength;
  int        LineNo;
  /********************************************************************/
  /* Statistics                                                       */
  /********************************************************************/
  int        InMsgs;
  int        InBytes;
  int        OutMsgs;
  int        OutBytes;
  /********************************************************************/
  /* Summary                                                          */
  /********************************************************************/
  time_t     OldestMsg;
  MQBYTE24   OldestMsgMsgId;
  MQBYTE24   OldestMsgCorrelId;
  time_t     YoungestMsg;
  MQBYTE24   YoungestMsgMsgId;
  MQBYTE24   YoungestMsgCorrelId;
  MQLONG     BiggestMsg;
  MQBYTE24   BiggestMsgMsgId;
  MQBYTE24   BiggestMsgCorrelId;
  MQLONG     SmallestMsg;
  MQBYTE24   SmallestMsgMsgId;
  MQBYTE24   SmallestMsgCorrelId;
  UINT32     SizeBuckets[MAX_SIZE_BUCKETS];
  UINT32     AgeBuckets[MAX_AGE_BUCKETS];
} QLOPT;

/**********************************************************************/
/* Functions                                                          */
/**********************************************************************/
MQLONG QloadOpenFiles(QLOPT * pOpt,BOOL Prompt,BOOL Input, BOOL Output);
MQLONG QloadFnc      (QLOPT * pOpt,BOOL Prompt);
void   QloadInit     (QLOPT * pOpt);
void   TestQload     ();
