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
/*  FILE   : QLPARSE.C                                                */
/*  PURPOSE: Parses a message                                         */
/**********************************************************************/
                                       /* Defines                     */
#define MIN(a,b) (a<b ? a:b)
                                       /* Include files               */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "qlcs.h"
#include "qlutil.h"
#include "cmqc.h"
#include "cmqcfc.h"
#include "qlvalues.h"

#include "qlparse.h"
#if defined(AMQ_AS400)
#include <ctype.h>
#endif

#define MAX_LINE_SIZE 16384
#define INDENT_AMOUNT     2

#define WHITE_SPACE(a) ((a)==' '||(a)=='\t'||(a)=='\n'||(a)=='\r')

#define ROUNDUP4(a) (((a)+3) & ~0x03)

typedef struct
{
  unsigned char * TagName;
  int             TagNameLength;
  unsigned char * TagStart;            /* '<' char                    */
  unsigned char * TagEnd;              /* '>' char                    */
  unsigned char * TagData;             /* After name                  */
  int             TagLength;
  int             IsTagEnd;
  int             IsTagSingleton;
  unsigned char * TagTitle;
  int             TagTitleLength;
} TAG;
                                       /* Prototype                   */

static void AddValues                 (MQFIELD     * pField,
                                       int           Data,
                                       char        * Buffer,
                                       CBOPT       * pOptions);

static void AddParmValue              (unsigned int  ParmId,
                                       int           Value,
                                       char        * Buffer);

static int  AddFlags                  (int           (outfn)(void *,CBOPT *,char *,int ),
                                       void                 * Parm,
                                       MQFIELD              * pField,
                                       int                    Data,
                                       int                    Indent,
                                       CBOPT                * pOptions);

static int  AddDeferredParmValue      (int           (outfn)(void *,CBOPT *,char *,int ),
                                       void                 * Parm,
                                       MQFIELD              * pField,
                                       int                    Data,
                                       int                    Indent,
                                       CBOPT                * pOptions);

static int CheckDepend                (MQSCHAIN             * pChain,
                                       MQDEPEND             * pDepend);

static unsigned int  GetValue         (MQFIELD              * pField,
                                       char                 * p,
                                       int                    Encoding);


static MQINT64       GetValue64       (MQFIELD              * pField,
                                       char                 * p,
                                       int                    Encoding);

static unsigned int  GetValueIndirect (MQSCHAIN             * pChain,
                                       MQFIELD              * pField);


static int Always      = 1;
static MQFIELD XmlLength = { "XML Length   :", MQFT_LONG   , 4, NULL };
                                       /* Assume Intel & Ascii        */
       int gEncoding   = CSENC_LOCAL;
       int gEbcdic     = 0;
       int gTimeZone   = 0;
extern int Initialised;

       MQCHAR8   FormatName = "";

static void CheckStack(MQSCHAIN * pChain);
static void CheckStack(MQSCHAIN * pChain)
{
  while (pChain)
    pChain = pChain->Prev;
}

typedef struct
{
  int    TagLength;
  char * Xml;
  int    Length;
  char * ShortName;
} XMLSTRINGS;

static XMLSTRINGS XMLStrings[] = {{0,"jms"     , 0,"JMSProperties" },
                                  {0,"usr"     , 0,"User Properties" },
                                  {0,"mcd"     , 0,"Message Properties" },

                                  {0,"Dst"     , 0,"JMSDestination" },
                                  {0,"Msd"     , 0,"Message Type" },
                                  {0,"Exp"     , 0,"JMSExpiration" },
                                  {0,"Pri"     , 0,"JMSPriority" },
                                  {0,"Dlv"     , 0,"JMSDeliveryMode" },
                                  {0,"Cid"     , 0,"JMSCorrelationID" },
                                  {0,"Rto"     , 0,"JMSReplyTo" },
                                  {0,"Tms"     , 0,"JMSTimestamp" },
                                  {0,"Type"    , 0,"JMSType" },
                                  {0,"Set"     , 0,"JMSType (Set)" },
                                  {0,"Fmt"     , 0,"JMSType (Fmt)" },
                                  {0,"Dst"     , 0,"JMSXGroupID" },
                                  {0,"Dst"     , 0,"JMSXGroupSeq" } };

static int       XmlInit        = 0;
static int       XmlTitleLength = 0;
static char    * Buffer = NULL;
extern MQFIELD   FieldMessage;
extern MQFIELD   FieldXmlMsg;
extern MQFIELD   ParameterId;
extern MQFIELD   OpenOptions;


/**********************************************************************/
/* Function : DumpString                                              */
/* Purpose  : Print out a string of characters                        */
/**********************************************************************/
static int  DumpString (int                 (outfn)(void *,CBOPT *,char *,int ),
                        void              * Parm,
                        unsigned char     * pStartOfData,
                        CBOPT             * pOptions,
                        int                 IndentSp,
                        int                 MinWidth,
                        MQLONG              FieldLength,
                        unsigned char     * p)
{
  int  rc = 0;
  int  maxlen = pOptions -> ScreenWidth - IndentSp - 2;
  BOOL forceline;
  int  i,j;
  int  len;
  int  offlen = 0;

  pOptions -> OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  if (pOptions->Options & MF_OFFSET) maxlen -= 10;
  if (maxlen < MinWidth) maxlen = MinWidth;

  if (pOptions->Options2 & MF2_ZERO_LENGTH_STRINGS) forceline = TRUE;
                                               else forceline = FALSE;

  if (IndentSp <0) IndentSp = 0;
  for (i=0 ; i<(int )FieldLength || forceline ; i+=len)
  {
    forceline = FALSE;
    len = FieldLength-i;
    if (len>maxlen) len=maxlen;

    if (!(pOptions->Options2 & MF2_IGNORE_CRLF))
    {
      /* Do we have any CRLF ? */
      for (j=0; j<len; j++)
      {
        if (p[i+j] == '\n' || p[i+j] =='\r')
        {
          forceline = TRUE;
          len = j;
          break;
        }
      }
    }

    if (FORMATTING)
    {
      char * pData = Buffer;

      pOptions -> LineDataMaxLength = maxlen;
      if (pOptions->Options & MF_OFFSET)
      {
        sprintf(pData,"+%8.8X ",&p[i]-(unsigned char *)pStartOfData);
        pData += 10;
        offlen = 10;
        pOptions -> LineDataOffset = 10;
      }
      else
      {
        pOptions -> LineDataOffset = 0;
      }
      memset(pData,' ',IndentSp);
      pData  += IndentSp;

      pOptions -> LineMsgOffset  = i;
      pOptions -> LineMsgLength  = len;
      pOptions -> LineDataLength = len;

      if (gEbcdic)
      {
        /* Copy 'as-is                  */
        memcpy(pData,p+i,len);
        /* Convert to ASCII             */
        CSToAscii(pData,pData,len);

        for (j=0 ; j<len ; j++)
        {
          if (!isprint(pData[j])) pData[j] = '.';
        }
      }
      else
      {
        for (j=0 ; j<len ; j++)
        {
          if (isprint(p[i+j])) pData[j] = p[i+j];
          else pData[j] = '.';
        }
      }
      pData[len] = 0;
    }
    CALLOUTFN(Parm,Buffer,len+offlen+IndentSp);
    if (!(pOptions->Options2 & MF2_IGNORE_CRLF))
    {
      if (p[i+len] == '\r') len++;
      if (p[i+len] == '\n') len++;
    }
  }

MOD_EXIT:
  return rc;
}

