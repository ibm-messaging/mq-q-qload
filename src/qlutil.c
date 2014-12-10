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
/*  FILE   : QLUTIL.C                                                 */
/*  PURPOSE: Various utility functions                                */
/**********************************************************************/
#ifdef MVS
  #pragma csect(code,"CMQUDUTL")
  #pragma csect(static,"CSTUDUTL")
#endif
#include <stdio.h>
#include "ctype.h"
#include "string.h"
#include "qlutil.h"
#include "qlcs.h"

#if defined(AMQ_AS400) 
#include "stdlib.h"
#endif

char HEX[] = "0123456789ABCDEF";

typedef struct
{
  char * Name;
  int    secs;
} TIMEBIT;


TIMEBIT timebits[] =
{
  {"seconds", 1},
  {"minutes", 60},
  {"hours"  , 60 * 60},
  {"days"   , 60 * 60 * 24},
  {"weeks"  , 60 * 60 * 24 * 7},
  {"years"  , 60 * 60 * 24 * 365},
  {NULL     , 1}
};

TIMEBIT * FindTimeBit(char ** ppName)
{
  TIMEBIT * ptb   = timebits;
  char    * pName = *ppName;
  char    * p     = pName;
  int       len;

  while (*p && *p != ' ' && !isdigit(*p)) p++;
  len = p - pName;

  while(ptb->Name)
  {
#if defined(WIN32) || defined(WIN64)
    if (!_memicmp(pName,ptb->Name,len)) goto MOD_EXIT;
#else
 #ifdef memicmp
    if (!memicmp(pName,ptb->Name,len)) goto MOD_EXIT;
 #else
    if (!memcmp(pName,ptb->Name,len)) goto MOD_EXIT;
 #endif
#endif
    ptb++;
  }
  ptb=NULL;
MOD_EXIT:
  *ppName = p;
  return ptb;
}

#ifdef AMQ_UNIX

char * ltoa(int value, char * str, int radix)
{
  sprintf(str,"%d",value);
  return str;
}

char * ultoa(unsigned int value, char * str, int radix)
{
  sprintf(str,"%ud",value);
  return str;
}

/**********************************************************************/
/* Function : strupr                                                  */
/* Purpose  : Uppercase a string                                      */
/**********************************************************************/
char * strupr(char * p)
{
  char * pRet = p;
  while(*p)
  {
    if (islower(*p)) *p = toupper(*p);
    p++;
  }
  return pRet;
}

#endif
/**********************************************************************/
/* Function : strlcmp                                                 */
/* Purpose  : Compare a string against a mem block                    */
/**********************************************************************/
int strlcmp(char * p,int len,char * pStr)
{
  int v = memcmp(p,pStr,len);
  if (!v) v = !(pStr[len] == 0);
  return v;
}

/**********************************************************************/
/* Function : stristr                                                 */
/* Purpose  : Find a string in another string                         */
/**********************************************************************/
char * stristr   (char * p,
                  char * s)
{
  char * p1,* s1;
  while (*p)
  {
    p1 = p; s1=s;
    while(*s1 && (toupper(*p1) == toupper(*s1)))
    {
      p1++; s1++;
    }
    if (!*s1) return p;
    p++;
  }
  return NULL;
}
/**********************************************************************/
/* Function : bitcpy                                                  */
/* Purpose  : Copy a bit mask from source to target                   */
/**********************************************************************/
void bitcpy      (int  * t,            /* Target                      */
                  int    s,            /* Source                      */
                  int    b)            /* Bit                         */
{
  int p = *t;

  p &= ~b;                             /* NULL out current mask       */
  p |= (s & b);                        /* Set the 'set' bits          */

  *t = p;
}
/**********************************************************************/
/* Function : cmpnchr                                                 */
/* Purpose  : Returns TRUE if all characters for the length are       */
/*            the given character                                     */
/**********************************************************************/
int  cmpnchr     (char * p,            /* String                      */
                  int    n,            /* length                      */
                  char   c)            /* Character                   */
{
  while (n)
  {
    if (*p != c) return 0;
    p++;n--;
  }
  return 1;
}
/**********************************************************************/
/* Function : cmpnstr                                                 */
/* Purpose  : Returns TRUE if all characters for the length are       */
/*            in the given string                                     */
/**********************************************************************/
int  cmpnstr     (char * p,            /* String                      */
                  int    n,            /* length                      */
                  char * s)            /* Character String            */
{
  while (n)
  {
    if (!strchr(s,*p)) return 0;
    p++;n--;
  }
  return 1;
}
/**********************************************************************/
/* Function : bitset                                                  */
/* Purpose  : Set a bit depending on the boolean                      */
/**********************************************************************/
void bitset      (int  * t,            /* Target                      */
                  int    s,            /* set boolean                 */
                  int    b)            /* Bit                         */
{
  int p = *t;

  if (s) p |=  b;
    else p &= ~b;

  *t = p;
}
/**********************************************************************/
/* Function : memfind                                                 */
/* Purpose  : Find string s in string p                               */
/**********************************************************************/
char * memfind   (char * p,
                  int    plen,
                  char * s,
                  int    slen)
{
  char * x;

  if (!slen)       return p;

  while (1)
  {
    if (plen < slen) return NULL;

    x = memchr(p,*s,plen);
    if (!x) break;
                                       /* Ok, we have a possible      */
    if (!memcmp(s,x,slen)) return x;
                                       /* Skip this one               */
    plen -= ((x-p)+1);
    p     = x+1;
  }
  return NULL;
}

