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
/*  FILE   : QLSTRUCT.H                                               */
/*  PURPOSE: MQSeries structure definitions                           */
/**********************************************************************/
#ifndef MQSTRUCT_INCLUDED
#define MQSTRUCT_INCLUDED yes
                                       /* Field types                 */
#define MQFT_HEX              1
#define MQFT_LONG             2
#define MQFT_BYTE             3
#define MQFT_FLAG1            4
#define MQFT_CHAR             5
#define MQFT_HEX4             6
#define MQFT_SHORT            7
#define MQFT_STRUCT           8
#define MQFT_FLAG4            9
#define MQFT_ARRAY            10
#define MQFT_CHARN            11
#define MQFT_HEXN             12
#define MQFT_OSTR             13
#define MQFT_PSTR             14
#define MQFT_COUNT            15
#define MQFT_BLONG            16
#define MQFT_FORMAT           17
#define MQFT_DUMP             18
#define MQFT_REPEAT           19
#define MQFT_DUMPSTR          20
#define MQFT_LONGH            21       /* Output as long and hex      */
#define MQFT_MSG              22
#define MQFT_XML              23       /* Call XML parser             */
#define MQFT_XMLMSG           24       /* Call XML parser             */
#define MQFT_LONGH64          25
#define MQFT_FARRAY           26       /* Fixed array                 */
#define MQFT_DATE             27
#define MQFT_TIME             28
#define MQFT_CHARV            29
#define MQFT_ALIGN            30
#define MQFT_FLAG2            31

#define MQFT_HEXCHAR          90       /* Output as hex and char      */
#define MQFT_CHAR1F           91       /* Character flag              */

                                       /* Structure Field Attributes  */
                                       /* First byte is detail level  */
#define SF_DETAIL           0x000000FF
#define SF_OPT              0x00000100
#define SF_ENCODING         0x00000200
#define SF_IMMEDIATE        0x00000400
#define SF_CCSID            0x00000800
#define SF_DEPSTRUCT        0x00001000
#define SF_STRUCLENGTH      0x00002000
#define SF_PARMVALUE        0x00004000
#define SF_VERSION          0x80000000

#define FO_DEPENDED         0x00000001
#define FO_HEX_DISPLAY      0x00000002
                                       /* Constants                   */
                                       /* Field description           */
typedef struct _MQFIELD
{
  char    * Name;
  short     Type;                      /* MQFT_*                      */
  int       Length;
                                       /* Either MQVALUE or MQSTRUCT  */
  void    * Value;
  int       FldOptions;                /* FO_*                        */
} MQFIELD;
                                       /* Describe field presence     */
                                       /* depending on other field    */
typedef struct _MQDEPEND
{
  MQFIELD  * Field;
  int        MinValue;
  int        MaxValue;
  char     * StrValue;
} MQDEPEND;
                                       /* List of fields in structure */
typedef struct _MQFIELDS
{
  int        Offset;
  MQFIELD  * Field;
  MQDEPEND * Depend;
  int        Attrs;                    /* SF_*                        */
} MQFIELDS;
                                       /* Description of structure    */
typedef struct _MQSTRUCT
{
  char              * Name;
  int                 StrucAttrs;      /* SF_*                        */
  MQFIELDS          * Fields;
  struct  _MQSTRUCT * NextStructure;
} MQSTRUCT;

typedef struct _MQSTRUCTS
{
  MQSTRUCT          * Struct;
  char              * Eyecatcher;
  int                 Skipbytes;
} MQSTRUCTS;
                                       /* Chain of processed structs  */
typedef struct _MQSCHAIN
{
  struct _MQSCHAIN  volatile * Prev;
  MQSTRUCT                   * Struct;
  unsigned char              * Start;
  unsigned char              * StartOfData;
  size_t                       MsgLen;
  unsigned int                 StrucLength;
  int                          Encoding;
  int                          Ebcdic;
} volatile MQSCHAIN;

#ifndef MQSTRUCT_DEFINITIONS

extern MQSTRUCT   DefID;
extern MQSTRUCT   DefMQMD;
extern MQSTRUCT   DefMQOD;
extern MQSTRUCT   DefMQPMO;
extern MQSTRUCT   DefMQGMO;
extern MQSTRUCT   DefMQMSG;
extern MQFIELD    Format;
extern MQSTRUCTS  MQStructs[];

extern MQSTRUCT DefReason;
extern MQSTRUCT DefCompcode;
extern MQSTRUCT DefOpenOpts;
extern MQSTRUCT DefCloseOpts;
extern MQSTRUCT DefLong;
extern MQSTRUCT DefHex;
extern MQSTRUCT DefMQPCF;
extern MQSTRUCT DefMQPCF2;
extern MQSTRUCT DefMQEPH;
extern MQSTRUCT DefMQOR;
extern MQSTRUCT DefMQPMR;
extern MQSTRUCT DefMQAPI;
extern MQSTRUCT   DefMQSD;
extern MQSTRUCT   DefMQSRO;
extern MQFIELD    GlobalFormat;
extern MQSTRUCT   DefMQINQSel;
#endif
extern MQSTRUCT DefODType;
extern MQSTRUCT DefODName;

void InitialiseOffsets         (void);
#endif