/**********************************************************************/
/* Function : GetTagName                                              */
/* Purpose  : Return the name of current tag                          */
/*            Tags are of the form                                    */
/*                                                                    */
/*            <TagName>                 Nesting tag                   */
/*            </TagName>                Ending Tag                    */
/*            <TagName/>                Singleton Tag                 */
/*            <?TagName>                Singleton Tag                 */
/*            <!- Anything ->           Comment Tag                   */
/*            <!Something  >                                          */
/*            <![Anything]]>                                          */
/*            <!DOCTYPE  [Anything] >                                 */
/*            <!DOCTYPE  Something  >                                 */
/**********************************************************************/
int GetTagName(MQLONG          * pMsgLen,
               unsigned char  ** pp,
               CBOPT           * pOptions,
               TAG             * Tag)
{
  unsigned char * p     = *pp;
  MQLONG MsgLen         = *pMsgLen;
  MQLONG OriginalMsgLen = MsgLen;
  unsigned char   EndChar        = 0;
  unsigned char   EndCharCount   = 0;
  unsigned char   GotCharCount   = 0;
  unsigned char * pName;
  int             NameLength;

  /********************************************************************/
  /* We are called pointing to the '<' character                      */
  /********************************************************************/
  Tag->TagStart       = p;
  Tag->TagNameLength  = 0;
  Tag->IsTagSingleton = 0;
  Tag->IsTagEnd       = 0;
                                       /* Find the end                */
  p++;
  Tag->TagEnd = memchr(p,'>',MsgLen);
  if (!Tag->TagEnd) goto MOD_EXIT;

  /********************************************************************/
  /* Ok, let's find the first non-blank char                          */
  /********************************************************************/
  while (WHITE_SPACE(*p))p++;
  switch(*p)
  {
                                       /* Ok, this must be an end tag */
    case '/': Tag->IsTagEnd = 1;
              p++;
              break;

    case '?': Tag->IsTagSingleton = 1;
              p++;
              break;

    case '!': Tag->IsTagSingleton = 1;
              p++;
              switch (*p)
              {
                case '-':              /* Comment                     */
                     EndChar      = '-';
                     EndCharCount = 1;
                     p++;
                     break;
                case '[':
                     EndChar      = ']';
                     EndCharCount = 2;
                     p++;
                     break;
                case 'D':
                     if (!memcmp(p,"DOCTYPE",7))
                     {
                       /***********************************************/
                       /* Now, this is odd..                          */
                       /*       If we have a '[' before the '>'       */
                       /* then we must see a ']' before the '>'       */
                       /***********************************************/
                       if (memchr(p,'[',Tag->TagEnd-p))
                       {
                         EndChar      = ']';
                         EndCharCount = 1;
                       }
                     }
                                       /* DELIBERATELY NO BREAK HERE  */
                default:
                     pName = p;
                     while (*pName!=' ' && *pName!='>')pName++;
                     if (*pName==' ') p = pName;
                     break;
              }
              break;
                                       /* Anything else is normal tag */
    default:  break;
  }
                                       /* Skip spaces                 */
  while (WHITE_SPACE(*p))p++;
  /********************************************************************/
  /* Ok, this is the start of the Tag Name....now find the Name end   */
  /********************************************************************/
  Tag->TagName = p;

  while (!WHITE_SPACE(*p) &&
         *p !='>'         &&
         *p !='/'         &&
         *p !='?'         &&
         *p !='!') p++;
  NameLength = p - Tag->TagName;

  /********************************************************************/
  /* Now we need to find the end of the tag itself                    */
  /********************************************************************/
  while (Always)
  {
    p = Tag->TagEnd-1;
                                       /* Search for the end char     */
    if (EndChar)
    {
                                       /* OK we need an endchar       */
      GotCharCount = EndCharCount;
      while (GotCharCount)
      {
        if (*p == EndChar)
        {
          GotCharCount--;
          p--;
        }
        else if (WHITE_SPACE(*p))
        {
          p--;
        }
        else
        {
          break;
        }
      }
                                       /* Found them all ?            */
      if (!GotCharCount) break;
                                       /* No, we need the next '>'    */
      MsgLen = OriginalMsgLen - (Tag->TagEnd - Tag->TagStart) - 1;
      Tag->TagEnd = memchr(Tag->TagEnd+1,'>',MsgLen);
      if (!Tag->TagEnd) goto MOD_EXIT;
    }
    else
    {
                                       /* Now is this a singleton ?   */
      if (*p == '/') Tag->IsTagSingleton = 1;
      break;
    }
  }

  Tag->TagNameLength = NameLength;
  Tag->TagData       = Tag->TagName + NameLength;
  Tag->TagLength     = Tag->TagEnd - Tag->TagStart + 1;
  p                  = Tag->TagEnd+1;
  MsgLen             = OriginalMsgLen - Tag->TagLength;

  /********************************************************************/
  /* Now...if we are asked for shortened XML then apply substitution  */
  /********************************************************************/
  if (pOptions->Options & MF_XML_SHORTFORM)
  {
    int i;

    for (i=0; i<sizeof(XMLStrings)/sizeof(XMLStrings[0]) ; i++)
    {
                                       /* Performance compare         */
      if (*(short *)XMLStrings[i].Xml == *(short *)Tag->TagName)
      {
        if (        XMLStrings[i].TagLength == Tag->TagNameLength &&
            !memcmp(XMLStrings[i].Xml       ,  Tag->TagName,Tag->TagNameLength))
        {
          Tag->TagName        = (unsigned char *)XMLStrings[i].ShortName;
          Tag->TagNameLength  = XMLStrings[i].Length;
          break;
        }
      }
    }
  }
  Tag->TagTitleLength = Tag->TagLength;
  Tag->TagTitle       = Tag->TagStart;

  *pp      = p;
  *pMsgLen = MsgLen;

MOD_EXIT:
  return Tag->TagNameLength;
}