/**********************************************************************/
/* Function : Match                                                   */
/* Purpose  : Perform a string match                                  */
/*           Mask can contain '*' to indicate zero or more chars      */
/*                            '?' to indicate one char                */
/**********************************************************************/
int  Match(char * s,                   /* String to be matched        */
           int    sl,                  /* Length of string            */
           char * m,                   /* Mask to be matched          */
           int    ml)                  /* length of mask              */
{
                                       /* Match if both are empty     */
  if (!sl && !ml) return 1;
                                       /* If end of mask then fail    */
  if (!ml) return 0;
                                       /* Do we have an '*'           */
  if  (*m == '*')
  {
    m++;
    ml--;
                                       /* End of mask then must match */
    if (!ml) return 1;
                                       /* Repeat for each combination */
    do
    {
      if (Match(s,sl,m,ml)) return 1;
      s++;
      sl--;
    }
    while (sl>0);
                                       /* Failed to match...          */
    return 0;
  }
  else
  {
                                       /* If end of string then fail  */
    if  (!sl) return 0;
                                       /* check chars match           */
    while (sl>0 && ml>0)
    {
      if (*s == *m || *m == '?') {s++ ; sl--; m++; ml--;}
                            else break;
    }
    if (ml>0)
    {
      if (*m == '*') return Match(s,sl,m,ml);
                else return 0;
    }
    if (sl>0) return 0;
  }
  return 1;
}

/**********************************************************************/
/* Function : Matchi                                                  */
/* Purpose  : Perform a string insensitive match                      */
/*           Mask can contain '*' to indicate zero or more chars      */
/*                            '?' to indicate one char                */
/**********************************************************************/
int  Matchi(char * s,                  /* String to be matched        */
            int    sl,                 /* Length of string            */
            char * m,                  /* Mask to be matched          */
            int    ml)                 /* length of mask              */
{
                                       /* Match if both are empty     */
  if (!sl && !ml) return 1;
                                       /* If end of mask then fail    */
  if (!ml) return 0;
                                       /* Do we have an '*'           */
  if  (*m == '*')
  {
    m++;
    ml--;
                                       /* End of mask then must match */
    if (!ml) return 1;
                                       /* Repeat for each combination */
    while (sl)
    {
      if (Matchi(s,sl,m,ml)) return 1;
      s++;
      sl--;
    }
                                       /* Failed to match...          */
    return 0;
  }
  else
  {
                                       /* If end of string then fail  */
    if  (!sl) return 0;
                                       /* check chars match           */
    while (sl && ml)
    {
      if (toupper(*s) == toupper(*m) || *m == '?')
      {s++ ; sl--; m++; ml--;}
      else break;
    }
    if (ml)
    {
      if (*m == '*') return Matchi(s,sl,m,ml);
                else return 0;
    }
    if (sl) return 0;
  }
  return 1;
}

