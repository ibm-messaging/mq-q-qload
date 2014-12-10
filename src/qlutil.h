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
/*  FILE   : QLUTIL.H                                                 */
/*  PURPOSE: Various utility functions                                */
/**********************************************************************/
#include <time.h>
#ifdef MVS
#include <sys/time.h>
#endif

#define HO_ASCII             0x00000001
#define HO_SEPARATORS        0x00000002
#define HO_TEXTISEBCDIC      0x00000004
#define HO_NO_NULLTERMINATE  0x00000008

#define AVG_MAX_INTERVALS 20

typedef struct _AVGSET
{
  short Interval;                      /* Time of each interval       */
  short UsedIntervals;
  int   FirstTime;
  int   LatestInterval;                /* Time of last interval       */
  int   Values[AVG_MAX_INTERVALS];
} AVGSET;

#ifdef AMQ_UNIX

#ifndef _HPUX_SOURCE
char * ltoa (int value         , char * str, int radix);
char * ultoa(unsigned int value, char * str, int radix);
#endif
char * strupr(char * p);
#endif

int  strlcmp     (char * p,
                  int    len,
                  char * pStr);

int  Match       (char * s,            /* String to be matched        */
                  int    sl,           /* Length of string            */
                  char * m,            /* Mask to be matched          */
                  int    ml);          /* length of mask              */

int  Matchi      (char * s,            /* String to be matched        */
                  int    sl,           /* Length of string            */
                  char * m,            /* Mask to be matched          */
                  int    ml);          /* length of mask              */

void WriteHex    (unsigned char * in,
                  int             length,
                  int             separators,
                  FILE          * file);

void HexStr      (unsigned char * in,
                  int             inlength,
                  int             maxinlength,
                  unsigned char * out,
                  int             ascii,
                  int             Options); /* HO_                    */

int  StrHex      (unsigned char * in,
                  int    MaxLen,
                  char * Buffer);

int  CopyHexStr  (unsigned char * Buffer,
                  unsigned char * in,
                  int    MaxLen);

void HexStrn     (unsigned char * in,
                  int    inlength,
                  unsigned char * out,
                  int    outlength);

int  Strip       (char * in,
                  int    length);

int  StripHex    (char * in,
                  int    length);

int  StripString (char * in,
                  int    length,
                  char * s);

char * TrimString(char * in,
                  char * s);

int  StripLen    (char * in,
                  int    length);

int  blcpy       (char * out,
                  char * in,
                  int    length,
                  int    maxlength);

int  blcpya      (char * out,
                  char * in,
                  int    length,
                  int    maxlength);

int  cmpnchr     (char * p,            /* String                      */
                  int    n,            /* length                      */
                  char   c);           /* Character                   */

int  cmpnstr     (char * p,            /* String                      */
                  int    n,            /* length                      */
                  char * s);           /* Character string            */

char * memfind   (char * p,
                  int    plen,
                  char * s,
                  int    slen);

char * stristr   (char * p,
                  char * s);
#ifndef MVS
time_t ConvertToTime(char * FromDate,
                     int    FromDateLen,
                     char * FromTime,
                     int    FromTimeLen);

time_t ConvertToTime88(char * FromDate,
                       char * FromTime);

void ConvertTo108Time(char * FromDate,
                      char * FromTime,
                      char * ToDate,
                      char * ToTime);

time_t DiffTime88    (char * d1,
                      char * t1,
                      char * d2,
                      char * t2);

time_t Get108Time(time_t * pNow,
                  char   * Date,
                  char   * Time);

time_t Get88Time (time_t * pNow,
                  char   * Date,
                  char   * Time);
#endif
void GetCharTime (char * Buffer,
                  int    Seconds,
                  int    shortform);

int ParseCharTime(char * p);

void bitcpy      (int  * t,            /* Target                      */
                  int    s,            /* Source                      */
                  int    b);           /* Bit                         */

void bitset      (int  * t,            /* Target                      */
                  int    s,            /* set or not boolean          */
                  int    b);           /* Bit                         */

void AvgReset      (AVGSET * Avg,int   Time);
void AvgSetInterval(AVGSET * Avg,short Interval);
void AvgAdd        (AVGSET * Avg,int   Time);
int  AvgGetAvg     (AVGSET * Avg,int   TIme);
int  AvgGetPeak    (AVGSET * Avg,int   Time);
