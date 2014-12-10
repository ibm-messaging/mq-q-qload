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
/*  FILE   : QLOAD.C                                                  */
/*  PURPOSE: MQSeries Queue Unload/Reload Utility                     */
/**********************************************************************/

#ifdef MVS
  #pragma csect(code,"CMQUDLOD")
  #pragma csect(static,"CSTUDLOD")
  #pragma options(RENT)
  #pragma runopts(POSIX(ON))
  #define _XOPEN_SOURCE                /* Required for optarg, optind */
  #define _XOPEN_SOURCE_EXTENDED 1     /* Required for timeb          */
  #define _SHARE_EXT_VARS              /* because of APAR PQ03847     */
#endif
                                       /* Defines                     */
#define MIN(a,b) (a<b ? a:b)
                                       /* Include files               */
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#if defined(AMQ_UNIX) || defined(MVS)
  #include "sys/time.h"
  #include "unistd.h"
  #include "errno.h"
  #include "ctype.h"
#else
#include "qlgetopt.h"
#endif


#ifdef AMQ_AS400
  #include "qlgetopt.h"
#endif

#ifdef WIN32
  #include "windows.h"
#endif

#define NOLOAD                /* No longer using dynload on any platform */

#include "sys/timeb.h"

#include "cmqc.h"

#include "qlcs.h"
#include "qlutil.h"
#include "qlvalues.h"
#include "qload.h"

/* #define DEBUG_TIMEZONE 1 */

#if defined(AMQ_UNIX) && !defined(AMQ_AS400)
  #define GLOBAL_TIMEZONE 1
  extern long timezone;
#endif


#define NOTQUIET  (! (opt.Options & qoQUIET))

extern SIFT AgeBuckets[];
extern SIFT SizeBuckets[];

static CSPARTS SecondTimeParts[] =
{ { 60 * 60 * 24 * 7,  "Week"   ,"Week" },
  { 60 * 60 * 24    ,  "Day"    ,"Day"  },
  { 60 * 60         ,  "Hour"   ,"Hr"   },
  { 60              ,  "Minute" ,"Min"  },
  { 1               ,  "Second" ,"Sec"  },
  { 0               ,  NULL     }};

static CSPARTS MessageSizeParts[] =
{ { 1024*1024*1024  ,  "GB"     ,"GB"  },
  { 1024*1024       ,  "MB"     ,"MB"  },
  { 1024            ,  "Kb"     ,"Kb"  },
  { 1               ,  "byte"   ,"byte"},
  { 0               ,  NULL     }};

/**********************************************************************/
/* Function: Title                                                    */
/* Purpose : Simple function to identify the program                  */
/**********************************************************************/
void Title(void)
{
  char * pYear = __DATE__;
  fprintf(stdout,"5724-H72 (C) Copyright IBM Corp. 1994, %4.4s.\n"
                  "WebSphere MQ Queue Load/Unload Utility\n",pYear+7);
}

/**********************************************************************/
/* Function : Usage                                                   */
/* Purpose  : Print out QLOAD command format                          */
/**********************************************************************/
void Usage(void)
{

  Title();
  fprintf(stdout,"\nUsage: dmpmqmsg <Optional flags as below>\n");
  fprintf(stdout,"        [-a <File access mode for fopen()>\n");
  fprintf(stdout,"            a  Append\n");
  fprintf(stdout,"            b  Binary\n");
  fprintf(stdout,"        [-b <initial buffer size in Kb>]\n");
#ifndef MVS
  fprintf(stdout,"        [-c Connect in client mode]\n");
#endif
  fprintf(stdout,"        [-C [A][I][a][i][d][n] Context]\n");
  fprintf(stdout,"            A  Set all context (default)\n");
  fprintf(stdout,"            I  Set identity context\n");
  fprintf(stdout,"            a  Pass all context\n");
  fprintf(stdout,"            i  Pass identity context\n");
  fprintf(stdout,"            d  Default context\n");
  fprintf(stdout,"            n  No context\n");
  fprintf(stdout,"        [-d <Display Options>]\n");
  fprintf(stdout,"            a         Add ASCII columns to HEX file\n");
  fprintf(stdout,"            A         Write output file in ASCII lines rather than HEX\n");
  fprintf(stdout,"            c         Output ApplData MQMD fields as char\n");
  fprintf(stdout,"            C         Display Correlation Id in summary\n");
  fprintf(stdout,"            H         Don't write file header\n");
  fprintf(stdout,"            i         Write message index\n");
  fprintf(stdout,"            p         Write output file in printable characters format\n");
  fprintf(stdout,"            s         Display queue summary\n");
  fprintf(stdout,"            M         Display Message Ids in summary\n");
  fprintf(stdout,"            N         Don't write MQMD to file\n");
  fprintf(stdout,"            t         Write output file in text lines\n");
  fprintf(stdout,"            T         Display time on queue\n");
#ifdef DEBUG
  fprintf(stdout,"            xtest     Run file format test\n");
#endif
  fprintf(stdout,"            w<Length> Data width\n");
  fprintf(stdout,"        [-D <Delay>]\n");
  fprintf(stdout,"          +ve fixed delay in milliseconds\n");
  fprintf(stdout,"          -ve random delay in milliseconds\n");
  fprintf(stdout,"          r[time%%] relative delay\n");
  fprintf(stdout,"        [-f <File Name> or -f <stdout>]\n");
  fprintf(stdout,"        [-F <File Name>   (forcing overwrite)]\n");
  fprintf(stdout,"        [-g [x][c][m][g] <Id> Get by Id\n");
  fprintf(stdout,"            x  Id is in Hex\n");
  fprintf(stdout,"            m  Id is Message Id\n");
  fprintf(stdout,"            c  Id is Correlation Id\n");
  fprintf(stdout,"            g  Id is Group Id\n");
  fprintf(stdout,"        [-h Strip headers\n");
  fprintf(stdout,"        [-i <Input Queue> (Browse)]\n");
  fprintf(stdout,"        [-I <Input Queue> (Get)]\n");
#ifndef NOLOAD
  fprintf(stdout,"        [-l <library name> e.g.mqm or mqic32]\n");
#endif
  fprintf(stdout,"        [-m <Queue Manager name>]\n");
  fprintf(stdout,"        [-o <Output Queue>]\n");
  fprintf(stdout,"        [-p  Purge source queue]\n");
  fprintf(stdout,"        [-P <CCSID>[:X'Encoding'] CodePage conversion]\n");
  fprintf(stdout,"        [-q  Quiet]\n");
  fprintf(stdout,"        [-r <Message Range>]\n");
  fprintf(stdout,"           x    Message x only\n");
  fprintf(stdout,"           x..y Message x to message y\n");
  fprintf(stdout,"           x#y  y messages starting at message x\n");
  fprintf(stdout,"           #x   First x messages\n");
  fprintf(stdout,"        [-t <Transaction Message Limit>]\n");
  fprintf(stdout,"        [-T <Time on queue (Age) selection>\n");
  fprintf(stdout,"            [[[DD:]HH:]MM ][,[[DD:]HH:]MM]\n");
  fprintf(stdout,"        [-s <Ascii Search String>]\n");
  fprintf(stdout,"        [-S <Ascii Not Search String>]\n");
  fprintf(stdout,"        [-e <Ebcdic Search String>]\n");
  fprintf(stdout,"        [-E <Ebcdic Not Search String>]\n");
  fprintf(stdout,"        [-w <MQGET Wait interval(seconds)]\n");
  fprintf(stdout,"        [-x <Hex Search String>]\n");
  fprintf(stdout,"        [-X <Hex Not Search String>]\n");
  fprintf(stdout,"\n");
  fprintf(stdout,"Examples \n");
  fprintf(stdout,"-------- \n");
  fprintf(stdout,"Unload a queue to a file        : dmpmqmsg -iQ1 -fmyfile\n");
  fprintf(stdout,"Load a queue from a file        : dmpmqmsg -oQ1 -fmyfile\n");
  fprintf(stdout,"Copy a queue from Q1 to Q2      : dmpmqmsg -iQ1 -oQ2\n");
  fprintf(stdout,"Move a queue from Q1 to Q2      : dmpmqmsg -IQ1 -oQ2\n");
  fprintf(stdout,"Move old messages from Q1 to Q2 : dmpmqmsg -IQ1 -oQ2 -T1440\n");
  fprintf(stdout,"Provide Summary of messages     : dmpmqmsg -iQ1 -ds\n");
}