/**********************************************************************/
/* Function : PrintXmlBits                                            */
/* Purpose  : Print out lines separated by white space                */
/**********************************************************************/
int  PrintXmlBit(int        (outfn)(void *,CBOPT *,char *,int ),
                 void              * Parm,
                 unsigned char     * pStartOfData,
                 CBOPT             * pOptions,
                 int                 IndentSp,
                 unsigned char     * pStart,
                 unsigned char     * pEnd)
{
  int    rc = 0;
  char * p;
  BOOL   InString = FALSE;
  BOOL   Added;
  int    MaxLineLength = pOptions -> ScreenWidth;
  int    Used;
  char * pBestEnd,* pBestEndp;

  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  if (MaxLineLength < (IndentSp + 30)) MaxLineLength = IndentSp + 30;

  while (pStart <= pEnd)
  {
                                       /* Find start                  */
    p     = Buffer;
    Added = FALSE;
    if (FORMATTING)
    {
      if (pOptions->Options & MF_OFFSET)
      {
        sprintf(p,"+%8.8X ",pStart-pStartOfData);
        p += 10;
      }
      memset(p,' ',IndentSp);
      p += IndentSp;
    }

    while (pStart <= pEnd && WHITE_SPACE(*pStart)) pStart++;
    if (pStart > pEnd) break;
    Added    = TRUE;
    Used     = p - Buffer;
    pBestEnd = NULL;
    while (pStart <= pEnd && Used<MaxLineLength )
    {
      if (InString)
      {
        if (WHITE_SPACE(*pStart)) *p++ = ' ';
                             else *p++ = *pStart;
        if (*pStart=='"') InString = FALSE;
        pStart++;
      }
      else
      {
        if (*pStart == ' ')
        {
          pBestEndp = p;
          pBestEnd  = pStart;
        }
        else
        {
          if (WHITE_SPACE(*pStart))
          {
            pBestEnd = NULL;
            break;
          }
          if (*pStart=='"') InString = TRUE;
        }
        *p++ = *pStart++;
      }
      Used++;
    }
    if (pBestEnd && !InString && pStart<pEnd)
    {
      p      = pBestEndp;
      pStart = pBestEnd;
    }
                                       /* Output the line             */
    if (Added)
    {
      CALLOUTFN(Parm,Buffer,p-Buffer);
      if (rc == MFRC_STOP) goto MOD_EXIT;
    }
  }
MOD_EXIT:
  return rc;
}
/**********************************************************************/
/* Function : PrintTag                                                */
/* Purpose  : Print out a tag                                         */
/**********************************************************************/
int  PrintTag(int        (outfn)(void *,CBOPT *,char *,int ),
              void              * Parm,
              unsigned char     * pStartOfData,
              CBOPT             * pOptions,
              int                 IndentSp,
              TAG               * pTag)
{
  int  rc        = 0;
  char * p = Buffer;

  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  if (pTag -> IsTagEnd)
  {
    if (pOptions->Options & MF_XML_SHORTFORM) goto MOD_EXIT;
    IndentSp -= INDENT_AMOUNT;
  }

  if (pTag->TagTitleLength > 80 || (pOptions->Options & MF_XML_SHORTFORM))
  {
    /******************************************************************/
    /* Just print out the tag name                                    */
    /******************************************************************/
    if (FORMATTING)
    {
      if (pOptions->Options & MF_OFFSET)
      {
        sprintf(p,"+%8.8X ",pTag->TagStart-pStartOfData);
        p += 10;
      }
      memset(p,' ',IndentSp);
      p += IndentSp;

      if (!(pOptions->Options & MF_XML_SHORTFORM)) *p++ = '<';

      memcpy(p,pTag->TagName,pTag->TagNameLength);
      p+=pTag->TagNameLength;

      if (pOptions->Options & MF_XML_SHORTFORM)
      {
#ifdef NOTUSED
        int Spaces = XmlTitleLength - pTag->TagNameLength;
        if (Spaces > 0)
        {
          memset(p,' ',Spaces);
          p+=Spaces;
        }
#endif
                                         /* Put on a colon              */
        *p++ = ':';
        pOptions->OutFormat |= CSF_HEADER;
      }
    }
    CALLOUTFN(Parm,Buffer,p-Buffer);
    pOptions->OutFormat &= ~CSF_HEADER;

    rc = PrintXmlBit(outfn,
                     Parm,
                     pStartOfData,
                     pOptions,
                     IndentSp+INDENT_AMOUNT,
                     pTag->TagData,
                     pTag->TagEnd-1);
    if (rc == MFRC_STOP) goto MOD_EXIT;

    if (!(pOptions->Options & MF_XML_SHORTFORM))
    {
      p = Buffer;
      if (FORMATTING)
      {
        if (pOptions->Options & MF_OFFSET)
        {
          sprintf(p,"+%8.8X ",pTag->TagStart-pStartOfData);
          p += 10;
        }
        memset(p,' ',IndentSp);
        p += IndentSp;
        *p++ = '>';
      }
      CALLOUTFN(Parm,Buffer,p-Buffer);
    }
  }
  else
  {
    if (FORMATTING)
    {
      if (pOptions->Options & MF_OFFSET)
      {
        sprintf(p,"+%8.8X ",pTag->TagStart-pStartOfData);
        p += 10;
      }
      memset(p,' ',IndentSp);
      p += IndentSp;

      memcpy(p,pTag->TagTitle,pTag->TagTitleLength);
      p+=pTag->TagTitleLength;
    }
    CALLOUTFN(Parm,Buffer,p-Buffer);
  }
MOD_EXIT:
  return rc;
}
/**********************************************************************/
/* Function : PrintXmlPiece                                           */
/* Purpose  : Print out a nested bit of XML                           */
/**********************************************************************/
int  PrintXmlPiece(int                 (outfn)(void *,CBOPT *,char *,int ),
                   void              * Parm,
                   unsigned char     * pStartOfData,
                   unsigned char     * pStartOfSection,
                   MQLONG              SectionLength,
                   CBOPT             * pOptions,
                   int                 IndentSp,
                   TAG               * TagKey,
                   MQLONG            * pMsgLen,
                   unsigned char    ** pp)
{
  int           rc = 0;
  MQLONG        MsgLen;
  unsigned char  * p;
  unsigned char * pStart = NULL;
  unsigned char * pEnd;
  BOOL          Incomplete = FALSE;
  BOOL          BadFormed  = FALSE;
  unsigned char * pTagStart = NULL;
  TAG           Tag;

  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  MsgLen = *pMsgLen;
  p      = *pp;

  if (!MsgLen) Incomplete = TRUE;
  while (MsgLen > 0)
  {
    while (Always)
    {
      /****************************************************************/
      /* Find the next tag                                            */
      /****************************************************************/
      pStart    = p;
      pTagStart = memchr(p,'<',MsgLen);
      if (!pTagStart)
      {
        Incomplete = TRUE;
        goto MOD_EXIT;
      }
      /****************************************************************/
      /* Any bits between the tags                                    */
      /****************************************************************/
      rc = PrintXmlBit(outfn,
                       Parm,
                       pStartOfData,
                       pOptions,
                       IndentSp+INDENT_AMOUNT,
                       pStart,
                       pTagStart-1);
      MsgLen -= (pTagStart-p);
      if (rc == MFRC_STOP) goto MOD_EXIT;

      pEnd   = pTagStart;
      p      = pTagStart;
      pStart = pTagStart;

      if (!GetTagName(&MsgLen,&pEnd,pOptions,&Tag))
      {
        Incomplete = TRUE;
        goto MOD_EXIT;
      }
      /****************************************************************/
      /* Ok, this is the end, it really should be the end for the one */
      /* we already have                                              */
      /****************************************************************/
      if (Tag.IsTagEnd)
      {
        if (TagKey->TagNameLength != Tag.TagNameLength ||
            memcmp(TagKey->TagName,Tag.TagName,Tag.TagNameLength))
        {
                                       /* Oh dear wrong tag           */
          BadFormed = TRUE;
          goto MOD_EXIT;
        }
      }
      p = pEnd;
                                       /* Print out this tag          */
      if ((pOptions->Options & MF_DETAIL)>=1)
      {
        rc = PrintTag(outfn,
                      Parm,
                      pStartOfData,
                      pOptions,
                      IndentSp+INDENT_AMOUNT,
                     &Tag);
        if (rc == MFRC_STOP) goto MOD_EXIT;
      }
                                       /* We need do no more          */
      if ( Tag.IsTagEnd) goto MOD_EXIT;
                                       /* Not single                  */
      if (!Tag.IsTagSingleton) break;
    }
    /******************************************************************/
    /* Is there anything in the middle                                */
    /******************************************************************/
    if (pTagStart != pStart)
    {
      rc = PrintXmlBit(outfn,
                       Parm,
                       pStartOfData,
                       pOptions,
                       IndentSp+INDENT_AMOUNT,
                       pStart,
                       pTagStart-1);
      MsgLen -= (pTagStart-p);
      if (rc == MFRC_STOP) goto MOD_EXIT;
    }
    /******************************************************************/
    /* This is a new tag                                              */
    /******************************************************************/
    rc = PrintXmlPiece(outfn,
                       Parm,
                       pStartOfData,
                       pStartOfSection,
                       SectionLength,
                       pOptions,
                       IndentSp+INDENT_AMOUNT,
                       &Tag,
                       &MsgLen,
                       &p);
    if (rc == MFRC_STOP) goto MOD_EXIT;
  }

MOD_EXIT:
  MsgLen   = SectionLength - (p - pStartOfSection);
  if (!rc && (Incomplete || BadFormed))
  {
    if (Incomplete)
    {
      if (FORMATTING) strcpy(Buffer,"** XML Incomplete **");
    }
    else
    {
      if (FORMATTING) strcpy(Buffer,"** XML Badly formed **");
    }
    pOptions->OutFormat |= CSF_HEADER;
    CALLOUTFNX(Parm,Buffer,(int )strlen(Buffer));
    pOptions->OutFormat &= ~CSF_HEADER;
    /****************************************************************/
    /* We shouldn't really end up here with data left but if we do  */
    /* dump it out                                                  */
    /****************************************************************/
    rc = DumpString (outfn,
                     Parm,
                     pStartOfData,
                     pOptions,
                     IndentSp,
                     20,
                     MsgLen,
                     p );
    p     += MsgLen;
    MsgLen = 0;
  }

  *pp      = p;
  *pMsgLen = MsgLen;
  return rc;
}
/**********************************************************************/
/* Function : PrintXml                                                */
/* Purpose  : Print out a sequence of XML                             */
/**********************************************************************/
int  PrintXml   (MQSCHAIN * pChain,
                 int        (outfn)(void *,CBOPT *,char *,int ),
                 void     * Parm,
                 unsigned char     * pStartOfData,
                 MQLONG     OriginalMsgLen,
                 CBOPT    * pOptions,
                 int        IndentSp,
                 int        Sectioned,
                 MQLONG   * pMsgLen,
                 unsigned char    ** pp)
{
  int           rc = 0;
  MQLONG        MsgLen;
  unsigned int  Length;
  MQLONG        SectionLength = 0;
  MQLONG        OriginalSectionLength = 0;
  unsigned char * p;
  /*  int           NewArray = 0;       */
  /*  unsigned char * Lastp    = NULL;  */
  MQFIELD     * pField = &XmlLength;
  unsigned char * pTagName;
  unsigned char * pSection = NULL;
  TAG           Tag;
  int           i;
  BOOL          Incomplete = FALSE;
  BOOL          BadFormat  = FALSE;

  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  MsgLen = *pMsgLen;
  p      = *pp;

  memset(&Tag,0,sizeof(Tag));
  /********************************************************************/
  /* Initialise the XML Strings etc                                   */
  /********************************************************************/
  if (!XmlInit)
  {
    XmlInit = 1;
    for (i=0; i<sizeof(XMLStrings)/sizeof(XMLStrings[0]) ; i++)
    {
      XMLStrings[i].TagLength = strlen(XMLStrings[i].Xml);
      XMLStrings[i].Length    = strlen(XMLStrings[i].ShortName);
      if (XMLStrings[i].Length < 2) CSTrap(XMLStrings[i].Length);

      XmlTitleLength = max(XmlTitleLength,XMLStrings[i].Length);
    }
  }

  while (MsgLen > 0)
  {
    /******************************************************************/
    /* Ok, the first thing we must have is a length of the variable   */
    /******************************************************************/
    if (Sectioned)
    {
                                       /* Add integer value           */
      SectionLength = GetValue(pField,(char *)p,pChain->Encoding);
      if ((pOptions->Options & SF_DETAIL) > 2)
      {
        if (FORMATTING)
        {
          pOptions->OutFormat |= CSF_HEADER;
          strcpy(Buffer,pField -> Name);
          Length = strlen(Buffer);
          sprintf(&Buffer[Length],"%d",SectionLength);
        }
        CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
        pOptions->OutFormat &= ~CSF_HEADER;
      }
      p      += sizeof(MQLONG);
      MsgLen -= sizeof(MQLONG);
    }
    else
    {
      SectionLength = MsgLen;
    }
    if (SectionLength >= 0) SectionLength = min(MsgLen,SectionLength);
                      else  SectionLength = MsgLen;
    OriginalSectionLength = SectionLength;
    pSection = p;

    while (SectionLength > 0)
    {
      while(Always)
      {
        /****************************************************************/
        /* Find the next tag                                            */
        /****************************************************************/
        pTagName = memchr(p,'<',SectionLength);
        if (!pTagName)
        {
          /**************************************************************/
          /* Now then the remaining chars are probably just blanks      */
          /**************************************************************/
          for (i=0 ; i<SectionLength ; i++) if (!WHITE_SPACE(p[i])) break;
          if (i==SectionLength)
          {
            SectionLength = 0;
            break;
          }
          /**************************************************************/
          /* Nope...looks like we have real data                        */
          /**************************************************************/
          BadFormat = TRUE;
          goto MOD_EXIT;
        }

        /****************************************************************/
        /* Ok, we have a new tag                                        */
        /****************************************************************/
        if (pTagName != p)
        {
          rc = PrintXmlBit(outfn,
                           Parm,
                           pStartOfData,
                           pOptions,
                           IndentSp+INDENT_AMOUNT,
                           p,
                           pTagName-1);
          SectionLength -= (pTagName-p);
          if (rc == MFRC_STOP) goto MOD_EXIT;
          p = pTagName;
        }

        if (!GetTagName(&SectionLength,&p,pOptions,&Tag))
        {
          Incomplete = TRUE;
          goto MOD_EXIT;
        }

        rc = PrintTag(outfn,
                      Parm,
                      pStartOfData,
                      pOptions,
                      IndentSp+INDENT_AMOUNT,
                     &Tag);
        if (rc == MFRC_STOP) goto MOD_EXIT;

        if (!Tag.IsTagSingleton) break;
      }
      if (!SectionLength) break;
                                         /* Shouldn't be the end tag    */
      if (Tag.IsTagEnd)
      {
        BadFormat = TRUE;
        goto MOD_EXIT;
      }
                                         /* Ok, parse this piece        */
      rc = PrintXmlPiece(outfn,
                         Parm,
                         pStartOfData,
                         pSection,
                         OriginalSectionLength,
                         pOptions,
                         IndentSp+INDENT_AMOUNT,
                        &Tag,
                         &SectionLength,
                         &p);
      if (rc == MFRC_STOP) goto MOD_EXIT;
    }
    MsgLen -= OriginalSectionLength;
    p       = pSection + OriginalSectionLength;
    SectionLength         = 0;
    OriginalSectionLength = 0;
  }

MOD_EXIT:
  if (rc == 0)
  {
    MQLONG Used = (OriginalSectionLength - SectionLength);
    MsgLen -= Used;
    p       = pSection + Used;
    if (MsgLen)
    {
      if (Incomplete)
      {
        if (FORMATTING) strcpy(Buffer,"** XML Incomplete **");
        pOptions->OutFormat |= CSF_HEADER;
        CALLOUTFNX(Parm,Buffer,(int )strlen(Buffer));
        pOptions->OutFormat &= ~CSF_HEADER;
      }
      else if (BadFormat)
      {
        if (FORMATTING) strcpy(Buffer,"** Unrecognised XML Format **");
        pOptions->OutFormat |= CSF_HEADER;
        CALLOUTFNX(Parm,Buffer,(int )strlen(Buffer));
        pOptions->OutFormat &= ~CSF_HEADER;
      }
      /****************************************************************/
      /* We shouldn't really end up here with data left but if we do  */
      /* dump it out                                                  */
      /****************************************************************/
      rc = DumpString (outfn,
                       Parm,
                       pStartOfData,
                       pOptions,
                       IndentSp,
                       20,
                       MsgLen,
                       p );
      MsgLen = 0;
      p     += MsgLen;
    }
  }

  *pp      = p;
  *pMsgLen = MsgLen;
  return rc;
}

