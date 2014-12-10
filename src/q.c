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
/*  FILE   : Q.C                                                      */
/*  PURPOSE: MQSeries Queueing Program                                */
/**********************************************************************/
#ifdef MVS
  #pragma csect(code,"QCODE")
  #pragma csect(static,"QSTAT")
  #pragma options(RENT)
  #pragma runopts(POSIX(ON))
  #define _POSIX_SOURCE                /* Required for sleep()        */
  #define _XOPEN_SOURCE                /* Required for optarg, optind */
  #define _SHARE_EXT_VARS              /* because of APAR PQ03847     */
  #define _XOPEN_SOURCE_EXTENDED 1     /* Required for timeval        */
  #define _ALL_SOURCE                  /* Required for timezone on    */
#endif                                 /*              gettimeofday() */
                                       /* Defines                     */
#define MIN(a,b) (a<b ? a:b)
                                       /* Include files               */
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  #include <sys/time.h>
  #include "unistd.h"
  #include "errno.h"
#else
  #include "qlgetopt.h"
#endif
#ifdef WIN32
  #include "windows.h"
  #include "io.h"
  #define putenv _putenv
#endif

#ifdef AMQ_AS400
  #include "qlgetopt.h"
#endif

#ifdef MVS
  #define MQCCSI_APPL 1047
#endif
#include "cmqc.h"
#include "cmqxc.h"
#include "qlcs.h"
#ifdef MVS
  #define NOLOAD                       /* Can't use MQAccess on MVS   */
  #define NODISTLIST                   /* No distribution lists on MVS*/
#endif
#include "qlaccess.h"
#include "qlvalues.h"
#include "qlparse.h"

#define Q_GLOBALS
#include "q.h"
#include "qlutil.h"

/**********************************************************************/
/* Constants                                                          */
/**********************************************************************/
#define SEPARATOR_LINE "============================================================================"
#define MAX_DEST  10
#define MAX_MQAIR 10
                                       /* Option constants            */
#define qoUSE_GET_MSGID           0x00000001
#define qoUSE_GET_CORRELID        0x00000002
#define qoUSE_PUT_MSGID           0x00000004
#define qoUSE_PUT_CORRELID        0x00000008
#define qoCCSID                   0x00000010
#define qoECHO                    0x00000020
#define qoREPORT_TIMES            0x00000040
#define qoUSE_MQPUT1              0x00000080
#define qoZERO_MSGID              0x00000100
#define qoUSE_MQCONNX             0x00000200
#define qoUSE_STDOUT              0x00000400
#define qoQUIET                   0x00000800
#define qoBROWSE                  0x00001000
#define qoLOCK                    0x00002000
#define qoNO_HANDLER              0x00004000
#define qoWAIT_REPLY              0x00008000
#define qoEXPIRE                  0x00010000
#define qoNO_QUIESCE              0x00020000
#define qoUSER_MQCD               0x00040000
#define qoEXPLICIT_MSG            0x00080000
#define qoNO_RESPONSE             0x00100000
#define qoUSER_FORMAT             0x00200000
#define qoUSE_OFFSETS             0x00400000
#define qoNO_MQDISC               0x00800000
#define qoCONTEXT_PASS_ALL        0x01000000
#define qoCONTEXT_PASS_IDENTITY   0x02000000
#define qoECHO_QMGR               0x04000000
#define qoNEWLINE                 0x08000000
#define qoREQUEST                 0x10000000
#define qoCONTEXT_SET_ALL         0x20000000
#define qoCONTEXT_SET_IDENTITY    0x40000000
#define qoGET_SLEEP               0x80000000

#define qo2RESOLVE_LOCAL          0x00000001
#define qo2USER_MSGTYPE           0x00000002
#define qo2SET_USER               0x00000004
#define qo2LOCAL_TIMES            0x00000008
#define qo2RETVAL_ZERO            0x00000010
#define qo2RETVAL_MQCC            0x00000020
#define qo2DUAL_QM                0x00000040
#define qo2CLOSE_QUIESCE          0x00000080
#define qo2READ_AHEAD             0x00000100
#define qo2NO_READ_AHEAD          0x00000200
#define qo2USE_INDIVIDUAL_QUEUES  0x00000400
#define qo2NO_CONTEXT             0x00000800
#define qo2USE_GET_GROUPID        0x00001000
#define qo2USE_PUT_GROUPID        0x00002000
#define qo2RETAIN                 0x00004000
#define qo2SUPRESS_REPLYTO        0x00008000
#define qo2CLOSE_DELETE           0x00010000
#define qo2TRUNCATE               0x00020000
#define qo2NO_SPECIAL_MSG         0x00040000
#define qo2COMPLETE_MESSAGE       0x00080000
#define qo2SYNCPOINT_IF_PERSIST   0x00100000
#define qo2ALTERNATE_USERID       0x00200000
#define qo2NOT_OWN_SUBS           0x00400000
#define qo2FORCE_RFH2             0x00800000
#define qo2RECORD_BASED_IO        0x01000000 /* z/OS only */
#define qo2PROPERTIES_FORCE_RFH2  0x02000000
#define qo2NO_PROPERTIES          0x04000000
#define qo2EVENT_HANDLER          0x08000000 /* Non z/OS */
#define qo2INPUT_EXCLUSIVE        0x10000000
#define qo2CLOSE_HSUB             0x20000000
#define qo2PROPERTIES_ALL         0x40000000
#define qo2NO_MATCH               0x80000000

#ifndef MQPMO_WARN_IF_NO_SUBS_MATCHED
  #define MQPMO_WARN_IF_NO_SUBS_MATCHED 0x00080000
#endif

#define qo2CLOSE_EXPLICIT (qo2CLOSE_DELETE | qo2CLOSE_QUIESCE)
                                       /* Subscribe option constants  */
#define soCREATE_SUB              0x00000001
#define soRESUME_SUB              0x00000002
#define soALTER_SUB               0x00000004
#define soDURABLE                 0x00000008
#define soRESOURCE_PROVIDED       0x00000010 /* Some kind of topic provided */
#define soANY_USER                0x00000020
#define soFIXED_USER              0x00000040
#define soNEW_PUBS_ONLY           0x00000100
#define soPUBS_ON_REQ             0x00000200
#define soWILDCARD_CHAR           0x00000400
#define soWILDCARD_TOPIC          0x00000800
#define soGROUP_SUB               0x00080000
#define soREMOVE_SUB              0x00800000 /* MQCLOSE option for dur  */

#define soCREATE_OPTS   (soCREATE_SUB   |   \
                         soRESUME_SUB   |   \
                         soALTER_SUB  )

#define qoCONTEXT      (qoCONTEXT_PASS_ALL      |   \
                        qoCONTEXT_PASS_IDENTITY |   \
                        qoCONTEXT_SET_ALL       |   \
                        qoCONTEXT_SET_IDENTITY )

#define qoPASS_CONTEXT (qoCONTEXT_PASS_ALL      |   \
                        qoCONTEXT_PASS_IDENTITY)

#define DEFAULT_PROPERTY_VALUE_BUFFER_SIZE 4096
#define DEFAULT_PROPERTY_NAME_BUFFER_SIZE  256
#if MQAT_DEFAULT == MQAT_WINDOWS_NT /* printf 64-bit integer type */
  #define  Int64 "I64"
#elif defined(MQ_64_BIT)
  #define  Int64 "l"
#else
  #define  Int64 "ll"
#endif

#ifndef MVS
  #define FORCE_STDERR fflush(stderr)
#else
  #define FORCE_STDERR fputc('\n',stderr)
#endif

MQCD     Mqcd1           = { MQCD_DEFAULT };
MQCD     Mqcd2           = { MQCD_DEFAULT };

#ifndef MVS                                    /* No clients from MVS */
typedef struct _EXITARRAY
{
  char      * ExitName;
#ifdef AMQ_AS400
  MQCHAR20  * SingleExit;
#else
  MQCHAR128 * SingleExit;
#endif
  MQCHAR32  * SingleExitArea;
  char     ** MultExit;
  char     ** MultExitArea;
  MQLONG    * MultDefined;

} EXITARRAY;

EXITARRAY ExitArray1[] =
{
  {"Send",
         &Mqcd1.SendExit,
         &Mqcd1.SendUserData,
(char **)&Mqcd1.SendExitPtr,
(char **)&Mqcd1.SendUserDataPtr,
         &Mqcd1.SendExitsDefined},
  {"Receive",
         &Mqcd1.ReceiveExit,
         &Mqcd1.ReceiveUserData,
(char **)&Mqcd1.ReceiveExitPtr,
(char **)&Mqcd1.ReceiveUserDataPtr,
         &Mqcd1.ReceiveExitsDefined},
  {"Message",
         &Mqcd1.MsgExit,
         &Mqcd1.MsgUserData,
(char **)&Mqcd1.MsgExitPtr,
(char **)&Mqcd1.MsgUserDataPtr,
         &Mqcd1.MsgExitsDefined},
  {"Security",
         &Mqcd1.SecurityExit,
         &Mqcd1.SecurityUserData,
         NULL,
         NULL,
         NULL}
};

EXITARRAY ExitArray2[] =
{
  {"Send",
         &Mqcd2.SendExit,
         &Mqcd2.SendUserData,
(char **)&Mqcd2.SendExitPtr,
(char **)&Mqcd2.SendUserDataPtr,
         &Mqcd2.SendExitsDefined},
  {"Receive",
         &Mqcd2.ReceiveExit,
         &Mqcd2.ReceiveUserData,
(char **)&Mqcd2.ReceiveExitPtr,
(char **)&Mqcd2.ReceiveUserDataPtr,
         &Mqcd2.ReceiveExitsDefined},
  {"Message",
         &Mqcd2.MsgExit,
         &Mqcd2.MsgUserData,
(char **)&Mqcd2.MsgExitPtr,
(char **)&Mqcd2.MsgUserDataPtr,
         &Mqcd2.MsgExitsDefined},
  {"Security",
         &Mqcd2.SecurityExit,
         &Mqcd2.SecurityUserData,
         NULL,
         NULL,
         NULL}
};

#endif                                         /* No clients from MVS */

char * UsageText[] = {
 "\nUsage: Q <Optional flags as below>\n"
,"        [-a[dnpqcRrstfFaAcD] Message attributes\n"
,"+          p:Persistent,n:Non-Persisent,q:Persistence as Q,s:Allow Segmentation\n"
,"+          c:complete message,d:Datagram,R:Request,r:Reply,t:Report\n"
,"+          f:Async put,             F:no async put\n"
,"+          a:Read Ahead,            A:No Read Ahead\n"
,"+          c:Complete message\n"
,"+          C:Close Quiesce,         D:Close Delete\n"
,"+          i:Individual queues      o:Use offsets\n"
,"+          x:No # message procesing X:Input exclusive \n"
,"+          2:Force message properties as MQRFH2s\n"
,"        [-A[i|o|a]<value> Application Identity|Origin Data|Acct Token\n"
#ifndef MVS /* No historical need to retain on MVS */
,"  (c)   [-b Browse input Q (DEP:use i rather than I]\n"
#endif
,"        [-cCCSid[:X'Encoding'] Convert]\n"
,"        [-C[a][i][A][I] Context]\n"
,"+          a: pass all context     i: pass identity context\n"
,"+          A: set all context      I: set identity context\n"
,"+          n: no context                                  \n"
,"  (c)   [-d[h][f][n][w<width>][1][2][3] Display Detail\n"
,"+          h:Print message in hex  n:Don't print message\n"
,"+          d/D:Print out MQMD      o/O:Print out MQOD\n"
,"+          p/P:Print out MQPMO     g/G:Print out MQGMO\n"
,"+          s/S:Print out MQSD      r/R:Print out MQSRO\n"
,"+          a:All properties        n:No properties\n"
,"+          F:Force RFH2\n"
,"+          x:Use XML Shortform     X:No XML Auto detect\n"
,"+          l:Print message length  f:Format recognised messages\n"
,"+          1:Low level of detail   2:Medium level of detail\n"
,"+          3:High level of detail\n"
,"        [-e Echo to Reply Queue]\n"
,"        [-E Echo to Reply Queue and set Reply QMgr]\n"
#ifdef MVS
,"        [-f[=] Input file] Each line is one message\n"
,"+          =: use full record length\n"
#else
,"        [-f Input file] Each line is one message\n"
#endif
#ifdef MVS
,"        [-F[+] Load/Unload file] Entire file for one message\n"
,"+          +: retain attributes of an existing output dataset\n"
#else
,"        [-F Load/Unload file] Entire file for one message\n"
#endif
,"        [-X] Load a file in hex as one message\n"
,"        [-g[p][x][C][m|c|g]identifier      Get or Put by identifier\n"
,"+          x:ID is in hex   p:Put with ID       C:Use MQCI New session\n"
,"+          m:Message ID     c:Correlation ID    g:Group ID\n"
,"        [-h Filter string]\n"
,"        [-H Selection string]\n"
,"  (c)   [-i Input Queue (browse)]\n"
,"  (c)   [-I Input Queue (get)]\n"
,"        [-j Specify format name ]   [-k Browse Lock]   \n"
#ifndef MVS
,"        [-l Library name] e.g.mqm or mqic\n"
#endif
,"        [-L Message Limit] Maximum number of messages to process\n"
,"        [-m LocalQueueManager]      [-M Simple text message]\n"
,"        [-n\"Confirm options\" below :-\n"
,"+          [ca|cad|cafd] [cd|cdd|cdfd] [e|ed|efd] [x|xd|xfd]\n"
,"+          [pan][nan][newm][passm][copym][disc][passd][act]\n"
#ifndef MVS
,"  (c)   {-o Output Queue} Multiple use uses distribution list\n"
#else
,"  (c)   {-o Output Queue}\n"
#endif
,"        {-O Output Queue} Bind on Open version of above\n"
,"        [-p Commit interval]        [-P Message Priority]\n"
,"        [-q]  Quiet....don't write messages to screen\n"
,"        [-r[+] Reply Queue]         [-s Force msg output to stdout]\n"
,"        [-$ Queue Separator character]\n"
,"        [-t] Print timings\n"
,"        [-U[+] User Identifier [+]: Alternate Userid\n"
,"        [-v[p|P|1|2|3|4|5] Verbose Level\n"
,"+          p: Pause, P: Cmd line parms >=1: Message info,>=2: API info]\n"
,"        [-V[z][c] Return value, default MQRC]\n"
,"+          z:Force zero return          c:MQCC mapped to 0,4,8\n"
,"  (c)   [-w Wait for messages (in seconds)]\n"
,"        [-W Sleep before issuing MQGET (in milliseconds)]\n"
#ifdef MVS
,"        [-x[q][N][t][u] Use MQCONNX]\n"
#else
,"        [-x[f][s][i][b|n][c][q][N][t][u] Use MQCONNX]\n"
,"+          f:Fast binding     s:Standard binding     i:Isolated\n"
,"+          b:Shared Blocking Connection n:Shared Nonblocking Connection\n"
,"+          c:Specify channel on MQCONNX\n"
#endif
,"+          q:No fail if quiescing       N:No MQDISC\n"
,"+          t:Specify connection tag on MQCONNX \n"
,"+          u:Specify User ID and Password MQCONNX \n"
#ifndef MVS
,"+          v:Register event handler (client mode only)\n"
#endif
,"        [-y Expire time in 1/10th's second\n"
,"        [-z Zero out MsgId before MQPUT\n"
,"        [-Z TimeZone (hours) Put Date/Time\n"
,"        [-1 Use MQPUT1 rather than MQPUT\n"
,"        [-# Structure versions] eg. #m1p2g2o1x4 or #c for current\n"
,"        [-* Repeat execution]\n"
,"        [-=[n] Set Max Msg Length - 'n' will not truncate]\n"
,"        [-! Do not use exithandler]\n"
,"        [-S[o:TopicObject] Subscribe to Topic Object\n"
,"+       [  [s:TopicString] Subscribe to Topic String\n"
,"+       [  [n:SubName]     Subscription Name\n"
,"+       [  [u:SubUserData] Subscription User Data\n"
,"+       [  [l SubLevel]    Subscription Level\n"
,"+       [  [c][r][a]  [d][v][f][g] [N][R] [C][T]\n"
,"+          c:Create (def) r:Resume        a:Alter subscription\n"
,"+          d:Durable\n"
,"+          v:Any User     f:Fixed User    g:Group Sub\n"
,"+          N:New pubs only                R:Pubs on request\n"
,"+          C:Wildcard Char                T:Wildcard Topic\n"
,"+          D:Delete durable sub\n"
,"        [-T[o:TopicObject] Publish to Topic Object\n"
,"+       [  [s:TopicString] Publish to Topic String\n"
,"+       [  [r][p][n][w]\n"
,"+          r:Retain       p:Supress ReplyTo\n"
,"+          n:Not own Subs w:Warn if no match\n"
,"\n"
,"Common options marked with (c)\n"
," \n"
,"+Examples \n"
,"+-------- \n"
,"+Write to a queue                : q -oQ1 \n"
,"+Write to two queues (Dist list) : q -oQ1 -oQ2\n"
,"+Write to a queue on remote Qmgr : q -oQM2/Q1 \n"
,"+Read from a queue               : q -IQ1 \n"
,"+Browse from a queue             : q -iQ1 \n"
,"+Show Message Descriptor         : q -iQ1 -dd3\n"
,"+Wait for messages               : q -IQ1 -w60\n"
,"+Move messages between Queues    : q -IQ1 -oQ2\n"
,"+Copy messages between Queues    : q -iQ1 -oQ2\n"
,"+Subscribe to a topic            : q -Ss:TopicStr -w60\n"
,"+Make a durable subscription     : q -Sds:TopicStr -Sn:MySubName -w60\n"
,"+Publish on a topic              : q -Ts:TopicStr \n"
};


#ifndef MVS                                    /* No clients from MVS */
#define MAX_EXITS 10

MQCHAR128 ExitName1    [MAX_EXITS];
MQCHAR32  ExitUserData1[MAX_EXITS];
MQCHAR128 ExitName2    [MAX_EXITS];
MQCHAR32  ExitUserData2[MAX_EXITS];
#endif                                         /* No clients from MVS */
/**********************************************************************/
/* Structures  (copied from amqrrxha.h)                               */
/**********************************************************************/
typedef struct _rrxMQCNO
{
  MQCHAR4  StrucId;  /* Structure identifier */
  MQLONG   Version;  /* Structure version number */
  MQLONG   Options;  /* Options that control the action of MQCONNX */
  MQLONG   Reserved1;
  MQPTR    ClientConnectionPtr; /* Pointer to Channel definition      */
} rrxMQCNO;
/**********************************************************************/
/* Local Functions                                                    */
/**********************************************************************/
static void          strip         (char     * p,
                                    long       len);

static void          Cleanup       (void);

static void          PrintMsg      (char     * pMsg,
                                    MQLONG     MsgLen,
                                    MQMD2    * Mdesc,
                                    char     * msgopts,
                                    BOOL       ShowMQMD);

static int           LineOut       (void     * Parm,
                                    CBOPT    * CbOpt,
                                    char     * pLine,
                                    int        Len);

static void          ParseQueueName(char       Separator,
                                    char     * pQ,
                                    char     * Qm,
                                    char     * Q);

static void          GetId         (char     * Id,
                                    char     * Value);

static void          GetHexId      (char     * Id,
                                    char     * Value);

static void        * QGetMem        (long       size);

static void          QFreeMem       (void     * p);

static BOOL          UsageLine      (char     * pLine);

static BOOL          ConfirmExit(void);

/**********************************************************************/
/* Global/Static Variables                                            */
/**********************************************************************/
static MQBYTE24  GetGroupId;
static MQBYTE24  PutGroupId;
static MQBYTE24  GetMsgId;
static MQBYTE24  PutMsgId;
static MQBYTE24  GetCorrelId;
static MQBYTE24  PutCorrelId;
static MQHMSG    hmsg     = MQHO_NONE;
static MQCHAR8   MyFormat = MQFMT_STRING;
static int       Repeat   = FALSE;
static int       Priority = -1;
static unsigned char      HEX[] = "0123456789ABCDEF";

static long     Options         = qoBROWSE;
static long     Options2        = 0;
static long     SubOpts         = 0;
static int      ScreenWidth     = 0;
#ifdef MVS
static int      ScreenHeight    = 99999999;
#else
static int      ScreenHeight    = 24;
#endif
static int      lineno          = 0;
static MQ       Mq2;
static int      Pause = 0;
static int      ExitConfirm     = 0;
static int      PerformanceLimit = 5000;

#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  struct timeval StartTime,EndTime;
  struct timezone tz;
#endif
#ifdef WIN32
  LARGE_INTEGER StartTime,EndTime,Frequency;
#endif
/**********************************************************************/
/* External Variables                                                 */
/**********************************************************************/
extern MQCHAR8 FormatName;
extern int     gEncoding;
extern int     gEbcdic;
extern int     gTimeZone;

static void AllSleep(int d)
{
  if (d<0) d= -d * rand() / RAND_MAX;

  if (verbose >= VL_MQAPI)
  {
    fprintf(stderr,"sleep(%d milliseconds)",d);
    FORCE_STDERR;
  }
  CSMSSleep(d);
  if (verbose >= VL_MQAPI) fprintf(stderr," - done\n");
}

#ifdef WIN32
static void PrintTime(char * p,double t)
{
       if (t > 1000000) sprintf(p,"%.2fs"  ,t/1000000);
  else if (t > 1000)    sprintf(p,"%.2fms" ,t/1000);
  else                  sprintf(p,"%.2fus" ,t);
}
#endif

