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
/*  FILE   : QLCS.H                                                   */
/*  PURPOSE: Common Service routines                                  */
/**********************************************************************/
#include <stdio.h>
#include <stddef.h>
#ifndef CS_H_INCLUDED
#define CS_H_INCLUDED
                                       /* Constants                   */
#define CSENC_NORMAL   1
#define CSENC_REVERSED 2

#if defined (WIN32) || defined(WIN64)
#define CSENC_LOCAL    2
#define CSENC_NONLOCAL 1
#endif
#ifdef MVS
#define CSENC_LOCAL    1
#define CSENC_NONLOCAL 2
#endif
#ifdef AMQ_UNIX
  #ifdef _LINUX_2
    #define CSENC_LOCAL    2
    #define CSENC_NONLOCAL 1
  #else
    #define CSENC_LOCAL    1
    #define CSENC_NONLOCAL 2
  #endif
  #include <pthread.h>
  typedef void * THREAD_START_ROUTINE(void *);
  typedef THREAD_START_ROUTINE * LPTHREAD_START_ROUTINE;
#endif

#ifdef AMQ_AS400
  #include <pthread.h>
  typedef void * THREAD_START_ROUTINE(void *);
  typedef THREAD_START_ROUTINE * LPTHREAD_START_ROUTINE;
  #define CSENC_LOCAL    1
  #define CSENC_NONLOCAL 2
#endif

#if defined(MVS) || defined(AMQ_AS400)
  #define CS_EBCDIC
  #define CS_IS_EBCDIC   1
#else
  #define CS_IS_EBCDIC   0
#endif

#ifdef DEBUG
  #define CHECKSTORAGE yes
#endif

#ifndef MVS
#if defined (WIN32) || defined(WIN64)
  #ifndef WINDOWS
    #define WINDOWS
  #endif
  #define stricmp _stricmp
  #define memicmp _memicmp
  #include <windows.h>
  typedef HANDLE CSMUTEX;
  typedef HANDLE CSEVENT;
  typedef HMODULE CSLIBHANDLE;
  #define PATH_SEP   '\\'

  #define CSMUTEX_NULL NULL
  #define CSEVENT_NULL NULL
  #define CSMUTEX_IS_NULL(s) (*s == NULL)
  #define CSEVENT_IS_NULL(s) (*s == NULL)

#else
#ifdef AMQ_AS400
  typedef unsigned long long CSLIBHANDLE;
#else
  typedef void * CSLIBHANDLE;
#endif
  #define PATH_SEP   '/'
  #define stricmp strcmp
  #define INFINITE -1
  typedef struct
  {
    int              defined;
    pthread_mutex_t  mutex;
  } CSMUTEX;
  typedef struct
  {
    int              defined;
    int              posted;
    pthread_cond_t   cond;
    pthread_mutex_t  mutex;
  } CSEVENT;

  #define CSMUTEX_NULL {0}
  #define CSEVENT_NULL {0}
  #define CSMUTEX_IS_NULL(s) ((s)->defined == 0)
  #define CSEVENT_IS_NULL(s) ((s)->defined == 0)
#endif
#else
  #define stricmp strcmp
#endif /* End of #ifndef MVS */

typedef struct _MYCOLOURS
{
  int    colour;
#if defined (WIN32) || defined(WIN64)
  HBRUSH hBrush;
#else
  int    hBrush;
#endif
} MYCOLOURS;

  typedef          int   INT32;
  typedef unsigned int   UINT32;
  typedef unsigned int   BIT32;
  typedef unsigned short BIT16;
  typedef UINT32       * PUINT32;

#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  typedef int            LONG;
  typedef unsigned short USHORT;
  typedef char           CHAR;
  typedef unsigned int   ULONG;
  typedef ULONG *        PULONG;
  typedef int            BOOL;
  typedef unsigned char  BYTE;
  typedef unsigned int   UINT;
  typedef int            INT;
  typedef short          SHORT;
  typedef unsigned char  UCHAR;


  #ifndef pascal
    #define pascal
  #endif

  #ifndef TRUE
    #define TRUE 1
  #endif

  #ifndef FALSE
    #define FALSE 0
  #endif

  typedef struct tagSIZE
  {
      LONG        cx;
      LONG        cy;
  } SIZE, *PSIZE, *LPSIZE;
#endif

#ifdef _HPUX_SOURCE
  char * CSltoa (         int  v, char * s,int r);
  char * CSultoa(unsigned int  v, char * s,int r);

  #define LTOA  CSltoa
  #define ULTOA CSultoa
#else
  #define LTOA  _ltoa
  #define ULTOA _ultoa
#endif