int OutputString(MQSCHAIN * pChain,
                 CBOPT    * pOptions,
                 int       (outfn)(void *,CBOPT *,char *,int ),
                 void     * Parm,
                 char     * Buffer,
                 int        Length,
                 MQFIELD  * pField,
                 int        FieldLength,
                 char     * p)
{
  int  left,len,rc=0;
  int  maxlen = pOptions->ScreenWidth - Length - 2;
  int  offset = 0;
  if (maxlen<1) maxlen = 1;

  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
  left = FieldLength;

  while (left > 0)
  {
    len = left;
    if (len>maxlen) len=maxlen;

    if (FORMATTING)
    {
      Buffer[Length]='\'';
      pOptions->LineMsgOffset     = p + offset - (char *)pChain->StartOfData;
      pOptions->LineMsgLength     = len;
      pOptions->LineDataOffset    = Length+1;
      pOptions->LineDataLength    = len;
      pOptions->LineDataMaxLength = maxlen;
           if (pChain->Ebcdic && 'A'==0x41)    CSToAscii (&Buffer[Length+1],p+offset,len);
      else if (pChain->Ebcdic==0 && 'A'==0xC1) CSToEbcdic(&Buffer[Length+1],p+offset,len);
                                          else CopyHexStr(&Buffer[Length+1],p+offset,len);
      Buffer[Length+len+1] = '\'';
      Buffer[Length+len+2]  = 0;
    }
    left   -= len;
    offset += len;

    /* Add formating of single character flags */
    if (pField -> Type == MQFT_CHAR1F) {
        unsigned long Data     = 0;
        if (*(p+offset) == 0x00) {
            Data = 0x00;
        } else {
            Data = (unsigned long) Buffer[Length+len];
        }
        AddValues(pField,Data,Buffer,pOptions);
    }

    CALLOUTFN(Parm,Buffer,Length+len+2);

    if (left)
    {
      memset(Buffer,' ',Length);
    }
  }
MOD_EXIT:
  return rc;
}
/**********************************************************************/
/* Function : PrintStruct                                             */
/* Purpose  : Print out a structure                                   */
/**********************************************************************/
int  PrintStruct(MQSTRUCT          * pStruct,
                 MQSCHAIN          * pChain,
                 int                 (outfn)(void *,CBOPT *,char *,int ),
                 void              * Parm,
                 CBOPT             * pOptions,
                 MQLONG            * pMsgLen,
                 unsigned char    ** pp)
{
  int              rc = 0;
  MQFIELDS       * pFields;
  MQFIELD        * pField;
  MQLONG           MsgLen;
  MQSCHAIN         Chain;
  unsigned int     Length;
  unsigned int     Data     = 0;
  unsigned char  * p;
  unsigned int     Index    = 0;
  unsigned int     Array    = 0;
  int              NewArray = 0;
  unsigned char  * Lastp    = NULL;
  unsigned int     FieldLength;
  int              FirstField;
  char             DateTime[20];

  int           deferredEncoding = gEncoding;
  static unsigned long FieldOffset = 0;
  pOptions->OutFormat = pOptions->Options & MF_NOFORMAT ? 0 : CSO_FORMATTED;
                                       /* Initialise offsets          */
  if (!Initialised)
  {
    InitialiseOffsets();

    if (!Buffer) Buffer = CSGetMem(MAX_LINE_SIZE,"StructLine");
    if (!Buffer) return 0;
  }

  if (!pOptions->Checked) CSCheckOpts(pOptions);

    /* If first time in for a new structure, initialize field offsets */
    if (pChain == NULL) FieldOffset = 0;

  /* Allow parameter to reset the encoding back to machine format */
  if (pOptions->Options & MF_ENCLOCAL)
  {
     pOptions->Options   &= ~MF_ENCLOCAL;
     gEncoding   = CSENC_LOCAL;
  } else if (pOptions->Options & MF_ENCBYTESWAP)
  {
     pOptions->Options   &= ~MF_ENCBYTESWAP;
     gEncoding   = CSENC_NONLOCAL;
  }

  /* Allow parameter to reset the encoding back to machine format */
  if (pOptions->Options & MF_ENCASCII)
  {
    pOptions->Options   &= ~MF_ENCASCII;
    gEbcdic     = 0;
  } else if (pOptions->Options & MF_ENCEBCDIC)
  {
    pOptions->Options   &= ~MF_ENCEBCDIC;
    gEbcdic     = 1;
  }

  MsgLen            = *pMsgLen;
  p                 = *pp;

  /* Have we been given a growable area ?                             */
  if (MsgLen == -1 && pOptions ->pData)
  {
    if (p == (unsigned char *)pOptions->pData->Data)
    {
      MsgLen = pOptions->pData->Length;
    }
  }
  Chain.Prev        = pChain;
  Chain.Struct      = pStruct;
  Chain.Start       = p;
  Chain.MsgLen      = MsgLen;
  Chain.StrucLength = 0;
  Chain.Encoding    = gEncoding;
  Chain.Ebcdic      = gEbcdic;

  if (pChain)
    Chain.StartOfData = pChain -> StartOfData;
  else
  {
//  pOptions -> FieldStartOfData = p;
    Chain.StartOfData            = p;
  }

  while (pStruct)
  {
    /* CheckStack(&Chain); */

    FirstField = TRUE;
                                       /* Get to the fields           */
    pFields = pStruct -> Fields;
    while (Always)
    {
      /****************************************************************/
      /* If we've run out of data then quit                           */
      /* This check has been removed because we need a 0 length       */
      /* message to call the callback function at least once          */
      /****************************************************************/
      //if (!MsgLen) goto MOD_EXIT;

NEXTFIELD:
      pField = pFields -> Field;
      if (!pField) break;
                                       /* Is this field present ?     */
      if (pFields -> Depend && pField -> Type != MQFT_ARRAY)
      {
        if (!CheckDepend(&Chain,pFields -> Depend))
        {
                                       /* Check whether whole struct  */
          if (pFields -> Attrs & SF_DEPSTRUCT) goto MOD_EXIT;
                                       /* Nope....skip to next field  */
          pFields++;
          goto NEXTFIELD;
        }
      }
                                       /* Is this optional ?          */
      if (pFields -> Attrs & SF_OPT)
      {
        if (MsgLen < pField -> Length || MsgLen == 0)
        {
                                       /* Don't seem to have it       */
          pFields++;
          goto NEXTFIELD;
        }
      }

      Data      = 0;
      Buffer[0] = 0;
      /****************************************************************/
      /* Initialise the output structure                              */
      /****************************************************************/
      pOptions -> pField            = NULL;
      pOptions -> FieldMsgOffset    = p - Chain.StartOfData;
      pOptions -> LineMsgOffset     = p - Chain.StartOfData;
      pOptions -> LineDataOffset    = 0;
      pOptions -> LineDataMaxLength = 0;
      /****************************************************************/
      /* Ok, do we print out a structure header                       */
      /****************************************************************/
      if (FirstField                                        &&
          (!Chain.Prev       || (pOptions->Options & MF_DETAIL) > 1) &&
          pStruct -> Name[0]                                &&
          pStruct -> Name[0] != '!')
      {
        int Display = 1;
        FirstField = FALSE;

        if (pStruct->StrucAttrs & SF_DETAIL)
        {
          int Detail = pStruct->StrucAttrs & SF_DETAIL;

          if (Detail > (pOptions->Options & MF_DETAIL)) Display = 0;
        }

        if (Display)
        {
          if (FORMATTING)
          {
           pOptions->OutFormat |= CSF_HEADER;
           if (MsgLen > 0) sprintf(Buffer,"[%5d bytes] %s",MsgLen, pStruct->Name);
                       else sprintf(Buffer,"%s", pStruct->Name);
          }
          CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
          pOptions->OutFormat &= ~CSF_HEADER;
        }
      }

      while (Index <= Array)
      {
        /* CheckStack(&Chain); */
                                     /* Copy name of field          */
        if (FORMATTING)
        {
          if (Array) sprintf(Buffer,"[%3d] %s",Index,pField -> Name);
                else strcpy(Buffer,pField -> Name);
          Length = strlen(Buffer);

          if (pOptions->Options & MF_FIELDOFFSET) {
              sprintf(&Buffer[Length-1]," {+%3.3X}:",FieldOffset);      /* MEE */
              Length = strlen(Buffer);
          }

        }
        else
          Length = 0;
                                     /* How long is the field ?     */
        if (pField->Type   == MQFT_CHARN)
        {
          if (pField->Value)
            FieldLength = GetValueIndirect(&Chain,(MQFIELD *)pField->Value) +
                          pField -> Length;
          else
                                       /* NULL field means whatevers left */
            FieldLength = -1;
        }
        else if (pField->Type   == MQFT_HEXN )
          FieldLength = GetValueIndirect(&Chain,(MQFIELD *)pField->Value) +
                        pField -> Length;
        else
          FieldLength = pField -> Length;
                                     /* If -1 then it's all of it   */
        if (FieldLength == -1)
        {
          if (Chain.StrucLength)
          {

            int len = p - ((unsigned char *)Chain.Start);
            FieldLength = Chain.StrucLength - len;
          }
          else
            FieldLength = MsgLen;
        }
                                     /* Do we have enough left      */
        if (MsgLen > 0 && MsgLen < (int )FieldLength)
        {
          pOptions -> pField = NULL;
          CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
          if (FORMATTING) strcpy(Buffer,"** Field Incomplete **");
          CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
          rc = MFRC_STOP;
          goto MOD_EXIT;
        }
                                      /* Set up default output flds  */
        pOptions -> pField            = pField;
        pOptions -> FieldType         = 0;
        pOptions -> FieldMsgOffset    = p - Chain.StartOfData;
        pOptions -> LineMsgOffset     = pOptions->FieldMsgOffset;
        pOptions -> LineMsgLength     = FieldLength;
        pOptions -> LineDataLength    = FieldLength;
        pOptions -> LineDataOffset    = 0;
                                      /* Add hex string              */
        if (pOptions->Options & MF_HEXBITS     &&
            FORMATTING                &&
            FieldLength   <= 8        &&
            pField->Type  != MQFT_MSG   &&
            pField->Type  != MQFT_DUMP  &&
            pField->Type  != MQFT_HEX   &&
            pField->Type  != MQFT_HEXN  &&
            pField->Type  != MQFT_CHAR  &&
            pField->Type  != MQFT_DATE  &&
            pField->Type  != MQFT_TIME  &&
            pField->Type  != MQFT_CHARN)
        {
          HexStrn(p,FieldLength,&Buffer[Length],16);
          Length+= 16;
          Buffer[Length++] = ':';
        }

        pOptions -> LineDataOffset = (short)Length;
        pOptions -> LineDataLength = 0;

        if ((pField ->Type == MQFT_DUMPSTR || pField ->Type == MQFT_MSG) &&
            (pOptions->Options & MF_AUTO_DETECT_XML))
        {
          int i,IsXml = 0;
          /************************************************************/
          /* Ok, we're looking at a message let's see whether the     */
          /* first few characters are what we want                    */
          /************************************************************/
          for (i = 0; i<min(10,MsgLen) ; i++)
          {
            if (p[i] == '<') IsXml = 1;
            if (!WHITE_SPACE(p[i])) break;
          }
          if (IsXml) pField = &FieldXmlMsg;
        }

        switch(pField -> Type)
        {
          case MQFT_HEXN:
               pField = pField;
          case MQFT_HEX:
          case MQFT_HEXCHAR:
                                       /* Print out hex string        */
               if (DETAILOK)
               {
                 if (pOptions->Options & MF_HEXCH && FieldLength<50)
                 {
                   if (FORMATTING)
                   {
                     unsigned int i,ascii_count,ebcdic_count;
                     memset(&Buffer[Length],' ',FieldLength*2);
                     ascii_count = 0;
                     for (i=0 ;i<FieldLength ; i++)
                     {
                       if (isalnum(*(p+i)) || *(p+i)==' ')
                         ascii_count ++;

                       if (isprint(*(p+i)))
                         Buffer[Length+2*i] = *(p+i);
                       else
                         Buffer[Length+2*i] = '.';
                     }

                     pOptions->LineDataLength = FieldLength*2;

                     if (pOptions->Options & MF_HEXCH_PREDICT)
                     {
                       /***********************************************/
                       /* Let's just see whether it's in EBCDIC       */
                       /***********************************************/
                       unsigned char field[50];
                       unsigned char outfield[100];
                       memcpy(field   ,p  ,FieldLength);
                       memset(outfield,' ',FieldLength*2);
                       if ('A'==0xC1) CSToEbcdic(field,field,FieldLength);
                       else CSToAscii (field,field,FieldLength);
                       ebcdic_count = 0;
                       for (i=0 ;i<FieldLength ; i++)
                       {
                         if (isalnum(field[i]) || field[i]==' ')
                           ebcdic_count ++;

                         if (isprint(field[i]))
                           outfield[2*i] = field[i];
                         else
                           outfield[2*i] = '.';
                       }
                       if (ebcdic_count > ascii_count)
                       {
                                       /* Looks like ebcdic           */
                         memcpy(&Buffer[Length],outfield,2*FieldLength);
                       }
                     }

                     Buffer[Length+FieldLength*2] = 0;
                   }
                   CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   memset(Buffer,' ',Length);
                 }
                 if (FORMATTING)
                 {
                   HexStrn(p,FieldLength,&Buffer[Length],0);
                   pOptions->LineDataLength = FieldLength*2;
                 }
                 CALLOUTFN(Parm,Buffer,(int)strlen(Buffer));

                 /* Output hex and str formats of the fields */
                 if (pField -> Type == MQFT_HEXCHAR)
                 {
                     Length = 14;
                     if (pOptions->Options & MF_FIELDOFFSET) {
                         Length = Length + 7;
                     }
                     memset(Buffer,' ',Length);
                     pOptions -> LineMsgLength  = FieldLength;
                     pOptions -> LineDataOffset = Length+1;
                     pOptions -> LineDataLength = FieldLength;
                     pOptions -> FieldType      = MQFT_CHAR;
                     Buffer[Length]='\'';
                     Length ++;
                          if (Chain.Ebcdic && 'A'==0x41)    CSToAscii  (&Buffer[Length],p,FieldLength);
                     else if (Chain.Ebcdic==0 && 'A'==0xC1) CSToEbcdic (&Buffer[Length],p,FieldLength);
                     else                                   CopyHexStr (&Buffer[Length],p,FieldLength);
                     /* Data = (unsigned long) Buffer[Length]; */
                     Length += FieldLength;
                     Buffer[Length++] = '\'';
                     Buffer[Length]   = 0;
                     CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                 }
               }
               break;
          case MQFT_HEX4:
                                       /* Add value                   */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"0x'%X'",Data);
                   pOptions->LineDataOffset = Length+3;
                   pOptions->LineDataLength = strlen(&Buffer[Length+3])-1;
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
               break;
          case MQFT_BLONG:
                                       /* Big-endian long             */
               Data = GetValue(pField,p,CSENC_NORMAL);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%d",Data);
                   pOptions->LineDataLength = strlen(&Buffer[Length]);
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
               break;
          case MQFT_LONG:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);

               /* If the version field is byte reversed, redo the encoding */
               if (pFields->Attrs & SF_VERSION &&
                   ((Data < 0) || (Data > 0xFFFFFF)))
               {
                 if (!(pOptions->Options2 & MF2_NO_BYTESWAP_VERSION))
                 {
                   /* Swap encoding */
                   if (Chain.Encoding != CSENC_LOCAL) {
                       Chain.Encoding = CSENC_LOCAL;
                   } else {
                       Chain.Encoding = CSENC_NONLOCAL;
                   }

                   /* Get the version field again */
                   Data = GetValue(pField,p,Chain.Encoding);
                 }
               }


               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%d",Data);
                   pOptions->LineDataLength = strlen(&Buffer[Length]);
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
               break;
          case MQFT_LONGH:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 int          Type = MQFT_LONGH;
                 unsigned int val;
                 if (pFields -> Attrs & SF_PARMVALUE)
                 {
                   val = GetValueIndirect (&Chain,&ParameterId);
                   if (val == MQIACF_OPEN_OPTIONS) Type = MQFT_FLAG4;
                 }
                 if (FORMATTING)
                 {
                   if (Type == MQFT_FLAG4)
                   {
                     sprintf(&Buffer[Length],"%8.8X",Data);
                   }
                   else
                   {
                     sprintf(&Buffer[Length],"%d [0x'%X']",Data,Data);
                     if (pFields -> Attrs & SF_PARMVALUE && val)
                     {
                       AddParmValue(val,Data,Buffer);
                     }
                   }
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));

                 if (pFields -> Attrs & SF_PARMVALUE)
                 {
                   if (Length == 0) Length = 14;
                   switch(val)
                   {
                     case MQIACF_OPEN_OPTIONS:
                          rc = AddDeferredParmValue(outfn,Parm,&OpenOptions,Data,Length,pOptions);
                          if (rc == MFRC_STOP) goto MOD_EXIT;
                          break;
                     default:
                          break;
                   }
                 }
               }
               break;
          case MQFT_LONGH64:
                                       /* Add integer value           */
               {
                 MQINT64          Data64;
                 Data64 = GetValue64(pField,p,Chain.Encoding);
                 if (DETAILOK)
                 {
                   if (FORMATTING)
                   {
                     sprintf(&Buffer[Length],"%I64d [0x'%I64X']",Data64,Data64);
                   /* if (pField -> Value) AddValues(pField,Data,Buffer,pOptions); */
                   }
                   CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                 }
               }
               break;
          case MQFT_SHORT:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%hd",Data);
                   pOptions->LineDataLength = strlen(&Buffer[Length]);
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
               break;
          case MQFT_BYTE:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%d",Data);
                   pOptions->LineDataLength = strlen(&Buffer[Length]);
                   if (pField -> Value) AddValues(pField,Data,Buffer,pOptions);
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
               break;
          case MQFT_FLAG1:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%2.2X",Data);
                   pOptions->LineDataLength = 2;
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                 if (pField -> Value)
                 {
                   rc = AddFlags(outfn,Parm,pField,Data,Length,pOptions);
                   if (rc == MFRC_STOP) goto MOD_EXIT;
                 }
               }
               break;
          case MQFT_FLAG2:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%4.4X",Data);
                   pOptions->LineDataLength = 4;
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                 if (pField -> Value)
                 {
                   rc = AddFlags(outfn,Parm,pField,Data,Length,pOptions);
                   if (rc == MFRC_STOP) goto MOD_EXIT;
                 }
               }
               break;
          case MQFT_FLAG4:
                                       /* Add integer value           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                 {
                   sprintf(&Buffer[Length],"%8.8X",Data);
                   pOptions->LineDataLength = 8;
                 }
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                 if (pField -> Value)
                 {
                   pOptions ->pField = NULL;
                   rc = AddFlags(outfn,Parm,pField,Data,Length,pOptions);
                   if (rc == MFRC_STOP) goto MOD_EXIT;
                 }
               }
               break;

          case MQFT_CHARV:
                                       /* This is risky - it 'assumes' that the data is a full */
                                       /* MQCHARV.                                             */
               FieldLength = sizeof(MQCHARV);

               if (DETAILOK)
               {
                 MQCHARV * pCharv = (MQCHARV *) p;
                 char    * pValue;

                 if (gEncoding != CSENC_LOCAL)
                 {
                   CSByteSwapLong((unsigned char *)&pCharv->VSOffset,4);
                 }

                      if (pCharv->VSOffset) pValue = Chain.Start + pCharv->VSOffset;
                 else if (pCharv->VSPtr)    pValue = pCharv->VSPtr;
                 else pValue = NULL;

                 Buffer[Length] = 0;
                 if (!pValue) strcpy(&Buffer[Length],"NULL");
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));

                 if ((pOptions->Options & MF_DETAIL)>1)
                 {
                   if (FORMATTING)
                     sprintf(Buffer,"   VSPtr     :%p", pCharv -> VSPtr);
                   CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   if ((pOptions->Options & MF_DETAIL)>2)
                   {
                     if (FORMATTING)
                       sprintf(Buffer,"   VSOffset  :%d", pCharv -> VSOffset);
                     CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                     if (FORMATTING)
                       sprintf(Buffer,"   VSBufSize :%d", pCharv -> VSBufSize);
                     CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   }
                   if (FORMATTING)
                     sprintf(Buffer,"   VSLength  :%d", pCharv -> VSLength);
                   CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   if ((pOptions->Options & MF_DETAIL)>2)
                   {
                     if (FORMATTING)
                       sprintf(Buffer,"   VSCCSID   :%d", pCharv -> VSCCSID);
                     CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   }
                 }
                 if (!(pOptions->Options & MF_NOFOLLOWPTR) || pCharv->VSOffset)
                 {
                   if (pValue)
                   {
                     strcpy(Buffer,"   Value     :");
                     rc = OutputString(&Chain,pOptions,outfn,Parm,Buffer,13,pField,
                                       min(pCharv->VSLength,pCharv->VSBufSize),pValue);
                     if (rc == MFRC_STOP) goto MOD_EXIT;
                     if (pCharv->VSLength)
                     {
                       Lastp =  pValue + ROUNDUP4(pCharv->VSLength);
                     }
                   }
                 }
               }
               break;

          case MQFT_MSG:
          case MQFT_DUMP:
               if (DETAILOK)
               {
                 int hexopts = 0;
                 if (pOptions->Options&MF_HEXSEP) hexopts |= HO_SEPARATORS;
                 if (gEbcdic)                     hexopts |= HO_TEXTISEBCDIC;

                 if (FieldLength)
                 {
                   int  i;
                   int  len;
                   int  maxlen = pOptions->DataWidth;

                   for (i=0 ; i<(int )FieldLength ; i+=maxlen)
                   {
                     len = FieldLength-i;
                     if (len>maxlen) len=maxlen;
                     if (FORMATTING)
                     {
                       char * pData = Buffer;

                       if (pOptions->Options & MF_OFFSET)
                       {
                         sprintf(pData,"+%8.8X ",i+(p-Chain.StartOfData));
                         pData += 10;
                         pOptions -> LineDataOffset = 10;
                       }
                       else
                       {
                         pOptions -> LineDataOffset = 0;
                       }
                       pOptions -> LineMsgOffset     = i;
                       pOptions -> LineMsgLength     = len;
                       pOptions -> LineDataLength    = len*2;
                       pOptions -> LineDataMaxLength = maxlen*2;

                       HexStr( p+i,
                               len,
                               maxlen,
                               pData,
                               pOptions->AsciiColumn,
                               hexopts);
                     }
                     CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
                   }
                 }
               }
               break;

          case MQFT_DUMPSTR:
               if (DETAILOK)
               {
                 if (FieldLength || (pOptions->Options2 & MF2_ZERO_LENGTH_STRINGS))
                 {
                   rc = DumpString (outfn,
                                    Parm,
                                    Chain.StartOfData,
                                    pOptions,
                                    0,
                                    1,
                                    FieldLength,
                                    p );
                 }
               }
               break;

          case MQFT_FORMAT:
          case MQFT_CHAR:
          case MQFT_CHARN:
          case MQFT_CHAR1F:
               if (DETAILOK)
               {
                 rc = OutputString(&Chain,pOptions,outfn,Parm,Buffer,Length,pField,FieldLength,p);
                 if (rc == MFRC_STOP) goto MOD_EXIT;
               }
               if (pField -> Type == MQFT_FORMAT)
               {
                 int c;
                      if (Chain.Ebcdic && 'A'==0x41)  c = CSToAscii  (FormatName,p,FieldLength);
                 else if (!Chain.Ebcdic && 'A'==0xC1) c = CSToEbcdic (FormatName,p,FieldLength);
                 else c = CopyHexStr(FormatName,p,FieldLength);

                 if (c != FieldLength) memcpy(FormatName,p,8);
               }
               break;

          case MQFT_DATE:
               if (DETAILOK)
               {
                 if (pOptions->Options & MF_LOCAL_TIMES)
                 {
                   if (!Chain.Ebcdic && 'A'==0xC1) CSToEbcdic(DateTime,p,16);
                   else if ( Chain.Ebcdic && 'A'==0x41) CSToAscii (DateTime,p,16);
                   else CopyHexStr(DateTime,p,16);

                   CSAdjustTime(&DateTime[0],&DateTime[8],gTimeZone);
                 }
               }
                                       /* DELIBERATELY NO BREAK HERE  */
          case MQFT_TIME:
               if (FORMATTING && DETAILOK)
               {
                 Buffer[Length]='\'';
                 Length ++;
                 pOptions->LineDataOffset = Length;

                 if (pOptions->Options & MF_LOCAL_TIMES)
                 {
                   if (pField -> Type == MQFT_DATE)
                     memcpy(&Buffer[Length],&DateTime[0],FieldLength);
                   else
                     memcpy(&Buffer[Length],&DateTime[8],FieldLength);
                 }
                 else
                 {
                 if (!Chain.Ebcdic && 'A'==0xC1) CSToEbcdic(&Buffer[Length],p,FieldLength);
                 if ( Chain.Ebcdic && 'A'==0x41) CSToAscii (&Buffer[Length],p,FieldLength);
                                 else CopyHexStr(&Buffer[Length],p,FieldLength);
                 }
                 Length += FieldLength;
                 pOptions->LineDataLength = FieldLength;
                 Buffer[Length++] = '\'';
                 Buffer[Length]   = 0;
               }
               if (DETAILOK)
               {
                 CALLOUTFN(Parm,Buffer,Length);
               }
               break;
                                       /* Embedded structure          */
          case MQFT_STRUCT:
               rc = PrintStruct((MQSTRUCT *)pField->Value,
                                &Chain,
                                outfn,
                                Parm,
                                pOptions,
                                &MsgLen,
                                &p);
               if (rc == MFRC_STOP) goto MOD_EXIT;
               break;
                                       /* Call the XML Parser         */
          case MQFT_XML:
               {
                 int  Used = p - Chain.Start;
                 int  left = Chain.StrucLength - Used;
                 if (left < 0 || left > MsgLen)
                 {
                   left = MsgLen;
                   Chain.StrucLength = Chain.MsgLen;
                 }
                 MsgLen = left;
               }
               rc = PrintXml(&Chain,
                              outfn,
                              Parm,
                              Chain.StartOfData,
                              MsgLen,
                              pOptions,
                             -INDENT_AMOUNT,
                              TRUE,
                             &MsgLen,
                             &p);
               if (rc == MFRC_STOP) goto MOD_EXIT;
               break;

          case MQFT_XMLMSG:
               rc = PrintXml(&Chain,
                              outfn,
                              Parm,
                              Chain.StartOfData,
                              MsgLen,
                              pOptions,
                             -INDENT_AMOUNT,
                              FALSE,
                             &MsgLen,
                             &p);
                 goto MOD_EXIT;
               break;
                                       /* Lots of embedded structures */
          case MQFT_REPEAT:
               {
                 unsigned char * oldp;

                 do
                 {
                   oldp = p;

                   rc = PrintStruct((MQSTRUCT *)pField->Value,
                                    &Chain,
                                    outfn,
                                    Parm,
                                    pOptions,
                                    &MsgLen,
                                    &p);
                   if (rc == MFRC_STOP) goto MOD_EXIT;
                 }
                 while (oldp != p && MsgLen > 0);
               }
               break;
                                       /* Array item                  */
          case MQFT_ARRAY:
               if (pFields -> Depend)
               {
                 if (!CheckDepend(&Chain,pFields -> Depend))
                 {
                                       /* Nope....this and next field */
                   pFields+=2;
                   goto NEXTFIELD;
                 }
               }
               Array = GetValueIndirect(&Chain,(MQFIELD *)pField->Value);
               if (FORMATTING && DETAILOK)
               {
                 if (Array) CALLOUTFN(Parm,Buffer,Length);
               }
                                       /* Goto to next field          */
               pFields++;
               pField = pFields -> Field;
               NewArray = 1;
               break;
                                       /* Array item                  */
          case MQFT_FARRAY:
               if (pFields -> Depend)
               {
                 if (!CheckDepend(&Chain,pFields -> Depend))
                 {
                                       /* Nope....this and next field */
                   pFields+=2;
                   goto NEXTFIELD;
                 }
               }
               Array = pField->Length;
               if (FORMATTING && DETAILOK)
               {
                 if (Array) CALLOUTFN(Parm,Buffer,Length);
               }
                                       /* Goto to next field          */
               pFields++;
               pField = pFields -> Field;
               NewArray = 1;
               break;

          case MQFT_OSTR:
                                       /* Print out offset            */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                   sprintf(&Buffer[Length],"%d",Data);
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
                                       /* Offset to structure         */
               {
                 MQSTRUCT * pStruct = (MQSTRUCT *)pField -> Value;
                 int        i,Count = 1;
                 unsigned char * pStr    = Chain.Start + Data;
                 pFields ++;
                 pField = pFields -> Field;
                 /* if (!(pOptions->Options & MF_NOFOLLOWPTR)) */
                 {
                   if (pField && pField -> Type == MQFT_COUNT)
                   {
                     Count = GetValueIndirect(&Chain,(MQFIELD *)pField -> Value);
                   }
                   else
                     pFields--;
                                       /* Do we have the parameter ?  */
                   if (Data)
                   {
                     for (i=0 ; i<Count ; i++)
                     {
                       rc = PrintStruct(pStruct,
                                       &Chain,
                                        outfn,
                                        Parm,
                                        pOptions,
                                        &MsgLen,
                                        &pStr);
                       if (rc == MFRC_STOP) goto MOD_EXIT;
                     }
                                       /* Remember where we got to    */
                     if (pStr > Lastp) Lastp = pStr;
                   }
                 }
               }
               break;

          case MQFT_PSTR:
                                       /* Print out pointer           */
               Data = GetValue(pField,p,Chain.Encoding);
               if (DETAILOK)
               {
                 if (FORMATTING)
                   sprintf(&Buffer[Length],"%p",Data);
                 CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
               }
                                       /* Pointer to structure        */
               {
                 MQSTRUCT * pStruct = (MQSTRUCT *)pField -> Value;
                 int        i,Count = 1;
                 unsigned char     * pStr    = (unsigned char *)(ptrdiff_t)Data;
                 pFields ++;
                 pField = pFields -> Field;
                 if (!(pOptions->Options & MF_NOFOLLOWPTR))
                 {
                   if (pField -> Type == MQFT_COUNT)
                   {
                     Count = GetValueIndirect(&Chain,(MQFIELD *)pField -> Value);
                   }
                   else
                     pFields--;
                                       /* Do we have the parameter ?  */
                   if (!Data) Count = 0;

                   for (i=0 ; i<Count ; i++)
                   {
                     rc = PrintStruct(pStruct,
                                     &Chain,
                                      outfn,
                                      Parm,
                                      pOptions,
                                     &MsgLen,
                                     &pStr);
                     if (rc == MFRC_STOP) goto MOD_EXIT;
                   }
                 }
               }
               break;

          case MQFT_ALIGN:
               {
                 char * pEnd   = p;
                 FieldLength   = p - Chain.StartOfData;

                 FieldLength +=   pField->Length - 1;
                 FieldLength &= ~(pField->Length - 1);

                 pEnd        = Chain.StartOfData + FieldLength;
                 FieldLength = pEnd - (char *)p;
               }
               break;

          default:
               if (FORMATTING)
               {
                 sprintf(Buffer,"** INTERNAL ERROR ** Unknown field type %d\n",pField->Type);
                 pOptions->LineDataOffset = 41;
               }
               CALLOUTFN(Parm,Buffer,-1);
               rc = MFRC_STOP;
               goto MOD_EXIT;
               break;
        }
        if (!NewArray)
        {
          FieldOffset += FieldLength;
          p       += FieldLength;
          if (MsgLen < 0)
          {
                                       /* Do nothing if we don't know */
          }
          else if (MsgLen > (int )FieldLength)
          {
                                       /* Reduce the size we do know  */
            MsgLen  -= FieldLength;
          }
          else
          {
                                       /* Ok we've run out            */
            MsgLen   = 0;
          }
          Index   ++;
        }
        else
        {
          Index    = 1;
          NewArray = 0;
        }
                                       /* End of array loop           */
      }
      /****************************************************************/
      /* Did that field change the encoding                           */
      /*   - Note EPH header uses encoding of '0' so ignore that      */
      /****************************************************************/
      if (pFields -> Attrs & SF_ENCODING && Data)
      {
                                       /* Only want values 1 or 2     */
        deferredEncoding = Data & 0x3;
        gEncoding = deferredEncoding; /* since recursive, gEncoding gets overwritten anyway */
        if (pFields -> Attrs & SF_IMMEDIATE) {
            Chain.Encoding = deferredEncoding;
            gEncoding = deferredEncoding;
        }
      }
      /****************************************************************/
      /* Did that field change the CCSID                              */
      /****************************************************************/
      if (pFields -> Attrs & SF_CCSID)
      {
        gEbcdic = CSIsEbcdic(Data);
        if (pFields -> Attrs & SF_IMMEDIATE) Chain.Ebcdic = gEbcdic;
      }
      /****************************************************************/
      /* Did that field set the length of the structure               */
      /****************************************************************/
      if (pFields -> Attrs & SF_STRUCLENGTH)
      {
                                       /* Only want values 1 or 2     */
        Chain.StrucLength = Data;
      }
                                       /* Goto to next field          */
      Index   = 0;
      Array   = 0;
      pFields ++;
    }
                                       /* Goto next structure (if any)*/
    pStruct = pStruct -> NextStructure;
  }