/**********************************************************************/
/* Function : GetId                                                   */
/* Purpose  : Set a msgid or correlid to input value                  */
/**********************************************************************/
static void GetId(char * Id,
                  char * Value)
{
  size_t len = strlen(Value);
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

static int ParseAge(char * p)
{
  int Days,Hours,Minutes;
        Days = Hours = Minutes = 0;
        Minutes = atoi(p);
        p = strchr(p,':');
        if (p)
        {
                p++;
                Hours = Minutes;
                Minutes = atoi(p);
                p = strchr(p,':');
                if (p)
                {
                        p++;
                        Days    = Hours;
                        Hours   = Minutes;
                        Minutes = atoi(p);
                }
        }
        /* How many seconds is that ?  */
        return Days    * (24 * 60*60) +
                     Hours   * (60*60)      +
                     Minutes * (60);
}
/**********************************************************************/
/* Function: QueryQMNm                                                */
/* Purpose : Get the name of the Queue Manager                        */
/**********************************************************************/
MQLONG QueryQMNm(MQHCONN  hQm,
                 MQCHAR48 Qm)
{
  MQOD   ObjDesc      = { MQOD_DEFAULT };
  MQLONG flQueueFlags = MQOO_INQUIRE;
  MQHOBJ QmHobj       = MQHO_UNUSABLE_HOBJ;
  MQLONG CompCode, Reason;
  MQLONG Selectors[1];
  char   * p;
                                       /* Open QM Object              */
  ObjDesc.ObjectType  = MQOT_Q_MGR;

  MQOPEN( hQm,
          &ObjDesc,
           flQueueFlags,
          &QmHobj,
          &CompCode,
          &Reason );
  if (CompCode == MQCC_FAILED) goto MOD_EXIT;
                                       /* Now issue query             */
  Selectors[0] = MQCA_Q_MGR_NAME;

  MQINQ( hQm,                       /* Connection handle              */
         QmHobj,                    /* Object handle                  */
         1,                         /* Count of selectors             */
         Selectors,                 /* Array of attribute selectors   */
         0,                         /* Count of integer attributes    */
         NULL,                      /* Array of integer attributes    */
         MQ_Q_MGR_NAME_LENGTH,      /* Length of character-attributes */
                                    /* buffer                         */
         Qm,                        /* Character attributes           */
         &CompCode,
         &Reason);
  if (CompCode == MQCC_FAILED) goto MOD_EXIT;

  p = memchr(Qm,' ',48);
  if (p) *p = 0;

MOD_EXIT:

  if (QmHobj != MQHO_UNUSABLE_HOBJ)
  {
                                   /* Close Queue Manager Desc.     */
    MQCLOSE( hQm,
            &QmHobj,
             MQCO_NONE,
            &CompCode,
            &Reason);
  }
  return Reason;
}

#ifdef WIN32
/**********************************************************************/
/* Handle C runtime invalid parameters                                */
/**********************************************************************/
void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function,
   const wchar_t* file,
   unsigned int line,
   uintptr_t pReserved)
{
   /* wprintf(L"Invalid parameter detected in function %s."
           L" File: %s Line: %d\n", function, file, line); */
   if (expression) wprintf(L"Expression: %s\n", expression);
}