/*********************************************************************/
/* Function name:    PrintProperties                                 */
/*                                                                   */
/* Description:      Prints the name of each property that matches   */
/*                   the supplied name filter together with its      */
/*                   value in the appropriate format, viz:           */
/*                                                                   */
/*                    boolean values as TRUE or FALSE                */
/*                    byte string values as a series of hex digits   */
/*                    floating-point values as a number (%g)         */
/*                    integer values as a number (%d)                */
/*                    null values as NULL                            */
/*                    string values as characters (%s)               */
/*********************************************************************/
MQLONG PrintProperties(MQHCONN hconn, MQHMSG hmsg, char *filter)
{
  MQLONG  rc = MQRC_NONE;

  int     i;                              /* loop counter            */
  int     j;                              /* another loop counter    */
  MQIMPO  impo = {MQIMPO_DEFAULT};        /* inquire prop options    */
  MQLONG  namebuffersize;             /* returned name buffer length */
  PMQCHAR namebuffer = NULL;              /* returned name buffer    */
  MQCHARV inqname = {MQPROP_INQUIRE_ALL}; /* browse all properties   */
  MQPD    pd = {MQPD_DEFAULT};            /* property descriptor     */
  MQLONG  type;                           /* property type           */
  MQLONG  valuebuffersize;                /* value buffer size       */
  PMQBYTE valuebuffer = NULL;             /* value buffer            */
  MQLONG  valuelength;                    /* value length            */
  MQLONG  compcode = MQCC_OK;             /* MQINQMP completion code */
  MQLONG  reason = MQRC_NONE;             /* MQINQMP reason code     */

  /*******************************************************************/
  /* Initialise the property name with filter if one was specified   */
  /*******************************************************************/
  if (filter)
  {
    inqname.VSPtr = filter;
    inqname.VSLength = MQVS_NULL_TERMINATED;
  }

  /*******************************************************************/
  /* Initialize storage                                              */
  /*******************************************************************/
  valuebuffersize = DEFAULT_PROPERTY_VALUE_BUFFER_SIZE;
  valuebuffer = (PMQBYTE)malloc(valuebuffersize);

  if (valuebuffer == NULL)
  {
    rc = MQRC_STORAGE_NOT_AVAILABLE;
    printf("Unable to allocate property value buffer\n");
    goto MOD_EXIT;
  }

  namebuffersize = DEFAULT_PROPERTY_NAME_BUFFER_SIZE;
  namebuffer = (PMQCHAR)malloc(namebuffersize);

  if (namebuffer == NULL)
  {
    rc = MQRC_STORAGE_NOT_AVAILABLE;
    printf("Unable to allocate property name buffer\n");
    goto MOD_EXIT;
  }

  /*******************************************************************/
  /* Initialise the inquire prop options structur                    */
  /*******************************************************************/
  impo.Options |= MQIMPO_CONVERT_VALUE;
  impo.ReturnedName.VSPtr = namebuffer;
  impo.ReturnedName.VSBufSize = namebuffersize;

  /*******************************************************************/
  /* Dump the message properties                                     */
  /*******************************************************************/
  if (filter)
    printf("Message Properties matching '%s':\n", filter);

  /*******************************************************************/
  /* Loop until MQINQMP unsuccessful                                 */
  /*******************************************************************/
  for (i = 0; compcode == MQCC_OK; i++)
  {
    IMQINQMP(Mq.Api,
             hconn,                  /* connection handle             */
             hmsg,                   /* message handle                */
             &impo,                  /* inquire msg properties opts   */
             &inqname,               /* property name                 */
             &pd,                    /* property descriptor           */
             &type,                  /* property type                 */
             valuebuffersize,        /* value buffer size             */
             valuebuffer,            /* value buffer                  */
             &valuelength,           /* value length                  */
             &compcode,              /* completion code               */
             &reason);               /* reason code                   */

    if (compcode != MQCC_OK)
    {
      switch(reason)
      {
        case MQRC_PROPERTY_NOT_AVAILABLE:
          /***********************************************************/
          /* The message contains no more properties, report if this */
          /* is the first time around the loop, i.e. none are found. */
          /***********************************************************/
          if (i == 0)
          {
            /* printf("  None\n"); */
          }
          break;

        case MQRC_PROPERTY_VALUE_TOO_BIG:
        {
          PMQBYTE newbuffer = NULL;

          /***********************************************************/
          /* The value buffer is too small, reallocate the buffer    */
          /* and inquire the same property again.                    */
          /***********************************************************/
          newbuffer = (PMQBYTE)realloc(valuebuffer, valuelength);

          if (newbuffer == NULL)
          {
            rc = MQRC_STORAGE_NOT_AVAILABLE;
            printf("Unable to re-allocate property value buffer\n");
            goto MOD_EXIT;
          }

          compcode = MQCC_OK;
          valuebuffer = newbuffer;
          valuebuffersize = valuelength;

          impo.Options = MQIMPO_CONVERT_VALUE | MQIMPO_INQ_PROP_UNDER_CURSOR;
        }
        break;

        case MQRC_PROPERTY_NAME_TOO_BIG:
        {
          PMQBYTE newbuffer = NULL;

          /***********************************************************/
          /* The name buffer is too small, reallocate the buffer and */
          /* inquire the same property again.                        */
          /***********************************************************/
          newbuffer = (PMQBYTE)realloc(namebuffer, impo.ReturnedName.VSLength);

          if (newbuffer == NULL)
          {
            rc = MQRC_STORAGE_NOT_AVAILABLE;
            printf("Unable to re-allocate property name buffer\n");
            goto MOD_EXIT;
          }

          compcode = MQCC_OK;
          namebuffer = newbuffer;
          namebuffersize = impo.ReturnedName.VSLength;

          impo.ReturnedName.VSPtr = namebuffer;
          impo.ReturnedName.VSBufSize = namebuffersize;
          impo.Options = MQIMPO_CONVERT_VALUE | MQIMPO_INQ_PROP_UNDER_CURSOR;
        }
        break;

        default:
          /***********************************************************/
          /* MQINQMP failed for some other reason                    */
          /***********************************************************/
          rc = reason;
          printf("MQINQMP ended with reason code\n", reason);
          goto MOD_EXIT;
      }
    }
    else
    {
      printf("  %.*s", impo.ReturnedName.VSLength, impo.ReturnedName.VSPtr);
                                       /* Pad with blanks            */
      for (i = impo.ReturnedName.VSLength; i<12; i++) putc(' ',stdout);
      putc(':',stdout);


      /***************************************************************/
      /* Print the property value in an appropriate format           */
      /***************************************************************/
      switch (type)
      {
        /*************************************************************/
        /* Boolean value                                             */
        /*************************************************************/
        case MQTYPE_BOOLEAN:
          printf("%s\n", *(PMQBOOL)valuebuffer ? "TRUE" : "FALSE");
          break;

        /*************************************************************/
        /* Byte-string value                                         */
        /*************************************************************/
        case MQTYPE_BYTE_STRING:
          printf("X'");
          for (j = 0 ; j < valuelength ; j++)
            printf("%02X", valuebuffer[j]);
          printf("'\n");
          break;

        /*************************************************************/
        /* 32-bit floating-point number value                        */
        /*************************************************************/
        case MQTYPE_FLOAT32:
          printf("%.12g\n", *(PMQFLOAT32)valuebuffer);
          break;

        /*************************************************************/
        /* 64-bit floating-point number value                        */
        /*************************************************************/
        case MQTYPE_FLOAT64:
          printf("%.18g\n", *(PMQFLOAT64)valuebuffer);
          break;

        /*************************************************************/
        /* 8-bit integer value                                       */
        /*************************************************************/
        case MQTYPE_INT8:
          printf("%d\n", valuebuffer[0]);
          break;

        /*************************************************************/
        /* 16-bit integer value                                      */
        /*************************************************************/
        case MQTYPE_INT16:
          printf("%hd\n", *(PMQINT16)valuebuffer);
          break;

        /*************************************************************/
        /* 32-bit integer value                                      */
        /*************************************************************/
        case MQTYPE_INT32:
          printf("%d\n", *(PMQLONG)valuebuffer);
          break;

        /*************************************************************/
        /* 64-bit integer value                                      */
        /*************************************************************/
        case MQTYPE_INT64:
          printf("%"Int64"d\n", *(PMQINT64)valuebuffer);
          break;

        /*************************************************************/
        /* Null value                                                */
        /*************************************************************/
        case MQTYPE_NULL:
          printf("NULL\n");
          break;

        /*************************************************************/
        /* String value                                              */
        /*************************************************************/
        case MQTYPE_STRING:
          printf("'%.*s'\n", valuelength, valuebuffer);
          break;

        /*************************************************************/
        /* A value with an unrecognized type                         */
        /*************************************************************/
        default:
          printf("<unrecognized data type>\n");
          break;
      }

      /***************************************************************/
      /* Inquire on the next property                                */
      /***************************************************************/
      impo.Options = MQIMPO_CONVERT_VALUE | MQIMPO_INQ_NEXT;
    }
  }

MOD_EXIT:

  free(valuebuffer);                          /* free(NULL) is valid */
  free(namebuffer);

  return rc;
}

/*********************************************************************/
/* Function name:    SetProperties                                   */
/*                                                                   */
/* Description:      Simple function to change the properties        */
/*                   in the message handle                           */
/*********************************************************************/
void SetProperties(void)
{
  MQCMHO  cmho = {MQCMHO_DEFAULT};
  MQLONG  CompCode,Reason;
  MQSMPO  spo = {MQSMPO_DEFAULT};
  MQDMPO  dpo = {MQDMPO_DEFAULT};
  MQDMHO  dmo = {MQDMHO_DEFAULT};
  MQPD    pd  = {MQPD_DEFAULT};
  MQCHARV name;
  char    Buffer[100];
  char    Name[100];
  MQLONG  Value;

  while(1)
  {
    if (hmsg == MQHM_NONE)
    {
      IMQCRTMH(Mq.Api,
                Mq.hQm,                /* connection handle             */
              &cmho,                  /* create message handle options */
              &hmsg,                  /* message handle                */
              &CompCode,              /* completion code               */
              &Reason);               /* reason code                   */

      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQError("MQCRTMH","",Reason);
      if (CompCode == MQCC_FAILED) goto MOD_EXIT;
    }
    printf("i. Add Integer\n");
    printf("c. Clear Properties\n");
    printf("d. Delete Properties\n");
    printf("D. Delete Property handle\n");
    printf("l. List Properties\n");
    printf("s. Add String\n");
    printf("q. Quit\n");
    fgets(Buffer,sizeof(Buffer),stdin);
    switch(Buffer[0])
    {

      case   0: break;
      case  'c':
      case  'D':
                IMQDLTMH(Mq.Api,
                          Mq.hQm,
                         &hmsg,
                         &dmo,
                         &CompCode,
                         &Reason);
                if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
                   MQError("MQDLTMH","",Reason);
                if (CompCode == MQCC_OK)
                {
                  hmsg = MQHM_NONE;
                  if (Buffer[0] == 'D') goto MOD_EXIT;
                }
                break;
      case  'd':printf("Enter name\n");
                fgets(Name,sizeof(Name),stdin);
                memset(&name,0,sizeof(name));
                name.VSLength = strlen(Name);
                name.VSPtr    = Name;
                name.VSCCSID  = MQCCSI_APPL;

                IMQDLTMP(Mq.Api,
                         Mq.hQm,
                         hmsg,
                        &dpo,
                        &name,
                         &CompCode,
                         &Reason);
                 if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
                   MQError("MQDLTMP","",Reason);
                 if (CompCode == MQCC_OK) printf("Property '%s' deleted\n",Name);
                 break;

      case 'i': printf("Enter name\n");
                fgets(Name,sizeof(Name),stdin);
                printf("Enter value\n");
                fgets(Buffer,sizeof(Buffer),stdin);
                Value = atoi(Buffer);

                memset(&name,0,sizeof(name));
                name.VSLength = strlen(Name);
                name.VSPtr    = Name;
                name.VSCCSID  = MQCCSI_APPL;

                IMQSETMP(Mq.Api,
                         Mq.hQm,
                         hmsg,
                        &spo,
                        &name,
                        &pd,
                         MQTYPE_INT32,
                         4,
                         &Value,
                         &CompCode,
                         &Reason);
                 if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
                   MQError("MQSETMP","",Reason);
                break;
      case 's': printf("Enter name\n");
                fgets(Name,sizeof(Name),stdin);
                printf("Enter value\n");
                fgets(Buffer,sizeof(Buffer),stdin);

                memset(&name,0,sizeof(name));
                name.VSLength = strlen(Name);
                name.VSPtr    = Name;
                name.VSCCSID  = MQCCSI_APPL;

                IMQSETMP(Mq.Api,
                         Mq.hQm,
                         hmsg,
                        &spo,
                        &name,
                        &pd,
                         MQTYPE_STRING,
                         strlen(Buffer),
                         Buffer,
                         &CompCode,
                         &Reason);
                 if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
                   MQError("MQSETMP","",Reason);
                break;

      case 'l': PrintProperties(Mq.hQm, hmsg,NULL);  break;
      case 'q': goto MOD_EXIT;     break;
                break;
    }
  }
MOD_EXIT:
  ;
}
/**********************************************************************/
/* Function : LoadHexFile                                             */
/* Purpose  : Load a file which contains hex strings                  */
/**********************************************************************/
char * LoadHexFile(FILE * In,MQLONG * pMsgLen)
{
  MQLONG          MsgLen;
  unsigned char * p = NULL;
  int             c;
  int             Success = FALSE;
  unsigned char * pHex;
  unsigned char   v;
  /********************************************************************/
  /* Let's find the end of the file                                   */
  /********************************************************************/
  fseek(In,0,SEEK_END);

  MsgLen = ftell(In);
  if (MsgLen < 0)
  {
    fprintf(stderr,"Failed to find end of file '%s' RC(%d)\n",optarg,errno);
    goto MOD_EXIT;
  }
  /*******************************************************************/
  /* Get a buffer as big as the file.                                */
  /* This is quite wasteful as it will be twice as big as we need    */
  /*******************************************************************/
  p = (unsigned char *)QGetMem(MsgLen);
  if (!p)
  {
    fprintf(stderr,"Failed to allocate %ld bytes\n",MsgLen);
    goto MOD_EXIT;
  }
                                     /* Go to beginning of file     */
  rewind(In);

  MsgLen = 0;
  while (1)
  {
    int offset;

    c = fgetc(In);
    if (c == EOF) break;
    pHex = (unsigned char *)strchr(HEX,(char)c);
    if (pHex)
    {
      v = (unsigned char) (pHex - HEX);
      c = fgetc(In);
      pHex = (unsigned char *)strchr(HEX,c);
      if (!pHex)
      {
        fprintf(stderr,"Hex File does not contain character pairs\n");
        goto MOD_EXIT;
      }
      offset = (pHex - HEX);
      v  = (v<<4) + offset;
      p[MsgLen] = v;
      MsgLen++;
    }
  }
  Success = TRUE;
MOD_EXIT:
  if (!Success && p)
  {
    QFreeMem(p);
    p = NULL;
  }
  *pMsgLen = MsgLen;
  return p;
}
static InitCBopt(CBOPT * cbopt,long Options,long Options2)
{
  memset(cbopt,0,sizeof(CBOPT));
  cbopt->Options     = Options;
  cbopt->Options2    = Options2;
  cbopt->ScreenWidth = ScreenWidth;
}

static BOOL ConfirmExit()
{
  BOOL DoExit = TRUE;
  if (ExitConfirm)
  {
    char Buffer[100];
    fprintf(stderr,"Error: would you like to exit ? [y]\n");
    fgets(Buffer,sizeof(Buffer),stdin);
    if (Buffer[0] != 'y') DoExit = FALSE;
  }
  return DoExit;
}
#define PRINTDETAILS(a, b, c, d) if (*(b)) PrintDetails((a),(b),(c),(d));

/**********************************************************************/
/* Function : PrintDetails                                            */
/* Purpose  : Print out structures before and after API calls.        */
/**********************************************************************/
static PrintDetails(long Det, char * msgopts, char * Api, unsigned char * p)
{
  long       StructDetail = 1;
  MQLONG     StructLength = -1;
  MQSTRUCT * pStruct      = NULL;
  CBOPT      cbopt;

  if (strchr(msgopts,MO_DETAIL_2)) StructDetail = 2;
  if (strchr(msgopts,MO_DETAIL_3)) StructDetail = 3;
  gEncoding   = CSENC_LOCAL;
  gEbcdic     = CS_IS_EBCDIC;
  InitCBopt(&cbopt,StructDetail,0);

  if (strchr(msgopts, Det))
  {
    switch (Det)
    {
      case MO_MQOD_BEFORE:
           fprintf(stderr,"MQOD before %s\n", Api);
           pStruct = &DefMQOD;
           break;
      case MO_MQOD_AFTER:
           fprintf(stderr,"MQOD after %s\n", Api);
           pStruct = &DefMQOD;
           break;
      case MO_MQSD_BEFORE:
           fprintf(stderr,"MQSD before %s\n", Api);
           pStruct = &DefMQSD;
           break;
      case MO_MQSD_AFTER:
           fprintf(stderr,"MQSD after %s\n", Api);
           pStruct = &DefMQSD;
           break;
      case MO_MQSRO_BEFORE:
           fprintf(stderr,"MQSRO before %s\n", Api);
           pStruct = &DefMQSRO;
           break;
      case MO_MQSRO_AFTER:
           fprintf(stderr,"MQSRO after %s\n", Api);
           pStruct = &DefMQSRO;
           break;
      case MO_MQGMO_BEFORE:
           fprintf(stderr,"MQGMO before %s\n", Api);
           pStruct = &DefMQGMO;
           break;
      case MO_MQGMO_AFTER:
           fprintf(stderr,"MQGMO after %s\n", Api);
           pStruct = &DefMQGMO;
           break;
      case MO_MQMD_BEFORE:
           fprintf(stderr,"MQMD before %s\n", Api);
           pStruct = &DefMQMD;
           break;
      case MO_MQMD_AFTER:
           fprintf(stderr,"MQMD after %s\n", Api);
           pStruct = &DefMQMD;
           break;
      case MO_MQPMO_BEFORE:
           fprintf(stderr,"MQPMO before %s\n", Api);
           pStruct = &DefMQPMO;
           break;
      case MO_MQPMO_AFTER:
           fprintf(stderr,"MQPMO after %s\n", Api);
           pStruct = &DefMQPMO;
           break;
      default:
           fprintf(stderr,"Unknown detail type\n");
           break;
    }
    if (pStruct)
    {
      PrintStruct(pStruct, NULL, &LineOut, NULL, &cbopt,
                  &StructLength,&p);
    }
  }
}
/**********************************************************************/
/* Function , MQSubError                                              */
/* Purpose  : Print out MQSUB error with appropriate insert           */
/**********************************************************************/
void MQSubError(char * Api, MQLONG Options, MQSD sd, int reason)
{
  char InsertType[10] = { 0 };

  if (Options & MQSO_CREATE)
  {
    if (sd.ObjectName[0] == 0)
      fprintf(stderr,"%s on topic string '%s' returned %ld %s.\n",
              Api, sd.ObjectString.VSPtr, reason,MQReason(reason));
    else if (sd.ObjectString.VSPtr == NULL)
      fprintf(stderr,"%s on topic object '%s' returned %ld %s.\n",
              Api, sd.ObjectName, reason,MQReason(reason));
    else
      fprintf(stderr,
              "%s on topic object '%s' and string '%s'\nreturned %ld %s.\n",
              Api,sd.ObjectName, sd.ObjectString.VSPtr,
              reason,MQReason(reason));
  }
  else
    fprintf(stderr,"%s on subscription name '%s' returned %ld %s.\n",
            Api,sd.SubName.VSPtr, reason,MQReason(reason));
}
/**********************************************************************/
/* Function : GetOptVarString                                         */
/* Purpose  : getopt provides a variable string after a colon         */
/**********************************************************************/
MQLONG GetOptVarString(char ** ppStr, MQLONG * pLen, char * ErrInsert)
{
  if (*ppStr == NULL)
  {
    *pLen  = strlen(optarg) - 1;               /* Minus 1 for ':'     */
    *ppStr = (char *)QGetMem(*pLen + 1);       /* Add 1 for null term */
    if (!*ppStr)
    {
      fprintf(stderr,"Failed to allocate %ld bytes\n",
              *pLen);
      *pLen = 0;
      return MQRC_STORAGE_NOT_AVAILABLE;
    }
    memset(*ppStr, 0, *pLen + 1);
    strncpy(*ppStr, optarg+1, *pLen);
  }
  else
  {
    fprintf(stderr,
            "You can only specify one %s\n", ErrInsert);
    return MQRC_OPTIONS_ERROR;
  }
  return 0;
}
/**********************************************************************/
/* Function : CloseExplicit                                           */
/* Purpose  : Issue a close and explicit close                        */
/**********************************************************************/
BOOL CloseExplicit(MQ     * Mq,
                   char   * Q,
                   MQHOBJ * hObj)
{
  MQLONG CompCode,Reason;
  char   CloseType[50];
  BOOL   MoreMessages = FALSE;
  MQLONG Options      = 0;

  CloseType[0] = 0;
  if (Options2 & qo2CLOSE_QUIESCE)
  {
    strcat(CloseType," QUIESCE ");
    Options |= MQCO_QUIESCE;
  }
  if (Options2 & qo2CLOSE_DELETE)
  {
    strcat(CloseType," DELETE ");
    Options |= MQCO_DELETE;
  }
  if (Pause)
  {
    fprintf(stderr,"About to MQCLOSE(%s)\n",CloseType);
    getchar();
  }

  IMQCLOSE(Mq->Api,
           Mq->hQm,
           hObj,
           Options,
          &CompCode,
          &Reason);
  if (verbose >= VL_MQAPI || Reason) MQError("MQCLOSE",Q,Reason);

  switch (Reason)
  {
#ifdef MQRC_READ_AHEAD_MSGS
    case MQRC_READ_AHEAD_MSGS:
#endif
    case MQRC_Q_NOT_EMPTY:
         MoreMessages = TRUE;
         break;
    default:
         MoreMessages = FALSE;
         break;
  }
  return MoreMessages;
}