MOD_EXIT:
                                       /* Set return values           */
  if (Chain.StrucLength)
  {
    *pp      = Chain.Start  + Chain.StrucLength;
    MsgLen   = Chain.MsgLen - Chain.StrucLength;
    if ((Chain.StrucLength > (unsigned int)Chain.MsgLen) ||
        (MsgLen < 0)) MsgLen = 0;
  }
  else if (Lastp)
  {
    *pp      = Lastp;
  }
  else
    *pp      = p;
  *pMsgLen = MsgLen;

  if (deferredEncoding) {
      gEncoding = deferredEncoding;
  }

  if (!pChain && (pOptions->Options & MF_TRUNCATED))
  {
    pOptions->OutFormat |= CSF_HEADER;
    CALLOUTFNX(Parm,"<Message Truncated>",19);
    pOptions->OutFormat &= ~CSF_HEADER;
  }

  return rc;
}

/**********************************************************************/
/* Function : GetValue                                                */
/* Purpose  : Returns the integer value at the given address          */
/**********************************************************************/
static unsigned int  GetValue (MQFIELD * pField,
                               char    * p,
                               int       Encoding)
{
  unsigned int  Data;
  switch(pField -> Type)
  {
    case MQFT_HEX4:
    case MQFT_LONG:
    case MQFT_LONGH:
    case MQFT_BLONG:
    case MQFT_FLAG4:
    case MQFT_OSTR:
    case MQFT_PSTR:
         Data = *(unsigned int  *)p;
         if (Encoding != CSENC_LOCAL) CSByteSwapLong((unsigned char *)&Data,1);
         break;
    case MQFT_SHORT:
    case MQFT_FLAG2:
         Data = *(unsigned short *)p;
         if (Encoding != CSENC_LOCAL) CSByteSwapShort((unsigned char *)&Data,1);
         break;
    case MQFT_BYTE:
    case MQFT_FLAG1:
         Data = *(unsigned char *)p;
         break;
    case MQFT_CHAR:
         Data = *(unsigned long *)p;
         if (Encoding != CSENC_LOCAL) CSByteSwapLong((unsigned char *)&Data,1);
         break;
    default:
         printf("** INTERNAL ERROR ** GetValue of %s.\n",pField->Name);
         Data = 0;
         break;
  }
  return Data;
}