/**********************************************************************/
/* Function: WriteHex                                                 */
/* Purpose : Similar to fwrite but will write the string as a HEX     */
/*           string                                                   */
/**********************************************************************/
void WriteHex(unsigned char * in,
              int             length,
              int             separators,
              FILE          * file)
{
  int i;
  for (i=0 ; i<length ; i++)
  {
    if (!(i%4) && separators && i) fputc(' ',file);

    fputc(HEX[((unsigned char)*in)/16],file);
    fputc(HEX[((unsigned char)*in)%16],file);
    in++;
  }
}
/**********************************************************************/
/* Function: HexStr                                                   */
/* Purpose : Convert string to hex string and ascii                   */
/**********************************************************************/
void HexStr(unsigned  char * in,
            int              inlength,     /* Number of bytes given       */
            int              maxinlength,  /* Maximum bytes will be given */
            unsigned  char * out,
            int              ascii,
            int              Options)      /* HO_                     */

/* Function changed....were there to Options flags                    */
/*            int              separators,                            */
/*            int              textisebcdic)                          */
{
  int   i;
  int   separators    = Options & HO_SEPARATORS;
  int   textisebcdic  = Options & HO_TEXTISEBCDIC;
  int   nonullterm    = Options & HO_NO_NULLTERMINATE;
  int   asciioffset;
  int   outlength;
  unsigned char *p1;
  unsigned char *p2;
  unsigned char   AsciiText[100];
  unsigned char * pAscii;

  if (textisebcdic && ascii && inlength < sizeof(AsciiText))
  {
    CSToAscii(AsciiText,in,inlength);
    pAscii  = AsciiText;
  }
  else
    pAscii = in;
                                       /* Check maxinlength           */
  if (!maxinlength) maxinlength = inlength;
                                       /* Calculate out length        */
  if (ascii)
  {
    asciioffset = maxinlength*2  + ascii;
    if (separators && maxinlength>7)
      asciioffset += maxinlength/4 - 1;

    outlength = asciioffset + maxinlength;
                                       /* Blanks it out               */
    memset(out,' ',(int)outlength);
  }
  else
  {
    outlength = maxinlength*2;
  }
  p1 = out;
  p2 = out + asciioffset;

  for (i=0 ; i<inlength ; i++)
  {
    if (!(i%4) && separators && i)
    {
      *p1++ = ' ';
    }

    *p1++ = HEX[((unsigned char)*in)/16];
    *p1++ = HEX[((unsigned char)*in)%16];

    if (ascii)
    {
      *p2 ++ = isprint(*pAscii) ? *pAscii : '.';
      pAscii++;
    }
    in++;
  }
  while (i<maxinlength)
  {
    *p1++= ' ';
    *p1++= ' ';
    i++;
  }
  if (!nonullterm)
  {
    if (ascii) *p2 = 0;
          else *p1 = 0;
  }
}


#ifdef _WINDOWS
/**********************************************************************/
/* Function : ConvertTo108Time                                        */
/* Purpose  : Convert time from 20030406     111412   format (GMT)    */
/*                           to 2003-04-06   11.14.12 format (Local)  */
/**********************************************************************/
void ConvertTo108Time(char * FromDate,
                      char * FromTime,
                      char * ToDate,
                      char * ToTime)
{
  SYSTEMTIME st;
  char       s[12];

  *(int  *)s  = *(int  *)FromDate;
  s[4]        = 0;
  FromDate   += 4;
  st.wYear    = atoi(s);

  *(short *)s = *(short *)FromDate;
  s[2]        = 0;
  FromDate   += 2;
  st.wMonth   = atoi(s);

  *(short *)s = *(short *)FromDate;
  s[2]        = 0;
  FromDate   += 2;
  st.wDay     = atoi(s);

  *(short *)s = *(short *)FromTime;
  s[2]        = 0;
  FromTime   += 2;
  st.wHour    = atoi(s);

  *(short *)s = *(short *)FromTime;
  s[2]        = 0;
  FromTime   += 2;
  st.wMinute  = atoi(s);

  *(short *)s = *(short *)FromTime;
  s[2]        = 0;
  FromTime   += 2;
  st.wSecond  = atoi(s);
  st.wMilliseconds = 0;

  if (!SystemTimeToTzSpecificLocalTime(NULL,&st,&st))
  {
    CSDebugTrap(0);
  }

  if (ToDate)
  {
    sprintf(s,"%4.4d-%2.2d-%2.2d"  ,st.wYear,
                                    st.wMonth,
                                    st.wDay);
    memcpy(ToDate,s,10);
  }
  if (ToTime)
  {
    sprintf(s,"%2.2d.%2.2d.%2.2d"  ,st.wHour,
                                    st.wMinute,
                                    st.wSecond);
    memcpy(ToTime,s,8);
  }
}
#endif