/**********************************************************************/
/* Function : QueryMQSTAT                                             */
/* Purpose  : Query the state of the connection using MQSTAT          */
/**********************************************************************/
static void QueryMQSTAT(MQ * Mq)
{
  MQSTS  sts = {MQSTS_DEFAULT};
  MQLONG CompCode, Reason;
  char   SubName[200];
  char   ObjectString[200];
  int    len;
  sts.Version = 2;
  printf("  MQSTAT_TYPE_RECONNECTION\n");
  IMQSTAT(Mq->Api, Mq->hQm, MQSTAT_TYPE_RECONNECTION,&sts, &CompCode, &Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("    MQSTAT CompCode(%d) Reason(%d) %s\n",CompCode,Reason,MQReason(Reason));
  }
  else
  {
    printf("    CompCode   :%d\n"     , sts.CompCode);
    printf("    Reason     :%d %s\n"  , sts.Reason, MQReason(sts.Reason));
    printf("    Object Type:%d\n"     , sts.ObjectType);
    printf("    Object Name:'%.48s'\n", sts.ObjectName);
    printf("    Object Qmgr:'%.48s'\n", sts.ObjectQMgrName);
  }

  printf("  MQSTAT_TYPE_RECONNECTION_ERROR\n");
  sts.ObjectString.VSPtr     = ObjectString;
  sts.ObjectString.VSBufSize = sizeof(ObjectString);
  sts.SubName.VSPtr          = SubName;
  sts.SubName.VSBufSize      = sizeof(SubName);
  IMQSTAT(Mq->Api, Mq->hQm, MQSTAT_TYPE_RECONNECTION_ERROR, &sts, &CompCode, &Reason);
  if (CompCode == MQCC_FAILED)
  {
    printf("    MQSTAT CompCode(%d) Reason(%d) %s\n",CompCode,Reason,MQReason(Reason));
  }
  else
  {
    printf("    CompCode   :%d\n"     , sts.CompCode);
    printf("    Reason     :%d %s\n"  , sts.Reason, MQReason(sts.Reason));
    printf("    Object Type:%d\n"     , sts.ObjectType);
    printf("    Object Name:'%.48s'\n", sts.ObjectName);
    printf("    Object Qmgr:'%.48s'\n", sts.ObjectQMgrName);

    len = min(sts.SubName.VSLength,sizeof(SubName)-1);
    if (len)
    {
      SubName[len] = 0;
      printf("    SubName    :'%s'\n", SubName);
      printf("    SubOptions :%d\n"  , sts.SubOptions);
    }
    len = min(sts.ObjectString.VSLength,sizeof(ObjectString)-1);
    if (len)
    {
      SubName[len] = 0;
      printf("    ObjectString:'%s'\n", ObjectString);
      printf("    OpenOptions :%d\n"  , sts.OpenOptions);
    }
  }
}
/**********************************************************************/
/* Function : EventHandler                                            */
/* Purpose  : Simple event handler to report any async. events        */
/**********************************************************************/
static MQLONG EventHandler(MQHCONN        hConn,
                           MQMD          *pMsgDesc,
                           MQGMO         *pGetMsgOpts,
                           MQBYTE        *Buffer,
                           MQCBC         *pContext)
{
  MQ   ** pMq = (MQ **)&pContext->CallbackArea;
  char ** pp = (char **)Buffer;
  char  * msg, * addr;
  int   * port;
  MQ    * Mq  = *pMq;
  switch(pContext->CallType)
  {
    case MQCBCT_START_CALL:
         printf("EH : Start call !!!!\n");
         break;
    case MQCBCT_STOP_CALL:
         printf("EH : Stop call !!!!\n");
         break;
    case MQCBCT_REGISTER_CALL:
         printf("EH : Register call\n");
         break;
    case MQCBCT_DEREGISTER_CALL:
         printf("EH : Deregister call\n");
         break;
    case MQCBCT_EVENT_CALL:
         printf("EH : Event call RC(%d) %s\n",pContext->Reason,MQReason(pContext->Reason));
         switch(pContext->Reason)
         {
           case MQRC_RECONNECTING:
           case MQRC_RECONNECTED:
           case MQRC_RECONNECT_FAILED:
                QueryMQSTAT(Mq);
           default:
                break;
         }
         break;
    case MQCBCT_MC_EVENT_CALL:
         printf("EH : Event call RC(%d) %s\n",pContext->Reason,MQReason(pContext->Reason));
         msg  = *pp++;
         addr = *pp++;
         port = (int *)*pp;

         printf("  %s - %s(%d)\n",msg,addr,*port);
         break;
  default:
          printf("Unknown calltype %d\n",pContext->CallType);
          break;
  }

  return 0L;
}
/**********************************************************************/
/* Function : RegisterEventHandler                                    */
/* Purpose  : Register an event handler                               */
/**********************************************************************/
static void RegisterEventHandler(MQ     * Mq,
                                 MQLONG * CompCode,
                                 MQLONG * Reason)
{
  MQCBD EhCbd = {MQCBD_DEFAULT};
  MQ ** pMq = (MQ **)&EhCbd.CallbackArea;

  *pMq = Mq;

  EhCbd.CallbackType     = MQCBT_EVENT_HANDLER;
  EhCbd.Options          = MQCBDO_EVENT_CALL | MQCBDO_MC_EVENT_CALL;
  EhCbd.CallbackFunction = (MQPTR) EventHandler;

  while(1)
  {

    IMQCB(Mq->Api,
          Mq->hQm,
          MQOP_REGISTER,
         &EhCbd,
          MQHO_NONE,
          NULL,
          NULL,
          CompCode,
          Reason);
                                       /* Did it work ?                */
    if (*CompCode != MQCC_FAILED)
    {
      printf("Event Handler Registered\n");
      goto MOD_EXIT;
    }
                                       /* Nope....have we asked for MC */
    if (EhCbd.Options & MQCBDO_MC_EVENT_CALL)
    {
      EhCbd.Options &= ~MQCBDO_MC_EVENT_CALL;
      continue;
    }
                                       /* Failed                      */
    printf("Event Handler Register Failed RC(%d)\n",*Reason);
    goto MOD_EXIT;
  }
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : ConnectQm                                               */
/* Purpose  : Connect to a Queue Manager                              */
/**********************************************************************/
void ConnectQm(MQ     * Mq,
               MQCNO  * pCno,
               MQLONG * pCompCode,
               MQLONG * pReason)
{
  int k;

  fprintf(stderr,"Connecting ...");
  FORCE_STDERR;

  if (Options & qoUSE_MQCONNX)
  {
    if (Pause) { fprintf(stderr,"About to MQCONNX\n"); getchar(); }
    IMQCONNX(Mq->Api,
             Mq->Qm,
             pCno,
            &Mq->hQm,
             pCompCode,
             pReason);
    if (*pCompCode == MQCC_FAILED)
    {
      fprintf(stderr,"failed.\n");
      MQError("MQCONNX",Mq->Qm,*pReason);
      if (ConfirmExit()) goto MOD_EXIT;
    }

    if (verbose >= VL_MQAPI)
    {
      if (pCno->Version >= MQCNO_VERSION_5)
      {
        fprintf(stderr, "Connection Id '");
        for (k=0; k<MQ_CONNECTION_ID_LENGTH; k++)
        {
          fprintf(stderr, "%2.2x", pCno->ConnectionId[k]);
        }
        fputc('\'',stderr);
        fputc('\n',stderr);
      }
    }
  }
  else
  {
    if (Pause) { fprintf(stderr,"About to MQCONN\n"); getchar(); }
    IMQCONN(Mq->Api,
            Mq->Qm,
           &Mq->hQm,
            pCompCode,
            pReason);
    if (*pCompCode == MQCC_FAILED)
    {
      fprintf(stderr,"failed.\n");
      MQError("MQCONN",Mq->Qm,*pReason);
      if (ConfirmExit()) goto MOD_EXIT;
    }
  }

  if (verbose >= VL_MQAPI) MQError("MQCONN",Mq->Qm,*pReason);
  if (verbose >= VL_MQDETAILS) fprintf(stderr,"MQHCONN = %d\n",Mq->hQm);

#ifndef MVS
  if (Options2 & qo2EVENT_HANDLER)
  {
    RegisterEventHandler(Mq, pCompCode,pReason);
  }
#endif
                                       /* Get the name                */
  /*  printf("!!!Warning Query QM Name removed !!!\n"); */
  QueryQMName(Mq,Mq->Qm);
  /********************************************************************/
  /* Connect to the Local Queue Manager                               */
  /********************************************************************/
                                       /* NULL-Terminate the strings  */
  Mq->Qm[MQ_Q_MGR_NAME_LENGTH]=0;
  strip(Mq->Qm,MQ_Q_MGR_NAME_LENGTH);

  fprintf(stderr,"connected to '%.48s'.\n",Mq->Qm);

MOD_EXIT:
  ;
}

void ReportTimes(long * pOps,int Reduce)
{
  double   TotalTime   = 0;            /* Milliseconds                */
  int      Ops        = *pOps;
#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  gettimeofday(&EndTime,&tz);
  TotalTime = ((EndTime.tv_sec  - StartTime.tv_sec)*1000+
               (EndTime.tv_usec - StartTime.tv_usec) / 1000);
  TotalTime -= Reduce;
  if (TotalTime < 0) TotalTime = 0;
#endif
#ifdef WIN32
  char CharTotalTime[20];
  char CharAvgTime[20];

  QueryPerformanceCounter(&EndTime);
                      /* Time difference in milliseconds */
  TotalTime = (EndTime.QuadPart - StartTime.QuadPart) *
              (double)1000000.0/(double)Frequency.QuadPart;


  TotalTime -= Reduce*1000;
  if (TotalTime < 0) TotalTime = 0;
  PrintTime(CharTotalTime,TotalTime);
  PrintTime(CharAvgTime  ,Ops ? TotalTime/Ops : 0);
                      /* Convert to milliseconds     */
  TotalTime /= 1000;

  fprintf(stderr,"%d Iterations in %s, Average = %s or %.1f per second\n",
          Ops,
          CharTotalTime,
          CharAvgTime,
          TotalTime ? Ops * 1000/TotalTime : 99999999);

  QueryPerformanceCounter(&StartTime);

#else
  fprintf(stderr,"%d Iterations in %.2f ms, Average = %.2fms or %.1f per second\n",
          Ops,
          TotalTime,
          Ops       ? TotalTime/Ops          : 0,
          TotalTime ? Ops   * 1000/TotalTime : 99999999);

  gettimeofday(&StartTime,&tz);
#endif
  *pOps = 0;
}
/**********************************************************************/
/* Function : Main program                                            */
/* Purpose  : Get parameters from command line                        */
/* Description                                                        */
/* -----------                                                        */
/* This program acts like a pipe taking input from one source and     */
/* outputting to another source.                                      */
/*                                                                    */
/* Input : stdin                                                      */
/*         MQSeries Queue                                             */
/*                                                                    */
/* Output: stdout                                                     */
/*         MQSeries Queue                                             */
/**********************************************************************/
int main(int     argc,
         char ** argv)
{
  MQLONG   OpenOutputFlags = MQOO_OUTPUT;
  MQOD     IQueueDesc      = { MQOD_DEFAULT };
  MQOD     OQueueDesc      = { MQOD_DEFAULT };
  MQOD     RQueueDesc      = { MQOD_DEFAULT };
  MQSD     SubDesc         = { MQSD_DEFAULT };
  MQSRO    sro             = { MQSRO_DEFAULT };
  MQMD2    Mdesc           = { MQMD2_DEFAULT };
  MQMD     ReplyMdesc      = { MQMD_DEFAULT };
  MQGMO    gmo             = { MQGMO_DEFAULT };
  MQPMO    pmo             = { MQPMO_DEFAULT };
  MQCNO    Cno1            = { MQCNO_DEFAULT };
  MQCNO    Cno2            = { MQCNO_DEFAULT };
  MQCNO  * pCno            = &Cno1;
  MQCSP    Csp1            = { MQCSP_DEFAULT };
  MQCSP    Csp2            = { MQCSP_DEFAULT };
  MQCSP  * pCsp            = &Csp1;
  MQCD   * pMqcd           = &Mqcd1;
  MQAIR    DefMqair        = { MQAIR_DEFAULT };
  MQAIR    Mqair[MAX_MQAIR];
  MQCHAR64 LDAPUser[MAX_MQAIR];
  MQSCO    Mqsco           = { MQSCO_DEFAULT };
  MQHOBJ   hIObj           = MQHO_UNUSABLE_HOBJ;
  MQHOBJ   hOObj[MAX_DEST] = {MQHO_UNUSABLE_HOBJ};
  MQHOBJ   hRObj           = MQHO_UNUSABLE_HOBJ;
  MQHOBJ   hSub            = MQHO_UNUSABLE_HOBJ;
  MQTMC2   Tm,*pTm;
  int      LineRead        = FALSE;
  char     Parms[512], * pParms;
  FILE   * In      = stdin;
  FILE   * OutFile = NULL;
  char   * myargs[30];
  char   * library1        = NULL;
  char   * library2        = NULL;
  char   * filter          = NULL;
  char   * SelectionString = NULL;
  char     ResObjectString[100];
  char   * pEnv;
  MQLONG   MQEnc           = 0;
  MQLONG   MQCcsid         = 0;
  MQCHAR48 ReplyQ          = "";
  MQCHAR48 ReplyQMgr       = "";
  MQCHAR48 Qm1             = "";
  MQCHAR48 Qm2             = "";
  int      c;
  int      rc;
  MQLONG   CompCode,Reason,Reason2;
  char     msgopts[20];
  MQLONG   MsgSize, MsgLen, UserIdSize, PasswordSize;
  MQLONG   OpenFlags       = 0;
  MQLONG   WaitInterval;
  MQLONG   Report          = 0;
  int      finished        = 0;
  char   * pMsg,* pMsgArea,* pEnd,* pNewMsg,* pUserId,* pPassword;
  long     i,j;
  long     reps,ngets,delay,size,commits,commitmsg,chk;
  MQLONG   Persistence  = -1;
  MQLONG   Segmentation = 0;
  MQLONG   MQMDVersion  = 1;
  MQLONG   MQPMOVersion = 1;
  MQLONG   MQGMOVersion = 1;
  MQLONG   MQODVersion  = 1;
  MQLONG   MQCNOVersion = 1;
  MQLONG   DestCount    = 0;
  MQLONG   DestPerHobj,Dest;
  MQOR     OObjectRecord[MAX_DEST] = {0};
  MQRR     OResponseRecord[MAX_DEST];
  char     IQ[100];
  char     ITopic[100];                /* Input Topic object name     */
  char     OTopic[100];                /* Output Topic object name    */
  char   * pTopicString   = NULL;      /* Topic string to subscribe to*/
  char   * pOTopicString  = NULL;      /* Topic string to publish to  */
  char   * pSubName       = NULL;      /* Subscription Name           */
  char   * pSubUserData   = NULL;      /* Subscription User Data      */
  MQLONG   TopicStrLen    = 0;         /* Length of Topic String Sub  */
  MQLONG   OTopicStrLen   = 0;         /* Length of Topic String Pub  */
  MQLONG   SubNameLen     = 0;         /* Length of Subscription Name */
  MQLONG   SubUserDataLen = 0;         /* Length of Sub User Data     */
  MQLONG   SubLevel       = 1;         /* Set to default Sublevel init*/
  MQLONG   PubLevel       = 9;         /* Set to default Publevel init*/
  char     StringType;
  MQLONG   Expire;
  MQLONG   MaxMsgLen = -1;
  int      ArgumentOK = 0;
  int      GetSleep;
  char     Separator = 0;
  MQLONG   MsgLimit  = -1;
  MQLONG   MsgType;
  MQLONG   OriginalSubOptions = 0;     /* Save input version          */
  MQCHAR12 Userid;
  BOOL     bFileInput = FALSE;         /* Used to remove > prompt     */

  sprintf(Parms,"IBM MQ Q Program [Build:%s] \n",
                 __DATE__);
  UsageLine(Parms);

#ifdef WIN32
  if (_isatty(_fileno(stdout)))
  {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
      CONSOLE_SCREEN_BUFFER_INFO info;
      GetConsoleScreenBufferInfo(hConsole,&info);
      ScreenWidth  = info.dwSize.X - 3;
      ScreenHeight = info.dwSize.Y;

      // CloseHandle(hConsole); Don't close it closes stdout !
    }
  }
  else
  {
    ScreenHeight = 500;
  }
#endif

  if (pEnv = getenv("MA01_SCREENHEIGHT"))
  {
    ScreenHeight = atoi(pEnv);
    if (ScreenHeight <= 3) ScreenHeight = 24;
  }
  if (pEnv = getenv("MA01_PERF_LIMIT"))
  {
    PerformanceLimit = atoi(pEnv);
  }
  /********************************************************************/
  /* Set default values                                               */
  /********************************************************************/
  memset(&Mq ,0,sizeof(Mq));
  memset(&Mq2,0,sizeof(Mq));
  Mq.hQm        = MQHC_UNUSABLE_HCONN;
  Mq.Qm[0]      = 0;
  Mq2.hQm       = MQHC_UNUSABLE_HCONN;
  Mq2.Qm[0]     = 0;
  Mdesc.Version = MQMD_VERSION_1;
  verbose       = 0;
  WaitInterval  = 0;
  msgopts[0]    = 0;
  commits       = 0;
  commitmsg     = 0;
  IQ[0]         = 0;
  ITopic[0]     = 0;
  OTopic[0]     = 0;
  memset(Qm1,' ',48);
  memset(Qm2,' ',48);
#ifdef MVS
  Options2 |= qo2USE_INDIVIDUAL_QUEUES;
#endif
  /********************************************************************/
  /* Get a buffer area                                                */
  /********************************************************************/
  MsgSize = 2048;
  pMsgArea = (char *)QGetMem(MsgSize);
  if (!pMsgArea)
  {
    fprintf(stderr,"Failed to allocate %ld bytes\n",MsgSize);
    Reason = MQRC_STORAGE_NOT_AVAILABLE;
    if (ConfirmExit()) goto MOD_EXIT;
  }
  pMsg = pMsgArea;
  /********************************************************************/
  /* Have we been triggered                                           */
  /********************************************************************/
  if (argc == 2)
  {
    pTm = (MQTMC2 *)argv[1];
    if (memcmp(pTm->StrucId,MQTMC_STRUC_ID,4)==0)
    {
      fputs("Program Triggered\n",stderr);
      memcpy(&Tm,pTm,sizeof(Tm));
                                       /* Take the parameters         */
      memcpy(IQ,Tm.QName,48);
      IQ[48]   =0;
      memcpy(Mq.Qm,Tm.QMgrName,48);
      Mq.Qm[48]=0;
                                       /* Now copy in parameters      */
      pParms  = Parms;
                                       /* Trigger Data                */
      MsgLen  = MIN(strlen(Tm.TriggerData),sizeof(Tm.TriggerData));
      memcpy(pParms,Tm.TriggerData,(int)MsgLen);
      pParms    += MsgLen;
      *pParms ++ = ' ';
                                       /* User Data                   */
      MsgLen  = MIN(strlen(Tm.UserData),sizeof(Tm.UserData));
      memcpy(pParms,Tm.UserData   ,(int)MsgLen);
      pParms += MsgLen;
      *pParms=0;
                                       /* Construct argv list         */
      pParms    = Parms;
      argc      = 1;
      myargs[0] = argv[0];
      argv      = myargs;
      while (*pParms)
      {
        while(*pParms == ' ') {*pParms=0;pParms ++;}
        if (*pParms==0) break;
        argv[argc++] = pParms;
        while(*pParms != ' ' && *pParms != 0) pParms++;
      }
    }
  }
  /********************************************************************/
  /* Print out parameters                                             */
  /********************************************************************/
#ifdef DISPLAY_PARMS
  {
    int p;
    for (p = 0; p < argc ; p++) fprintf(stderr,"Arg %d : '%s'\n",p, argv[p]);
  }
#endif
  /********************************************************************/
  /* Grab the parameters from the command line                        */
  /********************************************************************/
  while ((c=getopt(argc, argv,
          "A:a:bc:C:d:eEf:F:g:h:H:i:I:j:kl:L:m:M:n:N:o:O:p:P:qr:RsS:T:tu:U:v:V:w:W:x:X:y:zZ:$:#:1!=:*?")) !=  EOF)
  {
    switch(c)
    {
      case '*':
           Repeat   = TRUE;
           break;
                                       /* Return value                */
      case 'V':
          if (strchr(optarg,'z')) Options2 |= qo2RETVAL_ZERO;
          if (strchr(optarg,'c')) Options2 |= qo2RETVAL_MQCC;
          break;

      case '=':
           if (*optarg == 'n')
           {
             MaxMsgLen = atoi(optarg+1);
           }
           else
           {
             Options2 |= qo2TRUNCATE;
             MaxMsgLen = atoi(optarg);
           }
           if (MaxMsgLen > MsgSize)
           {
             MsgSize = MaxMsgLen;
             QFreeMem(pMsgArea);
             pMsgArea = (char *)QGetMem(MsgSize);
             if (!pMsgArea)
             {
               fprintf(stderr,"Failed to allocate %ld bytes\n",MsgSize);
               Reason = MQRC_STORAGE_NOT_AVAILABLE;
               if (ConfirmExit()) goto MOD_EXIT;
             }
             pMsg    = pMsgArea;
           }
           break;
                                       /* Don't use exit handler      */
      case '!':
           Options |= qoNO_HANDLER;
           break;
                                       /* Local Queue Manager         */
      case 'm':
           if (Qm1[0]!=' ')
           {
             strncpy(Qm2,optarg,sizeof(Qm1));
             Options2 |= qo2DUAL_QM;
             pCno  = &Cno2;
             pMqcd = &Mqcd2;
             pCsp  = &Csp2;
           }
           else
           {
             strncpy(Qm1 ,optarg,sizeof(Qm1));
           }
           break;
                                       /* Message                     */
      case 'M':
           if (strcmp(optarg,"[]")) strcpy(pMsg,optarg);
           else
           {
             for (i=1; i<255; i++) pMsg[i-1] = (unsigned char)i;
             pMsg[255] = 0;
           }
           Options |= qoEXPLICIT_MSG;
           MsgLen = strlen(pMsg);
           break;
                                       /* Verbose option              */
      case 'v':
           verbose = 1;
           if (strchr(optarg,'1')) verbose = 1;
           if (strchr(optarg,'2')) verbose = 2;
           if (strchr(optarg,'3')) verbose = 3;
           if (strchr(optarg,'4')) verbose = 4;
           if (strchr(optarg,'5')) verbose = 5;
           if (strchr(optarg,'p')) Pause   = 1;
           if (strchr(optarg,'P'))
           {
             printf("Command Line Parameters\n");
             for (i=0; i<argc;i++)
             {
               printf("  argv[%d]='%s'\n",i,argv[i]);
             }
             printf("-----------------------\n");
           }
           if (strchr(optarg,'e')) ExitConfirm = 1;
           break;

      case 'C':
           if (strchr(optarg,'a')) Options  |= qoCONTEXT_PASS_ALL;
           if (strchr(optarg,'i')) Options  |= qoCONTEXT_PASS_IDENTITY;
           if (strchr(optarg,'A')) Options  |= qoCONTEXT_SET_ALL;
           if (strchr(optarg,'I')) Options  |= qoCONTEXT_SET_IDENTITY;
           if (strchr(optarg,'n')) Options2 |= qo2NO_CONTEXT;
           break;

      case 'x':
           Options |= qoUSE_MQCONNX;
#ifndef MVS
           if (strchr(optarg,'f')) pCno->Options |= MQCNO_FASTPATH_BINDING;
           if (strchr(optarg,'i')) pCno->Options |= MQCNO_ISOLATED_BINDING;
           if (strchr(optarg,'s')) pCno->Options |= MQCNO_STANDARD_BINDING;
           if (strchr(optarg,'b')) pCno->Options |= MQCNO_HANDLE_SHARE_BLOCK;
           if (strchr(optarg,'m')) pCno->Options |= MQCNO_RECONNECT_Q_MGR;
           if (strchr(optarg,'n')) pCno->Options |= MQCNO_HANDLE_SHARE_NO_BLOCK;
           if (strchr(optarg,'r')) pCno->Options |= MQCNO_RECONNECT;
           if (strchr(optarg,'R')) pCno->Options |= MQCNO_RECONNECT_DISABLED;
#endif
           if (strchr(optarg,'q')) Options       |= qoNO_QUIESCE;
           if (strchr(optarg,'S')) pCno->Options |= MQCNO_NO_CONV_SHARING;
           if (strchr(optarg,'N')) Options       |= qoNO_MQDISC;
           if (strchr(optarg,'l')) Options2      |= qo2RESOLVE_LOCAL;
           if (strchr(optarg,'C')) Options2      |= qo2CLOSE_HSUB;
           if (strchr(optarg,'t'))
           {
             fprintf(stderr,"Choose Connection Tag option\n");
             fprintf(stderr," 1: Serialize within Queue Manager\n");
             fprintf(stderr," 2: Serialize within Queue Sharing Group\n");
             fprintf(stderr," 3: Restrict within Queue Manager\n");
             fprintf(stderr," 4: Restrict within Queue Sharing Group\n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg=='1') pCno->Options |= MQCNO_SERIALIZE_CONN_TAG_Q_MGR;
             if (*pMsg=='2') pCno->Options |= MQCNO_SERIALIZE_CONN_TAG_QSG;
             if (*pMsg=='3') pCno->Options |= MQCNO_RESTRICT_CONN_TAG_Q_MGR;
             if (*pMsg=='4') pCno->Options |= MQCNO_RESTRICT_CONN_TAG_QSG;
             fprintf(stderr,"Enter Connection Tag\n");
             fgets(pMsg,MsgSize,stdin);
             strcpy(pCno->ConnTag,pMsg);
             if (MQCNOVersion < 3) MQCNOVersion = 3;
           }
           if (strchr(optarg,'u'))
           {
             UserIdSize = 2048;
             pUserId = (char *)QGetMem(UserIdSize);
             if (!pUserId)
             {
               fprintf(stderr,"Failed to allocate %ld bytes\n",UserIdSize);
               Reason = MQRC_STORAGE_NOT_AVAILABLE;
               if (ConfirmExit()) goto MOD_EXIT;
             }
             PasswordSize = 2048;
             pPassword = (char *)QGetMem(PasswordSize);
             if (!pPassword)
             {
               fprintf(stderr,"Failed to allocate %ld bytes\n",PasswordSize);
               Reason = MQRC_STORAGE_NOT_AVAILABLE;
               if (ConfirmExit()) goto MOD_EXIT;
             }
             fprintf(stderr,"Enter User ID\n");
             fgets(pUserId,UserIdSize,stdin);
             fprintf(stderr,"Enter Password\n");
             fgets(pPassword,PasswordSize,stdin);
             pCsp->AuthenticationType = MQCSP_AUTH_USER_ID_AND_PWD;
             pCsp->CSPUserIdPtr       = pUserId;
             pCsp->CSPUserIdLength    = strlen(pUserId);
             pCsp->CSPPasswordPtr     = pPassword;
             pCsp->CSPPasswordLength  = strlen(pPassword);
             pCno->SecurityParmsPtr   = pCsp;
             if (MQCNOVersion < 5) MQCNOVersion = 5;
           }
#ifndef MVS                                    /* No clients from MVS */
           if (strchr(optarg,'v')) Options2    |= qo2EVENT_HANDLER;
           if (strchr(optarg,'c'))
           {
             Options     |= qoUSER_MQCD;
             pMqcd->Version = 5;
             fprintf(stderr,"Enter Channel Name\n");
             fgets(pMsg,MsgSize,stdin);
             strcpy(pMqcd->ChannelName,pMsg);

             fprintf(stderr,"Enter Channel Type (NULL for CLNTCONN) \n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg) pMqcd->ChannelType   = atoi(pMsg);
                   else pMqcd->ChannelType   = MQCHT_CLNTCONN;

             fprintf(stderr,"Enter Transport Type (NULL for TCP)\n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg) pMqcd->TransportType = atoi(pMsg);
                   else pMqcd->TransportType = MQXPT_TCP;

             fprintf(stderr,"Enter connection name (NULL for localhost)\n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg) strcpy(pMqcd->ConnectionName,pMsg);
                   else strcpy(pMqcd->ConnectionName,"localhost");

             fprintf(stderr,"Do you want exits ?\n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg == 'y')
             {
               MQCHAR128 * pExitName;
               MQCHAR32  * pExitUserArea;
               MQCHAR128 * pExitNameStart;
               MQCHAR32  * pExitUserAreaStart;
               EXITARRAY * pExitArray;
               int         ExitsUsed          = 0;
               int         TotalExitsUsed     = 0;
               BOOL        LastOne;


               if (Options2 & qo2DUAL_QM)
               {
                 pExitName     = &ExitName2[0];
                 pExitUserArea = &ExitUserData2[0];
                 pExitArray    = ExitArray2;
               }
               else
               {
                 pExitName     = &ExitName1[0];
                 pExitUserArea = &ExitUserData1[0];
                 pExitArray    = ExitArray1;
               }
               /*******************************************************/
               /* Now we'll go through exit exit type                 */
               /*******************************************************/
               for (i=0; i<sizeof(ExitArray1)/sizeof(ExitArray1[0]);i++)
               {
                                       /* Keep going until the last   */
                 pExitNameStart      = pExitName;
                 pExitUserAreaStart  = pExitUserArea;
                 ExitsUsed           = 0;
                 LastOne             = FALSE;

                 while (!LastOne)
                 {
                   fprintf(stderr,"Enter %s exit\n",pExitArray[i].ExitName);
                   fgets(pMsg,MsgSize,stdin);
                   /***************************************************/
                   /* Have we been given one                          */
                   /***************************************************/
                   if (*pMsg)
                   {
                     /*************************************************/
                     /* Ok, we have a new exit name                   */
                     /*************************************************/
                     if (TotalExitsUsed >= MAX_EXITS)
                     {
                       printf("Sorry, only %d exits supported\n",MAX_EXITS);
                       Reason = MQRC_OPTIONS_ERROR;
                       if (ConfirmExit()) goto MOD_EXIT;
                     }

                     ExitsUsed ++;
                     memset(pExitName,' ',128);
                     memcpy(pExitName,pMsg,min(strlen(pMsg),128));

                     fprintf(stderr,"Enter %s user data\n",pExitArray[i].ExitName);
                     fgets(pMsg,MsgSize,stdin);

                     memset(pExitUserArea,' ',32);
                     memcpy(pExitUserArea,pMsg,min(strlen(pMsg),32));

                     TotalExitsUsed++;
                     pExitName     ++;
                     pExitUserArea ++;
                                         /* Must be last if no multi    */
                     LastOne = (pExitArray->MultExit == NULL);
                   }
                   else
                     LastOne = TRUE;
                 }

                 /*****************************************************/
                 /* Finished with this exit type                      */
                 /*****************************************************/
                 if (ExitsUsed)
                 {
                   if (ExitsUsed > 1)
                   {
                    *pExitArray[i].MultExit     = (char *)pExitNameStart;
                    *pExitArray[i].MultExitArea = (char *)pExitUserAreaStart;
                    *pExitArray[i].MultDefined  = ExitsUsed;
                   }
                   else
                   {
                     memcpy(pExitArray[i].SingleExit    ,pExitNameStart    ,128);
                     memcpy(pExitArray[i].SingleExitArea,pExitUserAreaStart,32);
                   }
                 }
               }
             }
             if (MQCNOVersion < 2) MQCNOVersion = 2;
             pCno->ClientConnPtr = pMqcd;

             fprintf(stderr,"Do you want SSL ?\n");
             fgets(pMsg,MsgSize,stdin);
             if (*pMsg == 'y')
             {
               int i;

               pMqcd->Version = 7;

               fprintf(stderr,"Enter Cipher Spec [NULL for RC4_SHA_US]\n");
               fgets(pMqcd->SSLCipherSpec,MQ_SSL_CIPHER_SPEC_LENGTH,stdin);
               if (!*pMqcd->SSLCipherSpec) strcpy(pMqcd->SSLCipherSpec,"RC4_SHA_US");

               if (MQCNOVersion < 4) MQCNOVersion = 4;
               pCno->SSLConfigPtr  = &Mqsco;

               fprintf(stderr,"Enter Key Repository (NULL for c:\\mqm\\key)\n");
               fgets(Mqsco.KeyRepository,MQ_SSL_KEY_REPOSITORY_LENGTH,stdin);
               if (!*Mqsco.KeyRepository) strcpy(Mqsco.KeyRepository,"c:\\mqm\\key");
               fprintf(stderr,"Enter CryptoHardware\n");
               fgets(Mqsco.CryptoHardware,256,stdin);

               i = 0;
               while (i < MAX_MQAIR)
               {
                 memcpy(&Mqair[i],&DefMqair,sizeof(MQAIR));
                 Mqair[i].Version = MQAIR_CURRENT_VERSION;

                 fprintf(stderr,"Enter Authinfo Conn Name\n");
                 fgets(Mqair[i].AuthInfoConnName,264,stdin);
                 if (!Mqair[i].AuthInfoConnName[0]) break;

                 Mqsco.AuthInfoRecPtr = &Mqair[0];

                 Mqair[i].AuthInfoType = MQAIT_CRL_LDAP;

                 fprintf(stderr,"Enter LDAP User Name\n");
                 fgets(LDAPUser[i],64,stdin);
                 if (*LDAPUser[i])
                 {
                   Mqair[i].LDAPUserNamePtr    = (char *)&LDAPUser[i];
                   Mqair[i].LDAPUserNameLength = strlen((char *)&LDAPUser[i]);

                   fprintf(stderr,"Enter LDAP Password\n");
                   fgets(Mqair[i].LDAPPassword,32,stdin);
                 }
                 i++;
               }
               Mqsco.AuthInfoRecCount = i;
             }
           }
#endif
           break;
                                       /* Use MQPUT1 instead          */
      case '1':
           Options |= qoUSE_MQPUT1;
           break;
                                       /* Structure version numbers   */
      case '#':
           for (i=0; i<(int)strlen(optarg); i++)
           {
             switch(optarg[i])
             {
               case 'm':
                    i++;
                    MQMDVersion  = optarg[i] - '0';
                    break;
               case 'o':
                    i++;
                    MQODVersion  = optarg[i] - '0';
                    break;
               case 'g':
                    i++;
                    MQGMOVersion = optarg[i] - '0';
                    break;
               case 'p':
                    i++;
                    MQPMOVersion = optarg[i] - '0';
                    break;
               case 'x':
                    i++;
                    MQCNOVersion = optarg[i] - '0';
               case 'c':
               case 'C':
                    MQPMOVersion = MQPMO_CURRENT_VERSION;
                    MQGMOVersion = MQGMO_CURRENT_VERSION;
                    MQODVersion  = MQOD_CURRENT_VERSION;
                    MQMDVersion  = MQMD_CURRENT_VERSION;
                    MQCNOVersion = MQCNO_CURRENT_VERSION;
                    break;
               default:
                    MQMDVersion   = optarg[i] - '0';
                    MQODVersion   = MQMDVersion;
                    MQPMOVersion  = MQMDVersion;
                    MQGMOVersion  = MQMDVersion;
                    break;
             }
           }
           break;
                                       /* Wait interval               */
      case 'w':
           WaitInterval = atol(optarg);
           if (WaitInterval < 0 || gmo.WaitInterval > 999999)
             WaitInterval = MQWI_UNLIMITED;
           else
             WaitInterval *= 1000L;
           if (WaitInterval) gmo.Options  |= MQGMO_WAIT;
           break;

      case 'W':
           Options |= qoGET_SLEEP;
           GetSleep = atol(optarg);
           break;
                                       /* Input Queue                 */
                                       /* i- Browse I - Get           */
      case 'I':
           Options &= ~qoBROWSE;
                                       /* DELIBERATELY NO BREAK HERE  */
      case 'i':
           strncpy(IQ,optarg,sizeof(IQ));
           break;
                                       /* Format name                 */
      case 'j':
           memset(MyFormat,' ',sizeof(MyFormat));
           if (strcmp(optarg,"NONE"))
           {
             memcpy(MyFormat,optarg,MIN(sizeof(MyFormat),strlen(optarg)));
           }
           Options |= qoUSER_FORMAT;
           break;
                                       /* Output Queue                */
      case 'O':
                                       /* Force targets               */
           OpenOutputFlags &= ~MQOO_BIND_NOT_FIXED;
           OpenOutputFlags |=  MQOO_BIND_ON_OPEN;
                                       /* DELIBERATELY NO BREAK HERE  */
      case 'o':
           switch(DestCount)
           {
             case 0: ParseQueueName(Separator,
                                    optarg,
                                    OObjectRecord[DestCount].ObjectQMgrName,
                                    OObjectRecord[DestCount].ObjectName);
                     break;

             case 1:
                     /*************************************************/
                     /* Are we using a distribution list ?            */
                     /*************************************************/
                     if (!(Options2 & qo2USE_INDIVIDUAL_QUEUES))
                     {
                       if (Options & qoUSE_OFFSETS)
                       {
                         OQueueDesc.ObjectRecOffset   = (char *)&OObjectRecord -
                                                        (char *)&OQueueDesc;
                         if (! (Options & qoNO_RESPONSE))
                         {
                           OQueueDesc.ResponseRecOffset   = (char *)&OResponseRecord -
                                                            (char *)&OQueueDesc;
#ifndef MVS /* Seems only the OD parts of dist lists are in cmqc.h !! */
                           pmo.ResponseRecOffset   = (char *)&OResponseRecord -
                                                     (char *)&pmo;
#endif
                         }
                       }
                       else
                       {
                         OQueueDesc.ObjectRecPtr      = &OObjectRecord;
                         if (! (Options & qoNO_RESPONSE))
                         {
                           OQueueDesc.ResponseRecPtr    = &OResponseRecord;
#ifndef MVS /* Seems only the OD parts of dist lists are in cmqc.h !! */
                           pmo.ResponseRecPtr           = &OResponseRecord;
#endif
                         }
                       }
                       if (MQODVersion  < 2) MQODVersion  = 2;
                       if (MQPMOVersion < 2) MQPMOVersion = 2;
                     }
                                       /* DELIBERATELY NO BREAK HERE  */
             default:
                     if (DestCount == MAX_DEST)
                     {
                       fprintf(stderr,"Sorry Q only supports %d output destinations.\n",
                              MAX_DEST);
                       Reason = MQRC_OPTIONS_ERROR;
                       if (ConfirmExit()) goto MOD_EXIT;
                     }
                     ParseQueueName(Separator,
                                    optarg,
                                    OObjectRecord[DestCount].ObjectQMgrName,
                                    OObjectRecord[DestCount].ObjectName);
                     break;
           }
           DestCount++;
           break;
                                       /* Quiet option                */
      case 'q':
           Options |= qoQUIET;
           break;
                                       /* Reply Queue                 */
      case 'r':
           Options &= ~qoBROWSE;
           Options |=  qoREQUEST;

           if (*optarg == '+')
           {
             Options |= qoWAIT_REPLY;
             ParseQueueName(Separator,
                            optarg+1,
                            ReplyQMgr,
                            ReplyQ);
           }
           else
             ParseQueueName(Separator,
                            optarg,
                            ReplyQMgr,
                            ReplyQ);
           break;

      case 'R':
           Options |= qoNO_RESPONSE;
           break;
                                       /* Time Reporting              */
      case 't':
           Options |= qoREPORT_TIMES;
           break;
#ifndef MVS                                    /* No clients from MVS */
                                       /* Library name                */
      case 'l':
           if (Options2 & qo2DUAL_QM) library2 = optarg;
                                 else library1 = optarg;
           break;
#endif
                                       /* Message Limit               */
      case 'L':
           MsgLimit = atoi(optarg);
           break;

           /***********************************************************/
           /* Is this a load of a hex message                         */
           /***********************************************************/
      case 'X':
           if (IQ[0])
           {
             fprintf(stderr,
                     "Sorry to load a file you can not also specify an input queue\n");
             fprintf(stderr,
                     "Do not specify either 'i' or 'I'\n");
             Reason = MQRC_OPTIONS_ERROR;
             goto MOD_EXIT;
           }
           if (!DestCount)
           {
             fprintf(stderr,
                     "Sorry to load a file you must specify an output queue\n");
             fprintf(stderr,
                     "Specify '-o'\n");
             Reason = MQRC_OPTIONS_ERROR;
             goto MOD_EXIT;
           }
           In=fopen(optarg,"rb");
           if (!In)
           {
             fprintf(stderr,"Can not open input file '%s'\n",optarg);
             goto MOD_EXIT;
           }
           pMsg = LoadHexFile(In,&MsgLen);
           if (!pMsg) goto MOD_EXIT;

           Options  |= qoEXPLICIT_MSG;
           Options2 |= qo2NO_SPECIAL_MSG;
           break;

           /***********************************************************/
           /* Ok, this is an load/unload file. User must have already */
           /* decided whether they are loading or unloading           */
           /***********************************************************/
      case 'F':
           if (DestCount && IQ[0])
           {
             fprintf(stderr,
                     "Sorry to use a load/unload file you must either be loading or unloading\n");
             fprintf(stderr,
                     "Specify *either* '-i' or '-o' before the -F flag\n");
             Reason = MQRC_OPTIONS_ERROR;
             goto MOD_EXIT;
           }
           if (!DestCount && !IQ[0])
           {
             fprintf(stderr,
                     "Sorry to use a load/unload file you must either be loading or unloading\n");
             fprintf(stderr,
                     "Specify either '-i' or '-o' before the -F flag\n");
             Reason = MQRC_OPTIONS_ERROR;
             goto MOD_EXIT;
           }

           /***********************************************************/
           /* If we're reading from a queue then we must be writing   */
           /* to a file                                               */
           /***********************************************************/
           if (IQ[0])
           {
#ifdef MVS
             /*********************************************************/
             /* If requested, ensure the datasets attributes are kept */
             /* as defined for an existing dataset                    */
             /*********************************************************/
             if (*optarg == '+') OutFile = fopen(optarg+1,"wb, recfm=*");
             else
#endif
             OutFile = fopen(optarg,"wb");
             if (!OutFile)
             {
               fprintf(stderr,"Can not open output file '%s'\n",optarg);
               goto MOD_EXIT;
             }
           }
           /***********************************************************/
           /* If we're writing to a queue then we must be loading from*/
           /* this file                                               */
           /***********************************************************/
           if (DestCount)
           {
             int ThisRead;
             int TotalRead;

             In=fopen(optarg,"rb");
             if (!In)
             {
               fprintf(stderr,"Can not open input file '%s'\n",optarg);
               goto MOD_EXIT;
             }

             Options  |= qoEXPLICIT_MSG;
             Options2 |= qo2NO_SPECIAL_MSG;
             /*********************************************************/
             /* Let's find the end of the file                        */
             /*********************************************************/
             fseek(In,0,SEEK_END);

             MsgLen = ftell(In);
             if (MsgLen < 0)
             {
               fprintf(stderr,"Failed to find end of file '%s' RC(%d)\n",optarg,errno);
               goto MOD_EXIT;
             }
             /*********************************************************/
             /* Ensure we have a big enough buffer                    */
             /*********************************************************/
             if (MsgLen > MsgSize)
             {
               QFreeMem(pMsgArea);
               pMsgArea = (char *)QGetMem(MsgLen);
               if (!pMsgArea)
               {
                 fprintf(stderr,"Failed to allocate %ld bytes\n",MsgLen);
                 Reason = MQRC_STORAGE_NOT_AVAILABLE;
                 goto MOD_EXIT;
               }
             }
                                       /* Go to beginning of file     */
             rewind(In);

             /*********************************************************/
             /* Go round and read the file                            */
             /*********************************************************/
             pMsg      = pMsgArea;
             TotalRead = 0;
             while (ThisRead = fread(pMsg, 1, MsgLen - TotalRead,In))
             {
               TotalRead += ThisRead;
               pMsg      += ThisRead;
               if (TotalRead >= MsgLen) break;
             }
             if (ferror(In))
             {
               fprintf(stderr,"Error reading from file '%s'\n",optarg);
               goto MOD_EXIT;
             }
             pMsg = pMsgArea;
           }
           break;
                                       /* Input file                  */
      case 'f':
#ifdef MVS
           /*********************************************************/
           /* Remember to use record based I/O                      */
           /*********************************************************/
           if (*optarg == '=')
           {
             Options2 |= qo2RECORD_BASED_IO;
             In=fopen(optarg+1,"rb, type=record");
           }
           else
#endif
           {
             In=fopen(optarg,"r");
           }
           if (!In)
           {
             fprintf(stderr,"Can not open input file '%s'\n",
                 ((Options2 & qo2RECORD_BASED_IO) ? optarg+1 : optarg));
             goto MOD_EXIT;
           }
           bFileInput = TRUE;
           break;
                                       /* Get keys                    */
      case 'g':
           if (*optarg == 'm')
           {
             GetId((char *)GetMsgId,optarg+1);
             Options |= qoUSE_GET_MSGID;
           }
           else if (*optarg == 'c')
           {
             GetId((char *)GetCorrelId,optarg+1);
             Options |= qoUSE_GET_CORRELID;
           }
           else if (*optarg == 'g')
           {
             GetId((char *)GetGroupId,optarg+1);
             Options2 |= qo2USE_GET_GROUPID;
           }
           else if (*optarg == 'x')
           {
             optarg++;
             if (*optarg == 'm')
             {
               GetHexId((char *)GetMsgId,optarg+1);
               Options |= qoUSE_GET_MSGID;
             }
             else if (*optarg == 'c')
             {
               GetHexId((char *)GetCorrelId,optarg+1);
               Options |= qoUSE_GET_CORRELID;
             }
             else if (*optarg == 'g')
             {
               GetHexId((char *)GetGroupId,optarg+1);
               Options2 |= qo2USE_GET_GROUPID;
             }
           }
           else if (*optarg == 'p')
           {
             optarg++;
             if (*optarg == 'm')
             {
               GetId((char *)PutMsgId,optarg+1);
               Options |= qoUSE_PUT_MSGID;
             }
             else if (*optarg == 'c')
             {
               GetId((char *)PutCorrelId,optarg+1);
               Options |= qoUSE_PUT_CORRELID;
             }
             else if (*optarg == 'g')
             {
               GetId((char *)PutGroupId,optarg+1);
               Options2 |= qo2USE_PUT_GROUPID;
             }
             else if (*optarg == 'C')  /* Use MQCI New Session (CICS) */
             {
               memcpy(PutCorrelId, MQCI_NEW_SESSION, MQ_CORREL_ID_LENGTH);
               Options |= qoUSE_PUT_CORRELID;
             }
             else if (*optarg == 'x')
             {
               optarg++;
               if (*optarg == 'm')
               {
                 GetHexId((char *)PutMsgId,optarg+1);
                 Options |= qoUSE_PUT_MSGID;
               }
               else if (*optarg == 'c')
               {
                 GetHexId((char *)PutCorrelId,optarg+1);
                 Options |= qoUSE_PUT_CORRELID;
               }
               else if (*optarg == 'c')
               {
                 GetHexId((char *)PutGroupId,optarg+1);
                 Options2 |= qo2USE_PUT_GROUPID;
               }
             }
           }
           break;
                                       /* Filter string               */
      case 'h':
           filter = optarg;
           break;
                                       /* Selection string            */
      case 'H':
           SelectionString = optarg;
           break;
                                       /* Convert                     */
      case 'c':
           Options |= qoCCSID;
           sscanf(optarg,"%ld:%lX",&MQCcsid,&MQEnc);
           if (MQEnc == 0) MQEnc = MQENC_NATIVE;
           break;
                                       /* Commit interval             */
      case 'p':
           commits = atol(optarg);
           if (commits < 0)
           {
             Options2 |= qo2SYNCPOINT_IF_PERSIST;
             commits = -commits;
           }
           break;
                                       /* Message Priority            */
      case 'P':
           Priority = atol(optarg);
           break;
                                       /* Detail level                */
      case 'd':
           strncpy(msgopts,optarg,sizeof(msgopts));
           msgopts[sizeof(msgopts)-1] = 0;
           {
             char * p = strchr(msgopts,'w');
             if (p) ScreenWidth = atoi(p+1);

             if (strchr(msgopts,MO_PROP_ALL       )) Options2 |= qo2PROPERTIES_ALL;
             if (strchr(msgopts,MO_PROP_FORCE_RFH2)) Options2 |= qo2PROPERTIES_FORCE_RFH2;
             if (strchr(msgopts,MO_PROP_NONE      )) Options2 |= qo2NO_PROPERTIES;
           }
           break;
                                       /* Attributes                  */
      case 'a':
           if (strchr(optarg,'a')) Options2 |= qo2READ_AHEAD;
           if (strchr(optarg,'A')) Options2 |= qo2NO_READ_AHEAD;
           if (strchr(optarg,'C')) Options2 |= qo2CLOSE_QUIESCE;
           if (strchr(optarg,'D')) Options2 |= qo2CLOSE_DELETE;
           if (strchr(optarg,'f')) pmo.Options |= MQPMO_ASYNC_RESPONSE;
           if (strchr(optarg,'F')) pmo.Options |= MQPMO_SYNC_RESPONSE;
           if (strchr(optarg,'2')) Options2 |= qo2FORCE_RFH2;
           if (strchr(optarg,'c')) Options2 |= qo2COMPLETE_MESSAGE;
           if (strchr(optarg,'d'))
           {
             Options2 |= qo2USER_MSGTYPE;
             MsgType = MQMT_DATAGRAM;
           }
           if (strchr(optarg,'n')) Persistence  = MQPER_NOT_PERSISTENT;
           if (strchr(optarg,'o')) Options     |= qoUSE_OFFSETS;
           if (strchr(optarg,'i')) Options2    |= qo2USE_INDIVIDUAL_QUEUES;
           if (strchr(optarg,'p')) Persistence  = MQPER_PERSISTENT;
           if (strchr(optarg,'q')) Persistence  = MQPER_PERSISTENCE_AS_Q_DEF;
           if (strchr(optarg,'r'))
           {
             Options2 |= qo2USER_MSGTYPE;
             MsgType = MQMT_REPLY;
           }
           if (strchr(optarg,'R'))
           {
             Options2 |= qo2USER_MSGTYPE;
             MsgType = MQMT_REQUEST;
           }
           if (strchr(optarg,'s')) Segmentation = 1;
           if (strchr(optarg,'t'))
           {
             Options2 |= qo2USER_MSGTYPE;
             MsgType = MQMT_REPORT;
           }
           if (strchr(optarg,'x')) Options2 |= qo2NO_SPECIAL_MSG;
           if (strchr(optarg,'X')) Options2 |= qo2INPUT_EXCLUSIVE;

           {
             char * p = optarg;
             while(1)
             {
               p = strchr(p,'#');
               if (!p) break;

               p++;
               switch(*p)
               {
                 case 'p': p++;
                           sscanf(p,"%X",&pmo.Options);
                           break;
                 case 'g': p++;
                           sscanf(p,"%X",&gmo.Options);
                           break;
                 default:  break;
               }
             }
           }
           break;

      case 'A':
                                       /* Attributes with values      */
           {
             int    MaxLen;
             char * pAttr    = NULL;
             char * pSubAttr = NULL;
             switch(optarg[0])
             {
               case 'i':
                                         /* Identity data               */
                    pSubAttr =    SubDesc.PubApplIdentityData;
                    pAttr  =        Mdesc.ApplIdentityData;
                    MaxLen = sizeof(Mdesc.ApplIdentityData);
                    break;

               case 'a':
                                         /* Accounting token            */
                    pSubAttr =    SubDesc.PubAccountingToken;
                    pAttr  =        Mdesc.AccountingToken;
                    MaxLen = sizeof(Mdesc.AccountingToken);
                    break;

               case 'o':
                                         /* Origin data                 */
                    pAttr  =        Mdesc.ApplOriginData;
                    MaxLen = sizeof(Mdesc.ApplOriginData);
                    break;

               default:

                    printf("Unrecognised flag 'A%c'\n",optarg[0]);
                    Reason = MQRC_OPTIONS_ERROR;
                    goto MOD_EXIT;
             }
             memset(pAttr,' ',MaxLen);
             memcpy(pAttr,&optarg[1],min(MaxLen,(int)strlen(&optarg[1])));
             if (pSubAttr)
             {
               memset(pSubAttr,' ',MaxLen);
               memcpy(pSubAttr,&optarg[1],min(MaxLen,(int)strlen(&optarg[1])));
             }
           }

                                       /* Newline                     */
      case 'N':
           Options |= qoNEWLINE;
           break;
                                       /* Report options              */
      case 'n':
                if (strstr(optarg,"cafd"))  Report |= MQRO_COA_WITH_FULL_DATA;
                if (strstr(optarg,"cad"))   Report |= MQRO_COA_WITH_DATA;
           else if (strstr(optarg,"ca" ))   Report |= MQRO_COA;

                if (strstr(optarg,"cdfd"))  Report |= MQRO_COD_WITH_FULL_DATA;
                if (strstr(optarg,"cdd"))   Report |= MQRO_COD_WITH_DATA;
           else if (strstr(optarg,"cd" ))   Report |= MQRO_COD;

                if (strstr(optarg,"efd"))   Report |= MQRO_EXCEPTION_WITH_FULL_DATA;
                if (strstr(optarg,"ed" ))   Report |= MQRO_EXCEPTION_WITH_DATA;
           else if (strstr(optarg,"e"  ))   Report |= MQRO_EXCEPTION;

                if (strstr(optarg,"xfd"))   Report |= MQRO_EXPIRATION_WITH_FULL_DATA;
                if (strstr(optarg,"xd" ))   Report |= MQRO_EXPIRATION_WITH_DATA;
           else if (strstr(optarg,"x"  ))   Report |= MQRO_EXPIRATION;

                if (strstr(optarg,"pan"))   Report |= MQRO_PAN;
                if (strstr(optarg,"nan"))   Report |= MQRO_NAN;
                if (strstr(optarg,"newm"))  Report |= MQRO_NEW_MSG_ID;
                if (strstr(optarg,"passm")) Report |= MQRO_PASS_MSG_ID;
                if (strstr(optarg,"passc")) Report |= MQRO_PASS_CORREL_ID;
                if (strstr(optarg,"copym")) Report |= MQRO_COPY_MSG_ID_TO_CORREL_ID;
                if (strstr(optarg,"disc"))  Report |= MQRO_DISCARD_MSG;
                if (strstr(optarg,"passd")) Report |= MQRO_PASS_DISCARD_AND_EXPIRY;
                if (strstr(optarg,"act"))   Report |= MQRO_ACTIVITY;
           break;
                                       /* Output to stdout            */
      case 's':
           Options |= qoUSE_STDOUT;
           break;

           /***********************************************************/
           /* Separator char - changed to $ from S as the flag since  */
           /* very non main stream flag, unlikely many will be using  */
           /* it and $ being varient EBCDIC is a real pain to use for */
           /* something (subscription to a topic) that I am adding    */
           /***********************************************************/
      case '$':
           Separator = *optarg;
           break;
#ifndef MVS                                    /* No clients from MVS */
                                       /* Set user environment var    */
      case 'u':
           pEnv = (char *)malloc(strlen(optarg)+20);
           sprintf(pEnv,"MQ_USER_ID=%s",optarg);
                                       /* DO NOT FREE pEnv            */
           putenv(pEnv);
           pEnv = getenv("MQ_USER_ID");
           fprintf(stderr,"User Id changed to :'%s'",pEnv);
           break;
#endif
      case 'U':
                                       /* Set userid                  */
           if (*optarg == '+')
           {
             Options2 |= qo2ALTERNATE_USERID;
             optarg++;
           }
           else
           {
             Options2 |= qo2SET_USER;
             Options  |= qoCONTEXT_SET_IDENTITY;
           }
           memset(Userid,' ',sizeof(Userid));
           if (strcmp(optarg,"\'\'"))
             memcpy(Userid,optarg,min(strlen(optarg),sizeof(Userid)));
           break;
#ifndef MVS
                                       /* Browse                      */
      case 'b':
           Options |= qoBROWSE;
           break;
#endif
                                       /* Lock                        */
      case 'k':
           Options |= qoLOCK;
           break;
                                       /* Echo                        */
      case 'e':
           Options |= qoECHO;
           break;
                                       /* Echo but set reply QMGR     */
      case 'E':
           Options |= qoECHO | qoECHO_QMGR;
           break;

      case 'y':
           Options |= qoEXPIRE;
           Expire = atol(optarg);
           break;
                                       /* Zero out messagid           */
      case 'z':
           Options |= qoZERO_MSGID;
           break;
                                       /* Timezone                    */
      case 'Z':
           {
             float tz;
             sscanf(optarg,"%f",&tz);
             gTimeZone = (int) (tz * 60);
             Options2 |= qo2LOCAL_TIMES;
           }
           break;
                                       /* Subscribe stuff             */
      case 'S':
           StringType = 0;
                                       /* Assume we want to consume   */
           Options   &= ~qoBROWSE;

           while(*optarg)
           {
             switch (*optarg)
             {
               case 'c': SubOpts |= soCREATE_SUB;     break;
               case 'r': SubOpts |= soRESUME_SUB;     break;
               case 'a': SubOpts |= soALTER_SUB;      break;
               case 'd': SubOpts |= soDURABLE;        break;
               case 'g': SubOpts |= soGROUP_SUB;      break;
               case 'v': SubOpts |= soANY_USER;       break;
               case 'f': SubOpts |= soFIXED_USER;     break;
               case 'N': SubOpts |= soNEW_PUBS_ONLY;  break;
               case 'R': SubOpts |= soPUBS_ON_REQ;    break;
               case 'C': SubOpts |= soWILDCARD_CHAR;  break;
               case 'T': SubOpts |= soWILDCARD_TOPIC; break;
               case 'D': SubOpts |= soREMOVE_SUB;     break;
               case 'l': SubLevel = atoi(optarg+1);
                         optarg += strlen(optarg)-1;  break;
               case 'n':
               case 'u':
               case 's':
               case 'o':
                    if (StringType && StringType != *optarg)
                    {
                      fprintf(stderr,
                              "Please choose one of n,u,s or o on each use of -S\n");
                      Reason = MQRC_OPTIONS_ERROR;
                      goto MOD_EXIT;
                    }
                    StringType = *optarg;
                    break;
               case ':':
                    switch (StringType)
                    {
                      case 'n':
                           Reason = GetOptVarString(&pSubName, &SubNameLen,
                                                    "subscription name");
                           if (Reason) goto MOD_EXIT;
                           break;
                      case 'u':
                           Reason = GetOptVarString(&pSubUserData,
                                                    &SubUserDataLen,
                                                    "sub user data");
                           if (Reason) goto MOD_EXIT;
                           break;
                      case 0:      /* Default - deliberately no break */
                      case 's':
                           Reason = GetOptVarString(&pTopicString,
                                                    &TopicStrLen,
                                                    "topic string");
                           if (Reason) goto MOD_EXIT;
                           SubOpts |= soRESOURCE_PROVIDED;
                           break;
                      case 'o':
                           strncpy(ITopic,optarg+1,sizeof(ITopic));
                           SubOpts |= soRESOURCE_PROVIDED;
                           break;
                    }
                    optarg += strlen(optarg)-1;
                    break;
               default:
                    fprintf(stderr,
                            "Unrecognised Subscription option '%c'\n", *optarg);
                    Reason = MQRC_OPTIONS_ERROR;
                    goto MOD_EXIT;
                    break;
             }
             *optarg++;
           }
           break;
                                       /* Publish Stuff               */
      case 'T':
           StringType = 0;
           while(*optarg)
           {
             switch (*optarg)
             {
               case 'r': Options2 |= qo2RETAIN;          break;
               case 'p': Options2 |= qo2SUPRESS_REPLYTO; break;
               case 'n': Options2 |= qo2NOT_OWN_SUBS;    break;
               case 'w': Options2 |= qo2NO_MATCH;        break;
               case 'l': PubLevel = atoi(optarg+1);
                         optarg += strlen(optarg)-1;     break;
               case 's':
               case 'o':
                    if (StringType && StringType != *optarg)
                    {
                      fprintf(stderr,
                              "Please choose one of s or o on each use of -T\n");
                      Reason = MQRC_OPTIONS_ERROR;
                      goto MOD_EXIT;
                    }
                    StringType = *optarg;
                    break;
               case ':':
                    switch (StringType)
                    {
                      case 0:      /* Default - deliberately no break */
                      case 's':
                           Reason = GetOptVarString(&pOTopicString,
                                                    &OTopicStrLen,
                                                    "topic string");
                           if (Reason) goto MOD_EXIT;
                           break;
                      case 'o':
                           strncpy(OTopic,optarg+1,sizeof(OTopic));
                           break;
                    }
                    optarg += strlen(optarg)-1;
                    break;
               default:
                    fprintf(stderr,
                            "Unrecognised Publication option '%c'\n", *optarg);
                    Reason = MQRC_OPTIONS_ERROR;
                    goto MOD_EXIT;
                    break;
             }
             *optarg++;
           }
           break;

      case '?':
           {
             char * pTopic = argv[optind];
             if (pTopic)
             {
               if (!memcmp(pTopic,"-?",2)) pTopic = argv[optind] + 2;
               if (!*pTopic) pTopic = argv[optind+1];
             }
             Usage(TRUE,pTopic);
             Reason = MQRC_OPTIONS_ERROR;
             goto MOD_EXIT;
           }
           break;
                                       /* Unrecognised                */
      default:
           fprintf(stderr,"Unrecognised parameter '%s'\n",argv[ArgumentOK+1]);
           Usage(FALSE,NULL);
           Reason = MQRC_OPTIONS_ERROR;
           goto MOD_EXIT;
           break;
    }
    ArgumentOK = optind-1;
  }
                                       /* No unused arguments ?       */
  if (optind != argc || argc < 2)
  {
    Usage(FALSE,NULL);
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }

  if (! (Options & qoNO_HANDLER))
  {
    /* No-op for all platforms apart from OS/2 which is now removed   */
  }

  if (! (Options & qoNO_QUIESCE))
  {
    OpenOutputFlags |= MQOO_FAIL_IF_QUIESCING;
  }
  if (Options2 & qo2RESOLVE_LOCAL)
  {
    OpenOutputFlags |= MQOO_RESOLVE_LOCAL_Q;
  }

  if (Options & qoUSE_MQPUT1)
  {
    pmo.ResponseRecOffset = 0;
    pmo.ResponseRecPtr    = NULL;
  }

    /******************************************************************/
    /* Open output queue, or topic for publishing to                  */
    /******************************************************************/
    if (DestCount && (OTopic[0] || pOTopicString))
    {
      fprintf(stderr,
              "Sorry, you cannot open a queue and a topic for output\n");
      fprintf(stderr,
              "Only use one of '-o' and '-T'\n");
      Reason = MQRC_OPTIONS_ERROR;
      goto MOD_EXIT;
    }


CONNECT:
  /********************************************************************/
  /* Let's check whether our structure versions are sufficient        */
  /********************************************************************/
  if (Options2 & (qo2USE_PUT_GROUPID |
                  qo2USE_GET_GROUPID |
                  qo2COMPLETE_MESSAGE))
  {
    MQMDVersion = max(MQMDVersion,2);
  }
  if (Options2 & qo2USE_GET_GROUPID)
  {
    MQGMOVersion = max(MQGMOVersion,2);
  }
  /********************************************************************/
  /* Set structure versions                                           */
  /********************************************************************/
  IQueueDesc.Version = MQODVersion;
  OQueueDesc.Version = MQODVersion;
  RQueueDesc.Version = MQODVersion;
  Mdesc.Version      = MQMDVersion;
  gmo.Version        = MQGMOVersion;
  pmo.Version        = MQPMOVersion;
  pCno->Version      = MQCNOVersion;

  /********************************************************************/
  /* Are any queue, topic or subscription operations required ?       */
  /********************************************************************/
  if (IQ[0] || DestCount || SubOpts || OTopic[0] || pOTopicString)
  {
#ifndef MVS
    /******************************************************************/
    /* Access the MQI Library                                         */
    /******************************************************************/
    rc = MQAccess(library1,&Mq.Api);
    if (rc)
    {
      fprintf(stderr,"Error loading MQ library %s RC(%d)\n",
                    library1 ? library1 : "",
                    rc);
      Reason = MQRC_ENVIRONMENT_ERROR;
      if (ConfirmExit()) goto MOD_EXIT;
    }
    if (verbose  >= VL_MQAPI)
    {
      char * p = getenv("MQ_CONNECT_TYPE");
      fprintf(stderr,"MQ_CONNECT_TYPE=%s\n",p ? p : "<NULL>");
    }
#endif
    /******************************************************************/
    /* Connect to the Local Queue Manager                             */
    /******************************************************************/
    memcpy(Mq.Qm ,Qm1 ,sizeof(Mq.Qm));
    ConnectQm(&Mq,&Cno1,&CompCode,&Reason);
    if (CompCode == MQCC_FAILED)
    {
      if (ConfirmExit()) goto MOD_EXIT;
    }

    if (Options2 & qo2DUAL_QM)
    {
#ifndef MVS
      /****************************************************************/
      /* Access the MQI Library                                       */
      /****************************************************************/
      rc = MQAccess(library2,&Mq2.Api);
      if (rc)
      {
        fprintf(stderr,"Error loading MQ library %s RC(%d)\n",
                      library2 ? library2 : "",
                      rc);
        Reason = MQRC_ENVIRONMENT_ERROR;
        if (ConfirmExit()) goto MOD_EXIT;
      }
      if (verbose  >= VL_MQAPI)
      {
        char * p = getenv("MQ_CONNECT_TYPE");
        fprintf(stderr,"MQ_CONNECT_TYPE=%s\n",p ? p : "<NULL>");
      }
#endif
      memcpy(Mq2.Qm,Qm2,sizeof(Mq.Qm));
      ConnectQm(&Mq2,&Cno2,&CompCode,&Reason);
      if (CompCode == MQCC_FAILED)
      {
        if (ConfirmExit()) goto MOD_EXIT;
      }
    }
    else
    {
      /****************************************************************/
      /* MQ2 is the same as MQ                                        */
      /****************************************************************/
      memcpy(&Mq2,&Mq,sizeof(Mq));
    }

    /******************************************************************/
    /* Do we want a message handle ?                                  */
    /******************************************************************/
    if (Options2 & qo2PROPERTIES_ALL)
    {
      MQCMHO cmho = { MQCMHO_DEFAULT };

      if (Pause) { fprintf(stderr,"About to MQOPEN\n"); getchar(); }

      IMQCRTMH(Mq.Api,
               Mq.hQm,               /* connection handle             */
              &cmho,                 /* create message handle options */
              &hmsg,                 /* message handle                */
              &CompCode,             /* completion code               */
              &Reason);              /* reason code                   */

      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQError("MQCRTMH","",Reason);
      if (CompCode == MQCC_FAILED)
      {
        if (ConfirmExit()) goto MOD_EXIT;
      }
      gmo.Version   = max(MQGMO_VERSION_4,gmo.Version);
      gmo.MsgHandle = hmsg;
      gmo.Options  |= MQGMO_PROPERTIES_IN_HANDLE;
    }
                                       /* Set up get options          */
    if (! (Options & qoNO_QUIESCE))
    {
      gmo.Options |= MQGMO_FAIL_IF_QUIESCING;
    }
    if (Options2 & qo2TRUNCATE)
    {
      gmo.Options |= MQGMO_ACCEPT_TRUNCATED_MSG;
    }
    if (Options2 & qo2COMPLETE_MESSAGE)
    {
      gmo.Options |= MQGMO_COMPLETE_MSG;
    }
    if (Options2 & qo2PROPERTIES_FORCE_RFH2)
    {
      gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
    }
    if (Options2 & qo2NO_PROPERTIES)
    {
      gmo.Options |= MQGMO_NO_PROPERTIES;
    }
    if (Options & qoBROWSE)
    {
      gmo.Options |=  MQGMO_BROWSE_FIRST;
      gmo.Options &= ~MQGMO_BROWSE_NEXT;
      if (Options & qoLOCK) gmo.Options |= MQGMO_LOCK;
    }
    if (Options & qoCCSID) gmo.Options |= MQGMO_CONVERT;
    gmo.WaitInterval = WaitInterval;
    if (Options2 & qo2FORCE_RFH2)
    {
      gmo.Options |= MQGMO_PROPERTIES_FORCE_MQRFH2;
    }

    /******************************************************************/
    /* Open input and output queues                                   */
    /******************************************************************/
    if (IQ[0])
    {
      ParseQueueName(Separator,IQ,IQueueDesc.ObjectQMgrName,IQueueDesc.ObjectName);
      OpenFlags = 0;
      if (Options & qoPASS_CONTEXT)      OpenFlags |= MQOO_SAVE_ALL_CONTEXT;
      if (Options & qoBROWSE)            OpenFlags |= MQOO_BROWSE;
      if (Options2 & qo2INPUT_EXCLUSIVE) OpenFlags |= MQOO_INPUT_EXCLUSIVE;
      else                               OpenFlags |= MQOO_INPUT_SHARED;
      if (Options2 & qo2ALTERNATE_USERID)
      {
        OpenFlags |= MQOO_ALTERNATE_USER_AUTHORITY;
        memcpy(IQueueDesc.AlternateUserId, Userid, MQ_USER_ID_LENGTH);
      }
      if (! (Options & qoNO_QUIESCE))
      {
        OpenFlags |= MQOO_FAIL_IF_QUIESCING;
      }
      if (Options2 & qo2READ_AHEAD)    OpenFlags |= MQOO_READ_AHEAD;
      if (Options2 & qo2NO_READ_AHEAD) OpenFlags |= MQOO_NO_READ_AHEAD;
      /****************************************************************/
      /* If we have a selection string and we are going to use this   */
      /* queue for a subscription destination, then we will use the   */
      /* Selection string on the sub and not here                     */
      /****************************************************************/
      if (SelectionString && !SubOpts)
      {
        IQueueDesc.SelectionString.VSPtr     = SelectionString;
        IQueueDesc.SelectionString.VSLength  = strlen(SelectionString);
        IQueueDesc.ResObjectString.VSPtr     = ResObjectString;
        IQueueDesc.ResObjectString.VSBufSize = sizeof(ResObjectString);
        if (MQODVersion  < 4) MQODVersion   = 4;
        IQueueDesc.Version = MQODVersion;
      }
      if (Pause) { fprintf(stderr,"About to MQOPEN\n"); getchar(); }

      PRINTDETAILS(MO_MQOD_BEFORE, msgopts, "MQOPEN", (unsigned char *) &IQueueDesc);

#ifdef TEST_AUTH
      IQueueDesc.ObjectType = MQOT_AUTH_INFO;
      OpenFlags = MQOO_INQUIRE;
      IMQOPEN( Mq.Api,
               Mq.hQm,
              &IQueueDesc,
               OpenFlags,
              &hIObj,
              &CompCode,
              &Reason );
      if (Reason)
      {
        MQError("MQOPEN(AuthInfo)",IQ,Reason);
        if (ConfirmExit()) goto MOD_EXIT;
      }
#endif

      IMQOPEN( Mq.Api,
               Mq.hQm,
              &IQueueDesc,
               OpenFlags,
              &hIObj,
              &CompCode,
              &Reason );

      PRINTDETAILS(MO_MQOD_AFTER, msgopts, "MQOPEN", (unsigned char *) &IQueueDesc);

      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQError("MQOPEN",IQueueDesc.ObjectName,Reason);
      if (CompCode == MQCC_FAILED)
      {
        if (ConfirmExit()) goto MOD_EXIT;
      }
      if (verbose >= VL_MQDETAILS) fprintf(stderr,"MQHOBJ = %d\n",hIObj);
    }
    if (Options & qoWAIT_REPLY)
    {
      memcpy(IQueueDesc.ObjectName,ReplyQ,MQ_Q_NAME_LENGTH);
      OpenFlags    = MQOO_INPUT_SHARED | MQOO_FAIL_IF_QUIESCING;
      if (Options2 & qo2ALTERNATE_USERID)
      {
        OpenFlags |= MQOO_ALTERNATE_USER_AUTHORITY;
        memcpy(IQueueDesc.AlternateUserId, Userid, MQ_USER_ID_LENGTH);
      }
      gmo.Options &= ~MQGMO_BROWSE_FIRST;
      if (Pause) { fprintf(stderr,"About to MQOPEN\n"); getchar(); }

      PRINTDETAILS(MO_MQOD_BEFORE, msgopts, "MQOPEN", (unsigned char *) &IQueueDesc);

      IMQOPEN( Mq.Api,
               Mq.hQm,
              &IQueueDesc,
               OpenFlags,
              &hRObj,
              &CompCode,
              &Reason );

      PRINTDETAILS(MO_MQOD_AFTER, msgopts, "MQOPEN", (unsigned char *) &IQueueDesc);

      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQError("MQOPEN",IQueueDesc.ObjectName,Reason);
      if (CompCode == MQCC_FAILED)
      {
        if (ConfirmExit()) goto MOD_EXIT;
      }
      if (verbose >= VL_MQDETAILS) fprintf(stderr,"MQHOBJ = %d\n",hRObj);

      memcpy(ReplyQ   ,IQueueDesc.ObjectName    ,MQ_Q_NAME_LENGTH);
      memcpy(ReplyQMgr,IQueueDesc.ObjectQMgrName,MQ_Q_MGR_NAME_LENGTH);
    }
    if (DestCount || OTopic[0] || pOTopicString)
    {
      if (Options2 & qo2USE_INDIVIDUAL_QUEUES)
      {
        DestPerHobj = 1;
      }
      else
      {
        DestPerHobj = max(DestCount,1);
        OQueueDesc.RecsPresent = DestCount == 1 ? 0 : DestCount;
#ifdef MQRR_DEFAULT           /* Strangly missing from cmqc.h for MVS */
        pmo.RecsPresent        = DestCount == 1 ? 0 : DestCount;
#endif
      }

      /****************************************************************/
      /* Should only ever have one topic, should never have an output */
      /* queue and a topic by this point.                             */
      /****************************************************************/
      if (OTopic[0] || pOTopicString) DestCount = 1;
      for (Dest = 0; Dest < DestCount; Dest += DestPerHobj)
      {
        if (! (Options & qoUSE_MQPUT1))
        {
          if (Options & qoCONTEXT)
          {
                 if (Options & qoCONTEXT_PASS_ALL)
            {
              OpenOutputFlags |= MQOO_PASS_ALL_CONTEXT;
            }
            else if (Options & qoCONTEXT_PASS_IDENTITY)
            {
              OpenOutputFlags |= MQOO_PASS_IDENTITY_CONTEXT;
            }
                 if (Options & qoCONTEXT_SET_ALL)
            {
              OpenOutputFlags |= MQOO_SET_ALL_CONTEXT;
            }
            else if (Options & qoCONTEXT_SET_IDENTITY)
            {
              OpenOutputFlags |= MQOO_SET_IDENTITY_CONTEXT;
            }
          }

          if ((Options2 & qo2USE_INDIVIDUAL_QUEUES) || DestCount==1)
          {
            memcpy(OQueueDesc.ObjectName    ,OObjectRecord[Dest].ObjectName,48);
            memcpy(OQueueDesc.ObjectQMgrName,OObjectRecord[Dest].ObjectQMgrName,48);
          }
          if (Options2 & qo2ALTERNATE_USERID)
          {
            OpenOutputFlags |= MQOO_ALTERNATE_USER_AUTHORITY;
            memcpy(OQueueDesc.AlternateUserId, Userid, MQ_USER_ID_LENGTH);
          }

          OQueueDesc.ResObjectString.VSPtr      = ResObjectString;
          OQueueDesc.ResObjectString.VSBufSize  = sizeof(ResObjectString);

          if (OTopic[0] || pOTopicString)
          {
            OQueueDesc.ObjectType = MQOT_TOPIC;
            memset(OQueueDesc.ObjectName,     ' ', MQ_Q_NAME_LENGTH);
            memset(OQueueDesc.ObjectQMgrName, ' ', MQ_Q_MGR_NAME_LENGTH);
            if (OTopic[0])
              strncpy(OQueueDesc.ObjectName, OTopic, MQ_Q_NAME_LENGTH);
            if (pOTopicString)
            {
              OQueueDesc.ObjectString.VSPtr    = pOTopicString;
              OQueueDesc.ObjectString.VSLength = OTopicStrLen;
              if (MQODVersion  < 4) MQODVersion   = 4;
              OQueueDesc.Version = MQODVersion;
            }
          }

          if (Pause) { fprintf(stderr,"About to MQOPEN\n"); getchar(); }
          PRINTDETAILS(MO_MQOD_BEFORE, msgopts, "MQOPEN", (unsigned char *) &OQueueDesc);

          IMQOPEN( Mq2.Api,
                   Mq2.hQm,
                  &OQueueDesc,
                   OpenOutputFlags,
                  &hOObj[Dest],
                  &CompCode,
                  &Reason );

          PRINTDETAILS(MO_MQOD_AFTER, msgopts, "MQOPEN", (unsigned char *) &OQueueDesc);

          if (verbose >= VL_MQAPI || CompCode != MQCC_OK)
          {
            if (Reason != MQRC_MULTIPLE_REASONS)
              MQError("MQOPEN",OQueueDesc.ObjectName,Reason);
            else
            {
              for (i=0 ; i<DestCount ; i++)
              {
                MQError("MQOPEN",OObjectRecord[i].ObjectName,
                                 OResponseRecord[i].Reason);
              }
            }
          }
          if (CompCode == MQCC_FAILED)
          {
            if (ConfirmExit()) goto MOD_EXIT;
          }
          if (verbose >= VL_MQDETAILS) fprintf(stderr,"MQHOBJ = %d\n",hOObj[Dest]);
          if (OQueueDesc.RecsPresent)
          {
            fprintf(stderr,"KnownDestCount   : %ld\n",OQueueDesc.KnownDestCount);
            fprintf(stderr,"UnknownDestCount : %ld\n",OQueueDesc.UnknownDestCount);
            fprintf(stderr,"InvalidDestCount : %ld\n",OQueueDesc.InvalidDestCount);
          }
        }
      }
    }
    /******************************************************************/
    /* Subscribe to topic                                             */
    /******************************************************************/
    if (SubOpts)
    {
      memcpy(SubDesc.ObjectName, ITopic ,MQ_Q_NAME_LENGTH);
      SubDesc.ObjectString.VSPtr     = pTopicString;
      SubDesc.ObjectString.VSLength  = TopicStrLen;
      SubDesc.SubName.VSPtr          = pSubName;
      SubDesc.SubName.VSLength       = SubNameLen;
      SubDesc.SubUserData.VSPtr      = pSubUserData;
      SubDesc.SubUserData.VSLength   = SubUserDataLen;
      SubDesc.SubLevel               = SubLevel;
      if (Priority != -1)           /* This will do for now */
        SubDesc.PubPriority          = Priority;
      if (SubOpts & soCREATE_SUB)      SubDesc.Options |= MQSO_CREATE;
      if (SubOpts & soRESUME_SUB)      SubDesc.Options |= MQSO_RESUME;
      if (SubOpts & soALTER_SUB)       SubDesc.Options |= MQSO_ALTER;
      /* If no creation options specified set our default one */
      if (!(SubOpts & soCREATE_OPTS))  SubDesc.Options |= MQSO_CREATE;

      if (SubOpts & soDURABLE)         SubDesc.Options |= MQSO_DURABLE;
      if (IQ[0] == 0)                { SubDesc.Options |= MQSO_MANAGED;
                                       hIObj = MQHO_NONE;
                                     }
      if (SubOpts & soANY_USER)        SubDesc.Options |= MQSO_ANY_USERID;
      if (SubOpts & soFIXED_USER)      SubDesc.Options |= MQSO_FIXED_USERID;
      if (SubOpts & soNEW_PUBS_ONLY)   SubDesc.Options |= MQSO_NEW_PUBLICATIONS_ONLY;
      if (SubOpts & soPUBS_ON_REQ)     SubDesc.Options |= MQSO_PUBLICATIONS_ON_REQUEST;
      if (SubOpts & soWILDCARD_CHAR)   SubDesc.Options |= MQSO_WILDCARD_CHAR;
      if (SubOpts & soWILDCARD_TOPIC)  SubDesc.Options |= MQSO_WILDCARD_TOPIC;
      if (SubOpts & soGROUP_SUB)       SubDesc.Options |= MQSO_GROUP_SUB;

      if (Options2 & qo2READ_AHEAD)    SubDesc.Options |= MQSO_READ_AHEAD;
      if (Options2 & qo2NO_READ_AHEAD) SubDesc.Options |= MQSO_NO_READ_AHEAD;
      if (Options2 & qo2ALTERNATE_USERID)
      {
        SubDesc.Options |= MQSO_ALTERNATE_USER_AUTHORITY;
        memcpy(SubDesc.AlternateUserId, Userid, MQ_USER_ID_LENGTH);
      }
      if (Options & qoUSE_GET_CORRELID)
      {
        memcpy(SubDesc.SubCorrelId,GetCorrelId,24);
               SubDesc.Options |= MQSO_SET_CORREL_ID;
      }
      if (! (Options & qoNO_QUIESCE))
      {
        SubDesc.Options |= MQSO_FAIL_IF_QUIESCING;
      }
      if (SelectionString)
      {
        SubDesc.SelectionString.VSPtr    = SelectionString;
        SubDesc.SelectionString.VSLength = strlen(SelectionString);
      }
      if (Options & qoEXPIRE) SubDesc.SubExpiry = Expire;

      SubDesc.ResObjectString.VSPtr      = ResObjectString;
      SubDesc.ResObjectString.VSBufSize  = sizeof(ResObjectString);

      if (Pause) { fprintf(stderr,"About to MQSUB\n"); getchar(); }
      PRINTDETAILS(MO_MQSD_BEFORE, msgopts, "MQSUB", (unsigned char *) &SubDesc);
      OriginalSubOptions = SubDesc.Options;

      IMQSUB( Mq.Api,
              Mq.hQm,
             &SubDesc,
             &hIObj,
             &hSub,
             &CompCode,
             &Reason );

      PRINTDETAILS(MO_MQSD_AFTER, msgopts, "MQSUB", (unsigned char *) &SubDesc);

      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQSubError("MQSUB",OriginalSubOptions,SubDesc,Reason);
      if (Reason) goto MOD_EXIT;
    }
  }
                                       /* Set stdout                  */
  if (!DestCount && !OutFile)
  {
    if (! (Options & qoQUIET)) Options |= qoUSE_STDOUT;
  }
                                       /* Start counters              */
#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  gettimeofday(&StartTime,&tz);
#endif
#ifdef WIN32
  QueryPerformanceFrequency(&Frequency);
  QueryPerformanceCounter(&StartTime);
#endif
  ngets = 0;
  /********************************************************************/
  /* Do transfer                                                      */
  /********************************************************************/
  while(!finished)
  {
    BOOL bGetLoopWait = TRUE;          /* Where to wait               */
                                       /* Set control variables       */
    size    = 0;
    delay   = 0;
    reps    = 1;

    /******************************************************************/
    /* Request some publications                                      */
    /******************************************************************/
    if (SubOpts & soPUBS_ON_REQ)
    {
      bGetLoopWait = FALSE;
      if (Options & qoGET_SLEEP)
      {
        AllSleep(GetSleep);
      }
      if (! (Options & qoNO_QUIESCE))
      {
        sro.Options |= MQSRO_FAIL_IF_QUIESCING;
      }
      if (Pause) { fprintf(stderr,"About to MQSUBRQ\n"); getchar(); }
      PRINTDETAILS(MO_MQSRO_BEFORE, msgopts, "MQSUBRQ", (unsigned char *) &sro);

      IMQSUBRQ(Mq.Api,
               Mq.hQm,
               hSub,
               MQSR_ACTION_PUBLICATION,
              &sro,
              &CompCode,
              &Reason );

      PRINTDETAILS(MO_MQSRO_AFTER, msgopts, "MQSUBRQ", (unsigned char *) &sro);
      if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
        MQSubError("MQSUBRQ",OriginalSubOptions,SubDesc,Reason);
      if (CompCode == MQCC_FAILED)
      {
        if (ConfirmExit()) goto MOD_EXIT;
      }
    }
    /******************************************************************/
    /* Get message                                                    */
    /******************************************************************/
    if (hIObj != MQHO_UNUSABLE_HOBJ)
                                       /* Input from a Queue          */
    {
      gmo.MatchOptions = 0;

      if (Options & qoUSE_GET_MSGID)
      {
        memcpy(Mdesc.MsgId,GetMsgId      ,24);
        gmo.MatchOptions |= MQMO_MATCH_MSG_ID;
      }
      else
        memset(Mdesc.MsgId,0             ,24);

      if (Options & qoUSE_GET_CORRELID)
      {
        memcpy(Mdesc.CorrelId,GetCorrelId,24);
        gmo.MatchOptions |= MQMO_MATCH_MSG_ID;
      }
      else
        memset(Mdesc.CorrelId,0          ,24);

      if (Options2 & qo2USE_GET_GROUPID)
      {
        memcpy(Mdesc.GroupId,GetGroupId,24);
        gmo.MatchOptions |= MQMO_MATCH_GROUP_ID;
      }

      if (commits)
      {
        if (Options & qoBROWSE)
          gmo.Options |= MQGMO_NO_SYNCPOINT;
        else
        {
          if (Options2 & qo2SYNCPOINT_IF_PERSIST)
            gmo.Options |= MQGMO_SYNCPOINT_IF_PERSISTENT;
          else
            gmo.Options |= MQGMO_SYNCPOINT;
        }
      }
      else
      {
        gmo.Options |= MQGMO_NO_SYNCPOINT;
      }
MQGET:
      if (Options & qoCCSID)
      {
        Mdesc.Encoding       = MQEnc;
        Mdesc.CodedCharSetId = MQCcsid;
      }
      if (MaxMsgLen != -1) MsgSize = MIN(MsgSize,MaxMsgLen);
      if (Pause) { fprintf(stderr,"About to MQGET\n"); getchar(); }
      if (Options & qoGET_SLEEP && bGetLoopWait)
      {
        AllSleep(GetSleep);
      }
      PRINTDETAILS(MO_MQGMO_BEFORE, msgopts, "MQGET", (unsigned char *) &gmo);
      PRINTDETAILS(MO_MQMD_BEFORE,  msgopts, "MQGET", (unsigned char *) &Mdesc);

      IMQGET(Mq.Api,
             Mq.hQm,
             hIObj,
            &Mdesc,
            &gmo,
             MsgSize,
             pMsg,
            &MsgLen,
            &CompCode,
            &Reason );

      if (CompCode != MQCC_FAILED)
      {
                                       /* Print out separators        */
        if (msgopts[0]) printf("%s\n",SEPARATOR_LINE);
        PRINTDETAILS(MO_MQMD_AFTER,  msgopts, "MQGET", (unsigned char *) &Mdesc);
        PRINTDETAILS(MO_MQGMO_AFTER, msgopts, "MQGET", (unsigned char *) &gmo);
      }
      if (verbose >= VL_MQAPI) MQError("MQGET",IQ,Reason);

      switch(Reason)
      {
        case MQRC_CONNECTION_QUIESCING:
             if (Options & qoNO_QUIESCE)
             {
               fprintf(stderr,"Connection Quiescing ignored\n");
               continue;
             }
             if (Options2 & qo2CLOSE_EXPLICIT)
             {
               if (CloseExplicit(&Mq,IQ,&hIObj)) continue;
             }
             if (verbose < VL_MQAPI) MQError("MQGET",IQ,Reason);
             if (ConfirmExit()) goto MOD_EXIT;
             else continue;
             break;

        case MQRC_Q_MGR_QUIESCING:
             if (Options & qoNO_QUIESCE)
             {
               fprintf(stderr,"Queue Manager Quiescing ignored\n");
               continue;
             }
             if (Options2 & qo2CLOSE_EXPLICIT)
             {
               if (CloseExplicit(&Mq,IQ,&hIObj)) continue;
             }
             if (verbose < VL_MQAPI) MQError("MQGET",IQ,Reason);
             if (ConfirmExit()) goto MOD_EXIT;
             else continue;
             break;

        case MQRC_TRUNCATED_MSG_ACCEPTED:
             if (MaxMsgLen != -1)
               MsgLen = MIN(MsgLen, MaxMsgLen);
                                       /* DELIBERATELY NO BREAK HERE  */
        case 0L:
        case MQRC_NOT_CONVERTED:
        case MQRC_CONVERTED_STRING_TOO_BIG:
             if (Reason) MQError("MQGET",IQ,Reason);
                                       /* Increment message count     */
                 ngets++;
                                       /* Count commits               */
                 if (commits)
                 {
                   if (gmo.Options & MQGMO_SYNCPOINT) commitmsg ++;
                   if (gmo.Options & MQGMO_SYNCPOINT_IF_PERSISTENT)
                   {
                     if (Mdesc.Persistence == MQPER_PERSISTENT) commitmsg++;
                   }
                 }
                 if (verbose >= VL_MQDETAILS)
                 {
                   fprintf(stderr,"MQGET %ld bytes\n",MsgLen);
                 }
                 else if (msgopts[0])
                 {
                   if (strchr(msgopts,MO_OUTPUT_MSGLEN))
                   {
                     fprintf(stderr,"MQGET %ld bytes\n",MsgLen);
                   }
                 }
                 /*****************************************************/
                 /* Report Timings if required                        */
                 /*****************************************************/
                 if (ngets >= PerformanceLimit)
                 {
                   if (Options & qoREPORT_TIMES) ReportTimes(&ngets,0);
                 }
                 break;

        case MQRC_TRUNCATED_MSG_FAILED:
                 if (MaxMsgLen != -1 &&
                     MsgLen    >  MaxMsgLen)
                 {
                   fprintf(stderr,"Message (%ld bytes) larger than Max size (%ld bytes)\n",
                           MsgLen, MaxMsgLen);
                   Reason = MQRC_STORAGE_NOT_AVAILABLE;
                   if (ConfirmExit()) goto MOD_EXIT;
                   else continue;
                 }
                 pNewMsg = (char *)QGetMem(MsgLen);
                 if (!pNewMsg)
                 {
                   fprintf(stderr,"Failed to allocate %ld bytes\n",MsgLen);
                   Reason = MQRC_STORAGE_NOT_AVAILABLE;
                   if (ConfirmExit()) goto MOD_EXIT;
                   else continue;
                 }
                 MsgSize = MsgLen;
                 QFreeMem(pMsg);
                 pMsg = pNewMsg;
                 goto MQGET;

        case MQRC_NO_MSG_AVAILABLE:
                 fprintf(stderr,"No more messages.\n");
                 if (Options2 & qo2CLOSE_EXPLICIT)
                 {
                   if (CloseExplicit(&Mq,IQ,&hIObj)) continue;
                 }
                                       /* Have we outstanding txn ?   */
                 if (commits && commitmsg)
                 {
                   if (Pause) { fprintf(stderr,"About to MQCMIT\n"); getchar(); }

                   if (Options2 & qo2DUAL_QM)
                   {
                     IMQCMIT(Mq2.Api,
                             Mq2.hQm,
                            &CompCode,
                            &Reason2 );
                     if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq2.Qm,Reason2);
                     switch(Reason2)
                     {
                       case 0L: break;

                       default: MQError("MQCMIT",Mq2.Qm,Reason2);
                                if (ConfirmExit()) goto MOD_EXIT;
                                else continue;
                     }
                   }

                   IMQCMIT(Mq.Api,
                           Mq.hQm,
                          &CompCode,
                          &Reason2 );
                   if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq.Qm,Reason2);
                   switch(Reason2)
                   {
                     case 0L: break;

                     default: MQError("MQCMIT",Mq.Qm,Reason2);
                              if (ConfirmExit()) goto MOD_EXIT;
                              else continue;
                   }
                   commitmsg = 0;
                 }
                 /*****************************************************/
                 /* Report Timeings if required                       */
                 /*****************************************************/
                 if (Options & qoREPORT_TIMES)
                 {
                   ReportTimes(&ngets,WaitInterval);
                 }
                 if (ConfirmExit()) goto MOD_EXIT;
                 else continue;
                 break;

        default: if (verbose < VL_MQAPI) MQError("MQGET",IQ,Reason);
                 if (ConfirmExit()) goto MOD_EXIT;
                 else continue;
      }
                                       /* Change browse options       */
      if (Options & qoBROWSE)
      {
        gmo.Options &= ~MQGMO_BROWSE_FIRST;
        gmo.Options |=  MQGMO_BROWSE_NEXT;
      }
                                       /* If echo then echo           */
      if (Options & qoECHO)
      {
        MQMD eMdesc;

        memcpy(&eMdesc, &Mdesc, sizeof(MQMD));

        memcpy(RQueueDesc.ObjectName    , eMdesc.ReplyToQ   , (int)MQ_Q_NAME_LENGTH);
        memcpy(RQueueDesc.ObjectQMgrName, eMdesc.ReplyToQMgr, (int)MQ_Q_MGR_NAME_LENGTH);

        if (Options & qoECHO_QMGR)
        {
          memset(eMdesc.ReplyToQ   ,0,sizeof(Mdesc.ReplyToQ));
          memset(eMdesc.ReplyToQMgr,0,sizeof(Mdesc.ReplyToQMgr));
        }
        eMdesc.MsgType = MQMT_REPLY;

        /**************************************************************/
        /* Deal with the report options                               */
        /* bear in mind we've already effectively passed msgid and    */
        /* correlid.                                                  */
        /**************************************************************/
        if (!(Mdesc.Report & MQRO_PASS_MSG_ID   )) memset(&eMdesc.MsgId   ,0           ,24);
        if (!(Mdesc.Report & MQRO_PASS_CORREL_ID)) memcpy(&eMdesc.CorrelId,&Mdesc.MsgId,24);

        if (Pause) { fprintf(stderr,"About to MQPUT1\n"); getchar(); }
        if (strchr(msgopts,MO_OUTPUT_MSGLEN))
        {
          fprintf(stderr,"MQPUT1 %ld bytes\n",MsgLen);
        }
        IMQPUT1(Mq.Api,
                Mq.hQm,
               &RQueueDesc,
               &eMdesc,
               &pmo,
                MsgLen,
                pMsg,
               &CompCode,
               &Reason );
        if (Reason || verbose >= VL_MQAPI)
          MQError("MQPUT1",RQueueDesc.ObjectName,Reason);
        if (verbose >= VL_MQDETAILS)
        {
          fprintf(stderr,"MQPUT1 %ld bytes\n",MsgLen);
        }
      }
    }
    else
    {
                                       /* Input from the keyboard     */
GETINP:
      if (! LineRead && !(Options & qoEXPLICIT_MSG))
      {

        int LineRead;
        int CurrentPos;

        if (!bFileInput) {fprintf(stderr,">") ; FORCE_STDERR;}
        pMsg = pMsgArea;

        LineRead = FALSE;
        while (1)
        {
          int ReadSuccess;
          memset(pMsg,0,100);

          CurrentPos = ftell(In);

          if (Options2 & qo2RECORD_BASED_IO)
               ReadSuccess =  fread(pMsg, 1, MsgSize, In);
          else ReadSuccess = (fgets(pMsg,    MsgSize, In) != 0);

          if (!ReadSuccess)
          {
            if ((Options2 & qo2CLOSE_DELETE) && hOObj[0] != MQHO_UNUSABLE_HOBJ)
            {
              BOOL cont = CloseExplicit(&Mq,OObjectRecord[0].ObjectName,&hOObj[0]);
              if (cont)
              {
                printf("Continue ?\n");
                fgets(pMsg,MsgSize,In);
                if (*pMsg == 'y') goto GETINP;
              }
            }
            goto MOD_EXIT;
          }

          if (Options2 & qo2RECORD_BASED_IO) MsgLen = ReadSuccess;
                                        else MsgLen = strlen(pMsg);
                                       /* Did we read it all ?        */
          if (MsgLen < (MsgSize-1)) break;
                                       /* Allocate a bigger area      */
          MsgSize = (MsgSize * 2) & 0xFFFFFC00;
          QFreeMem(pMsgArea);
          pMsgArea = (char *)QGetMem(MsgSize);
          if (!pMsgArea)
          {
            fprintf(stderr,"Failed to allocate %ld bytes\n",MsgSize);
            Reason = MQRC_STORAGE_NOT_AVAILABLE;
            goto MOD_EXIT;
          }
          pMsg    = pMsgArea;
                                       /* Now go back                 */
          fseek(In, CurrentPos, SEEK_SET );
        }
                                       /* Second char newline         */
        if (!(Options2 & qo2NO_SPECIAL_MSG))
        {
          if (MsgLen == 2 && *pMsg == '?')
          {
            fprintf(stderr,"Formatted message :\n");
            fprintf(stderr," #end     = End program\n\n");
            fprintf(stderr," #[!][c]Reps[/Size[/Delay[/Commits]]]\n");
            fprintf(stderr,"  [!]     = Don't send # string\n");
            fprintf(stderr,"  [c]     = Checksum the messages\n");
            fprintf(stderr,"  [p]     = Change properties\n");
            fprintf(stderr,"  Reps    = Number of messages to MQPUT\n");
            fprintf(stderr,"  Size    = Size of message (-ve range)\n");
#if defined(AMQ_UNIX) || defined(MVS)
            fprintf(stderr,"  Delay   = Number of seconds delay between MQPUTs (-ve range)\n");
#else
            fprintf(stderr,"  Delay   = Number of milliseconds delay between MQPUTs (-ve range)\n");
#endif
            fprintf(stderr,"  Commits = Commit after n messages\n");
            goto GETINP;
          }
        }

        if (!(Options & qoNEWLINE) && MsgLen)
        {
          if (pMsg[MsgLen-1] == '\n')
          {
            pMsg[MsgLen-1] = 0;
            MsgLen--;
          }
        }
      }
      else
      {
        /**************************************************************/
        /* If it's explicit then the message length is already set    */
        /* otherwise we'll take the length from the string            */
        /**************************************************************/
        if (!(Options & qoEXPLICIT_MSG)) MsgLen = strlen(pMsg);
      }

      LineRead = FALSE;
      memcpy(Mdesc.Format,MQFMT_STRING,sizeof(Mdesc.Format));

      if (Options & qoCCSID)
      {
        Mdesc.Encoding       = MQEnc;
        Mdesc.CodedCharSetId = MQCcsid;
      }
      /****************************************************************/
      /* User requesting a formatted message ?                        */
      /****************************************************************/
      if (*pMsg == '#'  && !(Options2 & qo2NO_SPECIAL_MSG))
      {
        char * p = pMsg+1;
        BOOL   Skip     = FALSE;
        BOOL   CheckSum = FALSE;
        int    Start = strlen(p)+1;

        if (memcmp(p, "end", 4) == 0) break;

        if (*p == '!')
        {
          Skip = TRUE;
          p++;
        }

        if (*p == 'c')
        {
          CheckSum = TRUE;
          p++;
        }

        if (*p == 'p')
        {
          SetProperties();
          continue;
        }

        sscanf(p,"%ld/%ld/%ld/%ld",&reps,&size,&delay,&commits);

        if (size == 0) size = strlen(pMsg);
                                       /* Increase buffer size if not */
                                       /* sufficient                  */
        if (labs(size) > MsgSize)
        {
          pNewMsg = (char *)QGetMem(labs(size));
          if (!pNewMsg)
          {
            fprintf(stderr,"Failed to allocate %ld bytes\n",size);
            Reason = MQRC_STORAGE_NOT_AVAILABLE;
            goto MOD_EXIT;
          }
          MsgSize = labs(size);
          memcpy(pNewMsg,pMsg,(int)MsgLen);
          QFreeMem(pMsgArea);
          pMsgArea = pNewMsg;
          pMsg     = pNewMsg;
        }
        MsgLen = labs(size);
                                       /* NULL endof line             */
        pEnd = (char *)memchr(pMsg,'\n',(int)MsgLen);
        if (pEnd) *pEnd = 0;

        if (CheckSum)
        {
                                       /* Set buffer to random chars  */
          for (i=Start; i<MsgLen-Start; i++) *(pMsg+i)=rand();
        }
        if (Skip)
        {
          char * p = memchr(pMsg,' ',MsgLen);
          if (p)
          {
            MsgLen -= ((p - pMsg) + 1);
            pMsg    = p + 1;
          }
        }
                                       /* End of formatted message    */
      }
    }
    /******************************************************************/
    /* Filter out unwanted messages                                   */
    /* NOTE: Usage of goto statements to prevent complicated loop     */
    /*       control variables.                                       */
    /******************************************************************/
    if (filter)
    {
      long fillen = (long)strlen(filter);
      for (i=0; i<MsgLen-fillen+1;i++)
      {
        for (j=0; j<fillen;j++)
        {
          if (*(pMsg+i+j) != *(filter+j)) goto CHK_FILTER;
        }
        goto DO_MSG;
CHK_FILTER:;
      }
      if (Options & qoEXPLICIT_MSG) goto MOD_EXIT;
      continue;
    }
DO_MSG:
    /******************************************************************/
    /* Dispose of message                                             */
    /******************************************************************/
                                       /* Set reply queue             */
    if (ReplyQ[0])
    {
      memcpy(Mdesc.ReplyToQ   , ReplyQ   , (int)MQ_Q_NAME_LENGTH);
      memcpy(Mdesc.ReplyToQMgr, ReplyQMgr, (int)MQ_Q_MGR_NAME_LENGTH);
    }

                                       /* Start counters              */
    if (hIObj == MQHO_UNUSABLE_HOBJ)
    {
#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
      gettimeofday(&StartTime,&tz);
#endif
#ifdef WIN32
      QueryPerformanceCounter(&StartTime);
#endif
    }

    for (i=0 ; i<reps ; i++)
    {
                                       /* Random size request ?       */
      if (size < 0)
      {
                                       /* Don't allow zero sizes      */
        do
        {
          MsgLen = (MQLONG)((((float)(-size)) * ((float)rand())) / RAND_MAX);
        }
        while (!MsgLen);
      }
      if (DestCount || OTopic[0] || pOTopicString)
      {
        if (commits)
        {
          pmo.Options |= MQPMO_SYNCPOINT;
          /************************************************************/
          /* Only increase the msg in transaction if we haven't       */
          /* done it already for MQGET                                */
          /************************************************************/
          if (hIObj == MQHO_UNUSABLE_HOBJ)
          {
            commitmsg++;
          }
        }
        else
        {
          pmo.Options |= MQPMO_NO_SYNCPOINT;
        }

        for (Dest = 0; Dest < DestCount; Dest += DestPerHobj)
        {
          if (! (Options & qoNO_QUIESCE))
          {
            pmo.Options |= MQPMO_FAIL_IF_QUIESCING;
          }
          if (Options2 & qo2RESOLVE_LOCAL)   pmo.Options |= MQPMO_RESOLVE_LOCAL_Q;
          if (Options2 & qo2RETAIN)          pmo.Options |= MQPMO_RETAIN;
          if (Options2 & qo2SUPRESS_REPLYTO) pmo.Options |= MQPMO_SUPPRESS_REPLYTO;
          if (Options2 & qo2NOT_OWN_SUBS)    pmo.Options |= MQPMO_NOT_OWN_SUBS;
          if (Options2 & qo2NO_MATCH)        pmo.Options |= MQPMO_WARN_IF_NO_SUBS_MATCHED;
          pmo.PubLevel = PubLevel;
                                       /* Set message attributes      */
          if (Persistence != -1) Mdesc.Persistence = Persistence;
          if (Segmentation)
          {
            Mdesc.Version   = 2;
            Mdesc.MsgFlags |= MQMF_SEGMENTATION_ALLOWED;
          }
                                         /* Checksum processing         */
          if (! (Options2 & qo2NO_SPECIAL_MSG) )
          {
            if (*pMsg =='#')
            {
              if (*(pMsg+1)=='c')
              {
                chk = 0;
                for (j=0 ; j<MsgLen ; j++) chk += *(pMsg+j);
                fprintf(stderr,"[Chk.%ld] ",chk);
                if (verbose < VL_MESSAGE) fputc('\n',stdout);
              }
              if (verbose >= VL_MESSAGE)
              {
                fprintf(stderr,"(%ld bytes) ",MsgLen);
                fwrite( pMsg, MIN(strlen(pMsg),MIN((unsigned int)MsgLen,40)), 1, stdout);
                fputc('\n',stdout);
              }
            }
          }
          Mdesc.Report  |= Report;
          Mdesc.Priority = Priority;

          if (Options & qoZERO_MSGID)
            memset(Mdesc.MsgId, 0, sizeof(Mdesc.MsgId));

          if (Options & qoEXPIRE)
            Mdesc.Expiry = Expire;

          if (Options & qoCCSID && hIObj == MQHO_UNUSABLE_HOBJ)
          {
            Mdesc.Encoding       = MQEnc;
            Mdesc.CodedCharSetId = MQCcsid;
          }

          if (Options2 & qo2USER_MSGTYPE)
          {
            Mdesc.MsgType = MsgType;
          }
          else
          {
            if (Options & qoREQUEST)
              Mdesc.MsgType = MQMT_REQUEST;
          }

          if (Options & qoUSER_FORMAT)
            memcpy(Mdesc.Format,MyFormat,sizeof(Mdesc.Format));

          if (Options & qoCONTEXT)
          {
            pmo.Context = hIObj;
                 if (Options & qoCONTEXT_PASS_ALL)
            {
              pmo.Options |= MQPMO_PASS_ALL_CONTEXT;
            }
            else if (Options & qoCONTEXT_PASS_IDENTITY)
            {
              pmo.Options |= MQPMO_PASS_IDENTITY_CONTEXT;
            }
            else if (Options & qoCONTEXT_SET_ALL)
            {
              pmo.Options |= MQPMO_SET_ALL_CONTEXT;
            }
            else if (Options & qoCONTEXT_SET_IDENTITY)
            {
              pmo.Options |= MQPMO_SET_IDENTITY_CONTEXT;
            }
          }
          if (Options2 & qo2NO_CONTEXT)
            pmo.Options |= MQPMO_NO_CONTEXT;

          if (Options  & qoUSE_PUT_MSGID)    memcpy(Mdesc.MsgId   ,PutMsgId   ,24);
          if (Options  & qoUSE_PUT_CORRELID) memcpy(Mdesc.CorrelId,PutCorrelId,24);
          if (Options2 & qo2USE_PUT_GROUPID)
          {
            Mdesc.MsgFlags = MQMF_MSG_IN_GROUP;
            memcpy(Mdesc.GroupId ,PutGroupId  ,24);
          }
          if (Options2 & qo2SET_USER)        memcpy(Mdesc.UserIdentifier,Userid,sizeof(Userid));

          if (hmsg)
          {
            if (pmo.Version < MQPMO_VERSION_3) pmo.Version = MQPMO_VERSION_3;
          }

          if (pmo.Version >= MQPMO_VERSION_3)
          {
            pmo.NewMsgHandle = hmsg;
          }
          /**************************************************************/
          /* Do we need to output the message we're MQPUTing on screen  */
          /**************************************************************/
          if (Options & qoUSE_STDOUT)
          {
            if (msgopts[0]) printf("%s\n",SEPARATOR_LINE);
            PrintMsg(pMsg, MsgLen, &Mdesc, msgopts, TRUE);
          }

          if (Options & qoUSE_MQPUT1)
          {
            for (Dest = 0; Dest < DestCount; Dest += DestPerHobj)
            {
              if (Pause) { fprintf(stderr,"About to MQPUT1\n"); getchar(); }

              if (Options2 & qo2USE_INDIVIDUAL_QUEUES || DestCount==1)
              {
                memcpy(OQueueDesc.ObjectName    ,OObjectRecord[Dest].ObjectName,48);
                memcpy(OQueueDesc.ObjectQMgrName,OObjectRecord[Dest].ObjectQMgrName,48);
              }

              PRINTDETAILS(MO_MQOD_BEFORE,  msgopts, "MQPUT1", (unsigned char *) &OQueueDesc);
              PRINTDETAILS(MO_MQPMO_BEFORE, msgopts, "MQPUT1", (unsigned char *) &pmo);
              PRINTDETAILS(MO_MQMD_BEFORE,  msgopts, "MQPUT1", (unsigned char *) &Mdesc);

              if (msgopts[0])
              {
                if (strchr(msgopts,MO_OUTPUT_MSGLEN))
                {
                  fprintf(stderr,"MQPUT1 %ld bytes\n",MsgLen);
                }
              }

              IMQPUT1(Mq2.Api,
                     Mq2.hQm,
                     &OQueueDesc,
                     &Mdesc,
                     &pmo,
                      MsgLen,
                      pMsg,
                     &CompCode,
                     &Reason );

              PRINTDETAILS(MO_MQOD_AFTER,  msgopts, "MQPUT1", (unsigned char *) &OQueueDesc);
              PRINTDETAILS(MO_MQPMO_AFTER, msgopts, "MQPUT1", (unsigned char *) &pmo);
              PRINTDETAILS(MO_MQMD_AFTER,  msgopts, "MQPUT1", (unsigned char *) &Mdesc);

              if (verbose >= VL_MQAPI) MQError("MQPUT1",OQueueDesc.ObjectName,Reason);
              if (verbose >= VL_MQDETAILS)
              {
                fprintf(stderr,"MQPUT1 %ld bytes\n",MsgLen);
              }

              if (CompCode == MQCC_FAILED)
              {
                 MQError("MQPUT1",OQueueDesc.ObjectName,Reason);
                 if (ConfirmExit()) goto MOD_EXIT;
                 else continue;
              }
            }
          }
          else
          {
            if (Pause) { fprintf(stderr,"About to MQPUT\n"); getchar(); }
            PRINTDETAILS(MO_MQPMO_BEFORE, msgopts, "MQPUT", (unsigned char *) &pmo);
            PRINTDETAILS(MO_MQMD_BEFORE,  msgopts, "MQPUT", (unsigned char *) &Mdesc);
            if (msgopts[0])
            {
              if (strchr(msgopts,MO_OUTPUT_MSGLEN))
              {
                fprintf(stderr,"MQPUT %ld bytes\n",MsgLen);
              }
            }

            IMQPUT(Mq2.Api,
                   Mq2.hQm,
                   hOObj[Dest],
                  &Mdesc,
                  &pmo,
                   MsgLen,
                   pMsg,
                  &CompCode,
                  &Reason );

            PRINTDETAILS(MO_MQPMO_AFTER, msgopts, "MQPUT", (unsigned char *) &pmo);
            PRINTDETAILS(MO_MQMD_AFTER,  msgopts, "MQPUT", (unsigned char *) &Mdesc);
            if (verbose >= VL_MQAPI) MQError("MQPUT",OObjectRecord[Dest].ObjectName,Reason);
            if (verbose >= VL_MQDETAILS)
            {
              fprintf(stderr,"MQPUT %ld bytes\n",MsgLen);
            }

            if (CompCode == MQCC_FAILED)
            {
              if (Reason != MQRC_MULTIPLE_REASONS)
                MQError("MQPUT",OObjectRecord[Dest].ObjectName,Reason);
              else
              {
                for (i=0 ; i<DestCount ; i++)
                {
                  MQError("MQPUT",OObjectRecord[i].ObjectName,
                                  OResponseRecord[i].Reason);
                }
              }
              if (ConfirmExit()) goto MOD_EXIT;
              else continue;
            }
          }
        }
                                         /* Delay if requested          */
        if (delay && i!=reps)
        {
          AllSleep(delay);
        }
                                         /* Reached commit point yet ?  */
        if (commits && commitmsg >= commits)
        {
          if (Pause) { fprintf(stderr,"About to MQCMIT\n"); getchar(); }

          if (Options2 & qo2DUAL_QM)
          {
            IMQCMIT(Mq2.Api,
                    Mq2.hQm,
                   &CompCode,
                   &Reason2 );
            if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq2.Qm,Reason2);
            switch(Reason2)
            {
              case 0L: break;

              default: MQError("MQCMIT",Mq2.Qm,Reason2);
                       if (ConfirmExit()) goto MOD_EXIT;
                       else continue;
            }
          }

          IMQCMIT(Mq.Api,
                  Mq.hQm,
                 &CompCode,
                 &Reason );
          if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq.Qm,Reason);
          switch(Reason)
          {
            case 0L: break;

            default: MQError("MQCMIT",Mq.Qm,Reason);
                     if (ConfirmExit()) goto MOD_EXIT;
                     else continue;
          }
          commitmsg = 0;
        }
                                             /* Get answer if wanted        */
        if (Options & qoWAIT_REPLY)
        {
                                         /* Reset possible values       */
          /******************************************************************/
          /* Now, the reply depends on what we asked for                    */
          /******************************************************************/
          if (Report & MQRO_PASS_MSG_ID)
            memcpy(ReplyMdesc.MsgId   , Mdesc.MsgId, sizeof(ReplyMdesc.MsgId));
          else
            memcpy(ReplyMdesc.MsgId   , MQMI_NONE, sizeof(ReplyMdesc.MsgId));

          if (Report & MQRO_PASS_CORREL_ID)
            memcpy(ReplyMdesc.CorrelId, Mdesc.CorrelId, sizeof(ReplyMdesc.CorrelId));
          else if (Report & MQRO_COPY_MSG_ID_TO_CORREL_ID)
            memcpy(ReplyMdesc.CorrelId, Mdesc.MsgId, sizeof(ReplyMdesc.CorrelId));
          else
            memcpy(ReplyMdesc.CorrelId, MQCI_NONE, sizeof(ReplyMdesc.CorrelId));

          if (Options & qoCCSID)
          {
            ReplyMdesc.Encoding       = MQEnc;
            ReplyMdesc.CodedCharSetId = MQCcsid;
          }

          if (commits)
          {
            gmo.Options |= MQGMO_SYNCPOINT;
            commitmsg++;
          }
          else
          {
            gmo.Options |= MQGMO_NO_SYNCPOINT;
          }
MQGET2:
          if (Pause) { fprintf(stderr,"About to MQGET\n"); getchar(); }

          PRINTDETAILS(MO_MQMD_BEFORE, msgopts, "MQGET", (unsigned char *) &ReplyMdesc);
          IMQGET(Mq.Api,
                 Mq.hQm,
                 hRObj,
                &ReplyMdesc,
                &gmo,
                 MsgSize,
                 pMsg,
                &MsgLen,
                &CompCode,
                &Reason );
          switch(Reason)
          {
            case 0L:
                   if (verbose >= VL_MQAPI) MQError("MQGET",IQ,Reason);
                   if (verbose >= VL_MQDETAILS)
                   {
                     fprintf(stderr,"MQGET %ld bytes\n",MsgLen);
                   }
                   else if (msgopts[0])
                   {
                     if (strchr(msgopts,MO_OUTPUT_MSGLEN))
                     {
                       fprintf(stderr,"MQGET %ld bytes\n",MsgLen);
                     }
                   }
                   break;

          case MQRC_TRUNCATED_MSG_FAILED:
                   pNewMsg = (char *)QGetMem(MsgLen);
                   if (!pNewMsg)
                   {
                     fprintf(stderr,"Failed to allocate %ld bytes\n",MsgLen);
                     Reason = MQRC_STORAGE_NOT_AVAILABLE;
                     if (ConfirmExit()) goto MOD_EXIT;
                     else continue;
                   }
                   MsgSize = MsgLen;
                   QFreeMem(pMsg);
                   pMsg = pNewMsg;
                   goto MQGET2;
                   break;

          default: MQError("MQGET",ReplyQ,Reason);
                   if (ConfirmExit()) goto MOD_EXIT;
                   else continue;
                   break;
          }
                                       /* End of WaitReply            */
        }
                                       /* End of destination loop     */
      }

      if (MsgLimit >  0) MsgLimit--;
                                       /* End of repeat MQPUT loop    */
    }
    if (hIObj == MQHO_UNUSABLE_HOBJ && (Options & qoREPORT_TIMES))
    {
      ReportTimes(&reps,0);
    }
    /******************************************************************/
    /* User wants it written to a file                                */
    /******************************************************************/
    if (OutFile)
    {
      int TotalLen = 0;
      int ThisLen;
      while (TotalLen < MsgLen)
      {
        ThisLen = fwrite(pMsg+TotalLen,1,MsgLen - TotalLen, OutFile);
        if (ThisLen <= 0)
        {
          fprintf(stderr,"Error writing to file RC(%d)\n",errno);
          Reason = MQRC_FILE_SYSTEM_ERROR;
          if (ConfirmExit()) goto MOD_EXIT;
          else continue;
        }
        TotalLen +=  ThisLen;
      }
    }

    /******************************************************************/
    /* Do we need to output the message we've just got                */
    /******************************************************************/
    if (Options & qoUSE_STDOUT && !DestCount)
    {
      PrintMsg(pMsg, MsgLen, &Mdesc, msgopts, FALSE);
    }
    if (Options & qoEXPLICIT_MSG) break;

    if (MsgLimit == 0)
    {
      /***************************************************************/
      /* Report Timeings if required                                 */
      /***************************************************************/
      if (Options & qoREPORT_TIMES)
      {
        ReportTimes(&ngets,WaitInterval);
      }
      /**************************************************************/
      /* According to the message limit we should end now....       */
      /* However, if close quiesce is set we should only end when   */
      /* the queue is empty                                         */
      /**************************************************************/
      if (Options2 & qo2CLOSE_EXPLICIT)
      {
        if (!CloseExplicit(&Mq,IQ,&hIObj)) break;
        MsgLimit = 99;
      }
      else
      {
        break;
      }
    }
  }
MOD_EXIT:
  /********************************************************************/
  /* Commit any outstanding UOW if all is ok                          */
  /********************************************************************/
  if (((Reason == 0) || (Reason == MQRC_NO_MSG_AVAILABLE)) &&
      commits &&
      commitmsg)
  {
    if (Pause) { fprintf(stderr,"About to MQCMIT\n"); getchar(); }

    if (Options2 & qo2DUAL_QM)
    {
      IMQCMIT(Mq2.Api,
              Mq2.hQm,
             &CompCode,
             &Reason2 );
      if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq2.Qm,Reason2);
      switch(Reason2)
      {
        case 0L: break;

        default: MQError("MQCMIT",Mq2.Qm,Reason2);
                 if (ConfirmExit()) goto MOD_EXIT;
                 break;
      }
    }

    IMQCMIT(Mq.Api,
            Mq.hQm,
           &CompCode,
           &Reason2 );
    if (verbose >= VL_MQAPI) MQError("MQCMIT",Mq.Qm,Reason2);
    switch(Reason2)
    {
      case 0L: break;

      default: MQError("MQCMIT",Mq.Qm,Reason2);
               if (ConfirmExit()) goto MOD_EXIT;
               break;
    }
    commitmsg = 0;
  }

  if (SubOpts & soDURABLE     &&
      SubOpts & soREMOVE_SUB  &&
      hSub != MQHO_UNUSABLE_HOBJ)
  {
    IMQCLOSE( Mq.Api,
              Mq.hQm,
             &hSub,
              MQCO_REMOVE_SUB,
             &CompCode,
             &Reason );

    if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
    {
      MQError("MQCLOSE",SubDesc.SubName.VSPtr,Reason);
    }
  }

  if (hSub != MQHO_UNUSABLE_HOBJ && Options2 & qo2CLOSE_HSUB)
  {
    IMQCLOSE( Mq.Api,
              Mq.hQm,
             &hSub,
              0,
             &CompCode,
             &Reason );

    if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
    {
      MQError("MQCLOSE",SubDesc.SubName.VSPtr,Reason);
    }
    /* printf("Sleeping.......\n");  */
    /* CSMSSleep(10000);             */
  }

  if (!(Options & qoNO_MQDISC)) Cleanup();

  if (Repeat && pMsg)
  {
    fprintf(stderr,"Do you want to connect again (y/n) ?\n");
    fgets(pMsg,MsgSize,stdin);
    if (*pMsg == 'y') goto CONNECT;
  }

  if (OutFile) fclose(OutFile);

  if (! (Options & qoNO_HANDLER))
  {
    /* No-op for all platforms apart from OS/2 which is now removed   */
  }
  if (Options2 & qo2RETVAL_ZERO) return 0;
  else if (Reason == MQRC_NO_MSG_AVAILABLE) return 0;
  else if (Options2 & qo2RETVAL_MQCC)
    switch (CompCode)
    {
      case MQCC_OK:      return 0;
      case MQCC_WARNING: return 4;
      default:           return 8;
    }
  else return (int)Reason;
}