#ifndef min
  #define min(a,b) (a<b ? a:b)
#endif

#ifndef max
  #define max(a,b) (a>b ? a:b)
#endif

#ifdef _SOLARIS_2
  #define CHK_EYE(a,b) (memcmp(a,b,4)==0)
  #define SET_EYE(a,b)  memcpy(a,b,4)
#else
  #define CHK_EYE(a,b) (*(int  *)a == *(int  *)b)
  #define SET_EYE(a,b)  *(int  *)a = *(int  *)b
#endif

#ifndef MQSTRUCT_INCLUDED
  #include <qlstruct.h>
#endif
                                       /* Error return codes          */
#define CSRC_FAILED    -1
#define CSRC_TIMEOUT    2
#define CSRC_NOT_FOUND  3
                                       /* CS Options                  */
#define CSO_MEMORY     0x00000001
#define CSO_MESSAGES   0x00000002
#define CSO_FRONT      0x00000004
#define CSO_NOHEADER   0x00000008
#define CSO_FORMATTED  0x00000010
#define CSO_EBCDIC     0x00000020
#define CSO_NOALLOC    0x00000040
#define CSO_NOT_UNIQUE 0x00000080
                                       /* CS Output format            */
#define CSF_HEADER   0x00000001
#define CSF_DUMP     0x00000002
#define CSF_TEXT     0x00000004

                                       /* Storage Types               */
#define ST_HEAP      0x00000001
#define ST_MAIN      0x00000002
                                       /* First byte is detail level  */
#define MF_DETAIL           0x000000FF
#define MF_HEX              0x00000100
#define MF_FORMAT           0x00000200
#define MF_MQMD             0x00000400
#define MF_NOMSG            0x00000800
#define MF_DETAILED         0x00001000
#define MF_NOFORMAT         0x00002000
#define MF_HEXBITS          0x00004000
#define MF_OFFSET           0x00008000
#define MF_HEXSEP           0x00010000 /* Hex space separators        */
#define MF_HEXCH            0x00020000 /* Output HEX also as Char     */
#define MF_HEXCH_PREDICT    0x00040000 /* Predict whether ASCII/EBCDIC*/
#define MF_AUTO_DETECT_XML  0x00080000
#define MF_XML_SHORTFORM    0x00100000
#define MF_TRUNCATED        0x00200000
#define MF_LOCAL_TIMES      0x00400000
#define MF_NO_ASCII_COLUMN  0x00800000

#define MF_SHOWCONST        0x02000000 /* Show const name plus descr  */
#define MF_FIELDOFFSET      0x04000000 /* Show field offset in string */
#define MF_ENCLOCAL         0x08000000 /* Reset to local encoding     */
#define MF_ENCBYTESWAP      0x10000000 /* Reset to nonlocal encoding  */
#define MF_ENCASCII         0x20000000 /* Data is in ascii            */
#define MF_ENCEBCDIC        0x40000000 /* Data is in ebcdic           */
#define MF_NOFOLLOWPTR      0x80000000 /* Dont Follow ptrs            */

#define MF2_NO_BYTESWAP_VERSION  0x00000001
#define MF2_IGNORE_CRLF          0x00000002
#define MF2_ZERO_LENGTH_STRINGS  0x00000004
                                       /* Return codes                */
#define MFRC_FORMATTING_ON  10
#define MFRC_FORMATTING_OFF 11
#define MFRC_STOP           12


#if defined (WIN32) || defined(WIN64)
#define CSTrap(v)  CSTrapfn(__FILE__, __TIMESTAMP__, __LINE__, (int)(v))
#else
#define CSTrap(v)  CSTrapfn(__FILE__, "DD/MM/YYYY" , __LINE__, (int)(v))
#endif

#ifdef _DEBUG
  #define CSDebugTrap(v) CSTrap(v)
#else
  #define CSDebugTrap(v)
#endif

#define FORMATTING (!(pOptions->Options & MF_NOFORMAT))

#define CALLOUTFN(p,b,l)                                             \
          rc=outfn(p,pOptions,b,l);                                  \
          if (rc)                                                    \
          {                                                          \
            if (rc == MFRC_FORMATTING_ON)                            \
            {pOptions->Options &= ~MF_NOFORMAT;  pOptions->OutFormat |= CSO_FORMATTED;} \
            if (rc == MFRC_FORMATTING_OFF)                           \
            {pOptions->Options |=  MF_NOFORMAT;  pOptions->OutFormat &=~CSO_FORMATTED;} \
            if (rc == MFRC_STOP)           goto MOD_EXIT;            \
          }