//#ifndef MVS
/**********************************************************************/
/* Function : ConvertToTime                                           */
/* Purpose  : Convert from from 2003-04-06   11.14.12 format (Local)  */
/*            to a time int  value                                    */
/**********************************************************************/
time_t ConvertToTime(char * FromDate,
                     int    FromDateLen,
                     char * FromTime,
                     int    FromTimeLen)
{
  struct tm  Tm;
  time_t     time;
  char       s[12];

  Tm.tm_isdst = -1;

  if (FromDate && FromDateLen>=10)
  {
    *(int  *)s  = *(int  *)FromDate;
    s[4]        = 0;
    FromDate   += 5;
    Tm.tm_year  = atoi(s) - 1900;

    *(short *)s = *(short *)FromDate;
    s[2]        = 0;
    FromDate   += 3;
    Tm.tm_mon   = atoi(s)-1;

    *(short *)s = *(short *)FromDate;
    s[2]        = 0;
    Tm.tm_mday   = atoi(s);
  }
  else
  {
    Tm.tm_year = 70;
    Tm.tm_mon  = 0;
    Tm.tm_mday = 1;
  }

  if (FromTime && FromTimeLen >= 8)
  {
    *(short *)s = *(short *)FromTime;
    s[2]        = 0;
    FromTime   += 3;
    Tm.tm_hour  = atoi(s);

    *(short *)s = *(short *)FromTime;
    s[2]        = 0;
    FromTime   += 3;
    Tm.tm_min   = atoi(s);

    *(short *)s = *(short *)FromTime;
    s[2]        = 0;
    Tm.tm_sec   = atoi(s);
  }
  else
  {
    Tm.tm_hour = 0;
    Tm.tm_min  = 0;
    Tm.tm_sec  = 0;
  }

  time = mktime(&Tm);
  return time;
}
/**********************************************************************/
/* Function : ConvertToTime88                                         */
/* Purpose  : Convert from GMT  20030406   111412 format (Local)      */
/*            to a time int  value                                    */
/**********************************************************************/
time_t ConvertToTime88(char * FromDate,
                       char * FromTime)
{
  struct tm  Tm;
  time_t     time = -1;

  if (!FromDate || !FromTime) goto MOD_EXIT;

  sscanf(FromDate,"%4d%2d%2d", &Tm.tm_year,&Tm.tm_mon,&Tm.tm_mday);
  sscanf(FromTime,"%2d%2d%2d", &Tm.tm_hour,&Tm.tm_min,&Tm.tm_sec);

  Tm.tm_year -= 1900;
  Tm.tm_mon  -= 1;
  Tm.tm_isdst = 0;
  time = mktime(&Tm);
MOD_EXIT:
  return time;
}
/**********************************************************************/
/* Function : DiffTime88                                              */
/* Purpose  : Subtract two times in 20030406 111412 format (Local)    */
/*            to get the number of milliseconds                       */
/**********************************************************************/
time_t  DiffTime88      (char * d1,
                         char * t1,
                         char * d2,
                         char * t2)
{
  struct tm Tm;
  int    H1,m1,s1,h1;
  int    H2,m2,s2,h2;
  time_t diff = 0;
  if (memcmp(d1,d2,8))
  {
    time_t     time1,time2;
                                      /* Different date               */
    sscanf(d1,"%4d%2d%2d", &Tm.tm_year,&Tm.tm_mon,&Tm.tm_mday);
    sscanf(t1,"%2d%2d%2d", &Tm.tm_hour,&Tm.tm_min,&Tm.tm_sec);

    Tm.tm_year -= 1900;
    Tm.tm_mon  -= 1;
    Tm.tm_isdst = 0;
    time1 = mktime(&Tm);
    sscanf(d2,"%4d%2d%2d", &Tm.tm_year,&Tm.tm_mon,&Tm.tm_mday);
    sscanf(t2,"%2d%2d%2d", &Tm.tm_hour,&Tm.tm_min,&Tm.tm_sec);

    Tm.tm_year -= 1900;
    Tm.tm_mon  -= 1;
    Tm.tm_isdst = 0;
    time2 = mktime(&Tm);

    diff = (time2 - time1) * 1000;
  }
  sscanf(t1,"%2d%2d%2d%2d", &H1,&m1,&s1,&h1);
  sscanf(t2,"%2d%2d%2d%2d", &H2,&m2,&s2,&h2);

  diff += (H2 - H1) * 3600 * 1000;
  diff += (m2 - m1) *   60 * 1000;
  diff += (s2 - s1)        * 1000;
  diff += (h2 - h1)        * 10;
  return diff;
}