#endif

/**********************************************************************/
/* Function: AvailableSearch                                          */
/* Purpose : Return the first search without a string                 */
/**********************************************************************/
SEARCH * AvailableSearch(SEARCH * p)
{
  int i;

  for (i=0; i< MAX_SEARCH; i++)
  {
    if (!p->Len) return p;
    p++;
  }
  return NULL;
}

void PrintSift(char * title, UINT32 * buckets, SIFT * sifts, int MaxSifts)
{
  int width = strlen(sifts[0].name);
  int i,j;

  fprintf(stderr,"%s\n",title);
  for (i=0; i<(int)strlen(title) ; i++) fputc('=',stderr);
  fputc('\n',stderr);
  fputc('\n',stderr);
  for (i=0;i<MaxSifts;i++)
  {
    fprintf(stderr,"%s ",sifts[i].name);
  }
  fputc('\n',stderr);
  for (i=0;i<MaxSifts;i++)
  {
    for (j=0;j<width;j++)  fputc('-',stderr);
   fputc(' ',stderr);
  }
  fputc('\n',stderr);
  for (i=0;i<MaxSifts;i++)
  {
    fprintf(stderr,"%*d ",width,buckets[i]);
  }
  fputc('\n',stderr);
}

void OutputIds(QLOPT    * opt,
               MQBYTE24 * pMsgId,
               MQBYTE24 * pCorrelId)
{
  char Id[50];
  if (opt->Options & qoSUMMARY_MSGID)
  {
    HexStrn((unsigned char *)pMsgId,24,Id,48);
    fprintf(stderr,"     Message Id:%48.48s\n",Id);
  }
  if (opt->Options & qoSUMMARY_CORRELID)
  {
    HexStrn((unsigned char *)pCorrelId,24,Id,48);
    fprintf(stderr,"     Correl  Id:%48.48s\n",Id);
  }
}

void PrintSummary(QLOPT * opt)
{
  char Buffer[100];
  fprintf(stderr,"Read    - Files:%4d  Messages:%6d  Bytes:%10d\n",opt->InFileIndex ,opt->InMsgs ,opt->InBytes);

  fputc('\n',stderr);
  PrintSift("Message Ages",opt->AgeBuckets ,AgeBuckets,MAX_AGE_BUCKETS);

  CSConvertParts(opt->YoungestMsg,Buffer,SecondTimeParts,TRUE);
  fprintf(stderr,"\nYoungest Message : %s\n",Buffer);
  OutputIds(opt,&opt->YoungestMsgMsgId,&opt->YoungestMsgCorrelId);

  CSConvertParts(opt->OldestMsg,Buffer,SecondTimeParts,TRUE);
  fprintf(stderr,"\nOldest   Message : %s\n",Buffer);
  OutputIds(opt,&opt->OldestMsgMsgId,&opt->OldestMsgCorrelId);

  fputc('\n',stderr);
  PrintSift("Message Sizes",opt->SizeBuckets,SizeBuckets,MAX_SIZE_BUCKETS);

  CSConvertParts(opt->SmallestMsg,Buffer,MessageSizeParts,FALSE);
  fprintf(stderr,"\nSmallest Message : %s\n",Buffer);
  OutputIds(opt,&opt->SmallestMsgMsgId,&opt->SmallestMsgCorrelId);

  CSConvertParts(opt->BiggestMsg,Buffer,MessageSizeParts,FALSE);
  fprintf(stderr,"\nBiggest  Message : %s\n",Buffer);
  OutputIds(opt,&opt->BiggestMsgMsgId,&opt->BiggestMsgCorrelId);

}

/**********************************************************************/
/* Function : OneOf                                                   */
/* Purpose  : Return boolean indicating whether only one opt. used    */
/**********************************************************************/
BOOL OneOf(char *p, char *opts)
{
  int count = 0;

  while (*p)
  {
    if (strchr(opts,*p)) count++;
    p++;
  }
  return count <= 1;
}