/**********************************************************************/
/* Function : LineOut                                                 */
/* Purpose  : Print out message                                       */
/**********************************************************************/
static int   LineOut(void  * Parm,
                     CBOPT * Opts,
                     char  * pLine,
                     int     Len)
{
  fwrite( pLine , (int)Len, 1, stdout);
  if (!strchr( pLine, '\n')) fputc('\n',stdout);
  return 0;
}

/**********************************************************************/
/* Function : PrintMsg                                                */
/* Purpose  : Print out message                                       */
/**********************************************************************/
static void  PrintMsg(char  * pMsg,
                      MQLONG  MsgLen,
                      MQMD2 * Mdesc,
                      char  * msgopts,
                      BOOL    ShowMQMD)

{
  long     Format = MF_HEXCH |
                    MF_AUTO_DETECT_XML;
  long     Format2 = 0;
  char     Buffer[256];
  char   * pBuffer;
  CBOPT    cbopt;
  long     chk;
  int      i;
  int      len;
  unsigned char    * p;
  MQLONG   Length;

  if (hmsg) PrintProperties(Mq.hQm,hmsg,NULL);

                                       /* What format required        */
  if (strchr(msgopts,MO_IGNORE_CRLF  )) Format2 |= MF2_IGNORE_CRLF;
  if (strchr(msgopts,MO_OUTPUT_MSGLEN)) Format  |= MF_MQMD   | 0x00000001;
  if (strchr(msgopts,MO_OUTPUT_FORMAT)) Format  |= MF_FORMAT | 0x00000001;
  if (strchr(msgopts,MO_OUTPUT_HEX   ))
  {
    if (Format & MF_FORMAT) Format |= MF_HEXBITS;
                       else Format |= (MF_HEX | MF_OFFSET | MF_HEXSEP);
  }
  if (strchr(msgopts,MO_OUTPUT_NOMSG )) Format |= MF_NOMSG;
  if (strchr(msgopts,MO_OUTPUT_OFFSET)) Format |= MF_OFFSET;
  if (strchr(msgopts,MO_OUTPUT_NOXML )) Format &= ~MF_AUTO_DETECT_XML;
  if (strchr(msgopts,MO_OUTPUT_XML   )) Format |= MF_XML_SHORTFORM;
  if (strchr(msgopts,MO_DETAIL_2))
  {
    Format &= 0xFFFFFF00;
    Format |= 0x00000010;
  }
  if (strchr(msgopts,MO_DETAIL_3)) Format |= 0x00000011;
  if (Options2 & qo2LOCAL_TIMES) Format |= MF_LOCAL_TIMES;

  InitCBopt(&cbopt,Format,Format2);

  if (ShowMQMD && (Format & MF_MQMD))
  {
    p  = (unsigned char *) Mdesc;
    Length      = -1;                /* Don't know the size ...     */

    gEncoding   = CSENC_LOCAL;
    gEbcdic     = CS_IS_EBCDIC;

    PrintStruct(&DefMQMD,
                NULL,
                &LineOut,
                NULL,
                &cbopt,
                &Length,
                &p);
  }
  else
  {
    memcpy(FormatName,Mdesc->Format,8);
    gEncoding = Mdesc->Encoding & 0x3;
    gEbcdic   = CSIsEbcdic(Mdesc->CodedCharSetId);
  }
                                     /* Been asked for Hex          */
  if (Format & MF_HEX)
  {
    p      = (unsigned char *) pMsg;
    Length = MsgLen;

    CSDumpHex(&LineOut,
              NULL,
              p,
              Length,
              &cbopt);
  }
                                     /* Should we format it         */
  if (Format & MF_FORMAT)
  {
    p      = (unsigned char *)pMsg;
    Length = MsgLen;
    PrintStruct(&DefMQMSG,
                NULL,
                &LineOut,
                NULL,
                &cbopt,
                &Length,
                &p);
  }
                                     /* Just dump out raw text ?    */
  if (! (Format & MF_FORMAT) &&
      ! (Format & MF_HEX   ) &&
      ! (Format & MF_NOMSG ))
  {
    if (*pMsg !='#' || (Options2 & qo2NO_SPECIAL_MSG) )
    {
      LineOut( NULL, 0,pMsg, MsgLen );
    }
    else
    {
      sprintf(Buffer,"(%ld bytes) ",MsgLen);
      pBuffer = Buffer + strlen(Buffer);
      if (*(pMsg+1)=='c')
      {
        chk = 0;
        for (i=0 ; i<MsgLen ; i++) chk += *(pMsg+i);
        sprintf(pBuffer,"[Chk.%ld] ",chk);
        pBuffer = pBuffer + strlen(pBuffer);
      }

      len = MIN((int)strlen(pMsg),MIN((int)MsgLen,40));
      memcpy(pBuffer,pMsg,(int)len);
      pBuffer[len] = 0;
      LineOut( NULL, 0,Buffer, (long)strlen(Buffer));
    }
  }

  fflush(stdout);
}
/**********************************************************************/
/* Function : UsageLine                                               */
/* Purpose  : Print out a line of usage                               */
/*            Returns TRUE if the user wants to go back to help       */
/**********************************************************************/
static BOOL UsageLine(char * pLine)
{
  char text[100];

  if (lineno>=(ScreenHeight-3))
  {
    fprintf(stdout,"More <Press Enter>...\n");
    fgets(text,sizeof(text),stdin);
    lineno = 0;
    if (strchr(text,'?')) return TRUE;
  }
  if (*pLine == '+')
  {
    fputc(' ',stdout);
    pLine++;
  }
  fprintf(stdout,pLine);
  lineno++;
  return FALSE;
}
/**********************************************************************/
/* Function : UsageBit                                                */
/* Purpose  : Print out bit of the usage text                         */
/**********************************************************************/
BOOL UsageBit(char * search)
{
  int    i,s;
  int    n = sizeof(UsageText)/sizeof(char *);
  char * pUsage;
  int    prev = FALSE;

  for (i=0; i<n; i++)
  {
    pUsage = UsageText[i];

    if (search)
    {
      if (stristr(pUsage,search) || (prev && *pUsage=='+'))
      {
        if (!prev && *pUsage=='+')
        {
          s = i;
          while(*UsageText[s] == '+' && s) s--;
          while(s < i) UsageLine(UsageText[s++]);
        }
        if (UsageLine(pUsage)) return TRUE;
        prev = TRUE;
      }
      else
        prev = FALSE;
    }
    else
    {
      if (UsageLine(pUsage)) return TRUE;
    }
  }
  return FALSE;
}
/**********************************************************************/
/* Function : Usage                                                   */
/* Purpose  : Print out Q command format                              */
/**********************************************************************/
void Usage(int AskedFor,char * topic)
{
  int n = sizeof(UsageText)/sizeof(char *);
  char * pYear = __DATE__;
  char search[200];

  sprintf(search,"5724-H72 (C) Copyright IBM Corp. 1994, %4.4s\n",pYear+7);
  UsageLine(search);

  if (topic && *topic)
  {
    if (*topic) UsageBit(topic);
  }

  while (1)
  {
    if (AskedFor)
    {
      while (1)
      {
        fprintf(stderr,"Help - Enter topic:\n");
        if (!fgets(search,sizeof(search),stdin)) goto MOD_EXIT;
        if (!search[0])    goto MOD_EXIT;
        lineno=0;
        UsageBit(search);
      }
    }
    else
    {
      if (UsageBit(NULL)) AskedFor = TRUE;
                     else break;
    }
  }
MOD_EXIT:
  ;
}
/**********************************************************************/
/* Function : strip                                                   */
/* Purpose  : Remove trailing spaces from fixed length string         */
/**********************************************************************/
static void strip(char * p,long len)
{
  p+=len-1;
  while(len && *p==' ') {*p--=0; len--;}
}

