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
/*  FILE   : QLOADFNC.C                                               */
/*  PURPOSE: MQSeries Queue Unload/Reload Utility                     */
/**********************************************************************/
#ifdef MVS
  #pragma csect(code,"CMQUDFNC")
  #pragma csect(static,"CSTUDFNC")
  #pragma options(RENT)
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
#ifdef WIN32
  #include "windows.h"
#endif

#include "cmqc.h"

#include "qlcs.h"
#include "qlutil.h"
#include "qlvalues.h"
#include "qload.h"

#define FT_LONG             1
#define FT_CHAR             2
#define FT_BYTE             3

#define DP_HEXCHAR_CHAR     0x00000001
#define DP_HEXCHAR_BYTE     0x00000002

#define ESCAPE_CHAR    '~'

MQMD2 DefaultMqmd = { MQMD2_DEFAULT };                           /*@1C*/
extern char HEX[];

typedef struct _FLD
{
  int    Type;                         /* FT_*                        */
  int    Offset;
  int    Length;
  int    Version;
  char * Id;
  char * Name;
  int    Depend;
} FLD;

SIFT   AgeBuckets[MAX_AGE_BUCKETS] =
{{ "< 2  secs", 2},
 { "< 10 secs",10},
 { "< 1   min",60},
 { "< 1  hour",60*60},
 { "< 1   day",24*60*60},
 { "> 1   day",0}
 };

SIFT   SizeBuckets[MAX_SIZE_BUCKETS] =
{{ "< 100b", 100},
 { "<  1Kb", 1024},
 { "<  4Kb", 4 * 1024},
 { "<100Kb", 100 * 1024},
 { "<  1Mb", 1024 * 1024},
 { ">  1Mb", 0}
 };

static char * LitDay[] = {"Sun"      ,"Mon"     ,"Tue"    ,
                          "Wed"      ,"Thu"     ,"Fri"    ,"Sat"};

static char * BigDay[] = {"Sunday"   ,"Monday"  ,"Tuesday",
                          "Wednesday","Thursday","Friday" ,"Saturday"};

static char * LitMon[] = {"Jan"      ,"Feb"     ,"Mar"    ,
                          "Apr"      ,"May"     ,"Jun"    ,
                          "Jul"      ,"Aug"     ,"Sep"    ,
                          "Oct"      ,"Nov"     ,"Dec"};

static char * BigMon[] = {"January"  ,"February","March"  ,
                          "April"    ,"May"     ,"June"   ,
                          "July"     ,"August"  ,"September",
                          "October"  ,"November","December"};

FLD Flds[] = {
{ FT_LONG, CSOFFSETOF(MQMD,Version)          , 4, 2,"VER","Version"},
{ FT_LONG, CSOFFSETOF(MQMD,Report)           , 4, 0,"RPT","Report" },
{ FT_LONG, CSOFFSETOF(MQMD,MsgType)          , 4, 0,"MST","MsgType" },
{ FT_LONG, CSOFFSETOF(MQMD,Expiry)           , 4, 0,"EXP","Expiry" },
{ FT_LONG, CSOFFSETOF(MQMD,Feedback)         , 4, 0,"FDB","Feedback" },
{ FT_LONG, CSOFFSETOF(MQMD,Encoding)         , 4, 0,"ENC","Encoding" },
{ FT_LONG, CSOFFSETOF(MQMD,CodedCharSetId)   , 4, 0,"CCS","CodedCharSetId" },
{ FT_CHAR, CSOFFSETOF(MQMD,Format)           , 8, 0,"FMT","Format" },
{ FT_LONG, CSOFFSETOF(MQMD,Priority)         , 4, 0,"PRI","Priority"},
{ FT_LONG, CSOFFSETOF(MQMD,Persistence)      , 4, 0,"PER","Persistence"},
{ FT_BYTE, CSOFFSETOF(MQMD,MsgId)            ,24, 0,"MSI","MsgId"},
{ FT_BYTE, CSOFFSETOF(MQMD,CorrelId)         ,24, 0,"COI","CorrelId"},
{ FT_LONG, CSOFFSETOF(MQMD,BackoutCount)     , 4, 0,"BOC","BackoutCount"},
{ FT_CHAR, CSOFFSETOF(MQMD,ReplyToQ)         ,48, 0,"RTQ","ReplyToQ"},
{ FT_CHAR, CSOFFSETOF(MQMD,ReplyToQMgr)      ,48, 0,"RTM","ReplyToQMgr"},
{ FT_CHAR, CSOFFSETOF(MQMD,UserIdentifier)   ,12, 0,"USR","UserIdentifier"},
{ FT_BYTE, CSOFFSETOF(MQMD,AccountingToken)  ,32, 0,"ACC","AccountingToken"},
{ FT_CHAR, CSOFFSETOF(MQMD,ApplIdentityData) ,32, 0,"AID","ApplIdentityData", DP_HEXCHAR_CHAR},
{ FT_BYTE, CSOFFSETOF(MQMD,ApplIdentityData) ,32, 0,"AIX","ApplIdentityData", DP_HEXCHAR_BYTE},
{ FT_LONG, CSOFFSETOF(MQMD,PutApplType)      , 4, 0,"PAT","PutApplType"},
{ FT_CHAR, CSOFFSETOF(MQMD,PutApplName)      ,28, 0,"PAN","PutApplName"},
{ FT_CHAR, CSOFFSETOF(MQMD,PutDate)          , 8, 0,"PTD","PutDate"},
{ FT_CHAR, CSOFFSETOF(MQMD,PutTime)          , 8, 0,"PTT","PutTime"},
{ FT_CHAR, CSOFFSETOF(MQMD,ApplOriginData)   , 4, 0,"AOD","ApplOriginData", DP_HEXCHAR_CHAR},
{ FT_BYTE, CSOFFSETOF(MQMD,ApplOriginData)   , 4, 0,"AOX","ApplOriginData", DP_HEXCHAR_BYTE},

{ FT_CHAR, CSOFFSETOF(MQMD,StrucId)          , 4, 0,""   ,"StrucId"},
{ FT_BYTE, CSOFFSETOF(MQMD2,GroupId)         ,24, 2,"GRP","GroupId"},
{ FT_LONG, CSOFFSETOF(MQMD2,MsgSeqNumber)    , 4, 2,"MSQ","MsgSeqNumber"},
{ FT_LONG, CSOFFSETOF(MQMD2,Offset)          , 4, 2,"OFF","Offset"},
{ FT_LONG, CSOFFSETOF(MQMD2,MsgFlags)        , 4, 2,"MSF","MsgFlags"},
{ FT_LONG, CSOFFSETOF(MQMD2,OriginalLength)  , 4, 2,"ORL","OriginalLength"},
{ 0      , 0                                , 0, 0,""   ,""}
};                                                               /*@1C*/


static char FirstDate[8] = {0};
static char FirstTime[8] = {0};
static int  StartTime = 0;


static MQLONG ConvertHex(QLOPT         * pOpt,
                  unsigned char * p,
                  unsigned char * pOut,
                  size_t        * Length,
                  BOOL            Sparse);
static MQLONG GetMessageArea(QLOPT * pOpt, size_t MsgLen);