#define CALLOUTFNX(p,b,l)                                             \
          rc=outfn(p,pOptions,b,l);                                 \
          if (rc)                                                    \
          {                                                          \
            if (rc == MFRC_FORMATTING_ON)                            \
            {pOptions->Options &= ~MF_NOFORMAT;  pOptions->OutFormat |= CSO_FORMATTED;} \
            if (rc == MFRC_FORMATTING_OFF)                           \
            {pOptions->Options |=  MF_NOFORMAT;  pOptions->OutFormat &=~CSO_FORMATTED;} \
          }

#define CSOFFSETOF(s,c) (int)(ptrdiff_t)&(((s *)0)->c)

#define CSTRACEIN(f)            if (CSTracing) CSTraceIn(f);
#define CSTRACEOUT(f,r)         if (CSTracing) CSTraceOut(f,r);
#define CSTRACEMSGIN(a,b,c,d,e) if (CSTracing) CSTraceMsgIn(a,(int)b,c,d,e);
#define CSTRACEMSGOUT(a,b,c,d)  if (CSTracing) CSTraceMsgOut(a,(int)b,c,d);
#define CSPRINTF(a)             if (CSTracing) CSPrintf a;

#define CSWITHIN(_v,_s,_l)  (((_v) >= (_s)) && ((_v) <= (_l)))

#define CSINVARIANT(_a)  ((_a) && (CSWITHIN((_a),'a','z') ||       \
                                   CSWITHIN((_a),'A','Z') ||       \
                                   CSWITHIN((_a),'0','9') ||       \
                                   strchr(" '()+,-./:=?_", (_a)) ))

                                       /* Typedefs                    */
#define STORAGE_EYECATCHERF "STGF"
#define STORAGE_EYECATCHER  "STG."

typedef struct _CSSTORAGE
{
  char                  ID[4];
  struct _CSSTORAGE   * Next;
  struct _CSSTORAGE   * Prev;
  int                   Size;
  int                   Type;
#ifdef WINDOWS
  HGLOBAL               hArea;
#endif
  char                  Name[20];
} CSSTORAGE;

typedef struct _CSSUBBLOCK
{
  short                 Size;
  struct _CSSUBBLOCK  * Next;
} CSSUBBLOCK;

typedef struct _CSBLOCK
{
  struct _CSBLOCK     * Next;
  CSSUBBLOCK          * pSub;
  int                   Size;
  int                   Used;
  short                 LargestSub;
} CSBLOCK;

typedef struct _CSPARTS
{
  int    Value;
  char * Name;
  char * sName;
  char * plural;
} CSPARTS;
                                       /* Structure for data indirection */
typedef struct _CSDATA
{
  int      Size;
  int      Length;
  char   * Data;
} CSDATA;

typedef struct
{
  char * Base;
  int    Size;
  char * Name;
} CSHEADROOM;

/**********************************************************************/
/* The following structure must match the start of CSLIST             */
/**********************************************************************/
typedef struct _CSNEXTPREV
{
  struct _CSNEXTPREV  * Next;
  struct _CSNEXTPREV  * Prev;
} CSNEXTPREV;

typedef struct _CSLISTROOT
{
  CSNEXTPREV          * Top;
  CSNEXTPREV          * Bottom;
  int                   Count;
} CSLISTROOT;

typedef struct _CSLIST
{
  struct _CSLIST      * Next;
  struct _CSLIST      * Prev;
  short                 Length;        /* Not including header        */
  short                 Options;       /* Callers use....not CS       */
/*CSLISTROOT            Links; */
  CSLISTROOT          * Root;
} CSLIST;

typedef struct _CSLINK
{
  struct _CSLINK      * Next;
  struct _CSLINK      * Prev;
  struct _CSLINK      * Other;
  void                * Source;
  void                * Target;
  short                 Dir;           /* 0 - source, 1 - target      */
  short                 Type;
  int                   Data;
} CSLINK;


typedef struct _CSTREE
{
  struct _CSTREE      * Sub[2];        /* Left and Right subtrees     */
  struct _CSTREE      * Coll[2];       /* Collating list              */
  CSLINK              * Links;
  short                 Type;          /* Free for caller of tree     */
  unsigned short        Height;
  int                   TreeIndex;     /* Collating index             */
#ifdef CSDEFINE_GLOBALS
  char                  Key[1];
#endif
} CSTREE;

#define CSTreeNext(a) (((CSTREE *)a)->Coll[1])
#define CSTreePrev(a) (((CSTREE *)a)->Coll[0])

typedef struct _CSITC
{
  CSLIST        List;
#ifdef WINDOWS
  HANDLE        EventSem;
#endif
} CSITC;