/**********************************************************************/
/* Function : Get108Time                                              */
/* Purpose  : Get the time in 2003-04-06   11.14.12 format            */
/**********************************************************************/
time_t Get108Time(time_t * pNow,char * Date,char * Time)
{
  struct tm * p;
  time_t      Now;
  char        Buffer[12];

  if (getenv("MQ_TIMEDEBUG"))
  {
    if (pNow)
    {
      time(&Now);
      printf("Time passed  : %d (Now is %d)\n",*pNow,Now);
    }
  }

  if (pNow) Now = *pNow;
       else time(&Now);
  p = localtime(&Now);
  if (p)
  {
    sprintf(Buffer,"%4.4d-%2.2d-%2.2d"  ,p->tm_year+1900,
                                         p->tm_mon+1,
                                         p->tm_mday);
  }
  else
  {
    if (getenv("MQ_TIMEDEBUG"))
    {
      time(&Now);
      printf("Invalid time received : %d (Now is %d)\n",*pNow,Now);
    }
    strcpy(Buffer,"0000-00-00");
  }
  memcpy(Date,Buffer,10);
  if (p)
  {
    sprintf(Buffer,"%2.2d.%2.2d.%2.2d"  ,p->tm_hour,
                                         p->tm_min,
                                         p->tm_sec);
  }
  else
  {
    if (getenv("MQ_TIMEDEBUG"))
    {
      time(&Now);
      printf("Invalid time received : %d (Now is %d)\n",*pNow,Now);
    }
    strcpy(Buffer,"00.00.00");
  }
  memcpy(Time,Buffer,8);
  return Now;
}
/**********************************************************************/
/* Function : Get88Time                                               */
/* Purpose  : Get the time in 20030406     11141200 format            */
/**********************************************************************/
time_t Get88Time(time_t * pNow,char * Date,char * Time)
{
  struct tm * p;
  time_t      Now;
  char        Buffer[12];

  if (pNow) Now = *pNow;
       else time(&Now);
  p = localtime(&Now);
  sprintf(Buffer,"%4.4d%2.2d%2.2d"  ,p->tm_year+1900,
                                     p->tm_mon+1,
                                     p->tm_mday);
  memcpy(Date,Buffer,8);
  sprintf(Buffer,"%2.2d%2.2d%2.2d00" ,p->tm_hour,
                                      p->tm_min,
                                      p->tm_sec);
  memcpy(Time,Buffer,8);
  return Now;
}
//#endif
/**********************************************************************/
/* Function : GetCharTime                                             */
/* Purpose  : Return the time in ascii format given seconds           */
/**********************************************************************/
void GetCharTime(char * Buffer,int Seconds,BOOL shortform)
{
  int        Years;
  int        Weeks;
  int        Days;
  int        Hours;
  int        Minutes;
  char * p = Buffer;

  Years    = Seconds / (365 * 24 * 60 * 60);
  Seconds -= Years   * (365 * 24 * 60 * 60);

  Weeks    = Seconds / (  7 * 24 * 60 * 60);
  Seconds -= Weeks   * (  7 * 24 * 60 * 60);

  Days     = Seconds / (      24 * 60 * 60);
  Seconds -= Days    * (      24 * 60 * 60);

  Hours    = Seconds / (           60 * 60);
  Seconds -= Hours   * (           60 * 60);

  Minutes  = Seconds/                   60;
  Seconds -= Minutes*                   60;

  if (Years)
  {
    if (shortform) sprintf(p,"%dy"      ,Years);
              else sprintf(p,"%d year%s",Years,Years!=1?"s":"");
    p+=strlen(p);
  }

  if (Weeks)
  {
    if (p != Buffer) *p++ = ' ';
    if (shortform) sprintf(p,"%dw"      ,Weeks);
              else sprintf(p,"%d week%s",Weeks,Weeks!=1?"s":"");
    p+=strlen(p);
  }

  if (Days)
  {
    if (p != Buffer) *p++ = ' ';
    if (shortform) sprintf(p,"%dd"      ,Days );
              else sprintf(p,"%d day%s" ,Days ,Days !=1?"s":"");
    p+=strlen(p);
  }

  if (Hours)
  {
    if (p != Buffer) *p++ = ' ';
    if (shortform) sprintf(p,"%dh"      ,Hours);
              else sprintf(p,"%d hour%s",Hours,Hours!=1?"s":"");
    p+=strlen(p);
  }

  if (Minutes)
  {
    if (p != Buffer) *p++ = ' ';
    if (shortform) sprintf(p,"%dm"      ,Minutes);
              else sprintf(p,"%d minute%s",Minutes,Minutes!=1?"s":"");
    p+=strlen(p);
  }

  if (Seconds || (p == Buffer))
  {
    if (p != Buffer) *p++ = ' ';
    if (shortform) sprintf(p,"%ds"      ,Seconds);
              else sprintf(p,"%d second%s",Seconds,Seconds!=1?"s":"");
    p+=strlen(p);
  }
}
/**********************************************************************/
/* Function : ParseCharTime                                           */
/* Purpose  : Parse the format above and return the number of seconds */
/**********************************************************************/
int ParseCharTime(char * p)
{
  int       seconds = 0;
  int       n;
  TIMEBIT * ptb;


  while(1)
  {
                                       /* Skip past spaces            */
    while (*p == ' ') p++;
    if (!*p) break;
                                       /* Take count                  */
    n = atoi(p);
                                       /* Skip past value             */
    while (isdigit(*p)) p++;
    while (*p == ' '  ) p++;
    if (!*p) return -1;

    ptb = FindTimeBit(&p);
    if (ptb) seconds += (n*ptb->secs);
        else return -1;
  }
  return seconds;
}
/**********************************************************************/
/* Function: HexStrn                                                  */
/* Purpose : Convert string to hex string                             */
/**********************************************************************/
void HexStrn(unsigned char * in,
             int    inlength,
             unsigned char * out,
             int    outlength)
{
  int  i;

  if (!outlength) outlength = inlength*2;

  memset(out,' ',(int)outlength);
  out[outlength]=0;

  for (i=0 ; i<inlength ; i++)
  {
    out[i*2]   = HEX[((unsigned char)*in)/16];
    out[i*2+1] = HEX[((unsigned char)*in)%16];
    in++;
  }
}