/**********************************************************************/
/* Function : GetFileName                                             */
/* Purpose  : Construct the file name                                 */
/**********************************************************************/
static MQLONG GetFileName(QLOPT * pOpt,
                          char  * fin,
                          char  * fout)
{
  char * pi = fin;
  char * po = fout;
  time_t Now;
  struct tm * pTm;

  time(&Now);
  pTm = localtime(&Now);

  while (*pi)
  {
    if (*pi == '%')
    {
      pi++;
      switch(*pi)
      {
        case   0:
             pi--;
             break;
        case '%':
             *po++ = '%';
             break;
        case 'i':
             sprintf(po,"%d",pOpt->InIndex);
             po+=strlen(po);
             break;
        case 'I':
             sprintf(po,"%5.5d",pOpt->InIndex);
             po+=strlen(po);
             break;
        case 'o':
             sprintf(po,"%d",pOpt->OutIndex+1);
             po+=strlen(po);
             break;
        case 'O':
             sprintf(po,"%5.5d",pOpt->OutIndex+1);
             po+=strlen(po);
             break;
        case 'n':
             if (pOpt->InFileIndex) sprintf(po,"%d",pOpt->InFileIndex);
                               else sprintf(po,"%d",pOpt->OutFileIndex+1);
             po += strlen(po);
             break;
        case 'N':
             if (pOpt->InFileIndex) sprintf(po,"%5.5d",pOpt->InFileIndex);
                               else sprintf(po,"%5.5d",pOpt->OutFileIndex+1);
             po += strlen(po);
             break;
        case 'p':
             if (pTm->tm_hour >= 12) memcpy(po,"pm",2);
                                else memcpy(po,"am",2);
             po += 2;
             break;

        case 'P':
             if (pTm->tm_hour >= 12) memcpy(po,"PM",2);
                                else memcpy(po,"AM",2);
             po += 2;
             break;

        case 'h':
             if (*(pi+1)=='h')
             {
               pi++;
               sprintf(po,"%d",pTm->tm_hour%12);
               po += strlen(po);
             }
             else
             {
               sprintf(po,"%2.2d",pTm->tm_hour%12);
               po += 2;
             }
             break;

        case 'H':
             if (*(pi+1)=='H')
             {
               pi++;
               sprintf(po,"%d",pTm->tm_hour);
               po += strlen(po);
             }
             else
             {
               sprintf(po,"%2.2d",pTm->tm_hour);
               po += 2;
             }
             break;

        case 'M':
             sprintf(po,"%2.2d",pTm->tm_min);
             po += 2;
             break;

        case 'S':
             sprintf(po,"%2.2d",pTm->tm_sec);
             po += 2;
             break;

        case 'd':
             if (*(pi+1)=='d')
             {
               pi++;
               sprintf(po,"%d",pTm->tm_mday);
               po += strlen(po);
               switch(pTm->tm_mday)
               {
                 case 1:
                 case 21:
                 case 31:
                         strcpy(po,"st");break;
                 case 2:
                 case 22:
                         strcpy(po,"nd");break;
                 case 3:
                 case 23:
                         strcpy(po,"rd");break;
                 default:strcpy(po,"th");break;
               }
               po += strlen(po);
             }
             else
             {
               sprintf(po,"%2.2d",pTm->tm_mday);
               po += 2;
             }
             break;

        case 'j':
                                       /* Zero based julian date      */
             sprintf(po,"%3.3d",pTm->tm_yday);
             po += strlen(po);
             break;

        case 'J':
                                       /* Make it one based           */
             sprintf(po,"%3.3d",pTm->tm_yday+1);
             po += strlen(po);
             break;

        case 'm':
             if (*(pi+1)=='m')
             {
               pi++;
               if (*(pi+1)=='m')
               {
                 pi++;
                 strcpy(po,BigMon[pTm->tm_mon]);
                 po += strlen(po);
               }
               else
               {
                 sprintf(po,"%2.2d",pTm->tm_mon+1);
                 po += 2;
               }
             }
             else
             {
               strcpy(po,LitMon[pTm->tm_mon]);
               po += 3;
             }
             break;

        case 'y':
             if (*(pi+1)=='y')
             {
               pi++;
               sprintf(po,"%2.2d",pTm->tm_year%100);
               po += 2;
             }
             else
             {
               sprintf(po,"%4.4d",pTm->tm_year + 1900);
               po += 4;
             }
             break;

        case 'D':
             if (*(pi+1)=='D')
             {
               pi++;
               strcpy(po,BigDay[pTm->tm_wday]);
               po += strlen(po);
             }
             else
             {
               strcpy(po,LitDay[pTm->tm_wday]);
               po += 3;
             }
             break;

        case 't':
             sprintf(po,"%2.2d%2.2d%2.2d",
                     pTm->tm_hour,pTm->tm_min,pTm->tm_sec);
             po += strlen(po);
             break;

        case 'c':
             sprintf(po,"%2.2d%2.2d%2.2d",
                     pTm->tm_year%100,pTm->tm_mon+1,pTm->tm_mday);
             po += strlen(po);
             break;
        case 'C':
             sprintf(po,"%4.4d%2.2d%2.2d",
                     pTm->tm_year+1900,pTm->tm_mon+1,pTm->tm_mday);
             po += strlen(po);
             break;

        default:  break;
      }
      pi++;
    }
    else
    {
      *po++ = *pi++;
    }
  }
  *po = 0;
  return 0;
}
/**********************************************************************/
/* Function : QloadOpenFiles                                          */
/* Purpose  : Open the files for the loading/unloading                */
/**********************************************************************/
MQLONG QloadOpenFiles(QLOPT * pOpt,
                      BOOL    Prompt,
                      BOOL    Input,
                      BOOL    Output)
{
  char Buffer[100];
  char FileName[MAX_FILE_NAME];
  MQLONG Reason = 0L;
  char * pErr = pOpt->ErrorMessage;

  *pErr = 0;

  if (Input && pOpt->InFileName[0] && !pOpt->hInFile)
  {
    pOpt->InFileIndex ++;
    GetFileName(pOpt,pOpt->InFileName,FileName);
    /* printf("File %d %s\n",pOpt->InFileIndex,FileName); */
    if (!strcmp(pOpt->InputFileName,FileName))
    {
      sprintf(pErr,"Duplicate input file '%s'\n",FileName);
      Reason = MQRC_FILE_SYSTEM_ERROR;
      goto MOD_EXIT;
    }
    strcpy(pOpt->InputFileName,FileName);
                                       /* We're loading               */
    if (pOpt->AccessMode)
      pOpt->hInFile = fopen(FileName,pOpt->AccessMode);
    else
      pOpt->hInFile = fopen(FileName,"r");
    if (!pOpt->hInFile)
    {
      pOpt->InFileIndex --;
                                       /* Only an error for the first file */
      if (pOpt->InFileIndex == 0)
      {
        if (pOpt->AccessMode)
          sprintf(pErr,"Can not open input file '%s'(%s)\n",FileName,pOpt->AccessMode);
        else
          sprintf(pErr,"Can not open input file '%s'\n",FileName);
      }
      Reason = MQRC_FILE_SYSTEM_ERROR;
      goto MOD_EXIT;
    }
  }
  if (Output && pOpt->OutFileName[0] && !pOpt->hOutFile)
  {
    GetFileName(pOpt,pOpt->OutFileName,FileName);
    memcpy(pOpt->ActOutFileName,FileName,sizeof(pOpt->ActOutFileName));

    if (strcmp(FileName,"stdout"))
    {
      if (!(pOpt->Options & qoFORCE_FILE))
      {
        pOpt->hOutFile = fopen(FileName,"r");
        if (pOpt->hOutFile)
        {
          fclose(pOpt->hOutFile);
          if (Prompt)
          {
            Buffer[0] = 0;
            while(Buffer[0] != 'y')
            {
              fprintf(stderr,"File '%s' exists - overwrite (y)es/(n)o/(a)ll ?\n",FileName);
              fgets(Buffer,sizeof(Buffer),stdin);
              switch(Buffer[0])
              {
                case 'a':
                     pOpt->Options |= qoFORCE_FILE;
                     Buffer[0]='y';
                     break;
                case 'y':
                     break;
                case 'n':
                     sprintf(pErr,"File '%s' already exists.\n",FileName);
                     Reason = MQRC_FILE_SYSTEM_ERROR;
                     goto MOD_EXIT;

                default:
                     break;
              }
            }
          }
          else
          {
            sprintf(pErr,"File '%s' already exists.\n",FileName);
            Reason = MQRC_FILE_SYSTEM_ERROR;
            goto MOD_EXIT;
          }
        }
      }

      if (pOpt->AccessMode)
        pOpt->hOutFile = fopen(FileName,pOpt->AccessMode);
      else
        pOpt->hOutFile = fopen(FileName,"w");
      if (!pOpt->hOutFile)
      {
        if (pOpt->AccessMode)
          sprintf(pErr,"Can not open output file '%s'(%s)\n",FileName,pOpt->AccessMode);
        else
          sprintf(pErr,"Can not open output file '%s'\n",FileName);
        Reason = MQRC_FILE_SYSTEM_ERROR;
        goto MOD_EXIT;
      }
      pOpt->OutFileIndex ++;
    }
    else
    {
      pOpt->hOutFile = stdout;
    }
  }
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : GetMessageArea                                          */
/* Purpose  : Get a bigger area for the message                       */
/**********************************************************************/
static MQLONG GetMessageArea(QLOPT * pOpt, size_t MsgLen)
{
  MQLONG Reason;
  int    OldSize = pOpt -> MsgSize;
  char * pNewMsg;
                                       /* Round to 4K                 */
  pOpt -> MsgSize = (MsgLen + 4095) & 0xFFFFF000;
                                       /* Ensure we haven't wrapped   */
  if (MsgLen <= 0) MsgLen = 0x7fffffff;

  if (pOpt->Verbose & vbDEBUG_MEMORY)
  {
    fprintf(stderr,"Allocating %d bytes\n",pOpt->MsgSize);
  }

  pNewMsg = malloc(pOpt->MsgSize);
  if (pNewMsg)
  {
    Reason = 0;
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Allocation success %p\n",pNewMsg);
    }
  }
  else
  {
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Allocation failed!\n");
    }
    Reason = MQRC_STORAGE_NOT_AVAILABLE;
    sprintf(pOpt->ErrorMessage,"Unable to allocate %d bytes message storage",
            pOpt->MsgSize);
    goto MOD_EXIT;
  }
  if (pOpt->pMsg)
  {
    memcpy(pNewMsg,pOpt->pMsg,OldSize);
    free(pOpt->pMsg);
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Freeing %p\n",pOpt->pMsg);
    }
  }
  pOpt->pMsg = pNewMsg;

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : GetLineArea                                             */
/* Purpose  : Get a bigger area for the line                          */
/**********************************************************************/
static MQLONG GetLineArea(QLOPT * pOpt, MQLONG LineLen)
{
  MQLONG Reason;

  if (pOpt->pLine)
  {
    free(pOpt->pLine);
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Freeing %p\n",pOpt->pLine);
    }
  }
                                       /* Round to 1K                 */
  pOpt -> LineSize = (LineLen + 1023) & 0xFFFFFC00;

  if (pOpt->Verbose & vbDEBUG_MEMORY)
  {
    fprintf(stderr,"Allocating %d bytes\n",pOpt->LineSize);
  }

  pOpt -> pLine = malloc(pOpt->LineSize);
  if (pOpt -> pLine)
  {
    Reason = 0;
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Allocation success %p\n",pOpt->pLine);
    }

  }
  else
  {
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Allocation failed\n");
    }

    Reason = MQRC_STORAGE_NOT_AVAILABLE;
    sprintf(pOpt->ErrorMessage,"Unable to allocate %d bytes line storage",
            pOpt->LineSize);
  }
  return Reason;
}
/**********************************************************************/
/* Function : ReadMQMessage                                           */
/* Purpose  : Read the next message from an MQ Queue                  */
/**********************************************************************/
static MQLONG ReadMQMessage(QLOPT * pOpt,BOOL * Complete,BOOL ForceGet)
{
  MQLONG CompCode,Reason;
  int    rc;
  MQGMO  gmo       = { MQGMO_DEFAULT };
  MQLONG messageLength = 0;

  gmo.Options  = MQGMO_NO_WAIT;

  if (ForceGet)
  {
    /******************************************************************/
    /* Ok, this is a forced get which means get the message under     */
    /* browse cursor                                                  */
    /******************************************************************/
    gmo.Options |= MQGMO_MSG_UNDER_CURSOR;
    /******************************************************************/
    /* We want to use a transaction if this is a move and we're       */
    /* allowed to use transactions.                                   */
    /******************************************************************/
    if (pOpt->TransactionLimit) gmo.Options |= MQGMO_SYNCPOINT;
                           else gmo.Options |= MQGMO_NO_SYNCPOINT;
  }
  else if (pOpt -> Options & qoFILTER)
  {
    /******************************************************************/
    /* If we're filtering then this is really a browse                */
    /******************************************************************/
    gmo.Options |= MQGMO_BROWSE_NEXT | MQGMO_NO_SYNCPOINT;
  }
  else if (pOpt -> Options & qoMOVE)
  {
    /******************************************************************/
    /* We want to use a transaction if this is a move and we're       */
    /* allowed to use transactions.                                   */
    /******************************************************************/
    if (pOpt->TransactionLimit) gmo.Options |= MQGMO_SYNCPOINT;
                           else gmo.Options |= MQGMO_NO_SYNCPOINT;
  }
  else
  {
    /******************************************************************/
    /* Ok, must be a normal non-filtered browse                       */
    /******************************************************************/
    gmo.Options |= MQGMO_BROWSE_NEXT | MQGMO_NO_SYNCPOINT;
  }

  while (1)
  {
    memcpy(&pOpt->mqmd, &DefaultMqmd, sizeof(MQMD));

    /******************************************************************/
    /* Copy the ids                                                   */
    /******************************************************************/
    if (pOpt->Options & qoMATCH_MSGID)
    {
      memcpy(pOpt->mqmd.MsgId   , pOpt->MsgId   , 24);
    }
    if (pOpt->Options & qoMATCH_CORRELID)
    {
      memcpy(pOpt->mqmd.CorrelId, pOpt->CorrelId, 24);
    }
    if (pOpt->Options & qoMATCH_GROUPID)
    {
      memcpy(pOpt->mqmd.GroupId, pOpt->GroupId, 24);
      gmo.MatchOptions |= MQMO_MATCH_GROUP_ID;
      if (gmo.Version < 2) gmo.Version = 2;
    }

    if (pOpt->Options & qoCONVERT)
    {
      gmo.Options |= MQGMO_CONVERT;
      pOpt -> mqmd.CodedCharSetId = pOpt -> RequiredCCSID;
      pOpt -> mqmd.Encoding       = pOpt -> RequiredEncoding;
    }

    MQGET( pOpt->hQm,
           pOpt->hInput,
          &pOpt->mqmd,
          &gmo,
           pOpt->MsgSize,
           pOpt->pMsg,
          &messageLength,
          &CompCode,
          &Reason );
    pOpt->MsgLen = (size_t) messageLength;
    switch(Reason)
    {
      case 0:
      case MQRC_NOT_CONVERTED:
           Reason = 0;
           goto MOD_EXIT;

      case MQRC_NO_MSG_AVAILABLE:
           /***********************************************************/
           /* Should we be waiting ?                                  */
           /***********************************************************/
           if (pOpt -> WaitInterval)
           {
             if (! (gmo.Options & MQGMO_WAIT))
             {
               /*******************************************************/
               /* Ok, well we weren't - go round again but wait       */
               /*******************************************************/
               gmo.Options     |= MQGMO_WAIT;
               gmo.WaitInterval = pOpt->WaitInterval;

               /*******************************************************/
               /* If we're writing to a file then flush it            */
               /*******************************************************/
               if (pOpt->hOutFile)
               {
                 rc = fflush(pOpt->hOutFile);
                 if (rc)
                 {
                   Reason = MQRC_RESOURCE_PROBLEM;
                   sprintf(pOpt->ErrorMessage,"Error flushing data to file '%s'RC(%d)",
                           pOpt->ActOutFileName,rc);
                   goto MOD_EXIT;
                 }
               }
               /*******************************************************/
               /* If we have outstanding transaction stuff commit it  */
               /*******************************************************/
               if (pOpt->TransactionSize  &&
                  (pOpt->TransactionLimit != -1))
               {
                 MQCMIT(pOpt->hQm,&CompCode,&Reason);
                 if (CompCode == MQCC_FAILED)
                 {
                   sprintf(pOpt->ErrorMessage,"MQCMIT failed RC(%d) %s",
                           Reason,MQReason(Reason));
                   goto MOD_EXIT;
                 }
                 pOpt->TransactionSize = 0;
               }
               continue;
             }
           }
           /***********************************************************/
           /* We ran out of messages.....end                          */
           /***********************************************************/
           Reason = 0;
           *Complete = TRUE;
           goto MOD_EXIT;

      case MQRC_TRUNCATED_MSG_FAILED:
           Reason = GetMessageArea(pOpt,pOpt->MsgLen);
           if (Reason) goto MOD_EXIT;
           break;

      default:
           sprintf(pOpt->ErrorMessage,"MQGET(%s) failed RC(%d) %s",
                   pOpt->InputQueue,Reason,MQReason(Reason));
           goto MOD_EXIT;
    }
  }

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : ReadLine                                                */
/* Purpose  : Read a line from the file                               */
/**********************************************************************/
static MQLONG ReadLine(QLOPT * pOpt,BOOL force,BOOL * eof,BOOL all)
{
  MQLONG Reason     = 0;
  int    CurrentPos,NewPos,Read;
  char * pEnd;

  pOpt -> pLex = NULL;
                                       /* We may have already read it!*/
  if (!force && pOpt->LineLength) goto MOD_EXIT;

  while (1)
  {
    CurrentPos = ftell(pOpt->hInFile);
    if (! fgets(pOpt->pLine, pOpt->LineSize, pOpt->hInFile))
    {
      *eof = TRUE;
      pOpt->LineLength  = 0;
      pOpt->pLine[0]    = 0;
      goto MOD_EXIT;
    }
    NewPos = ftell(pOpt->hInFile);
    Read   = NewPos - CurrentPos;
                                       /* Read up to newline          */
    pEnd   = memchr(pOpt->pLine,'\n',Read);

    /******************************************************************/
    /* If we don't find a newline then if we read less than we gave   */
    /* space for the line must end at the last character              */
    /******************************************************************/
    if (!pEnd)
    {
      if (Read < pOpt->LineSize) pEnd = &pOpt->pLine[Read];
    }
                                       /* Did we read whole line ?    */
    if (pEnd)
    {
      pOpt -> LineLength = pEnd - pOpt->pLine + 1;
      pOpt -> LineNo++;
      if (all) break;
                                       /* Ignore empty lines          */
      if (!pOpt -> LineLength) continue;

      if (pOpt -> pLine[pOpt->LineLength-1] == '\n')
      {
        pOpt -> pLine[pOpt->LineLength-1] = 0;
        pOpt->LineLength--;
                                       /* Ignore empty lines (again)  */
        if (!pOpt -> LineLength) continue;
      }
                                       /* Ignore comments             */
      if (pOpt -> pLine[0] == '*')
      {
        if (!(pOpt -> Options & qoAMQSBCG_INPUT)) continue;
      }
                                       /* Ok, we have something       */
      pOpt -> pLineEnd = pOpt -> pLine + pOpt->LineLength - 1;
      break;
    }
                                       /* We need a bigger buffer     */
    Reason = GetLineArea(pOpt,pOpt->LineSize * 2);
    if (Reason) goto MOD_EXIT;
                                 /* Now go back                 */
    fseek(pOpt->hInFile, CurrentPos, SEEK_SET );
  }
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : ConvertHex                                              */
/* Purpose  : Convert HEX string to ASCII                             */
/**********************************************************************/
static MQLONG ConvertHex(QLOPT         * pOpt,
                  unsigned char * p,
                  unsigned char * pOut,
                  size_t        * Length,
                  BOOL            Sparse)
{
  MQLONG   Reason = 0;
  unsigned char ch;
  unsigned char val;
  size_t   maxlen  = *Length;
  size_t   donelen = 0;

  if (maxlen < 0) maxlen = 99999999;

  while (*p && maxlen)
  {
    if (Sparse) while (*p == ' ') p++;

    ch = (unsigned char) *p++;
         if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
                                       /* Oops                        */
    else break;
    val       = ch * 16;
    ch        = (unsigned char) *p++;
         if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
                                       /* Oops                        */
    else
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Badly formed HEX string on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }
    val += ch;

    donelen++;
    maxlen--;
    *pOut++ = val;
  }
  *Length = donelen;

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : SetFileAttribute                                        */
/* Purpose  : Set the attribute in the MQMD from the file line        */
/**********************************************************************/
static MQLONG SetFileAttribute(QLOPT * pOpt)
{
  MQLONG          Reason = 0;
  FLD           * pFld = Flds;
  unsigned char * pAttr;
  unsigned char * pField;
  size_t          Length;

  /********************************************************************/
  /* Is the line long enough ?                                        */
  /********************************************************************/
  if (pOpt->LineLength < 5)
  {
    Reason = MQRC_FILE_SYSTEM_ERROR;
    sprintf(pOpt->ErrorMessage,"Invalid attribute line on line %d of file '%s'",
            pOpt->LineNo,pOpt->InputFileName);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Scan the known attributes                                        */
  /********************************************************************/
  pAttr    = (unsigned char *)&pOpt->pLine[2];
  pAttr[3] = 0;
  while(1)
  {
    if (!pFld->Type)
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Unrecognised attribute on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }
#ifdef WIN32
    if (SAME4(pAttr,pFld->Id)) break;
#else
    if (!memcmp(pAttr,pFld->Id,4)) break;
#endif

    pFld++;
  }
                                       /* Doesn't have a value        */
  if (pOpt->LineLength < 7) goto MOD_EXIT;

  pAttr += 4;
  pField = ((unsigned char *)&pOpt->mqmd)+pFld->Offset;
  /********************************************************************/
  /* Ok, we must have found it                                        */
  /********************************************************************/
  switch(pFld->Type)
  {
    case FT_LONG:
         *(MQLONG *)pField = atoi((char *)pAttr);
         break;

    case FT_BYTE:
         Length = min(pFld->Length,(int)strlen((char *)pAttr)/2);
         Reason = ConvertHex(pOpt,pAttr,pField,&Length,FALSE);
         if (Reason) goto MOD_EXIT;
         break;

    case FT_CHAR:
         Length = min(pFld->Length,(int)strlen((char *)pAttr));
         memcpy(pField,pAttr,Length);
         if (Length < pFld->Length)
         {
           memset(&pField[Length],' ',pFld->Length-Length);
         }
         break;

    default:
         Reason = MQRC_UNEXPECTED_ERROR;
         sprintf(pOpt->ErrorMessage,"Internal Eror:Unrecognised attribute type %d on line %d of file '%s'",
                 pFld->Type,pOpt->LineNo,pOpt->InputFileName);
         goto MOD_EXIT;
  }

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : ReadMessageLine                                         */
/* Purpose  : Read a string message line                              */
/**********************************************************************/
static MQLONG ReadMessageLine(QLOPT         * pOpt,
                              unsigned char * p,
                              unsigned char * pOut,
                              size_t        * pLength)
{
  MQLONG Reason = 0L;
  size_t  Length;
  char * pStart = (char *)pOut;
  char * pEnd   = pOpt->pLineEnd;
                                       /* Skip blanks                 */
  while (*p == ' ') p++;

  if (*p == '\"')
  {
    p++;
                                       /* Formatted string            */
    while ((char *)p <= pEnd)
    {
      switch(*p)
      {
        case '\"':
                                       /* We're done                  */
             Length = (char *)pOut - pStart;
             goto MOD_EXIT;

        case ESCAPE_CHAR:
             p++;
             switch(*p)
             {
               case '\"':
               case ESCAPE_CHAR:
                    *pOut++ = *p++;
                    break;
               default:
                    Reason = MQRC_FILE_SYSTEM_ERROR;
                    sprintf(pOpt->ErrorMessage,"Unexpected escape sequence on line %d of file '%s'",
                            pOpt->LineNo,pOpt->InputFileName);
                    goto MOD_EXIT;
             }
             break;
        default:
             *pOut++ = *p++;
             break;
      }
    }
    Reason = MQRC_FILE_SYSTEM_ERROR;
    sprintf(pOpt->ErrorMessage,"Invalid string line - no terminating \" symbol on line %d of file '%s'",
            pOpt->LineNo,pOpt->InputFileName);
    goto MOD_EXIT;

  }
  else
  {
                                       /* String literal              */
                                       /* Remove trailing spaces      */
    while (*pEnd == ' ') pEnd--;
    Length = pEnd - (char *)p;
    if (Length < 0) Length = 0;
    memcpy(pOut,p,Length);
  }

MOD_EXIT:
  *pLength = Length;
  return Reason;
}
/**********************************************************************/
/* Function : ReadMessageTextLine                                     */
/* Purpose  : Read a message text line                                */
/**********************************************************************/
static MQLONG ReadMessageTextLine(QLOPT         * pOpt,
                                  unsigned char * p,
                                  unsigned char * pOut,
                                  size_t        * pLength)
{
  MQLONG  Reason = 0L;
  char  * pStart = (char *)p;
  size_t     Length = pOpt->LineLength-2;

  if (Length > 0)
  {
    memcpy(pOut,p, Length);
    pOut += Length;
  }
  *pOut++ = '\n';
  Length = Length + 1;
  *pLength = Length;
  return Reason;
}
/**********************************************************************/
/* Function : ReadFileMessage                                         */
/* Purpose  : Read the next message from the file                     */
/**********************************************************************/
static MQLONG ReadFileMessage(QLOPT * pOpt,BOOL * Complete)
{
  MQLONG   Reason = 0;
  BOOL     eof = FALSE;
  char   * p;
  char   * pMsg;
  size_t   Length;

  memcpy(&pOpt->mqmd,&DefaultMqmd,sizeof(MQMD));
  pOpt -> MsgLen = 0;
  /********************************************************************/
  /* First read the message attributes                                */
  /********************************************************************/
  Reason = ReadLine(pOpt,FALSE,&eof,FALSE);
  if (Reason) goto MOD_EXIT;
  if (eof)
  {
    *Complete = TRUE;
    goto MOD_EXIT;
  }

  if (pOpt->pLine[0] != 'A' && pOpt->pLine[0] != 'N')
  {
    Reason = MQRC_FILE_SYSTEM_ERROR;
    sprintf(pOpt->ErrorMessage,"Expected message attribute at line %d of file '%s'",
            pOpt->LineNo,pOpt->InputFileName);
    goto MOD_EXIT;
  }

  if (pOpt->pLine[0] == 'N')
  {
    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason) goto MOD_EXIT;
  }

  while (pOpt->pLine[0] == 'A')
  {

    Reason = SetFileAttribute(pOpt);
    if (Reason) goto MOD_EXIT;

    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason) goto MOD_EXIT;
  }

  if (pOpt->pLine[0] != 'X' &&
      pOpt->pLine[0] != 'S' &&
      pOpt->pLine[0] != 'T')
  {
    Reason = MQRC_FILE_SYSTEM_ERROR;
    sprintf(pOpt->ErrorMessage,"Expected message at line %d of file '%s'",
            pOpt->LineNo,pOpt->InputFileName);
    goto MOD_EXIT;
  }

  while (pOpt->pLine[0] == 'X' ||
         pOpt->pLine[0] == 'S' ||
         pOpt->pLine[0] == 'T')
  {
                                       /* Check we have a big enough  */
                                       /* message area                */
    if (pOpt->MsgLen + pOpt->LineLength >= pOpt -> MsgSize)
    {
      /****************************************************************/
      /* We don't know how much memory we are going to need in total  */
      /* so we need to guess.                                         */
      /****************************************************************/
      size_t needed = pOpt->MsgLen + pOpt->LineLength;

      needed = needed * 4;
      if (needed <= 0) needed = 0x7fffffff;
      Reason = GetMessageArea(pOpt,needed);
      if (Reason) goto MOD_EXIT;
    }
    p = &pOpt->pLine[1];
    if (pOpt->pLine[0] == 'T')
    {
                                       /* Skip past the space         */
      p++;
    }
    else
    {
                                       /* Skip past spaces            */
      while (*p == ' ') p++;
    }
                                       /* Point to end of message     */
    pMsg = pOpt->pMsg + pOpt->MsgLen;

    switch(pOpt->pLine[0])
    {
      case 'X':
           /***********************************************************/
           /* Convert hex into the message                            */
           /***********************************************************/
           Length = -1;
           Reason = ConvertHex(pOpt,(unsigned char *)p,(unsigned char *)pMsg,&Length,FALSE);
           if (Reason) goto MOD_EXIT;
           pOpt -> MsgLen += Length;
           break;
      case 'S':
           Reason = ReadMessageLine(pOpt,(unsigned char *)p,(unsigned char *)pMsg,&Length);
           if (Reason) goto MOD_EXIT;
           pOpt -> MsgLen += Length;
           break;
      case 'T':
           Reason = ReadMessageTextLine(pOpt,(unsigned char *)p,(unsigned char *)pMsg,&Length);
           if (Reason) goto MOD_EXIT;
           pOpt -> MsgLen += Length;
           break;
    }
    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason) goto MOD_EXIT;
  }

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : getlex                                                  */
/* Purpose  : Get the next lexeme from the input stream               */
/**********************************************************************/
static MQLONG getlex(QLOPT * pOpt,BOOL * Complete,BOOL * Section)
{
  MQLONG  Reason = 0;
  BOOL    spaces;
  char  * pLex = pOpt -> pLex;
  char  * pLexEnd;

  if (!pLex)
  {
    pLex = pOpt -> pLine;
  }
  else
  {
    pLex = pOpt -> pLexEnd + 1;
  }


  while (1)
  {
                                       /* Skip whitespace             */
    spaces = TRUE;
    while (spaces)
    {
      switch(*pLex)
      {
        case ' ':
        case ':':
        case '\n':
        case '\r':
        case '\t':
             pLex ++;
             break;
        case '*':
                                        /* It's a comment line        */
             *pLex=0;
             spaces = FALSE;
             break;
        default:
             spaces = FALSE;
             break;
      }
    }
    if (!*pLex)
    {
                                       /* Read the next line          */
      Reason = ReadLine(pOpt,TRUE,Complete,FALSE);
      if (Reason)    goto MOD_EXIT;
      if (*Complete) goto MOD_EXIT;
      pLex = pOpt -> pLine;

      if (!memcmp(pLex,"****",4))
      {
        *Section = TRUE;
        goto MOD_EXIT;
      }

      continue;
    }
    break;
  }

  if (*pLex=='\'')
  {
    pLex++;
    pLexEnd = pLex+1;
                                       /* This is a string            */
    while (*pLexEnd && *pLexEnd != '\'') pLexEnd++;
    if (!pLexEnd)
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Unterminated string on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }
    *pLexEnd = 0;
  }
  else
  {
    pLexEnd = pLex+1;
    spaces = FALSE;
    while (!spaces)
    {
      switch(*pLexEnd)
      {
        case 0:
             *(pLexEnd+1)=0;
                                       /* DELIBERATELY NO BREAK HERE  */
        case ' ':
        case ':':
        case '\n':
        case '\r':
        case '\t':
             *pLexEnd = 0;
             spaces   = TRUE;
             break;
        default:
             pLexEnd++;
             break;
      }
    }
  }

  pOpt -> pLex    = pLex;
  pOpt -> pLexEnd = pLex += strlen(pLex);
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : ReadBCGMessage                                          */
/* Purpose  : Read the next message from an AMQSBCG output file       */
/**********************************************************************/
MQLONG ReadBCGMessage(QLOPT * pOpt,BOOL * Complete)
{
  MQLONG   Reason = 0;
  BOOL     eof = FALSE;
  BOOL     Found = FALSE;
  BOOL     Section;
  BOOL     valid;
  FLD    * pFld;
  char   * pField,* pAttr,*pEnd;
  size_t   Length;
  char   * pMsg;

  #define BCG_MQMD_START "****Message descriptor****"

  memcpy(&pOpt->mqmd,&DefaultMqmd,sizeof(MQMD));
  pOpt -> MsgLen = 0;
  /********************************************************************/
  /* Find a line which starts "***Message descriptor"                 */
  /********************************************************************/
  while(1)
  {
    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason) goto MOD_EXIT;
    if (eof)
    {
      *Complete = TRUE;
      goto MOD_EXIT;
    }
    if (Found) break;

    if (!memcmp(pOpt->pLine,BCG_MQMD_START,strlen(BCG_MQMD_START)))
    {
      Found = TRUE;
    }
  }
  /********************************************************************/
  /* Now, read through each lexeme                                    */
  /********************************************************************/
  Section = FALSE;
  while (1)
  {
    Reason = getlex(pOpt,Complete,&Section);
    if (Reason)    goto MOD_EXIT;
    if (*Complete)
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Unexpected end of file on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }
    if (Section) break;
    /******************************************************************/
    /* Ok, we should have an attribute now                            */
    /******************************************************************/
    pFld = Flds;
    while(1)
    {
      if (!strcmp(pFld->Name,pOpt->pLex)) break;
      if (!pFld->Type)
      {
                                       /* Reached the end of the list */
        Reason = MQRC_FILE_SYSTEM_ERROR;
        sprintf(pOpt->ErrorMessage,"Unrecognised attribute '%s' on line %d of file '%s'",
                pOpt->pLex,pOpt->LineNo,pOpt->InputFileName);
        goto MOD_EXIT;
      }
      pFld++;
    }
    /******************************************************************/
    /* Now, get the attribute value                                   */
    /******************************************************************/
    Reason = getlex(pOpt,Complete,&Section);
    if (Reason)    goto MOD_EXIT;
    if (*Complete)
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Unexpected end of file on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }
    if (Section)
    {
      Reason = MQRC_FILE_SYSTEM_ERROR;
      sprintf(pOpt->ErrorMessage,"Unexpected end of section on line %d of file '%s'",
              pOpt->LineNo,pOpt->InputFileName);
      goto MOD_EXIT;
    }

    /******************************************************************/
    /* Ok, now deal with the attribute                                */
    /******************************************************************/
    pField = ((char *)&pOpt->mqmd)+pFld->Offset;
    pAttr  = pOpt->pLex;
    switch(pFld->Type)
    {
      case FT_LONG:
           *(MQLONG *)pField = atoi(pAttr);
           break;

      case FT_BYTE:
           valid = TRUE;
           if (*pAttr++ != 'X')  valid = FALSE;
           if (*pAttr++ != '\'') valid = FALSE;
           pEnd = strchr(pAttr,'\'');
           if (!pEnd || !valid)
           {
             Reason = MQRC_FILE_SYSTEM_ERROR;
             sprintf(pOpt->ErrorMessage,"Badly formed hex string on line %d of file '%s'",
                     pOpt->LineNo,pOpt->InputFileName);
             goto MOD_EXIT;
           }
           *pEnd = 0;
           Length = min(pFld->Length,(int)strlen(pAttr)/2);
           Reason = ConvertHex(pOpt,(unsigned char *)pAttr,(unsigned char *)pField,&Length,FALSE);
           if (Reason) goto MOD_EXIT;
           break;

      case FT_CHAR:
           Length = min(pFld->Length,(int)strlen(pAttr));
           memcpy(pField,pAttr,Length);
           if (Length < pFld->Length)
           {
             memset(&pField[Length],' ',pFld->Length-Length);
           }
           break;

      default:
           Reason = MQRC_UNEXPECTED_ERROR;
           sprintf(pOpt->ErrorMessage,"Internal Eror:Unrecognised attribute type %d on line %d of file '%s'",
                   pFld->Type,pOpt->LineNo,pOpt->InputFileName);
           goto MOD_EXIT;
    }
  }
  /********************************************************************/
  /* That's the end of the MQMD, do we really have a version 1        */
  /********************************************************************/
  Length = sizeof(MQMD) - CSOFFSETOF(MQMD2,GroupId);

  if (!memcmp(&pOpt->mqmd.GroupId,&DefaultMqmd.GroupId,Length))
  {
    pOpt->mqmd.Version = 1;
  }

  /********************************************************************/
  /* Ok now, we need to read the message                              */
  /********************************************************************/
  pOpt -> MsgLen = 0;

  while (1)
  {
    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason) goto MOD_EXIT;
    if (eof)    break;
                                       /* End of message ?            */
    if (!memcmp(pOpt -> pLine," MQ",3)) break;
                                       /* Does it contain data ?      */
    pAttr = strchr(pOpt->pLine,':');
    if (!pAttr) continue;
    pAttr++;
                                       /* Check we have a big enough  */
                                       /* message area                */
    if (pOpt->MsgLen + pOpt->LineLength >= pOpt -> MsgSize)
    {
      Reason = GetMessageArea(pOpt,pOpt->MsgLen + pOpt->LineLength);
      if (Reason) goto MOD_EXIT;
    }
                                       /* Point to end of message     */
    pMsg = pOpt->pMsg + pOpt->MsgLen;

    Length = -1;
    Reason = ConvertHex(pOpt,(unsigned char *)pAttr,(unsigned char *)pMsg,&Length,TRUE);
    if (Reason) goto MOD_EXIT;
    pOpt -> MsgLen += Length;
  }

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : WriteMQMessage                                          */
/* Purpose  : Write the message to a queue                            */
/**********************************************************************/
MQLONG WriteMQMessage(QLOPT * pOpt)
{
  MQLONG CompCode,Reason;
  MQPMO  pmo = { MQPMO_DEFAULT };

  /********************************************************************/
  /* We want to do this in a transaction if we're allowed to          */
  /********************************************************************/
  if (pOpt->TransactionLimit) pmo.Options = MQPMO_SYNCPOINT;
                         else pmo.Options = MQPMO_NO_SYNCPOINT;

  switch(pOpt->Options & qoCONTEXT_MASK)
  {
    case qoSET_ALL_CONTEXT:        pmo.Options |= MQPMO_SET_ALL_CONTEXT;      break;
    case qoSET_IDENTITY_CONTEXT:   pmo.Options |= MQPMO_SET_IDENTITY_CONTEXT; break;
    case qoPASS_ALL_CONTEXT:       pmo.Options |= MQPMO_PASS_ALL_CONTEXT;     break;
    case qoPASS_IDENTITY_CONTEXT:  pmo.Options |= MQPMO_PASS_IDENTITY_CONTEXT;break;
    case qoNO_CONTEXT:             pmo.Options |= MQPMO_NO_CONTEXT;           break;
    case qoDEFAULT_CONTEXT:        break;
 }

  pmo.Context = pOpt -> hInput;

  MQPUT( pOpt->hQm,
          pOpt->hOutput,
         &pOpt->mqmd,
         &pmo,
          (MQLONG)pOpt->MsgLen,
          pOpt->pMsgStart,
         &CompCode,
         &Reason );

  if (CompCode == MQCC_FAILED)
  {
    sprintf(pOpt->ErrorMessage,"MQPUT(%s) failed RC(%d) %s",
            pOpt->OutputQueue,Reason,MQReason(Reason));
  }
  else
  {
    Reason = 0;
  }

  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMQMD                                           */
/* Purpose  : Write the various MQMD attributes                       */
/**********************************************************************/
MQLONG WriteFileMQMD(QLOPT * pOpt)
{
  MQLONG          Reason = MQRC_RESOURCE_PROBLEM;
  FLD           * pFld = Flds;
  char          * pMqmd = (char *)&pOpt->mqmd;
  unsigned char * pValue;
  int             i,n;
  BOOL            writeit;

  if (pOpt->Options2 & qo2NO_WRITE_MQMD)
  {
                                       /* Add a 'no-MQMD' line        */
    n = fprintf(pOpt->hOutFile,"N\n");
    if (n < 0) goto MOD_EXIT;
    Reason = 0;
    goto MOD_EXIT;
  }

  while (pFld -> Type)
  {
    writeit = pFld->Id[0] != 0;
    if (writeit && pFld->Version)
    {
      writeit = pOpt->mqmd.Version >= pFld->Version;
    }

    if (writeit && pFld->Depend)
    {
      if (pFld->Depend & DP_HEXCHAR_CHAR)
        writeit = pOpt -> Options & qoHEXCHAR_CHAR;

      if (pFld->Depend & DP_HEXCHAR_BYTE)
        writeit = ! (pOpt -> Options & qoHEXCHAR_CHAR);

    }

    if (writeit)
    {
      pValue = (unsigned char *)pMqmd + pFld -> Offset;

      switch (pFld -> Type)
      {
        case FT_LONG:
             n = fprintf(pOpt->hOutFile,"A %s %d\n",
                         pFld->Id,*(MQLONG *)pValue);
             if (n<=0) goto MOD_EXIT;
             break;

        case FT_CHAR:
             i = pFld->Length;
             n = fprintf(pOpt->hOutFile,"A %s ",
                         pFld->Id);
             if (n<=0) goto MOD_EXIT;
             while(i-- && *pValue) fputc(*pValue++,pOpt->hOutFile);
             fputc('\n',pOpt->hOutFile);
             break;

        case FT_BYTE:
             i = pFld->Length;
             n = fprintf(pOpt->hOutFile,"A %s ",
                         pFld->Id);
             if (n<=0) goto MOD_EXIT;
             while(i--)
             {
               fputc(HEX[*pValue/16],pOpt->hOutFile);
               fputc(HEX[*pValue%16],pOpt->hOutFile);
               pValue++;
             }
             fputc('\n',pOpt->hOutFile);
             break;

        default:
             break;
      }
    }
    pFld++;
  }
                                       /* All was well                */
  Reason = 0;
MOD_EXIT:
  if (Reason)
  {
    sprintf(pOpt->ErrorMessage,"Can not write to file '%s'",pOpt->ActOutFileName);
  }
  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessageHex                                     */
/* Purpose  : Write message in hex                                    */
/**********************************************************************/
MQLONG WriteFileMessageHex(QLOPT * pOpt,unsigned char * p,int MsgLen)
{
  MQLONG          Reason = MQRC_RESOURCE_PROBLEM;
  unsigned char * pStartLine;
  int             LineLength;
  int             n,i = 0;

  n = fprintf(pOpt->hOutFile,"X ");
  if (n <=0) goto MOD_EXIT;
  pStartLine = p;

  while (MsgLen > 0)
  {
    n = fputc(HEX[*p/16],pOpt->hOutFile);
    n = fputc(HEX[*p%16],pOpt->hOutFile);
    p++;i++;MsgLen--;

    if (i>=pOpt->DataWidth || !MsgLen)
    {
      if (pOpt->Options & qoDSP_ASCII_COLS_FILE)
      {
        LineLength = i;
        while (i < (pOpt->DataWidth+1))
        {
          n = fputc(' ',pOpt->hOutFile);
          n = fputc(' ',pOpt->hOutFile);
          i++;
        }
        n = fputc('<',pOpt->hOutFile);
        for(i=0 ; i<LineLength; i++)
        {
          unsigned char c = pStartLine[i];
          if (isprint(c)) n = fputc(c  ,pOpt->hOutFile);
                     else n = fputc('.',pOpt->hOutFile);
        }
        n = fputc('>',pOpt->hOutFile);
      }
      if (n <=0) goto MOD_EXIT;
      if (!MsgLen) break;

      n = fprintf(pOpt->hOutFile,"\nX ");
      if (n <=0) goto MOD_EXIT;
      pStartLine = p;
      i=0;
    }
  }
  n = fputc('\n',pOpt->hOutFile);
  if (n <=0) goto MOD_EXIT;
  Reason = 0;

MOD_EXIT:
  if (Reason)
  {
    sprintf(pOpt->ErrorMessage,"Can not write to file '%s'",pOpt->ActOutFileName);
  }
  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessageString                                  */
/* Purpose  : Write message in ASCII                                  */
/**********************************************************************/
MQLONG WriteFileMessageString(QLOPT * pOpt,unsigned char * p,int MsgLen)
{
  MQLONG          Reason = MQRC_RESOURCE_PROBLEM;
  int             LineLength;
  int             i = 0;

  while (1)
  {
    fprintf(pOpt->hOutFile,"S ");
    LineLength = min(pOpt->DataWidth*2-2,MsgLen);

    if (!MsgLen || !LineLength)
    {
      fputc('\n',pOpt->hOutFile);
      break;
    }

    fputc('\"',pOpt->hOutFile);
    for (i=0; i<LineLength; i++)
    {
      switch(p[i])
      {
        case '\"':
        case ESCAPE_CHAR:
             fputc(ESCAPE_CHAR,pOpt->hOutFile);
                                       /* DELIBERATELY NO BREAK HERE  */
        default:
             if (fputc(p[i],pOpt->hOutFile) == EOF) goto MOD_EXIT;
             break;
      }
    }
    MsgLen -= LineLength;
    p      += LineLength;
    fputc('\"',pOpt->hOutFile);
    fputc('\n',pOpt->hOutFile);
    if (!MsgLen)     break;
  }
  Reason = 0;
MOD_EXIT:
  if (Reason)
  {
    sprintf(pOpt->ErrorMessage,"Can not write to file '%s' RC(%d)",pOpt->ActOutFileName,errno);
  }

  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessageText                                    */
/* Purpose  : Write message as a single line                          */
/**********************************************************************/
MQLONG WriteFileMessageTextLine(QLOPT * pOpt,unsigned char * p,int MsgLen)
{
  MQLONG          Reason = MQRC_RESOURCE_PROBLEM;
  int             i = 0;
  size_t          nwritten;

  fprintf(pOpt->hOutFile,"T ");
  if (!MsgLen)
  {
    fputc('\n',pOpt->hOutFile);
    Reason = 0;
    goto MOD_EXIT;
  }

  nwritten = fwrite(p,1,MsgLen,pOpt->hOutFile);
  if (nwritten != MsgLen)
    goto MOD_EXIT;
  if (fputc('\n',pOpt->hOutFile) == EOF) goto MOD_EXIT;
  Reason = 0;
MOD_EXIT:
  if (Reason)
  {
    sprintf(pOpt->ErrorMessage,"Can not write to file '%s' RC(%d)",pOpt->ActOutFileName,errno);
  }

  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessageText                                    */
/* Purpose  : Write message text itself                               */
/**********************************************************************/
MQLONG WriteFileMessageText(QLOPT * pOpt)
{
  MQLONG          Reason = 0L;
  size_t          MsgLen = pOpt->MsgLen;
  unsigned char * pStart = (unsigned char *)pOpt->pMsgStart;
  unsigned char * pEnd   = pStart + MsgLen;
  unsigned char * p;
  BOOL            FirstLine = TRUE;

  if (pOpt -> Options & qoDSP_ASCII_FILE)
  {
    while (1)
    {
      MsgLen = 0;
      p      = pStart;
      while (CSINVARIANT(*p) && p<pEnd) { p++; MsgLen++; }
                                       /* As much as we can as string */
      if (MsgLen || FirstLine)
      {
        Reason = WriteFileMessageString(pOpt,pStart,MsgLen);
        if (Reason) goto MOD_EXIT;
      }

      pStart += MsgLen;
      MsgLen  = 0;
      p       = pStart;
      if (p >= pEnd) break;

      while (!CSINVARIANT(*p) && p<pEnd) { p++; MsgLen++; }
      Reason = WriteFileMessageHex(pOpt,pStart,MsgLen);
      if (Reason) goto MOD_EXIT;

      pStart += MsgLen;
      p       = pStart;
      if (p >= pEnd) break;
      FirstLine = FALSE;
    }
  }
  else if (pOpt -> Options2 & qo2DSP_PRINTABLE)
  {
    while (1)
    {
      MsgLen = 0;
      p      = pStart;
      while (isprint(*p) && p<pEnd) { p++; MsgLen++; }
                                       /* As much as we can as string */
      if (MsgLen || FirstLine)
      {
        Reason = WriteFileMessageString(pOpt,pStart,MsgLen);
        if (Reason) goto MOD_EXIT;
      }

      pStart += MsgLen;
      MsgLen  = 0;
      p       = pStart;
      if (p >= pEnd) break;

      while (!isprint(*p) && p<pEnd) { p++; MsgLen++; }
      Reason = WriteFileMessageHex(pOpt,pStart,MsgLen);
      if (Reason) goto MOD_EXIT;

      pStart += MsgLen;
      p       = pStart;
      if (p >= pEnd) break;
      FirstLine = FALSE;
    }
  }
  else if (pOpt -> Options2 & qo2DSP_TEXT)
  {
    while (1)
    {
      MsgLen = 0;
      p      = pStart;
      while (isprint(*p) && p<pEnd) { p++; MsgLen++; }
                                       /* As much as we can as string */
      if (MsgLen || FirstLine || *p == '\n' || *p == '\r')
      {
        Reason = WriteFileMessageTextLine(pOpt,pStart,MsgLen);
        if (Reason) goto MOD_EXIT;
      }
      FirstLine = FALSE;

      pStart += MsgLen;
      MsgLen  = 0;
      p       = pStart;
      if (p >= pEnd) break;
                                       /* Do we have line terminator */
      if (*p == '\n')
      {
        p++;
        if (*p == '\r') p++;
        pStart = p;
        continue;
      }
      if (*p == '\r')
      {
        p++;
        if (*p == '\n') p++;
        pStart = p;
        continue;
      }
                                       /* Must be unprintable        */
      while (!isprint(*p) && p<pEnd) { p++; MsgLen++; }
      Reason = WriteFileMessageHex(pOpt,pStart,MsgLen);
      if (Reason) goto MOD_EXIT;

      pStart += MsgLen;
      p       = pStart;
      if (p >= pEnd) break;
    }
  }
  else
  {
                                       /* Whole thing as HEX          */
    Reason = WriteFileMessageHex(pOpt,pStart,MsgLen);
    if (Reason) goto MOD_EXIT;
  }
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessageAge                                     */
/* Purpose  : Write the age message to the file                       */
/**********************************************************************/
MQLONG WriteFileMessageAge(QLOPT * pOpt)
{
  MQLONG Reason = 0;
  char   Buffer[100];
  char * p;
  size_t indent;
  time_t Age;
  size_t n,w;

  sprintf(Buffer,"%5d. MsgId:",pOpt->InIndex);
  indent = strlen(Buffer);

  p = Buffer + indent;
  HexStrn(pOpt->mqmd.MsgId,24,p,48);
  p +=48;
  *p++ = '\n';
  n = p - Buffer;
  w = fwrite(Buffer,1,n,pOpt->hOutFile);
  if (w != n)
  {
    printf("Error writing to file '%s'\n",pOpt->ActOutFileName);
    Reason = MQRC_FILE_SYSTEM_ERROR;
    goto MOD_EXIT;
  }

  p = Buffer;
  memset(p,' ',indent);
  p+=indent;
  Age = ConvertToTime88(pOpt->mqmd.PutDate, pOpt->mqmd.PutTime);
  if (Age<=0)
  {
    strcpy(p,"Unknown");
  }
  else
  {
    Age = pOpt->Now - Age;
    GetCharTime(p,(int)Age,FALSE);
  }
  p   +=strlen(p);
  *p++ = '\n';
  *p++ = '\n';
  n = p - Buffer;
  w = fwrite(Buffer,1,n,pOpt->hOutFile);
  if (w != n)
  {
    printf("Error writing to file '%s'\n",pOpt->ActOutFileName);
    Reason = MQRC_FILE_SYSTEM_ERROR;
    goto MOD_EXIT;
  }
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : WriteFileMessage                                        */
/* Purpose  : Write the message to the file                           */
/**********************************************************************/
MQLONG WriteFileMessage(QLOPT * pOpt)
{
  MQLONG Reason = 0;

  if (pOpt->Options & qoWRITE_AGE)
  {
    Reason = WriteFileMessageAge(pOpt);
  }
  else
  {
    if (pOpt->Options & qoWRITE_INDEX)
    {
      fprintf(pOpt->hOutFile,"* Index %d\n",pOpt->OutIndex);
    }

    Reason = WriteFileMQMD(pOpt);
    if (Reason) goto MOD_EXIT;

    Reason = WriteFileMessageText(pOpt);
    if (Reason) goto MOD_EXIT;

    fputc('\n',pOpt->hOutFile);
  }

MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : ReadMessage                                             */
/* Purpose  : Read the next message                                   */
/**********************************************************************/
MQLONG ReadMessage(QLOPT * pOpt,BOOL * Complete,BOOL Prompt)
{
  MQLONG Reason;
       if (pOpt->InputQueue[0])           Reason = ReadMQMessage  (pOpt,Complete,FALSE);
  else if (pOpt->Options&qoAMQSBCG_INPUT) Reason = ReadBCGMessage (pOpt,Complete);
  else
  {
    while (1)
    {
      Reason = ReadFileMessage(pOpt,Complete);
      if (Reason)     goto MOD_EXIT;
      if (!*Complete) goto MOD_EXIT;
      if (!(pOpt->Options & qoUNIQUE_FILES_IN)) goto MOD_EXIT;
      fclose(pOpt->hInFile);
      pOpt->hInFile = NULL;
      /************************************************************/
      /* Ok, now let's see whether there is another file to open  */
      /************************************************************/
      Reason = QloadOpenFiles(pOpt,Prompt,TRUE,FALSE);
      if (Reason)
      {
        *Complete = TRUE;
        Reason = 0;
        goto MOD_EXIT;
      }
      else
      {
        *Complete = FALSE;
        continue;
      }
    }
  }

MOD_EXIT:
  pOpt -> pMsgStart = pOpt -> pMsg;
  return Reason;
}
/**********************************************************************/
/* Function : Sift                                                    */
/* Purpose  : Sift a value into the right bucket                      */
/**********************************************************************/
void Sift(UINT32 value, UINT32 * buckets, SIFT * values,int MaxBuckets)
{
  int i;
  for (i=0; i<(MaxBuckets-1); i++)
  {
    if (value < values[i].val)
    {
      buckets[i]++;
      goto MOD_EXIT;
    }
  }
  buckets[i]++;
MOD_EXIT:
  ;
}
/**********************************************************************/
/* Function : Summarize                                               */
/* Purpose  : Collate summarizing information                         */
/**********************************************************************/
MQLONG Summarize(QLOPT * pOpt)
{
  time_t Age;
  BOOL   Youngest,Oldest,Smallest,Biggest;

  Age = ConvertToTime88(pOpt->mqmd.PutDate, pOpt->mqmd.PutTime);
  if (Age == -1) Age = 0;
  Age = pOpt->Now - Age;

  if (pOpt->OutIndex == 1)
  {
    Youngest = TRUE;
    Oldest   = TRUE;
    Biggest  = TRUE;
    Smallest = TRUE;
  }
  else
  {
    Youngest = Age < pOpt->YoungestMsg;
    Oldest   = Age > pOpt->OldestMsg;
    Smallest = pOpt->MsgLen < pOpt->SmallestMsg;
    Biggest  = pOpt->MsgLen > pOpt->BiggestMsg;
  }

  if (Youngest)
  {
    pOpt->YoungestMsg = Age;
    memcpy(pOpt->YoungestMsgMsgId   ,pOpt->mqmd.MsgId,24);
    memcpy(pOpt->YoungestMsgCorrelId,pOpt->mqmd.CorrelId,24);
  }
  if (Oldest)
  {
    pOpt->OldestMsg = Age;
    memcpy(pOpt->OldestMsgMsgId   ,pOpt->mqmd.MsgId,24);
    memcpy(pOpt->OldestMsgCorrelId,pOpt->mqmd.CorrelId,24);
  }

  Sift(Age,pOpt->AgeBuckets,AgeBuckets,MAX_AGE_BUCKETS);

  if (Smallest)
  {
    pOpt->SmallestMsg = pOpt->MsgLen;
    memcpy(pOpt->SmallestMsgMsgId   ,pOpt->mqmd.MsgId,24);
    memcpy(pOpt->SmallestMsgCorrelId,pOpt->mqmd.CorrelId,24);
  }
  if (Biggest)
  {
    pOpt->BiggestMsg = pOpt->MsgLen;
    memcpy(pOpt->BiggestMsgMsgId   ,pOpt->mqmd.MsgId,24);
    memcpy(pOpt->BiggestMsgCorrelId,pOpt->mqmd.CorrelId,24);
  }
  Sift(pOpt->MsgLen,pOpt->SizeBuckets,SizeBuckets,MAX_SIZE_BUCKETS);

  return 0;
}
/**********************************************************************/
/* Function : WriteMessage                                            */
/* Purpose  : Write the next message                                  */
/**********************************************************************/
MQLONG WriteMessage(QLOPT * pOpt)
{
  /********************************************************************/
  /* Do we need to add a delay ?                                      */
  /********************************************************************/
  if (pOpt->Options & qoRELATIVE_DELAY)
  {
    int diff1,diff2,Now;

    if (StartTime)
    {
      diff1 = DiffTime88(FirstDate,FirstTime,pOpt->mqmd.PutDate, pOpt->mqmd.PutTime);
      if (diff1 < 0)
      {
        /**************************************************************/
        /* We've gone backwards in time.....take a new start point    */
        /**************************************************************/
        StartTime = CSGetRelTime();
        memcpy(FirstDate,pOpt->mqmd.PutDate,8);
        memcpy(FirstTime,pOpt->mqmd.PutTime,8);
      }
      else
      {
        Now   = CSGetRelTime();
        diff2 = Now - StartTime;

        if (diff1 > diff2)
        {
          int diff = diff1-diff2;
          char cTime[100];

          if (pOpt->Delay != 100)
          {
            diff = (diff * pOpt->Delay) / 100;
          }

          if (diff > 5000)
          {
            time_t TimeNow;
            char * pTime;

            time(&TimeNow);
            pTime = ctime(&TimeNow);
            GetCharTime(cTime,(diff+500)/1000,FALSE);
            printf("%5.5s - sleeping for %s\n",pTime+11,cTime);
          }
          CSMSSleep(diff);
        }
      }
    }
    else
    {
      StartTime = CSGetRelTime();
      memcpy(FirstDate,pOpt->mqmd.PutDate,8);
      memcpy(FirstTime,pOpt->mqmd.PutTime,8);
    }
  }
  else
  {
    if (pOpt->Delay)
    {
      if (pOpt->Delay > 0)
      {
        CSMSSleep(pOpt->Delay);
      }
      else
      {
        int d = -pOpt->Delay * rand() / RAND_MAX;
        CSMSSleep(d);
      }
    }
  }
  /********************************************************************/
  /* Do we need to strip off any headers                              */
  /********************************************************************/
  if (pOpt->Options & qoSTRIP_HEADERS)
  {
    if (SAME8(pOpt->mqmd.Format,MQFMT_DEAD_LETTER_HEADER))
    {
      MQDLH * pDLH = (MQDLH *)pOpt->pMsgStart;

      pOpt->mqmd.Encoding    = pDLH->Encoding;
      pOpt->mqmd.PutApplType = pDLH->PutApplType;
      memcpy(pOpt->mqmd.Format      ,pDLH->Format     ,sizeof(pDLH->Format     ));
      memcpy(pOpt->mqmd.PutApplName, pDLH->PutApplName,sizeof(pDLH->PutApplName));
      memcpy(pOpt->mqmd.PutDate    , pDLH->PutDate    ,sizeof(pDLH->PutDate    ));
      memcpy(pOpt->mqmd.PutTime    , pDLH->PutTime    ,sizeof(pDLH->PutTime    ));

      pOpt->pMsgStart += sizeof(MQDLH);
      pOpt->MsgLen    -= sizeof(MQDLH);
    }
    if (SAME8(pOpt->mqmd.Format,MQFMT_XMIT_Q_HEADER))
    {
      MQXQH * pXQH = (MQXQH *)pOpt->pMsgStart;

      memcpy(&pOpt->mqmd, &pXQH->MsgDesc,sizeof(MQMD1));

      pOpt->pMsgStart += sizeof(MQXQH);
      pOpt->MsgLen    -= sizeof(MQXQH);
    }
  }
  /********************************************************************/
  /* Do we need to summarize the data                                 */
  /********************************************************************/
  if (pOpt->Options & qoSUMMARY)
  {
    return Summarize(pOpt);
  }
  else
  {
    if (pOpt->OutputQueue[0]) return WriteMQMessage(pOpt);
                         else return WriteFileMessage(pOpt);
  }
}
/**********************************************************************/
/* Function : CheckAge                                                */
/* Purpose  : Check the age of the message                            */
/*            Return TRUE is message too old                          */
/**********************************************************************/
BOOL CheckAge(QLOPT * pOpt)
{
  BOOL   IsOK;
  time_t Age;

  Age = ConvertToTime88(pOpt->mqmd.PutDate, pOpt->mqmd.PutTime);
  if (Age<=0) return FALSE;
  Age = pOpt->Now - Age;
  if (pOpt->OlderThanAge)   IsOK  = (Age >= pOpt->OlderThanAge);
                       else IsOK  = TRUE;
  if (pOpt->YoungerThanAge) IsOK &= (Age <= pOpt->YoungerThanAge);
  return IsOK;
}
/**********************************************************************/
/* Function : QloadInit                                               */
/* Purpose  : Initialise the options structure                        */
/**********************************************************************/
void QloadInit(QLOPT * pOpt)
{
  memset(pOpt,0,sizeof(QLOPT));

  pOpt->hQm              = MQHC_UNUSABLE_HCONN;
  pOpt->hInput           = MQHO_UNUSABLE_HOBJ;
  pOpt->hOutput          = MQHO_UNUSABLE_HOBJ;
  pOpt->DataWidth        = 25;
  pOpt->Options          = qoSET_ALL_CONTEXT;
  pOpt->TransactionLimit = 200;
  pOpt->StartMsgArea     = 1;
  pOpt->ClientMode       = FALSE;
}

/**********************************************************************/
/* Function : WriteFileheader                                         */
/* Purpose  : Write a simple comment line at the top of the file      */
/**********************************************************************/
static MQLONG WriteFileHeader(QLOPT * pOpt)
{
  MQLONG        Reason = 0;
  static time_t Now = 0;
  static char   Nowc[30];
  char        * pTime;
  int           n;

  if (pOpt->Options2 & qo2NO_WRITE_HEADER) goto MOD_EXIT;

  if (!Now)
  {
    time(&Now);
    pTime = ctime(&Now);
    strcpy(Nowc,pTime);
  }
                                     /* Ok, write header            */
  n = fprintf(pOpt->hOutFile,"* QLOAD Version:%s Created:%s",
              QLOAD_VERSION,Nowc);
  if (n<=0)
  {
    Reason = MQRC_RESOURCE_PROBLEM;
    sprintf(pOpt->ErrorMessage,"Can not write to file '%s'",pOpt->ActOutFileName);
    goto MOD_EXIT;
  }
  if (pOpt->hInput != MQHO_UNUSABLE_HOBJ)
  {
    if (pOpt->Qm[0])         fprintf(pOpt->hOutFile,"* Qmgr  = %.48s\n",pOpt->Qm);
    if (pOpt->InputQueue[0]) fprintf(pOpt->hOutFile,"* Queue = %.48s\n",pOpt->InputQueue);
  }
  fputc('\n',pOpt->hOutFile);
MOD_EXIT:
  return Reason;
}
/**********************************************************************/
/* Function : QloadFnc                                                */
/* Purpose  : Copy messages from one place to another                 */
/**********************************************************************/
MQLONG QloadFnc(QLOPT * pOpt,BOOL Prompt)
{
  MQLONG CompCode,Reason;
  BOOL   Complete = FALSE;
  BOOL   eof      = FALSE;
  BOOL   blank;
  BOOL   AddIt,Ok;
  int    rc;
  char * p;

  pOpt -> pLex = NULL;
  /********************************************************************/
  /* Set defaults                                                     */
  /********************************************************************/
  if (pOpt->DataWidth <= 0)      pOpt->DataWidth = 25;
  if (pOpt->DataWidth < 10)      pOpt->DataWidth = 10;
  /********************************************************************/
  /* Check the context options                                        */
  /********************************************************************/
  if (pOpt->Options & (qoPASS_ALL_CONTEXT | qoPASS_IDENTITY_CONTEXT))
  {
    if (pOpt->hInput == MQHO_UNUSABLE_HOBJ)
    {
      Reason = MQRC_CONTEXT_NOT_AVAILABLE;
      sprintf(pOpt->ErrorMessage,"An input queue must be supplied to pass context");
      goto MOD_EXIT;
    }
  }
  /********************************************************************/
  /* Check the options which should be used with a file               */
  /********************************************************************/
  if (pOpt->Options & (qoWRITE_INDEX          |
                       qoDSP_ASCII_FILE       |
                       qoDSP_ASCII_COLS_FILE  |
                       qoSUMMARY              |
                       qoWRITE_AGE))
  {
    if (!pOpt->OutFileName[0])
    {
      Reason = MQRC_OPTIONS_ERROR;
      sprintf(pOpt->ErrorMessage,"Display options require an output file");
      goto MOD_EXIT;
    }
  }
  if (pOpt->Options2 & (qo2DSP_PRINTABLE))
  {
    if (!pOpt->OutFileName[0])
    {
      Reason = MQRC_OPTIONS_ERROR;
      sprintf(pOpt->ErrorMessage,"Display options require an output file");
      goto MOD_EXIT;
    }
  }
  /********************************************************************/
  /* Get the message and line areas                                   */
  /********************************************************************/
  Reason = GetMessageArea(pOpt,pOpt->StartMsgArea);
  if (Reason) goto MOD_EXIT;

  Reason = GetLineArea(pOpt,1);
  if (Reason) goto MOD_EXIT;

  /********************************************************************/
  /* Are we dealing with a file ?                                     */
  /********************************************************************/
  if (pOpt -> hInFile)
  {
                                       /* We're reading a file        */
    while(1)
    {
      Reason = ReadLine(pOpt,TRUE,&eof,TRUE);
      if (Reason || eof) goto MOD_EXIT;
                                       /* Find first none blank       */
      p = pOpt->pLine;
      blank = TRUE;
      while (blank && *p)
      {
        switch(*p)
        {
          case ' ':
          case '\n':
          case '\t':
          case '\r':
               break;
          default:
               blank = FALSE;
               break;
        }
        p++;
      }
      if (!blank) break;
    }
    /******************************************************************/
    /* File should start 'AMQSBCG0' or '* QLOAD' or '* DMPMQMSG'      */
    /******************************************************************/
    if (!memcmp(pOpt->pLine,"AMQSBCG0",8))
    {
      pOpt->Options |= qoAMQSBCG_INPUT;
    }
    else
    {
      if (memcmp(pOpt->pLine,"* QLOAD",7) != 0 && memcmp(pOpt->pLine,"* DMPMQMSG",10) != 0)
      {
        Reason = MQRC_FILE_SYSTEM_ERROR;
        sprintf(pOpt->ErrorMessage,"File '%s' does not appear to be a qload file",
                pOpt->InputFileName);
        goto MOD_EXIT;
      }
    }
                                       /* Ok, read the next line      */
    Reason = ReadLine(pOpt,TRUE,&eof,FALSE);
    if (Reason || eof) goto MOD_EXIT;
  }
  if (pOpt -> hOutFile)
  {
    Reason = WriteFileHeader(pOpt);
    if (Reason) goto MOD_EXIT;
  }

  /********************************************************************/
  /* Have we got an input queue and we're moving messages ?           */
  /********************************************************************/
  if (pOpt->InputQueue[0] && (pOpt -> Options & qoMOVE))
  {
    pOpt->UseTransactions = TRUE;
  }
  /********************************************************************/
  /* Have we got an output queue ?                                    */
  /********************************************************************/
  if (pOpt->OutputQueue[0])
  {
    pOpt->UseTransactions = TRUE;
  }
  /********************************************************************/
  /* Switch off transasctions if we're not allowed them               */
  /********************************************************************/
  if (!pOpt->TransactionLimit) pOpt->UseTransactions = FALSE;

  /********************************************************************/
  /* Initialise message counters                                      */
  /********************************************************************/
  pOpt -> InIndex  = 0;
  pOpt -> OutIndex = 0;
  /********************************************************************/
  /* All looks good, do the transfer                                  */
  /********************************************************************/
  while(1)
  {
    pOpt -> InIndex ++;

    Reason = ReadMessage(pOpt,&Complete,Prompt);
    if (Reason)   goto MOD_EXIT;

    if (Complete) goto MOD_EXIT;

    pOpt -> InMsgs++;
    pOpt -> InBytes += pOpt -> MsgLen;

    if (pOpt -> hOutput != MQHO_UNUSABLE_HOBJ ||
        pOpt -> OutFileName[0])
    {
      int i;
      AddIt = TRUE;
      if (pOpt->FirstIndex)
      {
        AddIt = (pOpt->InIndex >= pOpt->FirstIndex) &&
                (pOpt->InIndex <= pOpt->LastIndex);
      }

      if (AddIt && pOpt->Options & qoMATCH_MSGID)
      {
        AddIt &= !memcmp(pOpt->MsgId,pOpt->mqmd.MsgId,24);
      }

      if (AddIt && pOpt->Options & qoMATCH_CORRELID)
      {
        AddIt &= !memcmp(pOpt->CorrelId,pOpt->mqmd.CorrelId,24);
      }

      if (AddIt && pOpt->Options & qoMATCH_GROUPID)
      {
        AddIt &= !memcmp(pOpt->GroupId,pOpt->mqmd.GroupId,24);
      }

      if (AddIt && pOpt->Ascii[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->Ascii[i].Len && i < MAX_SEARCH ; i++)
        {
          Ok &= (memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->Ascii[i].Str,pOpt->Ascii[i].Len) != NULL);
        }
        AddIt &= Ok;
      }

      if (AddIt && pOpt->NegAscii[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->NegAscii[i].Len && i < MAX_SEARCH ; i++)
        {
          Ok &= !memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->NegAscii[i].Str,pOpt->NegAscii[i].Len);
        }
        AddIt &= Ok;
      }

      if (AddIt && pOpt->Ebcdic[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->Ebcdic[i].Len && i < MAX_SEARCH ; i++)
        {
          Ok &= (memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->Ebcdic[i].Str,pOpt->Ebcdic[i].Len) != NULL);
        }
        AddIt &= Ok;
      }

      if (AddIt && pOpt->NegEbcdic[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->NegEbcdic[i].Len && i < MAX_SEARCH ; i++)
        {
          AddIt &= !memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->NegEbcdic[i].Str,pOpt->NegEbcdic[i].Len);
        }
        AddIt &= Ok;
      }

      if (AddIt && pOpt->Hex[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->Hex[i].Len && i < MAX_SEARCH ; i++)
        {
          AddIt &= (memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->Hex[i].Str,pOpt->Hex[i].Len) != NULL);
        }
        AddIt &= Ok;
      }

      if (AddIt && pOpt->NegHex[0].Len)
      {
        Ok = TRUE;
        for (i=0; pOpt->NegHex[i].Len && i < MAX_SEARCH ; i++)
        {
          AddIt &= !memfind(pOpt->pMsg,pOpt->MsgLen,pOpt->NegHex[i].Str,pOpt->NegHex[i].Len);
        }
        AddIt &= Ok;
      }

      if (AddIt && (pOpt->OlderThanAge || pOpt->YoungerThanAge))
      {
        AddIt = CheckAge(pOpt);
      }

      if (AddIt)
      {
                                       /* Do I need to open a file ?  */
        if (pOpt->OutFileName[0] && !pOpt->hOutFile)
        {
          Reason = QloadOpenFiles(pOpt,Prompt,FALSE,TRUE);
          if (Reason) goto MOD_EXIT;

          Reason = WriteFileHeader(pOpt);
          if (Reason) goto MOD_EXIT;
        }
        /**************************************************************/
        /* If we're filtering an getting from a queue then we'd better*/
        /* go and actually get the message                            */
        /**************************************************************/
        if (pOpt -> Options & qoFILTER)
        {
          Reason = ReadMQMessage(pOpt,&Complete,TRUE);
          if (Reason)   goto MOD_EXIT;
          if (Complete) goto MOD_EXIT;
        }

        pOpt -> OutIndex ++;

        Reason = WriteMessage(pOpt);
        if (Reason) goto MOD_EXIT;

        pOpt -> OutMsgs++;
        pOpt -> OutBytes += pOpt -> MsgLen;

        /**************************************************************/
        /* If we're writing to a file and it's unique then perhaps    */
        /* we need to re-open a file                                  */
        /**************************************************************/
        if (pOpt -> Options & qoUNIQUE_FILES_OUT)
        {
          fclose(pOpt->hOutFile);
          pOpt->hOutFile = NULL;
        }
      }

      if (pOpt->FirstIndex)
      {
        if (pOpt->InIndex >= pOpt->LastIndex) break;
      }
    }

    if (pOpt->UseTransactions)
    {
      pOpt->TransactionSize++;
      if ((pOpt->TransactionSize  >= pOpt->TransactionLimit) &&
          (pOpt->TransactionLimit != -1                    ))
      {
        if (pOpt->hOutFile)
        {
          rc = fflush(pOpt->hOutFile);
          if (rc)
          {
            Reason = MQRC_RESOURCE_PROBLEM;
            sprintf(pOpt->ErrorMessage,"Error flushing data to file '%s'RC(%d)",
              pOpt->ActOutFileName,rc);
            goto MOD_EXIT;
          }
        }

        pOpt->TransactionSize=0;
        MQCMIT(pOpt->hQm,&CompCode,&Reason);
        if (CompCode == MQCC_FAILED)
        {
           sprintf(pOpt->ErrorMessage,"MQCMIT failed RC(%d) %s",
                   Reason,MQReason(Reason));
           goto MOD_EXIT;
        }
      }
    }
  }
MOD_EXIT:
  /********************************************************************/
  /* Do we have any messages left over in a transaction ?             */
  /********************************************************************/
  if (pOpt->UseTransactions)
  {
    if (Reason)
    {
      MQLONG CompCode,Reason; /* Don't mask other reason code         */
      MQBACK(pOpt->hQm,&CompCode,&Reason);
      if (CompCode == MQCC_FAILED)
      {
         sprintf(pOpt->ErrorMessage,"MQBACK failed RC(%d) %s",
                 Reason,MQReason(Reason));
      }
    }
    else if (pOpt->TransactionSize)
    {
      MQCMIT(pOpt->hQm,&CompCode,&Reason);
      if (CompCode == MQCC_FAILED)
      {
         sprintf(pOpt->ErrorMessage,"MQCMIT failed RC(%d) %s",
                 Reason,MQReason(Reason));
      }
    }
  }

  if (pOpt->pMsg)
  {
    free(pOpt->pMsg);
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Freeing %p\n",pOpt->pMsg);
    }
    pOpt->pMsg = NULL;
  }
  if (pOpt->pLine)
  {
    free(pOpt->pLine);
    if (pOpt->Verbose & vbDEBUG_MEMORY)
    {
      fprintf(stderr,"Freeing %p\n",pOpt->pLine);
    }
    pOpt->pLine = NULL;
  }

  return Reason;
}

void TestQload()
{
  /*********************************************************************/
  /* We need a message                                                 */
  /*********************************************************************/
  #define TEST_FILE "c:\\temp\\qload.tst"
  int    MsgLength   = 4 * 1024;
  int    i;
  QLOPT  opt;
  MQMD   DefMd = { MQMD_DEFAULT };
  char * pMessage = malloc(MsgLength);
  MQLONG Complete,Reason;
  FLD  * pFld;
  char * pAttr;

  printf("\nRunning file format tests");
  printf("\n-------------------------\n");

  QloadInit(&opt);
  Reason = GetMessageArea(&opt,MsgLength);
  if (!pMessage || Reason)
  {
    printf("Can not allocate %d bytes\n",MsgLength);
    goto MOD_EXIT;
  }

  Reason = GetLineArea(&opt,1);
  if (Reason)
  {
    printf("Can not allocate line area\n");
    goto MOD_EXIT;
  }

  /*********************************************************************/
  /* Initialise the message                                            */
  /*********************************************************************/
  for (i=0; i<MsgLength; i++) pMessage[i] = (char)i;

  /*********************************************************************/
  /* Initialise the message descriptor                                 */
  /*********************************************************************/
  pFld = Flds;
  for (i= 0; pFld->Type; i++)
  {
    pAttr = ((char *)&DefMd) + pFld -> Offset;
    if (pFld->Id[0])
    {
      switch(pFld->Type)
      {
        case FT_LONG:
             *(MQLONG *)pAttr = i;
             break;
        case FT_CHAR:
             memset(pAttr,'a'+i,pFld->Length);
             break;
        case FT_BYTE:
             memset(pAttr,i,pFld->Length);
             break;
        default:
             CSTrap(pFld->Type);
             break;
      }
    }
    pFld++;
  }
  DefMd.Version = 2;
  opt.Options = 0;
  for (i=0 ; i< 3; i++)
  {
    if (opt.hInFile)  fclose(opt.hInFile);
    if (opt.hOutFile) fclose(opt.hOutFile);

    switch(i)
    {
      case 0:
           printf("  Testing Hex file\n");
           break;
      case 1:
           printf("  Testing Ascii file\n");
           opt.Options |= qoDSP_ASCII_FILE;
           break;
      case 2:
           printf("  Testing Ascii column file\n");
           opt.Options |= qoDSP_ASCII_FILE | qoDSP_ASCII_COLS_FILE;
           break;
    }
                                       /* Write it out to a test file */
    opt.hOutFile = fopen(TEST_FILE,"w");
    if (!opt.hOutFile)
    {
      printf("Can not open test file '%s' for output\n",TEST_FILE);
      goto MOD_EXIT;
    }
    memcpy(opt.pMsg, pMessage, MsgLength);
    opt.pMsgStart = opt.pMsg;
    opt.MsgLen    = MsgLength;
    memcpy(&opt.mqmd, &DefMd, sizeof(MQMD));
    Reason = WriteFileMessage(&opt);
    if (Reason)
    {
      printf("WriteFileMessage returned RC(%d)\n",Reason);
      goto MOD_EXIT;
    }
                                         /* Now, read it back again  */
    fclose(opt.hOutFile);
    opt.hInFile = fopen(TEST_FILE,"r");
    if (!opt.hInFile)
    {
      printf("Can not open test file '%s' for input\n",TEST_FILE);
      goto MOD_EXIT;
    }
    opt.MsgLen = 0;
    memset(&opt.mqmd,0,sizeof(MQMD));
    ReadFileMessage(&opt,(BOOL*)&Complete);

    if (memcmp(&opt.mqmd, &DefMd, sizeof(MQMD)))
    {
      printf("Message Descriptors don't match\n");
      CSTrap(0);
    }
    if (opt.MsgLen != MsgLength)
    {
      printf("Message length does not match\n");
      CSTrap(0);
    }
    if (memcmp(opt.pMsg, pMessage, MsgLength))
    {
      printf("Message does not match!\n");
      CSTrap(0);
    }
  }
  printf("All tests passed\n");
  printf("----------------\n\n");

MOD_EXIT:
  if (opt.hInFile)  fclose(opt.hInFile);
  if (opt.hOutFile) fclose(opt.hOutFile);
}