/**********************************************************************/
/* Function : Main program                                            */
/* Purpose  : Get parameters from command line                        */
/* Description                                                        */
/* -----------                                                        */
/* This program acts like a pipe taking input from one source and     */
/* outputting to another source.                                      */
/*                                                                    */
/* Input : File                                                       */
/*         MQSeries Queue                                             */
/*                                                                    */
/* Output: File                                                       */
/*         MQSeries Queue                                             */
/**********************************************************************/
int main(int     argc,
         char ** argv)
{
  QLOPT    opt;
  int      c;
  int      ArgumentOK = 0;
  MQLONG   CompCode;
  MQLONG   Reason = MQRC_OPTIONS_ERROR;
  char   * library = NULL;
  char   * p,*pYoung;
  BOOL     valid;
  int      len,newlen;
  BOOL     PrintStats = FALSE;
  BOOL     SetTime = FALSE;
  char     Buffer[100],YoungBuffer[100];
  SEARCH * pSearch;
  BOOL     RunTest = 0;
  BOOL     UsageError = TRUE;

#if !defined(AMQ_AS400)
  tzset();
#endif

#ifdef WIN32
  {
     _invalid_parameter_handler oldHandler, newHandler;
     newHandler = myInvalidParameterHandler;
     oldHandler = _set_invalid_parameter_handler(newHandler);
  }
#endif


#ifdef DEBUG_TIMEZONE
#ifdef GLOBAL_TIMEZONE
  printf("TZ has been set (Timezone is %d)\n",timezone);
#endif
#endif

  /********************************************************************/
  /* Set default values                                               */
  /********************************************************************/
  QloadInit(&opt);

  /********************************************************************/
  /* Grab the parameters from the command line                        */
  /********************************************************************/
  while ((c=getopt(argc, argv,"a:b:cC:d:D:e:E:f:F:g:hi:I:l:m:o:pP:qr:s:S:t:T:v:w:x:X:?")) !=  EOF)
  {
    switch(c)
    {
                                       /* Help                        */
      case '?':
           Usage();
           goto MOD_EXIT;
           break;
                                       /* File Access Mode            */
      case 'a':
           opt.AccessMode = optarg;
           break;
                                       /* Initial msg buffer size     */
      case 'b':
           opt.StartMsgArea = atoi(optarg)*1024;
           break;
                                       /* Client mode                 */
#ifndef MVS
      case 'c':
           opt.ClientMode   = TRUE;
           break;
#endif                                /* Convert                     */
      case 'P':
           opt.Options |= qoCONVERT;
           sscanf(optarg,"%ld:%lX",&opt.RequiredCCSID,&opt.RequiredEncoding);
           if (opt.RequiredCCSID    == 0) opt.RequiredCCSID    = MQCCSI_Q_MGR;
           if (opt.RequiredEncoding == 0) opt.RequiredEncoding = MQENC_NATIVE;
           break;
                                       /* Context                     */
      case 'C':
           if (optarg[1] != 0)
           {
             fprintf(stderr,"The context parameter can only be a single letter\n\n");
             Usage();
             goto MOD_EXIT;
           }
           opt.Options &= ~qoCONTEXT_MASK;
           switch(optarg[0])
           {
             case 'A': opt.Options |= qoSET_ALL_CONTEXT;       break;
             case 'I': opt.Options |= qoSET_IDENTITY_CONTEXT;  break;
             case 'a': opt.Options |= qoPASS_ALL_CONTEXT;      break;
             case 'i': opt.Options |= qoPASS_IDENTITY_CONTEXT; break;
             case 'd': opt.Options |= qoDEFAULT_CONTEXT;       break;
             case 'n': opt.Options |= qoNO_CONTEXT;            break;
             default:
                  fprintf(stderr,"I did not understand your context parameter\n\n");
                  Usage();
                  goto MOD_EXIT;
           }
           break;
                                       /* Display options             */
      case 'd':
           p = optarg;
           if (!OneOf(p,"Apt"))
           {
             fprintf(stderr,"Please enter only one display option, 'A', 'p' or 't'\n");
             goto MOD_EXIT;
           }
           while (*p)
           {
             switch(*p)
             {
               case 0: break;
                                       /* Add ASCII columns to output */
               case 'a':
                    opt.Options |= qoDSP_ASCII_COLS_FILE;
                    break;
                                       /* Output ASCII instead of hex */
               case 'A':
                    opt.Options |= qoDSP_ASCII_FILE;
                    break;
                                       /* HEX Chars as chars          */
               case 'c':
                    opt.Options |= qoHEXCHAR_CHAR;
                    break;
                                       /* Don't write file header     */
               case 'H':
                    opt.Options2 |= qo2NO_WRITE_HEADER;
                    break;
                                       /* Write out message index     */
               case 'i':
                    opt.Options |= qoWRITE_INDEX;
                    break;
                                       /* Sumary                      */
               case 's':
                    opt.Options |= qoSUMMARY;
                    if (!opt.OutFileName[0]) strcpy(opt.OutFileName,"stdout");
                    SetTime = TRUE;
                    break;
                                       /* Display CorrelId            */
               case 'C':
                    opt.Options |= qoSUMMARY_CORRELID;
                    break;
                                       /* Display MsgId               */
               case 'M':
                    opt.Options |= qoSUMMARY_MSGID;
                    break;
                                       /* Don't write MQMD            */
               case 'N':
                    opt.Options2 |= qo2NO_WRITE_MQMD;
                    break;
                                       /* Output as printable chars   */
               case 'p':
                    opt.Options2 |= qo2DSP_PRINTABLE;
                    break;
                                       /* Output as text lines        */
               case 't':
                    opt.Options2 |= qo2DSP_TEXT;
                    break;
                                       /* Write out message age       */
               case 'T':
                    opt.Options |= qoWRITE_AGE;
                    SetTime = TRUE;
                    break;
                                       /* Run file format test        */
               case 'x':
                    if (memcmp(p,"xtest",5))
                    {
                      fprintf(stderr,"I did not understand display parameter\n\n");
                      Usage();
                      goto MOD_EXIT;
                    }
                    p += 5;
                    RunTest = TRUE;
                    break;
                                       /* Data width                  */
               case 'w':
                    p++;
                    opt.DataWidth = atoi(p);
                    while (isdigit(*p)) p++;
                    p--;
                    break;
               default:
                    fprintf(stderr,"I did not understand your display parameter '%c'\n\n",*p);
                    Usage();
                    goto MOD_EXIT;
             }
             p++;
           }
           break;

      case 'D':
           p = optarg;
           if (*p == 'r')
           {
             opt.Options |= qoRELATIVE_DELAY;
             opt.Delay    = atoi(p+1);
             if (!opt.Delay) opt.Delay = 100;
           }
           else opt.Delay    = atoi(p);
           break;
                                       /* File Name                   */
      case 'F':
           opt.Options |= qoFORCE_FILE;
                                       /* DELIBERATELY NO BREAK HERE  */
      case 'f':
           if (opt.InFileName[0])
           {
             if (opt.OutFileName[0])
             {
               fprintf(stderr,"Sorry, you can only specify one input and one output file\n");
               goto MOD_EXIT;
             }
             strncpy(opt.OutFileName,optarg,MAX_FILE_NAME);
             opt.OutFileName[MAX_FILE_NAME-1] = 0;
           }
           else
           {
             strncpy(opt.InFileName,optarg,MAX_FILE_NAME);
             opt.InFileName[MAX_FILE_NAME-1] = 0;
           }
           break;
                                       /* Get keys                    */
      case 'g':
           if (*optarg == 'm')
           {
             GetId((char *)opt.MsgId,optarg+1);
             opt.Options |= qoMATCH_MSGID;
           }
           else if (*optarg == 'c')
           {
             GetId((char *)opt.CorrelId,optarg+1);
             opt.Options |= qoMATCH_CORRELID;
           }
           else if (*optarg == 'g')
           {
             GetId((char *)opt.GroupId,optarg+1);
             opt.Options |= qoMATCH_GROUPID;
           }
           else if (*optarg == 'x')
           {
             optarg++;
             if (*optarg == 'm')
             {
               GetHexId((char *)opt.MsgId,optarg+1);
               opt.Options |= qoMATCH_MSGID;
             }
             else if (*optarg == 'c')
             {
               GetHexId((char *)opt.CorrelId,optarg+1);
               opt.Options |= qoMATCH_CORRELID;
             }
             else if (*optarg == 'g')
             {
               GetHexId((char *)opt.GroupId,optarg+1);
               opt.Options |= qoMATCH_GROUPID;
             }
             else
             {
               printf("Unrecognised 'g' flag '%c', please specify 'm','c' or 'g'\n",*optarg);
               goto MOD_EXIT;
             }
           }
           else
           {
             printf("Unrecognised 'g' flag '%c', please specify 'm',c','g' or 'x'\n",*optarg);
             goto MOD_EXIT;
           }
           break;
                                       /* Strip any MQ headers        */
      case 'h':
           opt.Options |= qoSTRIP_HEADERS;
           break;
                                       /* Input Queue                 */
      case 'I':
           opt.Options |= qoMOVE;
                                       /* DELIBERATELY NO BREAK HERE  */
      case 'i':
           if (opt.InputQueue[0])
           {
             fprintf(stderr,"Sorry, you can only specify one input queue\n");
             goto MOD_EXIT;
           }
           strncpy(opt.InputQueue,optarg,MQ_Q_NAME_LENGTH);
           opt.InputQueue[MQ_Q_NAME_LENGTH] = 0;
           break;
#ifndef NOLOAD
                                       /* Library name                */
      case 'l':
           library = optarg;
           break;
#endif
                                       /* Output Queue                */
      case 'o':
           if (opt.OutputQueue[0])
           {
             fprintf(stderr,"Sorry, you can only specify one output queue\n");
             goto MOD_EXIT;
           }
           strncpy(opt.OutputQueue,optarg,MQ_Q_NAME_LENGTH);
           opt.OutputQueue[MQ_Q_NAME_LENGTH] = 0;
           break;
                                       /* Queue Manager               */
      case 'm':
           if (opt.Qm[0])
           {
             fprintf(stderr,"Sorry, you can only specify one Queue Manager\n");
             goto MOD_EXIT;
           }
           strncpy(opt.Qm,optarg,MQ_Q_MGR_NAME_LENGTH);
           opt.Qm[MQ_Q_MGR_NAME_LENGTH] = 0;
           break;
                                       /* Purge                       */
      case 'p':
           opt.Options |= qoPURGE;
           break;
                                       /* Quiet                       */
      case 'q':
           opt.Options |= qoQUIET;
           break;
                                       /* Range                       */
      case 'r':
           opt.Options |= qoFILTER;
           /***********************************************************/
           /* Allow the following (n) is number                       */
           /* n..n                        Number range                */
           /* n#n                         Start and Amount            */
           /* ..n                         Up to index n               */
           /* #n                          n messages                  */
           /***********************************************************/
           valid = TRUE;
           p     = optarg;
           if (isdigit(*p))
           {
             opt.FirstIndex = atoi(p);
             while (isdigit(*p)) p++;
           }
           switch(*p)
           {
             case 0:
                  opt.LastIndex = opt.FirstIndex;
                  break;
             case '.':
                  p++;
                  if (*p != '.') { valid = FALSE; break;}
                  p++;
                  opt.LastIndex = atoi(p);
                  while(*p)
                  {
                    if (!isdigit(*p)) valid = FALSE;
                    p++;
                  }
                  break;
             case '#':
                  p++;
                  if (opt.FirstIndex)
                    opt.LastIndex = opt.FirstIndex + atoi(p) - 1;
                  else
                    opt.LastIndex = atoi(p);
                  while(*p)
                  {
                    if (!isdigit(*p)) valid = FALSE;
                    p++;
                  }
                  break;
             default:
                  valid = FALSE;
                  break;
           }
           if (!valid)
           {
             fprintf(stderr,"Invalid message range parameter '%s'\n",optarg);
             goto MOD_EXIT;
           }
           if (opt.FirstIndex)
           {
             if (opt.LastIndex)
             {
               if (opt.FirstIndex > opt.LastIndex)
               {
                 int temp = opt.FirstIndex;
                 opt.FirstIndex = opt.LastIndex;
                 opt.LastIndex  = temp;
               }
               if (NOTQUIET)
               {
                 if (opt.FirstIndex == opt.LastIndex)
                   fprintf(stderr,"Processing message index %d only.\n",opt.FirstIndex);
                 else
                   fprintf(stderr,"Processing message indexes %d -> %d.\n",opt.FirstIndex,opt.LastIndex);
               }
             }
             else
             {
               if (NOTQUIET)
               {
                 fprintf(stderr,"Processing message indexes %d onwards.\n",opt.FirstIndex);
               }
             }
           }
           else
           {
             if (opt.LastIndex)
             {
               opt.FirstIndex = 1;
               if (NOTQUIET)
               {
                 fprintf(stderr,"Processing up to message index %d.\n",opt.LastIndex);
               }
             }
             else
             {
               if (NOTQUIET)
               {
                 fprintf(stderr,"Procesing all messages.\n");
               }
             }
           }
           fprintf(stderr,"\n");
           break;
                                       /* Ascii Search string         */
      case 's':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.Ascii);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d ASCII search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE)
           {
             fprintf(stderr,"Sorry the search string is limited to %d characters\n",MAX_SEARCH_SIZE);
             goto MOD_EXIT;
           }
           memcpy(pSearch->Str,optarg,len+1);
#ifdef MVS
           CSToAscii((unsigned char *)pSearch->Str,(unsigned char *)pSearch->Str,len);
           newlen = strlen(pSearch->Str);
           if (newlen != len)
           {
             fprintf(stderr,"Sorry the Ascii search string contains a non-variant character '%c'\n",optarg[newlen]);
             goto MOD_EXIT;
           }
#endif
           pSearch->Len = len;
           break;

      case 'S':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.NegAscii);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d ASCII 'not' search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE)
           {
             fprintf(stderr,"Sorry the search string is limited to %d characters\n",MAX_SEARCH_SIZE);
             goto MOD_EXIT;
           }
           memcpy(pSearch->Str,optarg,len+1);