typedef struct _CSITCBLOCK
{
  CSLIST        Header;
  void        * p;
  int           ParmLength;
} CSITCBLOCK;

/*********************************************************************/
/* Callback options                                                  */
/*********************************************************************/
typedef struct _CBOPT
{
  int       Options;                   /* MF_*                                  */
  int       Options2;                  /* MF2_*                                 */
  int       Checked;
  int       ScreenWidth;               /* Width of screen in chars              */
  int       AsciiColumn;               /* 0 = No                                */
  int       DataWidth;                 /* How many bytes of data/line           */
  int       DataGap;                   /* Spaces gap before any ASCII           */
  CSDATA  * pData;                     /* Callback data                         */
  int       OutFormat;                 /* Format of output data                 */
  int       FieldMsgOffset;            /* Offset in message till start of field */
  int       FieldLineMsgOffset;        /* Offset in message till start of field */
  int       LineMsgOffset;             /* Offset in message till start of data  */
  int       LineMsgLength;             /* Msg length represented by line        */
  short     LineDataOffset;            /* Offset of data in line                */
  int       LineDataLength;            /* Data Length on this line              */
  int       LineDataMaxLength;         /* Max Data Length on this line          */
  MQFIELD * pField;
  int       FieldType;                 /* To overrise MQFIELD        */
} CBOPT;
                                       /* Prototypes                  */
#ifndef MVS
int CSAccessLibrary(char         * Library,
                    char         * pPath,
                    char         * ErrorMsg,
                    int            ErrorMsgLen,
                    CSLIBHANDLE  * pLibHandle);

void * CSAccessProc(CSLIBHANDLE lib,char * fn);
#endif

#ifdef CHECKSTORAGE
void      CSCheckStorage (void);
#else
#define  CSCheckStorage()
#endif
int       CSGetRelTime   (void);
void      CSSleep        (int              d);
void      CSMSSleep      (int              d);
void      CSTrapfn       (char           * file,
                          char           * timestamp,
                          int              line,
                          int              value);
void      CSffdc         (void);
void    * CSGetMem       (int              size,
                          char           * Name);
void      CSFreeMem      (void           * p);
void      CSInitialise   (char           * filename);
void      CSTerminate    (void);

int       CSIssueCommand (char           * Command,
                          char          ** pErrorText);

CSBLOCK * CSGetBlock     (int              BlockSize);
void      CSFreeBlock    (CSBLOCK        * pcsb);
void    * CSGetSubBlock  (CSBLOCK        * pcsb,
                          int              Size);
                                       /* Tree functions              */
void    * CSTreeAdd       (void          * ppRoot,
                           int             Options,
                           void          * Elem,
                           int             Size,
                           int             KeySize);

int       CSTreeDelete    (void          * ppRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm);

int       CSTreeDeleteItem(void          * ppRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm,
                           void          * Elem,
                           int             KeySize);

void    * CSTreeFind      (void          * pRoot,
                           void          * Key,
                           int             KeySize);

int       CSTreeCount     (void          * pRoot);

void    * CSTreeGetIndex  (void          * pRoot,
                           int             Index);

void      CSTreeTraverse  (void          * pRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm);

void      CSTreeTraverseB (void          * pRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm);

void    * CSTreeFirst     (void          * pRoot);

void    * CSTreeLast      (void          * pRoot);

                                       /* List functions             */
void    * CSListAdd       (CSLISTROOT    * pRoot,
                           int             Options,
                           void          * Elem,
                           int             Size);

void    * CSListAddSort   (CSLISTROOT    * pRoot,
                           int             Options,
                           void          * Elem,
                           int             Size,
                           int             KeySize);

void      CSListDelete    (CSLISTROOT    * pRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm);

void      CSListDeleteItem(void          * Elem);

int       CSListCount     (CSLISTROOT    * pRoot );

void    * CSListFind      (CSLISTROOT    * pRoot,
                           void          * Elem);

void    * CSListFindEntry(CSLISTROOT    * pRoot,
                          int             Options,
                          void          * Elem,
                          int             Size);

void      CSListTraverse  (CSLISTROOT    * pRoot,
                           int             (* fn)(void *,void *),
                           void          * Parm);

                                       /* Link functions             */
CSLINK  * CSLinkAdd       (void          * Source,
                           void          * Target,
                           short           Type,
                           int             Data);

CSLINK  * CSLinkFind      (void          * Source,
                           void          * Target,
                           short           Type,
                           BOOL            Either);

void      CSLinkRmvAll    (void          * Source,
                           short           Type);

void      CSLinkRmv       (CSLINK        * pLink);