/**********************************************************************/
/* Function: StrHex                                                   */
/* Purpose : Convert a Hex string to the actual hex                   */
/**********************************************************************/
int StrHex  (unsigned char * in,
             int             MaxLen,
             char          * Buffer)
{
  unsigned char   val;
  unsigned char   ch;
  unsigned char * p = (unsigned char *) in;
  while (*p && MaxLen >=2)
  {
    ch        = (unsigned char) *p++;
         if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
                                       /* Oops                        */
    else return 1;
    val       = ch * 16;
    ch        = (unsigned char) *p++;
         if (ch >= '0' && ch <= '9') ch = ch - '0';
    else if (ch >= 'A' && ch <= 'F') ch = ch - 'A' + 10;
    else if (ch >= 'a' && ch <= 'f') ch = ch - 'a' + 10;
                                       /* Oops                        */
    else return 1;
    val      += ch;
    *Buffer++ = val;
    MaxLen   -=2;
  }
  memset(Buffer,0,MaxLen/2);
  return 0;
}

/**********************************************************************/
/* Function: CopyHexStr                                               */
/* Purpose : Copy a string which might contain hex characters to      */
/*           output string                                            */
/**********************************************************************/
int CopyHexStr(unsigned char * Buffer,
               unsigned char * in,
               int    MaxLen)
{
  int c = 0;

  while (MaxLen--)
  {
    if (*in)
    {
      if (isprint(*in)) { *Buffer++ = *in; c++; }
                     else *Buffer++ = '.';
    }
    else *Buffer++ = '.';
    in++;
  }
  return c;
}