#ifdef MVS
           CSToAscii((unsigned char *)pSearch->Str,(unsigned char *)pSearch->Str,len);
           newlen = strlen(pSearch->Str);
           if (newlen != len)
           {
             fprintf(stderr,"Sorry the Ascii search string contains a non-variant character '%c'\n",optarg[newlen]);
             goto MOD_EXIT;
           }
#endif
           pSearch->Len = len;
           break;

      case 't':
           opt.TransactionLimit = atoi(optarg);
           break;
                                       /* Message age selection       */
      case 'T':
           opt.Options |= qoFILTER;
           p  = optarg;
           pYoung = strchr(p,',');
           if (pYoung)
           {
             *pYoung = 0;
             pYoung++;
             opt.YoungerThanAge = ParseAge(pYoung);
           }
           opt.OlderThanAge = ParseAge(optarg);
           if (opt.YoungerThanAge && opt.OlderThanAge && opt.YoungerThanAge < opt.OlderThanAge)
           {
             int tmp            = opt.OlderThanAge;
             opt.OlderThanAge   = opt.YoungerThanAge;
             opt.YoungerThanAge = tmp;
           }

           SetTime = TRUE;

           if (opt.OlderThanAge)
           {
             GetCharTime(Buffer,opt.OlderThanAge,FALSE);
             if (opt.YoungerThanAge)
             {
               GetCharTime(YoungBuffer,opt.YoungerThanAge,FALSE);
               if (NOTQUIET)
               {
                 fprintf(stderr,"Processing messages between %s and %s old\n",Buffer,YoungBuffer);
               }
             }
             else
             {
               if (NOTQUIET)
                 fprintf(stderr,"Processing messages older than %s\n",Buffer);
             }
           }
           else
           {
             if (opt.YoungerThanAge)
             {
               GetCharTime(Buffer,opt.YoungerThanAge,FALSE);
               if (NOTQUIET)
                 fprintf(stderr,"Processing messages younger than %s\n",Buffer);
             }
           }
           break;
                                       /* Ebcdic Search string        */
      case 'e':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.Ebcdic);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d Ebcdic search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE)
           {
             fprintf(stderr,"Sorry the search string is limited to %d characters\n",MAX_SEARCH_SIZE);
             goto MOD_EXIT;
           }
           memcpy(pSearch->Str,optarg,len+1);