/**********************************************************************/
/* Function : GetValue                                                */
/* Purpose  : Returns the integer value at the given address          */
/**********************************************************************/
static MQINT64 GetValue64(MQFIELD * pField,
                          char    * p,
                          int       Encoding)
{
  MQINT64 Data;
  switch(pField -> Type)
  {
    case MQFT_LONGH64:
         Data = *(MQINT64 *)p;
         if (Encoding != CSENC_LOCAL) CSByteSwapLong((unsigned char *)&Data,2);
         break;
    default:
         printf("** INTERNAL ERROR ** GetValue of %s.\n",pField->Name);
         Data = 0;
         break;
  }
  return Data;
}
/**********************************************************************/
/* Function : CheckDepend                                             */
/* Purpose  : Check a dependant field for existence                   */
/**********************************************************************/
static int CheckDepend (MQSCHAIN * ipChain,
                        MQDEPEND * pDepend)
{
  int             rc = 0;
  MQSCHAIN      * pChain;
  MQFIELDS      * pFields;
  MQFIELD       * pField;
  unsigned int    Data;

  /********************************************************************/
  /* We special case the Format field, if this is a dependancy on the */
  /* format then we just check that individually.                     */
  /********************************************************************/
  pField = pDepend -> Field;
  if (pField)
  {
    if (pField -> Type == MQFT_FORMAT)
    {
      if (pDepend -> StrValue != NULL) {
        char * pFormat = pDepend -> StrValue;
        rc = !memcmp(pFormat, FormatName, 8);
        goto MOD_EXIT;
      } else {

        long minv = * (long *) FormatName;
        long maxv = * (long *) (FormatName+4);
        if (pDepend -> MinValue == minv && pDepend -> MaxValue == maxv) {
          rc = TRUE;
        } else {
          long minv2 = minv;
          long maxv2 = maxv;
          CSByteSwapLong((unsigned char *)&minv2,1);
          CSByteSwapLong((unsigned char *)&maxv2,1);
          if (pDepend -> MinValue == minv2 && pDepend -> MaxValue == maxv2) {
              rc = TRUE;
          } else {
              rc = FALSE;
          }
        }
      }
      goto MOD_EXIT;
    }
  }

  while(pDepend -> Field)
  {
    pChain = ipChain;
    while (pChain)
    {
      pFields = pChain -> Struct -> Fields;
      while(pFields->Field)
      {
        if (pFields -> Field == pDepend -> Field)
        {
                                       /* Ok, we've found it, does    */
                                       /* the value match             */
          Data = GetValue(pFields -> Field,
                          pChain  -> Start + pFields -> Offset,
                          pChain  -> Encoding);
          if (pFields -> Field -> Type == MQFT_FLAG1 ||
              pFields -> Field -> Type == MQFT_FLAG4)
            rc = Data  & pDepend -> MinValue;
          else
            rc = ((int )Data >= pDepend -> MinValue) &&
                 ((int )Data <= pDepend -> MaxValue);
                                       /* Return if no match          */
          if (!rc) goto MOD_EXIT;
                                       /* Go to next field            */
          goto NEXTFIELD;
        }
        pFields++;
      }
      pChain  = pChain -> Prev;
    }
                                       /* Didn't find the field !     */
    rc = 0;
    goto MOD_EXIT;
                                       /* Next dependant field        */
NEXTFIELD:
    pDepend++;
  }

MOD_EXIT:
  return rc;
}