CSLINK  * CSLinkNext      (void          * Object,
                           short           Type,
                           CSLINK        * pLink);

void    * CSListLink      (CSLISTROOT    * pRoot,
                           int             Options,
                           CSLIST        * pElem);

void    * CSListUnlink    (CSLISTROOT    * pRoot,
                           int             Options);

void      CSByteSwapShort (unsigned char * pShort,
                          int              array);

void      CSByteSwapLong  (unsigned char * pLong,
                           int             array);

int       CSToAscii       (unsigned char * pOut,
                           unsigned char * pIn,
                           int             Length);

int       CSToEbcdic      (unsigned char * pOut,
                           unsigned char * pIn,
                           int             Length);

void      CSConvTest      ();
void      CSConvGenerate  ();

int       CSIsEbcdic      (int             CCSID);

int       CSItcCreate     (CSITC         * itc);
int       CSItcDestroy    (CSITC         * itc);
int       CSItcPost       (CSITC         * itc,
                           void          * parms,
                           int             length,
                           void          * p);
int       CSItcWait       (CSITC         * itc,
                           void          * parms,
                           int             length,
                           void         ** p,
                           int             timeout);

int       CSDumpHex      (int             (outfn)(void *,CBOPT *,char *,int ),
                          void           * Parm,
                          unsigned char  * pMsg,
                          int              Len,
                          CBOPT          * pOptions);

void      CSCheckOpts    (CBOPT          * pOptions);

void      CSTraceIn      (char           * func);

void      CSTraceOut     (char           * func,
                          int              rc);
void      CSTraceGroup   ();
void      CSTraceGroupEnd();
void      CSTraceMsgIn   (char * proc, int hwnd, int msg, int mp1,int mp2);
void      CSTraceMsgOut  (char * proc, int hwnd, int msg, int res);
int       CSTraceStart   (char           * tracefile);

int       CSTraceStop    ();

void      CSPrintf       (int              nl,
                          char           * format,
                          ...);

char    * CSStringAdd    (char           * p,
                          int              len);

void      CSStringDelete (char           * p);

int       CSStringReplace(char          ** pString,
                          char           * p,
                          int              len);

int       CSStringReplaceMax(char       ** pString,
                             char        * p,
                             int           len,
                             BOOL          NoEmpty);

char   *  CSLoadFile     (FILE *           file,
                          int  *           byteread);

void      CSAdjustTime   (char           * pDate,
                          char           * pTime,
                          int              Adjust);

void      CSConvertParts (int              Amount,
                          char           * Buffer,
                          CSPARTS        * pParts,
                          BOOL             Parts);

int       CSParseParts   (char           * Buffer,
                          CSPARTS        * pParts,
                          int              check);

void      CSAtomicUpdate (char           * pNew,
                          char          ** pTarget,
                          char          ** pOld);
BOOL      CSHeadRoom    (CSHEADROOM      * Head,
                         void           ** ppCur,
                         int               Room);

#ifndef MVS
BOOL CSCreateThread(LPTHREAD_START_ROUTINE  fn,
                     void                *  parm,
                    int                  *  pTid);
BOOL CSCreateMutex (CSMUTEX * mutex);
void CSDestroyMutex(CSMUTEX * mutex);
BOOL CSRequestMutex(CSMUTEX * mutex,int Timeout);
void CSReleaseMutex(CSMUTEX * mutex);
BOOL CSCreateEvent (CSEVENT * event);
void CSDestroyEvent(CSEVENT * event);
void CSSetEvent    (CSEVENT * event);
void CSWaitEvent   (CSEVENT * event, int Timeout);
#endif

#if defined (WIN32) || defined(WIN64)

typedef struct
{
  HANDLE LockMutex;
  int    LockMutexOwner;
  int    LockMutexCount;
} CSLOCK;

void      CSLock         (int Index);
void      CSUnlock       (int Index);
void      CSLockIt       (CSLOCK * pLock);
void      CSUnlockIt     (CSLOCK * pLock);
#else
#define   CSLock(a)
#define   CSUnlock(a);
int GetLastError();
#endif

#ifdef CSDEFINE_GLOBALS
       int    CSOptions  = 0L;
       int    CSMemAlloc = 0;
       int    CSMemHigh  = 0;
       int    CSMemBlocks= 0;
       int    CSMemCount = 0;
       int    CSTracing  = 0;
       char   CSFileRoot[150]="";
#else
extern int    CSOptions;
extern int    CSMemAlloc;
extern int    CSMemHigh;
extern int    CSMemBlocks;
extern int    CSMemCount;
extern int    CSTracing;
extern char   CSFileRoot[];
#endif
#endif