#ifndef MVS
           CSToEbcdic((unsigned char *)pSearch->Str,(unsigned char *)pSearch->Str,len);
           newlen = strlen(pSearch->Str);
           if (newlen != len)
           {
             fprintf(stderr,"Sorry the Ebcdic search string contains a non-variant character '%c'\n",optarg[newlen]);
             goto MOD_EXIT;
           }
#endif
           pSearch->Len = len;
           break;

      case 'E':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.NegEbcdic);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d Ebcdic 'not' search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE)
           {
             fprintf(stderr,"Sorry the search string is limited to %d characters\n",MAX_SEARCH_SIZE);
             goto MOD_EXIT;
           }
           memcpy(pSearch->Str,optarg,len+1);
#ifndef MVS
           CSToEbcdic((unsigned char *)pSearch->Str,(unsigned char *)pSearch->Str,len);
           newlen = strlen(pSearch->Str);
           if (newlen != len)
           {
             fprintf(stderr,"Sorry the Ebcdic search string contains a non-variant character '%c'\n",optarg[newlen]);
             goto MOD_EXIT;
           }
#endif
           pSearch->Len = len;
           break;
      case 'v':
           if (strchr(optarg,'m')) opt.Verbose |= vbDEBUG_MEMORY;
           break;
      case 'w':
           opt.WaitInterval = atoi(optarg) * 1000;
           break;
      case 'x':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.Hex);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d Hex search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE*2)
           {
             fprintf(stderr,"Sorry the hex search string is limited to %d characters\n",MAX_SEARCH_SIZE*2);
             goto MOD_EXIT;
           }
           if (StrHex((unsigned char *)optarg,len,pSearch->Str))
           {
             fprintf(stderr,"Sorry the hex search string does not appear to be hex\n");
             goto MOD_EXIT;
           }
           pSearch->Len = len/2;
           break;
      case 'X':
           opt.Options |= qoFILTER;
           pSearch = AvailableSearch(opt.NegHex);
           if (!pSearch)
           {
             fprintf(stderr,"Sorry, you can only specify %d Hex 'not' search strings\n",MAX_SEARCH);
             goto MOD_EXIT;
           }
           len = strlen(optarg);
           if (len > MAX_SEARCH_SIZE*2)
           {
             fprintf(stderr,"Sorry the search string is limited to %d characters\n",MAX_SEARCH_SIZE*2);
             goto MOD_EXIT;
           }
           if (StrHex((unsigned char *)optarg,len,pSearch->Str))
           {
             fprintf(stderr,"Sorry the negative hex search string does not appear to be hex\n");
             goto MOD_EXIT;
           }
           pSearch->Len = len/2;
           break;
                                       /* Unrecognised                */
      default:
           fprintf(stderr,"Unrecognised parameter '%s'\n",argv[ArgumentOK+1]);
           Usage();
           Reason = MQRC_OPTIONS_ERROR;
           goto MOD_EXIT;
           break;
    }
    ArgumentOK = optind-1;
  }
                                       /* No unused arguments ?       */
  Reason = 0;
  if (optind != argc || argc < 2)
  {
    Usage();
    goto MOD_EXIT;
  }

  if (!(opt.Options & qoQUIET)) Title();

  /********************************************************************/
  /* Do we want to test the file processing ?                         */
  /********************************************************************/
  if (RunTest) TestQload();

  /********************************************************************/
  /* Make sure the filenames are right                                */
  /********************************************************************/
  if (opt.InFileName[0] && !opt.OutFileName[0])
  {
    if (opt.InputQueue[0])
    {
      strcpy(opt.OutFileName,opt.InFileName);
      opt.InFileName[0] = 0;
    }
  }
  /********************************************************************/
  /* Check that we have valid combinations                            */
  /********************************************************************/
  if (opt.Options & qoSUMMARY && opt.OutputQueue[0])
  {
    fprintf(stderr,"Sorry, you can't specify an output queue with summary display.\n");
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }

  if ( opt.InputQueue[0] &&  opt.InFileName[0]   ||
     (!opt.InputQueue[0] && !opt.InFileName[0]))
  {
    fprintf(stderr,"Please enter either an input queue or an input file.\n");
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }
  if (opt.OutputQueue[0] && opt.OutFileName[0])
  {
    fprintf(stderr,"Please enter either an output queue or an output file, not both.\n");
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }

  /********************************************************************/
  /* Now, check we just have valid combinations                       */
  /********************************************************************/
  if (opt.InputQueue[0]  &&
      opt.OutputQueue[0] &&
      opt.InFileName[0]  &&
      opt.OutFileName[0])
  {
    fprintf(stderr,"Please enter either \n");
    fprintf(stderr,"  Input Queue and Output Queue - to copy queues\n");
    fprintf(stderr,"  Input Queue and Output File  - to save queue to a file\n");
    fprintf(stderr,"  Input File  and Output Queue - to load queue from a file\n");
    fprintf(stderr,"  Input File  and Output File  - to reformat a file\n");
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Check that purge is not being used in combination with browse Q  */
  /********************************************************************/
  if (opt.InputQueue[0]       &&
      !(opt.Options & qoMOVE) &&
      opt.Options & qoPURGE)
  {
    fprintf(stderr,"Purge is invalid when you are browsing the Input Queue\n");
    Reason = MQRC_OPTIONS_ERROR;
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Are we using unique filenames                                    */
  /********************************************************************/
  if (strstr(opt.InFileName,"%i") ||
      strstr(opt.InFileName,"%n") ||
      strstr(opt.InFileName,"%o") ||
      strstr(opt.InFileName,"%I") ||
      strstr(opt.InFileName,"%N") ||
      strstr(opt.InFileName,"%O"))
  {
    opt.Options |= qoUNIQUE_FILES_IN;
  }
  if (strstr(opt.OutFileName,"%i") ||
      strstr(opt.OutFileName,"%n") ||
      strstr(opt.OutFileName,"%o") ||
      strstr(opt.OutFileName,"%I") ||
      strstr(opt.OutFileName,"%N") ||
      strstr(opt.OutFileName,"%O"))
  {
    opt.Options |= qoUNIQUE_FILES_OUT;
  }
  /********************************************************************/
  /* If we are asking for a summary then we should not be 'moving'    */
  /********************************************************************/
  if (opt.Options & qoSUMMARY)
  {
    if (opt.Options & qoMOVE)
    {
      fprintf(stderr,"Sumary option is not valid with move, use browse\n");
      Reason = MQRC_OPTIONS_ERROR;
      goto MOD_EXIT;
    }
  }

  UsageError = FALSE;
  /********************************************************************/
  /* Do we need MQ                                                    */
  /********************************************************************/
  if (opt.InputQueue[0] || opt.OutputQueue[0])
  {
    MQCNO cno = { MQCNO_DEFAULT };
    if (opt.ClientMode)
      cno.Options |= MQCNO_CLIENT_BINDING;

    /******************************************************************/
    /* Now, open what needs to be opened                              */
    /******************************************************************/
    MQCONNX( opt.Qm,
           &cno,
           &opt.hQm,
           &CompCode,
           &Reason);
    if (CompCode == MQCC_FAILED)
    {
      MQError("MQCONNX",opt.Qm,Reason);
      goto MOD_EXIT;
    }

    QueryQMNm(opt.hQm,opt.Qm);
  }

  if (opt.InputQueue[0])
  {
    MQOD   mqod      = {MQOD_DEFAULT};
    MQLONG OpenFlags;

    /******************************************************************/
    /* Filtering doeesn't make sense if we're not 'getting'           */
    /******************************************************************/
    if (!(opt.Options & qoMOVE)) opt.Options &= ~qoFILTER;

    /******************************************************************/
    /* If we've been told to explicitly purge the source queue then   */
    /* switch off any filter flag we might have                       */
    /******************************************************************/
    if (opt.Options & qoPURGE) opt.Options &= ~qoFILTER;

    if (opt.Options & qoMOVE)
    {
      OpenFlags = MQOO_INPUT_SHARED;
      if (opt.Options & qoFILTER) OpenFlags |= MQOO_BROWSE;
    }
    else
    {
      OpenFlags = MQOO_BROWSE;
    }

    if (opt.Options & (qoPASS_ALL_CONTEXT | qoPASS_IDENTITY_CONTEXT))
    {
      if (!(opt.Options & qoMOVE))
      {
        fprintf(stderr,"To pass context you must GET the source message not browse\n");
        Reason = MQRC_CONTEXT_NOT_AVAILABLE;
        goto MOD_EXIT;
      }
      OpenFlags |= MQOO_SAVE_ALL_CONTEXT;
    }

    strncpy(mqod.ObjectName,opt.InputQueue,MQ_Q_NAME_LENGTH);

    MQOPEN(opt.hQm,
            &mqod,
            OpenFlags,
            &opt.hInput,
            &CompCode,
            &Reason );
    if (CompCode == MQCC_FAILED)
    {
      MQError("MQOPEN",opt.InputQueue,Reason);
      goto MOD_EXIT;
    }
  }
  else
  {
    /******************************************************************/
    /* If we're not getting input from a queue then filtering doesn't */
    /* make sense                                                     */
    /******************************************************************/
    opt.Options &= ~qoFILTER;
  }

  if (opt.OutputQueue[0])
  {
    MQOD   mqod      = {MQOD_DEFAULT};
    MQLONG OpenFlags = MQOO_OUTPUT;

    switch(opt.Options & qoCONTEXT_MASK)
    {
      case qoSET_ALL_CONTEXT:        OpenFlags |= MQOO_SET_ALL_CONTEXT;      break;
      case qoSET_IDENTITY_CONTEXT:   OpenFlags |= MQOO_SET_IDENTITY_CONTEXT; break;
      case qoPASS_ALL_CONTEXT:       OpenFlags |= MQOO_PASS_ALL_CONTEXT;     break;
      case qoPASS_IDENTITY_CONTEXT:  OpenFlags |= MQOO_PASS_IDENTITY_CONTEXT;break;
      case qoNO_CONTEXT:
      case qoDEFAULT_CONTEXT:        break;
      default:
           Usage();
           goto MOD_EXIT;
    }

    strncpy(mqod.ObjectName,opt.OutputQueue,MQ_Q_NAME_LENGTH);

    MQOPEN(opt.hQm,
            &mqod,
            OpenFlags,
            &opt.hOutput,
            &CompCode,
            &Reason );
    if (CompCode == MQCC_FAILED)
    {
      MQError("MQOPEN",opt.OutputQueue,Reason);
      goto MOD_EXIT;
    }
  }
  /********************************************************************/
  /* Open any files we need                                           */
  /********************************************************************/
  opt.InIndex = -1;
  Reason = QloadOpenFiles(&opt,TRUE,TRUE,FALSE);
  if (Reason)              goto MOD_EXIT;
  if (opt.ErrorMessage[0]) goto MOD_EXIT;

  /********************************************************************/
  /* Do we need to know the current time ?                            */
  /********************************************************************/
  if (SetTime)
  {
    struct timeb tmb;
    time(&opt.Now);

#ifdef GLOBAL_TIMEZONE
    opt.Now += timezone;
#ifdef DEBUG_TIMEZONE
    printf("Now is %d\n",opt.Now);
#endif
#else
    ftime(&tmb);
    opt.Now += tmb.timezone*60;
#ifdef DEBUG_TIMEZONE
    printf("Timezone %d Now is %d\n",tmb.timezone,opt.Now);
#endif
#endif
  }
  /********************************************************************/
  /* Ok, now do it                                                    */
  /********************************************************************/
  PrintStats  = TRUE;
  opt.InIndex = -1;
  Reason = QloadFnc(&opt,TRUE);
  if (Reason)
  {
    if (opt.InIndex < 0) PrintStats = FALSE;
    goto MOD_EXIT;
  }

MOD_EXIT:
  /********************************************************************/
  /* Print out error message (if any)                                 */
  /********************************************************************/
  if (opt.ErrorMessage[0]) fprintf(stderr,"%s\n",opt.ErrorMessage);
  if (UsageError) goto CLEAN_UP;
  /********************************************************************/
  /* Print out statistics                                             */
  /********************************************************************/
  if (!(opt.Options & qoQUIET))
  {
    if (opt.Options & qoSUMMARY)
    {
      PrintSummary(&opt);
    }
    else
    if (PrintStats)
    {
      fprintf(stderr,"Read    - Files:%4d  Messages:%6d  Bytes:%10d\n",opt.InFileIndex ,opt.InMsgs ,opt.InBytes);
      if (opt.hOutput != MQHO_UNUSABLE_HOBJ || opt.OutFileName[0])
      {
        fprintf(stderr,"Written - Files:%4d  Messages:%6d  Bytes:%10d\n",opt.OutFileIndex,opt.OutMsgs,opt.OutBytes);
      }
    }
  }

CLEAN_UP:
  if (opt.hQm != MQHC_UNUSABLE_HCONN)
  {
    MQLONG CompCode2,Reason2;
    MQDISC(&opt.hQm,&CompCode2,&Reason2);
  }

#ifdef _DEBUG
  {
    char     Buffer[100];
    fprintf(stderr,"Press enter to finish RC(%d) %s\n",Reason,MQReason(Reason));
    gets(Buffer);
  }
#endif

  return Reason;
}