/**********************************************************************/
/* Function : GetValueIndirect                                        */
/* Purpose  : Return the value of a previous field                    */
/**********************************************************************/
static unsigned int  GetValueIndirect (MQSCHAIN * pChain,
                                       MQFIELD  * pField)
{
  unsigned int    Data = 0;
  MQFIELDS      * pFields;

  while (pChain)
  {
    pFields = pChain -> Struct -> Fields;
    while(pFields->Field)
    {
      if (pFields -> Field == pField)
      {
                                     /* Ok, we've found it, does    */
                                     /* the value match             */
        Data = GetValue(pFields -> Field,
                        pChain  -> Start + pFields -> Offset,
                        pChain  -> Encoding);
        goto MOD_EXIT;
      }
      pFields++;
    }
    pChain  = pChain -> Prev;
  }

MOD_EXIT:
  return Data;
}

/**********************************************************************/
/* Function : AddValues                                               */
/* Purpose  : Add a value description to a field value                */
/**********************************************************************/
static void AddValues (MQFIELD * pField,
                       int       Data,
                       char    * Buffer,
                       CBOPT   * pOptions)
{
  MQVALUE * pValue = (MQVALUE *)pField -> Value;
  int       Length;

  Length = strlen(Buffer);
  while (pValue -> Name)
  {
    if (pValue -> Const == Data && pValue->Desc)
    {
      if (pValue->Desc && pValue->Desc[0]) {  /* eg MQIACF_FIRST descr is NULL */
          if (pOptions->Options & MF_SHOWCONST) {
              sprintf(&Buffer[Length]," %s (%s)",pValue->Name, pValue->Desc);
          } else {
              sprintf(&Buffer[Length]," (%s)",pValue->Desc);
          }
          goto MOD_EXIT;   /* Only end once printed */
      }
    }
    pValue++;
  }
                                       /* Didn't find it !            */
  strcpy(&Buffer[Length]," (Unrecognised)");

MOD_EXIT:
  ;

}
/**********************************************************************/
/* Function : AddParmValues                                           */
/* Purpose  : Add a value based on the parm id                        */
/**********************************************************************/
static void AddParmValue  (unsigned int  ParmId,
                           int           Value,
                           char        * Buffer)
{
  char * p      = Buffer;
  char * pValue = MQParameterValue(ParmId,Value);

  pValue = MQParameterValue(ParmId,Value);
  if (pValue)
  {
    p  += strlen(Buffer);
    *p++ = ' ';
    strcpy(p,pValue);
  }
}
/**********************************************************************/
/* Function : AddDeferredParmValues                                   */
/* Purpose  : Add lines for each flag value                           */
/**********************************************************************/
static int  AddDeferredParmValue(int           (outfn)(void *,CBOPT *,char *,int ),
                                 void        * Parm,
                                 MQFIELD     * pField,
                                 int           Data,
                                 int           Indent,
                                 CBOPT       * pOptions)
{
  int rc = 0;
  switch(pField->Type)
  {
    case MQFT_FLAG4:
         rc = AddFlags(outfn,Parm,pField,Data,Indent,pOptions);
         break;
    default:
         break;
  }
  return rc;
}