/**********************************************************************/
/* Function : Cleanup                                                 */
/* Purpose  : Backout and disconnect from MQ resources                */
/**********************************************************************/
static void Cleanup(void)
{
  MQLONG   CompCode,Reason;
  /********************************************************************/
  /* Disconnect from the Queue Manager                                */
  /********************************************************************/
  if (Mq.hQm != MQHC_UNUSABLE_HCONN)
  {
                                       /* Rollback any transactions   */
    IMQBACK(Mq.Api,Mq.hQm,&CompCode,&Reason);
    if (verbose >= VL_MQAPI) MQError("MQBACK",Mq.Qm,Reason);

    IMQDISC(Mq.Api,&Mq.hQm,&CompCode,&Reason);
    if (verbose >= VL_MQAPI) MQError("MQDISC",Mq.Qm,Reason);
  }

  if (Options2 & qo2DUAL_QM)
  {
    if (Mq2.hQm != MQHC_UNUSABLE_HCONN)
    {
                                       /* Rollback any transactions   */
      IMQBACK(Mq2.Api,Mq2.hQm,&CompCode,&Reason);
      if (verbose >= VL_MQAPI) MQError("MQBACK",Mq2.Qm,Reason);

      IMQDISC(Mq2.Api,&Mq2.hQm,&CompCode,&Reason);
      if (verbose >= VL_MQAPI) MQError("MQDISC",Mq2.Qm,Reason);
    }
  }
}