/**********************************************************************/
/* Function: StripHex                                                 */
/* Purpose : Strip trailing NULLs                                     */
/**********************************************************************/
int  StripHex(char * in,
              int    Length)
{
  char *p = in + Length;

  while(Length >= sizeof(int ))
  {
    p -= sizeof(int );
    if (*(int  *)p)
    {
      p += sizeof(int );
      break;
    }
    Length -= sizeof(int );
  }
  while(Length >= sizeof(short))
  {
    p -= sizeof(short);
    if (*(short *)p)
    {
      p += sizeof(short);
      break;
    }
    Length -= sizeof(short);
  }
  while(Length >= 1)
  {
    p --;
    if (*p)
    {
      p ++;
      break;
    }
    Length --;
  }
  Length = p - in;
  return Length;
}

/**********************************************************************/
/* Function: Strip                                                    */
/* Purpose : Strip trailing chars                                     */
/**********************************************************************/
int  Strip(char * in,
           int    length)
{
  char *p;

  p= in + length - 1;
  while (length &&
         (*p == ' '  ||
          *p == '\n' ||
          *p == 0    ||
          *p == '\t'))
  {
    *p-- = 0;
    length--;
  }
  return length;
}

/**********************************************************************/
/* Function: StripString                                              */
/* Purpose : Strip trailing chars of a string                         */
/**********************************************************************/
int  StripString(char * in,
                 int    length,
                 char * s)
{
  char *p;

  p= in + length - 1;
  while (length)
  {
    if (!strchr(s,*p)) break;
    *p-- = 0;
    length--;
  }
  return length;
}

/**********************************************************************/
/* Function: TrimString                                               */
/* Purpose : Trim, front and back, characters of a string             */
/**********************************************************************/
char * TrimString(char * in,
                  char * s)
{
  char *p;

  /********************************************************************/
  /* Ok, bump forward until we find something not matching            */
  /********************************************************************/
  while (*in)
  {
    if (!strchr(s,*in)) break;
    in++;
  }
  /********************************************************************/
  /* Now, do the same for the end                                     */
  /********************************************************************/
  p = in + strlen(in) - 1;
  while (p >= in)
  {
    if (!strchr(s,*p)) break;
    *p-- = 0;
  }
  return in;
}

/**********************************************************************/
/* Function: StripLen                                                 */
/* Purpose : Return the length of blank stripped string               */
/**********************************************************************/
int  StripLen(char * in,
              int    length)
{
  char *p;

  p= in + length - 1;
  while (length &&
         (*p == ' '  ||
          *p == '\n' ||
          *p == 0    ||
          *p == '\t'))
  {
    length--;
    p--;
  }
  return length;
}