/**********************************************************************/
/* Function : AddFlags                                                */
/* Purpose  : Add lines for each flag value                           */
/**********************************************************************/
static int  AddFlags    (int           (outfn)(void *,CBOPT *,char *,int ),
                         void        * Parm,
                         MQFIELD     * pField,
                         int           Data,
                         int           Indent,
                         CBOPT       * pOptions)
{
  int       rc = 0;
  char      Buffer[200];
  MQVALUE * pValue = (MQVALUE *) pField -> Value;
  int       FoundOne = FALSE;
  BOOL      blanked  = FALSE;
#ifdef FIELDSORT
  BOOL      anyMatches;
#endif

  pOptions->OutFormat = 0;

#ifndef FIELDSORT
  while (pValue -> Name)
  {
    if ((pValue -> Const && (pValue -> Const & Data) == pValue -> Const)
      /* || (FoundOne == FALSE && pValue -> Const == 0 && pValue->Name && Data == 0) */
      )
    {
      FoundOne = TRUE;
      if (FORMATTING)
      {
        if (!blanked)
        {
          memset(Buffer,' ',Indent);
          blanked = TRUE;
        }
        if (pField -> Type == MQFT_FLAG1) {
          if (pOptions->Options & MF_SHOWCONST) {
            sprintf(&Buffer[Indent],"%2.2X %s (%s)",pValue -> Const,pValue -> Name, pValue->Desc);
          } else {
            sprintf(&Buffer[Indent],"%2.2X %s",pValue -> Const,pValue->Desc);
          }
        } else {
          if (pOptions->Options & MF_SHOWCONST) {
            sprintf(&Buffer[Indent],"%8.8X %s (%s)",pValue -> Const,pValue -> Name, pValue->Desc);
          } else {
            sprintf(&Buffer[Indent],"%8.8X %s",pValue -> Const,pValue->Desc);
          }
        }
      }
      CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
      Data &= ~pValue -> Const;
    }
    pValue++;
  }
                                       /* Didn't find it !            */
  if (Data)
  {
    if (FORMATTING)
    {
      if (!blanked)
      {
        memset(Buffer,' ',Indent);
        blanked = TRUE;
      }
      if (pField -> Type == MQFT_FLAG1)
        sprintf(&Buffer[Indent],"%2.2X ** Unrecognised **",Data);
      else
        sprintf(&Buffer[Indent],"%8.8X ** Unrecognised **",Data);
    }
    CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
  }
#else /* Code below sorts the flags from highest to lowest */
  anyMatches = TRUE;
  while (anyMatches)
  {
    MQVALUE   * thisValue = (MQVALUE *) pField -> Value;
    MQVALUE   * bestMatch = NULL;
    MQLONG      bestValue = -1;
    anyMatches = FALSE;

    while (thisValue -> Name)
    {
      //if (thisValue -> Const)
      {
        /* If the value is non zero and is contained in our flags OR
              the value is zero but our flags are zero as well and we havent matched any other values */
        if ((thisValue -> Const && (thisValue -> Const & Data) == thisValue->Const)
           ||(FoundOne == FALSE && thisValue -> Const == 0 && thisValue->Name && Data == 0)
          )
        {
          FoundOne = TRUE;
          anyMatches = TRUE;

          if (bestValue == -1 || bestValue > thisValue -> Const)
          {
              bestMatch = thisValue;
              bestValue = thisValue -> Const;
          }
        }
      }
      thisValue++;
    }

    if (anyMatches)
    {
      if (FORMATTING)
      {
        if (pField -> Type == MQFT_FLAG1)
        {
          if (pOptions->Options & MF_SHOWCONST)
            sprintf(&Buffer[Indent],"%2.2X %s (%s)",bestMatch -> Const,bestMatch -> Name, bestMatch->Desc);
          else
            sprintf(&Buffer[Indent],"%2.2X %s",bestMatch -> Const,bestMatch->Desc);
        } else
        {
          if (pOptions->Options & MF_SHOWCONST)
            sprintf(&Buffer[Indent],"%8.8X %s (%s)",bestMatch -> Const,bestMatch -> Name, bestMatch->Desc);
          else
            sprintf(&Buffer[Indent],"%8.8X %s",bestMatch -> Const,bestMatch->Desc);
        }
      }
      CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
      Data &= ~bestMatch -> Const;
    }
  }

                                       /* Didn't find it !            */
  if (Data)
  {
    if (FORMATTING)
    {
      if (pField -> Type == MQFT_FLAG1)
        sprintf(&Buffer[Indent],"%2.2X ** Unrecognised **",Data);
      else
        sprintf(&Buffer[Indent],"%8.8X ** Unrecognised **",Data);
    }
    CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
  }
#endif
MOD_EXIT:
  return rc;
}