/**********************************************************************/
/* Function : QGetMem                                                  */
/* Purpose  : Allocate storage                                        */
/**********************************************************************/
static void * QGetMem(long size)
{
  void   * p = NULL;
                                       /* Make room for size          */
  size += sizeof(long);
  if (verbose >= VL_MQAPI) fprintf(stderr,"Allocating %ld bytes\n",size);

  p = malloc((int)size);
  if (p)
  {
    * (long *)p = size;
    p =  (char *)p + sizeof(long);
  }
  return p;
}

/**********************************************************************/
/* Function : QFreeMem                                                 */
/* Purpose  : Free the stroage allocated by QGetMem                    */
/**********************************************************************/
static void QFreeMem(void * p)
{
  long * pl = (long *)p;

  if (verbose >= VL_MQAPI) fprintf(stderr,"Freeing %ld bytes\n",*(pl-1));
  pl--;
  free(pl);
}

/**********************************************************************/
/* Function : ParseQueueName                                          */
/* Purpose  : Parse a fully qualified Queuename in QM & Q             */
/**********************************************************************/
static void ParseQueueName(char   Separator,
                           char * pQ,
                           char * Qm,
                           char * Q)
{
  char * pnQ;
  /********************************************************************/
  /* A number of characters can be used as a separator,check for each */
  /********************************************************************/
  if (Separator)
  {
    pnQ = strchr(pQ,Separator);
  }
  else
  {
              pnQ = strchr(pQ,'/');
    if (!pnQ) pnQ = strchr(pQ,'#');
    if (!pnQ) pnQ = strchr(pQ,'\\');
    if (!pnQ) pnQ = strchr(pQ,',');
  }
  if (pnQ)
  {
                                       /* Separator found             */
    *pnQ++ = 0;
    strncpy(Qm,pQ,48);
    pQ = pnQ;
  }
  else
  {
    *Qm = 0;
  }
  strncpy(Q,pQ,48);
}

/**********************************************************************/
/* Function : GetId                                                   */
/* Purpose  : Set a msgid or correlid to input value                  */
/**********************************************************************/
static void GetId(char * Id,
                  char * Value)
{
  int len = strlen(Value);
  memcpy(Id,Value,MIN(len,24));
  if (len<24) memset(&Id[len],' ',24-len);
}
/**********************************************************************/
/* Function : GetHexId                                                */
/* Purpose  : Set a msgid or correlid to input value input in HEX     */
/**********************************************************************/
static void GetHexId(char * Id,
                     char * Value)
{
  StrHex(Value,48,Id);
}