/**********************************************************************/
/* Function: blcpy                                                    */
/* Purpose : Copy a string until the first blank....always NULL term. */
/**********************************************************************/
int  blcpy(char * out,
           char * in,
           int    length,
           int    maxlength)
{
  int len = 0;
  /********************************************************************/
  /* If we're given a maximum length then only copy a minimum of the  */
  /* supplied length and max length                                   */
  /********************************************************************/
  if (maxlength) length = min(length,maxlength);

  while (length && *in!=' ' && *in)
  {
    length--;
    len ++;
    *out++=*in++;
  }
  *out=0;
  return len;
}
/**********************************************************************/
/* Function: blcpya                                                   */
/* Purpose : Copy a string until the last blank.....always NULL term. */
/**********************************************************************/
int  blcpya(char * out,
            char * in,
            int    length,
            int    maxlength)
{
  char * p;
  if (maxlength) length = min(length,maxlength);
  p = in + length - 1;
  while (length && *p == ' ')
  {
    p--;
    length --;
  }
  if (length) memcpy(out,in,length);
  out[length]=0;
  return length;
}
/**********************************************************************/
/* Function: AvgReset                                                 */
/* Purpose : Reset the average to all zeros                           */
/**********************************************************************/
void AvgReset      (AVGSET * Avg,int  Time)
{
  short Interval = Avg->Interval;
  memset(Avg,0,sizeof(AVGSET));
  Avg->Interval       = Interval;
  Avg->FirstTime      = Time;
  Avg->UsedIntervals  = 1;
}
/**********************************************************************/
/* Function: AvgSetInterval                                           */
/* Purpose : Set the value used for average calculations              */
/**********************************************************************/
void AvgSetInterval(AVGSET * Avg,short Interval)
{
  Avg -> Interval = Interval;
  /* ?? should re-average the set */
}
/**********************************************************************/
/* Function: AvgAdjust                                                */
/* Purpose : Move the average fields down                             */
/**********************************************************************/
static void AvgAdjust(AVGSET * Avg,int   ThisInterval)
{
  int Diff = ThisInterval - Avg->LatestInterval;
  int ThisDiff;
  int Left = AVG_MAX_INTERVALS - Diff;
  int Used;
  if (Left > 0)
  {
    memmove(&Avg->Values[Diff],
            &Avg->Values[0],
            Left*sizeof(Avg->Values[0]));
  }
  ThisDiff = min(Diff,AVG_MAX_INTERVALS);
  memset(&Avg->Values[0],0,ThisDiff*sizeof(Avg->Values[0]));
  Used = Avg->UsedIntervals + Diff;
  if (Used > AVG_MAX_INTERVALS) Used = AVG_MAX_INTERVALS;
  Avg->UsedIntervals  = Used;
  Avg->LatestInterval = ThisInterval;
}
/**********************************************************************/
/* Function: AvgAdd                                                   */
/* Purpose : Add a new interval event                                 */
/**********************************************************************/
void AvgAdd        (AVGSET * Avg,int   Time)
{
  int  ThisInterval;

  if (!Avg->FirstTime) goto MOD_EXIT;
  if (!Avg->Interval)  goto MOD_EXIT;
  if (!Time)           goto MOD_EXIT;

  Time -= Avg->FirstTime;

  ThisInterval = Time/Avg->Interval;

  if (ThisInterval != Avg->LatestInterval)
  {
    AvgAdjust(Avg,ThisInterval);
  }
  Avg->Values[0]++;

MOD_EXIT:
  ;
}
/**********************************************************************/
/* Function: AvgGetAvg                                                */
/* Purpose : Return the average value in set                          */
/**********************************************************************/
int  AvgGetAvg     (AVGSET * Avg,int   Time)
{
  float Sum = 0;
  float Ints;
  float Average;
  int   UsedInterval;
  int   ThisInterval;
  int   i;

  if (!Avg->FirstTime)      return 0;
  if (!Avg->UsedIntervals)  return 0;
  if (!Avg->Interval)       return 0;

  Time -= Avg->FirstTime;
  ThisInterval = Time/Avg->Interval;
  if (ThisInterval != Avg->LatestInterval)
  {
    AvgAdjust(Avg,ThisInterval);
  }

  UsedInterval = (Time+1) % Avg->Interval;
  Ints         = ((float)UsedInterval)/Avg->Interval+(float)(Avg->UsedIntervals-1);

  for (i=0 ; i<Avg->UsedIntervals;i++) Sum += (float)Avg->Values[i];

  if (Ints == 0) return 0;
  if (Ints < 1) Average = Sum;
           else Average = Sum/Ints + (float)0.5;
  /* printf("Sum = %f Ints = %f Average = %f\n",Sum,Ints,Average); */

  return (int)Average;
}
/**********************************************************************/
/* Function: AvgGetPeak                                               */
/* Purpose : Return the peak value in the interval                    */
/**********************************************************************/
int  AvgGetPeak    (AVGSET * Avg,int  Time)
{
  int   Peak = 0;
  int   i;
  int   ThisInterval;

  if (!Avg->FirstTime)      return 0;
  if (!Avg->UsedIntervals)  return 0;
  if (!Avg->Interval)       return 0;

  Time -= Avg->FirstTime;
  ThisInterval = Time/Avg->Interval;
  if (ThisInterval != Avg->LatestInterval)
  {
    AvgAdjust(Avg,ThisInterval);
  }

  for (i=0 ; i<Avg->UsedIntervals;i++)
  {
    if (Avg->Values[i] > Peak) Peak = Avg->Values[i];
  }
  return Peak;
}
