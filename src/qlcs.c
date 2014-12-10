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
/*  FILE   : QLCS.C                                                   */
/*  PURPOSE: Common Service functions                                 */
/**********************************************************************/
#ifdef MVS
  #pragma csect(code,"CMQUDCS")
  #pragma csect(static,"CSTUDCS")
  #pragma runopts(POSIX(ON))
  #define _POSIX_SOURCE                /* Required for sleep()        */
  #define _XOPEN_SOURCE                /* Required for optarg, optind */
  #define _SHARE_EXT_VARS              /* because of APAR PQ03847     */
  #define _XOPEN_SOURCE_EXTENDED 1     /* Required for timeval        */
  #define _ALL_SOURCE                  /* Required for timezone on    */
#endif                                 /*              gettimeofday() */
                                       /* Include files               */
#include "string.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdarg.h"
#include "stdio.h"
#include "qlutil.h"

#if !defined(MVS) && !defined(WIN32) && !defined(WIN64)
#include <unistd.h>
#include <ctype.h>
#include <sys/time.h>
#endif

#if defined(WIN31) || defined(WIN32) || defined(WIN64)
  #include "windows.h"
  #define MAX_HEAP_SIZE 300

HANDLE TraceLock;
int    TraceLockOwner    = -1;
int    TraceLockUseCount = 0;

#include <dbghelp.h>

typedef BOOL (__stdcall *StackWalk64_t)(DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID, PREAD_PROCESS_MEMORY_ROUTINE64,
                                        PFUNCTION_TABLE_ACCESS_ROUTINE64, PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
typedef PVOID (__stdcall *SymFunctionTableAccess64_t)(HANDLE, DWORD64);
typedef DWORD64 (__stdcall *SymGetModuleBase64_t)(HANDLE, DWORD64);
typedef BOOL (__stdcall *SymCleanup_t)(HANDLE);
typedef BOOL (__stdcall *SymFromAddr_t)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
typedef BOOL (__stdcall *SymGetLineFromAddr64_t)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
typedef BOOL (__stdcall *SymInitialize_t)(HANDLE, PSTR, BOOL);
typedef DWORD (__stdcall *SymSetOptions_t)(DWORD);

// Pointers to explicitly dynamically linked functions exported from dbghelp.dll.
static StackWalk64_t              pStackWalk64                 = NULL;
static SymFunctionTableAccess64_t pSymFunctionTableAccess64    = NULL;
static SymGetModuleBase64_t       pSymGetModuleBase64          = NULL;
static SymCleanup_t               pSymCleanup                  = NULL;
static SymFromAddr_t              pSymFromAddr                 = NULL;
static SymGetLineFromAddr64_t     pSymGetLineFromAddr64        = NULL;
static SymInitialize_t            pSymInitialize               = NULL;
static SymSetOptions_t            pSymSetOptions               = NULL;

#endif
#if defined(AMQ_UNIX) || defined(AMQ_AS400)
  #include <errno.h>
  #define max(a,b) a>b?a:b
#endif
#if defined(AMQ_UNIX)
  #include <dlfcn.h>
#endif
#if defined(AMQ_AS400)
  #include <miptrnam.h>
  #include <qusec.h>
  #include <qleawi.h>
  #include <ctype.h>
  #include <sys/time.h>

  typedef void (OS_fct_t) (void);
  #pragma linkage(OS_fct_t,OS)

#endif
#define CSDEFINE_GLOBALS
#include "qlcs.h"

#ifdef _DEBUG
#include "stdio.h"
CSSTORAGE * pStorage = NULL;
#endif

#ifndef MAX_HEAP_SIZE
  #define MAX_HEAP_SIZE 1024
#endif

#define MAX_ERROR_TEXT_SIZE     500
#define MAX_STRING              399
#define MAX_THREADS              20

/**********************************************************************/
/*  Thread structure                                                  */
/**********************************************************************/
typedef struct _CSTHREAD
{
  int tid;
  int InUse;
  int Indent;
} CSTHREAD;

/**********************************************************************/
/*  Queue item structure                                              */
/**********************************************************************/
typedef struct qentry
{
  struct qentry * next;
  void          * d;
} QENTRY;
/**********************************************************************/
/*  Queue container structure                                         */
/*  This structure encapsulates a linked list of qentry items.        */
/**********************************************************************/
typedef struct
{
  QENTRY *  begin;
  QENTRY ** end;
} QUEUE;

typedef struct _CSTREESTACK
{
  struct _CSTREESTACK * pNext;
  CSTREE              * pNode;         /* This nodes address          */
  CSTREE             ** pSub;          /* Parent pointer address      */
  int                   dir;
} CSTREESTACK;

static int     LineOut(FILE    * file,
                       CBOPT   * CbOpt,
                       char    * pLine,
                       int       Length);

static int    zero        = 0;
FILE * CSTraceFile = NULL;

#ifdef AVL_DEBUG
void printtree(void);
#endif

int CSDoCheckStorage      = 1;
int CSDoCheckStorageTrace = 0;

CSTHREAD CSThreads[MAX_THREADS] = {0};

CSTREE * pStrings = NULL;

static char HEX[]="0123456789ABCDEF";

                                       /* EBCDIC to ASCII jumptable   */
#ifdef CS_EBCDIC
unsigned char CS_EBCDIC_TO_ASCII[256] =
/*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F       */
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*0 */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*1 */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*2 */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, /*3 */
  32,  0,  0,  0,  0,  0,  0,  0,  0,  0, 36, 46, 60, 40, 43,124, /*4 */
  38,  0,  0,  0,  0,  0,  0,  0,  0,  0, 33,156, 42, 41, 59,  0, /*5 */
  45, 47,  0,  0,  0,  0,  0,  0,  0,  0,124, 44, 37,  0, 62, 63, /*6 */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 35, 64, 58, 61, 34, /*7 */
   0, 97, 98, 99,100,101,102,103,104,105,  0,123,  0, 40, 43,  0, /*8 */
   0,106,107,108,109,110,111,112,113,114,  0,125,  0, 41,  0,  0, /*9 */
   0,126,115,116,117,118,119,120,121,122,  0,  0,  0,  0,  0,  0, /*A */
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 45, /*B */
 123, 65, 66, 67, 68, 69, 70, 71, 72, 73,  0,  0,  0,  0,  0,  0, /*C */
 125, 74, 75, 76, 77, 78, 79, 80, 81, 82,  0,  0,  0,  0,  0,  0, /*D */
   0,  0, 83, 84, 85, 86, 87, 88, 89, 90,  0,  0,  0,  0,  0,  0, /*E */
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57,  0,  0,  0,  0,  0,  0};/*F */
#else
  #ifdef AMQ_UNIX
         char CS_EBCDIC_TO_ASCII[256] =
  #else
unsigned char CS_EBCDIC_TO_ASCII[256] =
  #endif
/*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F       */
{'.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.', /*0 */
 '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.', /*1 */
 '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.', /*2 */
 '.','.','.','.','.','.','.','.','.','.','.','.','.','.','.','.', /*3 */
 ' ','.','.','.','.','.','.','.','.','.','$','.','<','(','+','|', /*4 */
 '&','.','.','.','.','.','.','.','.','.','!','$','*',')',';','.', /*5 */
 '-','/','.','.','.','.','.','.','.','.','.',',','%','_','>','?', /*6 */
 '.','.','.','.','.','.','.','.','.','.',':','#','@','\'','=','"',/*7 */
 '.','a','b','c','d','e','f','g','h','i','.','{','.','(','+','.', /*8 */
 '.','j','k','l','m','n','o','p','q','r','.','}','.',')','.','.', /*9 */
 '.','~','s','t','u','v','w','x','y','z','.','.','.','.','.','.', /*A */
 '.','.','.','.','.','.','.','.','.','.','[',']','.','.','.','-', /*B */
 '{','A','B','C','D','E','F','G','H','I','.','.','.','.','.','.', /*C */
 '}','J','K','L','M','N','O','P','Q','R','.','.','.','.','.','.', /*D */
 '.','.','S','T','U','V','W','X','Y','Z','.','.','.','.','.','.', /*E */
 '0','1','2','3','4','5','6','7','8','9','.','.','.','.','.','.'};/*F */
#endif

unsigned char CS_ASCII_TO_EBCDIC[256] = /* MEE changed all but 0x20 from 0 to 75 to default convert to periods instead of NULL */
/*0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F       */
{ 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*0 */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*1 */
  64, 90,127,123, 74,108, 80, 75,141,157, 92,142,107,191, 75, 97, /*2 MEE changed 2E from 255 to 75 so it would properly map to a period*/
 240,241,242,243,244,245,246,247,248,249,122, 94, 76,126,110,111, /*3 MEE changed 3A from 125 to 122 so it would properly map to a colon*/
 124,193,194,195,196,197,198,199,200,201,209,210,211,212,213,214, /*4 */
 215,216,217,226,227,228,229,230,231,232,233, 75,224, 75, 75, 75, /*5 MEE changed 5C from 75 to 224 so it would properly map to a backslash*/
  75,129,130,131,132,133,134,135,136,137,145,146,147,148,149,150, /*6 */
 151,152,153,162,163,164,165,166,167,168,169,192, 79,208,161, 75, /*7 */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*8 */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 91, 75, 75, 75, /*9 */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*A */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*B */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*C */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75,106, 75, 75, /*D */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, /*E */
  75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75, 75};/*F */


int DAYS[] = { 0,31,28,31,30,31,30,31,31,30,31,30,31 };

#ifdef _HPUX_SOURCE

char * CSltoa(int  v, char * s,int r)
{
  char * p = ltoa(v);
  strcpy(s,p);
  return s;
}
char * CSultoa(unsigned int  v, char * s,int r)
{
  char * p = ultoa(v);
  strcpy(s,p);
  return s;
}
#endif

#ifndef MVS
#if defined (WIN32) || defined(WIN64)

CSLOCK Locks[10] = {0};

BOOL CSCreateThread(LPTHREAD_START_ROUTINE  fn,
                    LPVOID                  parm,
                    int                   * pTid)
{
  HANDLE h = CreateThread(NULL,20000,fn,parm,0,pTid);
  return h != NULL;
}

/**********************************************************************/
/* Function : CSCreateMutex                                           */
/* Purpose  : Create a mutex semaphore                                */
/**********************************************************************/
BOOL CSCreateMutex(CSMUTEX * mutex)
{
  *mutex = CreateMutex(NULL,FALSE,NULL);
  return *mutex != NULL;
}
/**********************************************************************/
/* Function : CSDestroyMutex                                          */
/* Purpose  : Destroy the mutex semaphore                             */
/**********************************************************************/
void CSDestroyMutex(CSMUTEX * mutex)
{
  if (*mutex) CloseHandle(*mutex);
}
/**********************************************************************/
/* Function : CSRequestMutex                                          */
/* Purpose  : Request ownership of a mutex                            */
/**********************************************************************/
BOOL CSRequestMutex(CSMUTEX * mutex,int Timeout)
{
  DWORD rc = WaitForSingleObject(*mutex,Timeout);
  if (rc == WAIT_OBJECT_0) return TRUE;
                      else return FALSE;
}
/**********************************************************************/
/* Function : CSRequestMutex                                          */
/* Purpose  : Release ownership of a mutex                            */
/**********************************************************************/
void CSReleaseMutex(CSMUTEX * mutex)
{
  ReleaseMutex(*mutex);
}
/**********************************************************************/
/* Function : CSCreateEvent                                           */
/* Purpose  : Create an event semaphore                               */
/**********************************************************************/
BOOL CSCreateEvent(CSEVENT * event)
{
  *event = CreateEvent(NULL,FALSE,FALSE,NULL);
  return *event != NULL;
}
/**********************************************************************/

/* Purpose  : Destroy the event semaphore                             */
/**********************************************************************/
void CSDestroyEvent(CSEVENT * event)
{
  if (*event) CloseHandle(*event);
}
/**********************************************************************/
/* Function : CSSetEvent                                              */
/* Purpose  : Set the sempaphore                                      */
/**********************************************************************/
void CSSetEvent(CSEVENT * event)
{
  SetEvent(*event);
}
/**********************************************************************/
/* Function : CSWaitEvent                                             */
/* Purpose  : Wait for an event to be posted                          */
/**********************************************************************/
void CSWaitEvent(CSEVENT * event, int Timeout)
{
  WaitForSingleObject(*event,Timeout);
}
/**********************************************************************/
/* Function : CSLockIt                                                */
/* Purpose  : Lock access to critical resources                       */
/**********************************************************************/
void CSLockIt(CSLOCK * pLock)
{
  int Thread     = __threadid();

  if (CSTracing)
  {
    CSTRACEIN("CSLockIt");
    CSPRINTF((1,"     LOCK(%p) OWNER(%d) CNT(%d) %s\n",
             pLock,pLock->LockMutexOwner,pLock->LockMutexCount,
             pLock->LockMutexOwner!= Thread ? "WAIT":""));
  }

  if (!pLock->LockMutex) pLock->LockMutex = CreateMutex(NULL,FALSE,NULL);

  if (pLock->LockMutexOwner == Thread && pLock->LockMutexCount)
  {
    pLock->LockMutexCount++;
  }
  else
  {
    WaitForSingleObject(pLock->LockMutex,INFINITE);
    pLock->LockMutexOwner  = Thread;
    pLock->LockMutexCount  = 1;
  }
  if (CSTracing)
  {
    CSPRINTF((1,"     LOCK(%p) OWNER(%d) CNT(%d)\n",
             pLock,pLock->LockMutexOwner,pLock->LockMutexCount));
    CSTRACEOUT("CSLockIt",0);
  }
}
/**********************************************************************/
/* Function : CSLock                                                  */
/* Purpose  : Lock access to critical resources                       */
/**********************************************************************/
void CSLock(int Index)
{
  CSLockIt(&Locks[Index]);
}
/**********************************************************************/
/* Function : CSUnlockIt                                              */
/* Purpose  : Unlock access to critical resources                     */
/**********************************************************************/
void CSUnlockIt(CSLOCK * pLock)
{
  int Thread     = __threadid();

  if (CSTracing)
  {
    CSTRACEIN("CSUnlockIt");
    CSPRINTF((1,"     LOCK(%p) OWNER(%d) CNT(%d)\n",
             pLock,pLock->LockMutexOwner,pLock->LockMutexCount));
  }

  if (pLock->LockMutexCount && pLock->LockMutexOwner == Thread)
  {
    pLock->LockMutexCount--;
    if (!pLock->LockMutexCount)
    {
      pLock->LockMutexOwner  = 0;
      ReleaseMutex(pLock->LockMutex);
    }
  }
  else
  {
                                       /* This shouldn't happen       */
    CSTrap(pLock->LockMutexCount);
  }
  if (CSTracing)
  {
    CSPRINTF((1,"     LOCK(%p) OWNER(%d) CNT(%d)\n",
             pLock,pLock->LockMutexOwner,pLock->LockMutexCount));
    CSTRACEOUT("CSUnlockIt",0);
  }
}
/**********************************************************************/
/* Function : CSUnlock                                                */
/* Purpose  : Unlock access to critical resources                     */
/**********************************************************************/
void CSUnlock(int Index)
{
  CSUnlockIt(&Locks[Index]);
}
#else
/**********************************************************************/
/* Function : GetLastError                                            */
/* Purpose  : Return the last error                                   */
/**********************************************************************/
int GetLastError()
{
  return errno;
}
/**********************************************************************/
/* Function : CSCreateMutex                                           */
/* Purpose  : Create a mutex semaphore                                */
/**********************************************************************/
BOOL CSCreateMutex(CSMUTEX * mutex)
{
  BOOL Success = FALSE;
  int rc;

  rc = pthread_mutex_init(&mutex->mutex,NULL);
  if (rc) goto MOD_EXIT;
  mutex->defined = 1;

  Success = TRUE;

MOD_EXIT:
  return Success;
}
/**********************************************************************/
/* Function : CSDestroyMutex                                          */
/* Purpose  : Destroy the mutex                                       */
/**********************************************************************/
void CSDestroyMutex(CSMUTEX * mutex)
{
  if (mutex->defined)
  {
    pthread_mutex_destroy(&mutex->mutex);
    mutex->defined = 0;
  }
}
/**********************************************************************/
/* Function : CSRequestMutex                                          */
/* Purpose  : Request ownership of a mutex                            */
/**********************************************************************/
BOOL CSRequestMutex(CSMUTEX * mutex,int Timeout)
{
  pthread_mutex_lock(&mutex->mutex);
  return TRUE;
}
/**********************************************************************/
/* Function : CSRequestMutex                                          */
/* Purpose  : Release ownership of a mutex                            */
/**********************************************************************/
void CSReleaseMutex(CSMUTEX * mutex)
{
  pthread_mutex_unlock(&mutex->mutex);
}
/**********************************************************************/
/* Function : CSCreateEvent                                           */
/* Purpose  : Create an event semaphore                               */
/**********************************************************************/
BOOL CSCreateEvent(CSEVENT * event)
{
  BOOL Success = FALSE;
  int  rc;

  rc = pthread_mutex_init(&event->mutex,NULL);
  if (rc) goto MOD_EXIT;

  rc = pthread_cond_init(&event->cond,NULL);
  if (rc) goto MOD_EXIT;

  event->defined = 1;
  event->posted  = 0;
  Success = TRUE;

MOD_EXIT:
  return Success;
}
/**********************************************************************/
/* Function : CSDestroyEvent                                          */
/* Purpose  : Destroy the event semaphore                             */
/**********************************************************************/
void CSDestroyEvent(CSEVENT * event)
{
  if (event->defined)
  {
    pthread_mutex_destroy(&event->mutex);
    pthread_cond_destroy(&event->cond);
    event->defined = 0;
  }
}
/**********************************************************************/
/* Function : CSSetEvent                                              */
/* Purpose  : Set the sempaphore                                      */
/**********************************************************************/
void CSSetEvent(CSEVENT * event)
{
  int rc;

  rc = pthread_mutex_lock(&event->mutex);
  if (rc) goto MOD_EXIT;

  if (!event->posted)
  {
    pthread_cond_broadcast(&event->cond);
    event->posted = 1;
  }

MOD_EXIT:
  pthread_mutex_unlock(&event->mutex);
}
#if defined(_LINUX_2) || defined(_SOLARIS_2) || defined(_HPUX_SOURCE)   \
  || defined(AMQ_DARWIN)

#ifndef TIMEVAL_TO_TIMESPEC
#define TIMEVAL_TO_TIMESPEC(tv, ts) {                                   \
        (ts)->tv_sec = (tv)->tv_sec;                                    \
        (ts)->tv_nsec = (tv)->tv_usec * 1000; }
#endif

/*********************************************************************/
/* Function Name: pthread_get_expiration_np                          */
/* Description: Calculate absolutime time from offset                */
/*              SADLY not all pthread libraries have this function   */
/*********************************************************************/
int pthread_get_expiration_np(struct timespec *deltaTime,
                              struct timespec *absTime)
{
  int rc;
  struct timeval tempTimeVal;

  rc = gettimeofday(&tempTimeVal, NULL);

  TIMEVAL_TO_TIMESPEC(&tempTimeVal, absTime);

  if (rc == 0)
  {
    /* absTime set correctly */
    absTime -> tv_sec = absTime -> tv_sec + deltaTime -> tv_sec;
    absTime -> tv_nsec = absTime -> tv_nsec + deltaTime -> tv_nsec;
    if (absTime -> tv_nsec >= 1000000000 )
    {
      absTime -> tv_nsec = absTime -> tv_nsec - 1000000000;
      absTime -> tv_sec++;
    }
  }
  return rc;
}

#endif

/**********************************************************************/
/* Function : getabstime                                              */
/* Purpose  : Function to return absolute time                        */
/**********************************************************************/
void getabstime(int Timeout,struct timespec * abstime)
{
  struct timespec deltatime;

  deltatime.tv_sec   = Timeout / 1000;
  deltatime.tv_nsec  = Timeout % 1000;
  deltatime.tv_nsec *= 1000 * 1000;

  pthread_get_expiration_np(&deltatime,abstime);
}

/**********************************************************************/
/* Function : CSWaitEvent                                             */
/* Purpose  : Wait for an event to be posted                          */
/**********************************************************************/
void CSWaitEvent(CSEVENT * event, int Timeout)
{
  int rc;
  struct timespec abstime;

  rc = pthread_mutex_lock(&event->mutex);
  if (rc) goto MOD_EXIT;

  while (!event->posted)
  {
    if (Timeout == 0) goto MOD_EXIT;
    if (Timeout == INFINITE)
    {
      pthread_cond_wait(&event->cond,&event->mutex);
    }
    else
    {
      getabstime(Timeout,&abstime);
      pthread_cond_timedwait(&event->cond,&event->mutex,&abstime);
    }
  }
  event->posted = 0;

MOD_EXIT:
  pthread_mutex_unlock(&event->mutex);
}

#ifdef _REENTRANT
BOOL CSCreateThread(LPTHREAD_START_ROUTINE  fn,
                    void                  * parm,
                    int                   * pTid)
{
  int            rc;
  pthread_t      thread;
  pthread_attr_t attr;
  rc = pthread_create(&thread, NULL, fn, parm);
  return rc==0;
}
#endif

#endif

/**********************************************************************/
/* Function : CSAccessLibrary                                         */
/* Purpose  : CSLoad a library containing the MQI entry points        */
/**********************************************************************/
int CSAccessLibrary(char         * Library,
                    char         * pPath,
                    char         * ErrorMsg,
                    int            ErrorMsgLen,
                    CSLIBHANDLE  * pLibHandle)
{
  int         rc = 0;
  CSLIBHANDLE lib;
  CHAR        LibraryName[500+1];
  char      * p;
  BOOL        Debug = getenv("MQACCESS_DEBUG") != NULL;

#ifndef AMQ_AS400
  /********************************************************************/
  /* Do we have a fully qualified name ?                              */
  /********************************************************************/
  if (!strchr(Library,PATH_SEP) && pPath && *pPath)
  {
    strcpy(LibraryName,pPath);
    p = LibraryName + strlen(LibraryName) - 1;
    if (*p != PATH_SEP)
    {
      p++;
      *p = PATH_SEP;
    }
    p++;
    strcpy(p,Library);
  }
  else
  {
    strcpy(LibraryName,Library);
  }
  if (Debug) printf("Loading '%s'\n",LibraryName);
#endif
#if defined (WIN32) || defined(WIN64)
  lib = LoadLibraryEx(LibraryName, NULL, 0);
  if (lib == NULL)
  {
    rc = GetLastError();

    if (!ErrorMsg)
    {
      ErrorMsg    = LibraryName;
      ErrorMsgLen = sizeof(LibraryName) - 1;
    }
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  rc,
                  0,
                  ErrorMsg,
                  ErrorMsgLen,
                  0);
    if (Debug) printf("Load failed RC(%d) %s\n",rc,ErrorMsg);
    goto MOD_EXIT;
  }
#else
#ifdef AMQ_AS400
  {
    char              * pModuleName;
    OS_fct_t          * handle;
    Qle_ABP_Info_Long_t actinfo;
    int                 actinfolen;
    Qus_EC_t            errorinfo;

    strcpy(LibraryName,"*LIBL");
    if (pModuleName = strchr(Library,'/'))
    {
      int Len = pModuleName-Library;
      memcpy(LibraryName, Library, Len);
      LibraryName[Len] = 0;
      pModuleName++;
    }
    else
    {
      pModuleName = Library;
    }
    if (Debug) printf("Loading '%s/%s'\n",LibraryName,pModuleName);

    handle = rslvsp(WLI_SRVPGM,
                    pModuleName,
                    LibraryName,
                    _AUTH_NONE);
    if (!handle)
    {
      if (Debug) printf("Load failed of '%s/%s'\n",LibraryName,pModuleName);
      if (ErrorMsg)
      {
        sprintf(ErrorMsg,"Load failed of' %s/%s'",LibraryName,pModuleName);
      }
      rc = 99;
      goto MOD_EXIT;
    }

    errorinfo.Bytes_Provided = sizeof(errorinfo);
    actinfolen = sizeof(actinfo);

    QleActBndPgmLong(&handle,
                     &lib,
                     &actinfo,
                     &actinfolen,
                     &errorinfo);
    if (errorinfo.Bytes_Available > 0)
    {
      rc = 99;
      goto MOD_EXIT;
    }
  }
#else
  dlerror();
  lib = dlopen(LibraryName,RTLD_NOW | RTLD_GLOBAL
#ifdef _AIX
               | RTLD_MEMBER
#endif
               );
  if (!lib)
  {
    char * p = dlerror();
    if (Debug) printf("Load failed %s \n",p);
    if (ErrorMsg && p)
    {
      strncpy(ErrorMsg,p,ErrorMsgLen);
    }
    rc = errno;
  }
#endif
#endif
  *pLibHandle = lib;

MOD_EXIT:
  return rc;
}

#if defined (WIN32) || defined(WIN64)
/**********************************************************************/
/* Function : AccessProc                                              */
/* Purpose  : Access a procedure in a library                         */
/**********************************************************************/
void * CSAccessProc(CSLIBHANDLE lib,char * fn)
{
  return (void *)GetProcAddress(lib,fn);
}
#else
/**********************************************************************/
/* Function : AccessProc                                              */
/* Purpose  : Access a procedure in a library                         */
/**********************************************************************/
void * CSAccessProc(CSLIBHANDLE lib,char * fn)
{
#ifdef AMQ_AS400
  int                 actinfolen;
  Qus_EC_t            errorinfo;
  int                 exptype;
  _OPENPTR            fnptr;
  int                 fnnamelen = strlen(fn);

  errorinfo.Bytes_Provided = sizeof(errorinfo);

  QleGetExpLong(&lib,
                0,
               &fnnamelen,
                fn,
                &fnptr,
                &exptype,
                &errorinfo);
  return fnptr;
#else
  return (void *)dlsym(lib,fn);
#endif
}
#endif
#endif /* End of #ifndef MVS */

#ifdef CHECKSTORAGE
/**********************************************************************/
/* Function : CheckStorage                                            */
/* Purpose  : Check the storage eyecatchers                           */
/**********************************************************************/
void CSCheckStorage(void)
{
  CSSTORAGE * pCS;
  CSSTORAGE * ppCS = NULL;
  char      * pEyeCatcher;
  int         Blocks = 0;

  CSLock(0);

  pCS = pStorage;
  while(pCS)
  {
    Blocks ++;
    if (! CHK_EYE(pCS->ID,STORAGE_EYECATCHERF))
    {
      CSTrap(0);
    }
    pEyeCatcher = ((char *)pCS) + pCS -> Size - sizeof(int );

    if (! CHK_EYE(pEyeCatcher,STORAGE_EYECATCHER))
    {
      CSTrap(0);
    }
    ppCS = pCS;
    pCS  = pCS -> Next;
  }

  if (Blocks != CSMemBlocks)
  {
    CSTrap(Blocks);
  }

  CSUnlock(0);

}
#endif

#if defined (WIN32) || defined(WIN64)
/**********************************************************************/
/* Function : buildsymbolsearchpath                                   */
/* Purpose  : Load the debug entry points                             */
/**********************************************************************/
void buildsymbolsearchpath (char * path,int maxsize)
{
    char   * env;
    char   * p;
    char   * b,* l;

    p = path;

    GetModuleFileName(NULL,p,maxsize);
    b = strchr(p,'\\');
    if (b)
    {
      do
      {
        l = b;
      }
      while (b = strchr(b+1,'\\'));
      *l = 0;
    }
    p+=strlen(p);

    // When the symbol handler is given a custom symbol search path, it will no
    // longer search the default directories (working directory, system root,
    // etc). But we'd like it to still search those directories, so we'll add
    // them to our custom search path.
    strcpy(p,";.\\");
    p+=strlen(p);

    env = getenv("SYSTEMROOT");
    if (env)
    {
      sprintf(p,";%s",env);
      p+=strlen(p);

      sprintf(p,";%s\\system32",env);
      p+=strlen(p);
    }
    env = getenv("_NT_SYMBOL_PATH");
    if (env)
    {
      sprintf(p,";%s",env);
      p+=strlen(p);
    }
    env = getenv("_NT_ALT_SYMBOL_PATH");
    if (env)
    {
      sprintf(p,";%s",env);
      p+=strlen(p);
    }
}

/**********************************************************************/
/* Function : loaddbg                                                 */
/* Purpose  : Load the debug entry points                             */
/**********************************************************************/
BOOL loaddbg(FILE   * file,
             HANDLE   process)
{
  BOOL       Success = FALSE;
  HINSTANCE  dbghelp;
  char       Path[500];
  char     * functionname = "";

  dbghelp = LoadLibrary("dbghelp.dll");
  if (!dbghelp)
  {
    fprintf(file,"Can not load dbghelp.dll RC(%d)\n",GetLastError());
    goto MOD_EXIT;
  }

  functionname = "SymInitialize";
  pSymInitialize = (SymInitialize_t)GetProcAddress(dbghelp, functionname);
  if (pSymInitialize == NULL) goto MOD_EXIT;

  functionname = "SymSetOptions";
  pSymSetOptions = (SymSetOptions_t)GetProcAddress(dbghelp, functionname);
  if (pSymSetOptions == NULL) goto MOD_EXIT;

  buildsymbolsearchpath(Path,sizeof(Path));

  pSymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
  if (!pSymInitialize(process, Path, TRUE))
  {
    fprintf(file,"Symbol load failed RC(%d)\n",GetLastError());
  }

  functionname = "SymFunctionTableAccess64";
  pSymFunctionTableAccess64 = (SymFunctionTableAccess64_t)GetProcAddress(dbghelp, functionname);
  if (pSymFunctionTableAccess64 == NULL) goto MOD_EXIT;

  functionname = "SymGetModuleBase64";
  pSymGetModuleBase64 = (SymGetModuleBase64_t)GetProcAddress(dbghelp, functionname);
  if (pSymGetModuleBase64 == NULL) goto MOD_EXIT;

  functionname = "SymCleanup";
  pSymCleanup = (SymCleanup_t)GetProcAddress(dbghelp, functionname);
  if (pSymCleanup == NULL) goto MOD_EXIT;

  functionname = "SymFromAddr";
  pSymFromAddr = (SymFromAddr_t)GetProcAddress(dbghelp, functionname);
  if (pSymFromAddr == NULL) goto MOD_EXIT;

  functionname = "SymGetLineFromAddr64";
  pSymGetLineFromAddr64 = (SymGetLineFromAddr64_t)GetProcAddress(dbghelp, functionname);
  if (pSymGetLineFromAddr64 == NULL) goto MOD_EXIT;

  functionname = "StackWalk64";
  pStackWalk64 = (StackWalk64_t)GetProcAddress(dbghelp, functionname);
  if (pStackWalk64 == NULL) goto MOD_EXIT;

  Success = TRUE;

MOD_EXIT:
  return Success;
}

// getprogramcounterintelx86 - Helper function that retrieves the program
//   counter (aka the EIP register) for getstacktrace() on Intel x86
//   architecture. There is no way for software to directly read the EIP
//   register. But it's value can be obtained by calling into a function (in our
//   case, this function) and then retrieving the return address, which will be
//   the program counter from where the function was called.
//
//  Notes:
//
//    a) Frame pointer omission (FPO) optimization must be turned off so that
//       the EBP register is guaranteed to contain the frame pointer. With FPO
//       optimization turned on, EBP might hold some other value.
//
//    b) Inlining of this function must be disabled. The whole purpose of this
//       function's existence depends upon it being a *called* function.
//
//  Return Value:
//
//    Returns the return address of the current stack frame.
//
#ifdef _M_IX86
#pragma optimize ("y", off)
#pragma auto_inline(off)
unsigned int  getprogramcounterintelx86 ()
{
    unsigned int  programcounter;

    __asm mov eax, [ebp + 4]         // Get the return address out of the current stack frame
    __asm mov [programcounter], eax  // Put the return address into the variable we'll return

    return programcounter;
}
#pragma auto_inline(on)
#pragma optimize ("y", on)
#endif // _M_IX86

void printframe(FILE         * file,
                HANDLE         m_process,
                STACKFRAME64 * frame)
{
  DWORD             displacement;
  DWORD64           displacement64;
  IMAGEHLP_LINE64   sourceinfo;
  char            * pSource   = "";
  char            * pFunction = "Unknown";
  int               len;
  char              LineNo[10] = "";
#define MAXSYMBOLNAMELENGTH 256
#define SYMBOLBUFFERSIZE (sizeof(SYMBOL_INFO)+(MAXSYMBOLNAMELENGTH+sizeof(TCHAR))-1)
  unsigned char     symbolbuffer[SYMBOLBUFFERSIZE];
  SYMBOL_INFO     * pfunctioninfo = (SYMBOL_INFO *)symbolbuffer;


  memset(&sourceinfo,0,sizeof(sourceinfo));
  sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

  if (pSymGetLineFromAddr64(m_process,
//                            frame->AddrPC.Offset,
                            frame->AddrReturn.Offset,
                            &displacement,
                            &sourceinfo))
  {
    pSource = sourceinfo.FileName;
    len     = strlen(pSource);
    if (len > 30) pSource += (len - 30);
    sprintf(LineNo,"%d",sourceinfo.LineNumber);
  }

  // Try to get the name of the function containing this program
  // counter address.
  if (pSymFromAddr(m_process,
//                   frame->AddrPC.Offset,
                   frame->AddrReturn.Offset,
                   &displacement64,
                   pfunctioninfo))
  {
    pFunction = pfunctioninfo->Name;
  }

  fprintf(file,"%8.8I64X %-30.30s %5.5s %-30.30s\n",
          frame->AddrPC.Offset,
          pSource,
          LineNo,
          pFunction);
}

// getstacktrace - Traces the stack, starting from this function, as far
//   back as possible. Populates the provided CallStack with one entry for each
//   stack frame traced. Requires architecture-specific code for retrieving
//   the current frame pointer and program counter.
//
//  - callstack (OUT): Empty CallStack vector to be populated with entries from
//    the stack trace. Each frame traced will push one entry onto the CallStack.
//
//  Note:
//
//    Frame pointer omission (FPO) optimization must be turned off so that the
//    EBP register is guaranteed to contain the frame pointer. With FPO
//    optimization turned on, EBP might hold some other value.
//
//  Return Value:
//
//    None.
//
#ifdef LEAK_DETECTOR
#pragma optimize ("y", off)
void getstacktrace (FILE * file, HANDLE m_process,HANDLE m_thread)
{
  static unsigned int maxtraceframes = 100;
  DWORD         architecture;
  CONTEXT       context;
  unsigned int  count = 0;
  unsigned int  framepointer;
  STACKFRAME64  frame;
  unsigned int  programcounter;

    // Get the required values for initialization of the STACKFRAME64 structure
    // to be passed to StackWalk64(). Required fields are AddrPC and AddrFrame.
#ifdef _M_IX86
    architecture = IMAGE_FILE_MACHINE_I386;
    programcounter = getprogramcounterintelx86();
    __asm mov [framepointer], ebp  // Get the frame pointer (aka base pointer)
#else
// If you want to retarget Visual Leak Detector to another processor
// architecture then you'll need to provide architecture-specific code to
// retrieve the current frame pointer and program counter in order to initialize
// the STACKFRAME64 structure below.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86

    // Initialize the STACKFRAME64 structure.
    memset(&frame, 0x0, sizeof(frame));
    frame.AddrPC.Offset    = programcounter;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrFrame.Offset = framepointer;
    frame.AddrFrame.Mode   = AddrModeFlat;

    // Walk the stack.
    while (count < maxtraceframes) {
        count++;
        if (!pStackWalk64(architecture, m_process, m_thread, &frame, &context,
                          NULL, pSymFunctionTableAccess64, pSymGetModuleBase64, NULL))
        {
          fprintf(file,"No further stack frames\n");
            // Couldn't trace back through any more frames.
          break;
        }
        //if (frame.AddrFrame.Offset == 0) {
            // End of stack.
        //  fprintf(file,"End of stack frames\n");
        //  break;
        // }

        printframe(file,m_process,&frame);
        // Push this frame's program counter onto the provided CallStack.
        // callstack.push_back(frame.AddrPC.Offset);
    }
}
#pragma optimize ("y", on)
#else
void getstacktrace (FILE * file, HANDLE m_process,HANDLE m_thread)
{
  ;
}
#endif
/**********************************************************************/
/* Function : CSffdc                                                  */
/* Purpose  : Print out debugging information                         */
/**********************************************************************/
void CSffdc(void)
{
  HANDLE   process;
  HANDLE   thread;
  FILE   * file;
  char     Buffer[200];
  time_t   Now;
  char   * pTime;

  file = fopen("c:\\temp\\cs.fdc","w");
  if (!file) goto MOD_EXIT;

  GetModuleFileName(NULL,Buffer,sizeof(Buffer));
  time(&Now);

  pTime = ctime(&Now);

  fprintf(file,"FDC %s",pTime);
  fprintf(file,"Module:%s\n\n",Buffer);

  process = GetCurrentProcess();

  if (!pStackWalk64)
  {
    if (!loaddbg(file,process)) goto MOD_EXIT;
  }

  thread  = GetCurrentThread();

  fprintf(file,"Stack\n");
  getstacktrace(file,process,thread);

MOD_EXIT:
  if (file) fclose(file);
}
#else
/**********************************************************************/
/* FFDC Not supported on other platforms                              */
/**********************************************************************/
void CSffdc()
{
  ;
}
#endif

/**********************************************************************/
/* Function : CSTrapfn                                                */
/* Purpose  : Just trap for test purposes                             */
/**********************************************************************/
void CSTrapfn(char * file,
              char * timestamp,
              int    line,
              int    value)
{
  FILE * f;
  char * pfile= file;

  f = fopen("c:\\temp\\cstrap.txt","w");
  if (f)
  {
    while (strchr(pfile,'\\')) pfile = strchr(pfile,'\\')+1;
    fprintf(f,"cstrap line %d file %s (%s) value %d\n",
              line,pfile,timestamp,value);
    fclose(f);
  }

  /* CSffdc(); Doesn't work just yet */
#if defined (WIN32) || defined(WIN64)
  DebugBreak();
#else
  {
    int    i;
    i    = 1/zero;
    zero = 0;
  }
#endif
}

/*********************************************************************/
/* Function Name: getreltime                                         */
/* Description: Get a relative time expressed in milliseconds        */
/*********************************************************************/
int CSGetRelTime()
{
#if defined(AMQ_UNIX) || defined(MVS) || defined(AMQ_AS400)
  int reltime;
  static int reltimebase = 0;
  struct timeval TimeVal;

  gettimeofday(&TimeVal, NULL);

  if (!reltimebase) reltimebase = TimeVal.tv_sec;
  reltime  = TimeVal.tv_sec;
  reltime -= reltimebase;
  reltime *= 1000;
  reltime += TimeVal.tv_usec / 1000;

  return reltime;
#else
  return GetTickCount();
#endif
}

/**********************************************************************/
/* Function : CSSleep                                                 */
/* Purpose  : Sleep for so many seconds                               */
/**********************************************************************/
void CSSleep(int d)
{
#if defined (WIN32) || defined(WIN64)
   Sleep(d*1000);
#endif
#ifdef AMQ_UNIX
   sleep(d);
#endif
}

/**********************************************************************/
/* Function : CSMSSleep                                               */
/* Purpose  : Sleep for so many milliseconds                          */
/**********************************************************************/
void CSMSSleep(int d)
{
#if defined (WIN32) || defined(WIN64)
   Sleep(d);
#endif
#if defined(AMQ_UNIX)|| defined(MVS)
   int secs = d / 1000;

   if (secs)
   {
     sleep(secs);
     d -= secs*1000;
   }
   if (d > 0) usleep(d*1000);
#endif
}

/**********************************************************************/
/* Function : CSTreeFind                                              */
/* Purpose  : Find an element in a binary tree                        */
/**********************************************************************/
void * CSTreeFind(void   * pRoot,
                  void   * Key,
                  int      KeySize)
{
  CSTREE * pElem = (CSTREE *) pRoot;
  int      comp;

  if (KeySize == -1) KeySize = strlen((char *)Key)+1;
                                       /* Find the item               */
  while(pElem)
  {
    switch(KeySize)
    {
      case -2:
           {
             char * p1 = *(char **)pElem->Key;
             comp = stricmp(Key,p1);
             if (comp == 0) comp = strcmp(Key,p1);
           }
           break;
      case -3:
           {
             char * p1 = *(char **)pElem->Key;
             comp = strcmp(Key,p1);
           }
           break;
      case -4:
           {
             int * p1 = (int *)Key;
             int * p2 = (int *)&pElem->Key;
             comp = *p1 - *p2;
           }
           break;
      default:
#ifdef _DEBUG
          if (KeySize <= 0) CSDebugTrap(KeySize);
#endif
          comp = memcmp(Key, pElem -> Key, KeySize);
          break;
    }
    if (comp > 0)
    {
      pElem = pElem -> Sub[1];
    }
    else if (comp < 0)
    {
      pElem = pElem -> Sub[0];
    }
    else
                                       /* Found it                    */
      goto MOD_EXIT;
  }
MOD_EXIT:
  return pElem;
}

/**********************************************************************/
/*                                                                    */
/*  qinit: Initialize queue.                                          */
/*                                                                    */
/*  Parameters:                                                       */
/*                                                                    */
/*    q         Pointer to a queue, or NULL if the user wishes to     */
/*              leave it to qinit to allocate the queue.              */
/*                                                                    */
/*  Return values:                                                    */
/*                                                                    */
/*    non-NULL  Queue has been initialized.                           */
/*    NULL      Insufficient memory.                                  */
/**********************************************************************/
QUEUE * qinit(QUEUE * q)
{
  if (q || (q = malloc(sizeof(QUEUE))) != NULL)
  {
    q->begin = NULL;
    q->end   = &q->begin;
  }
  return q;
}

/**********************************************************************/
/*                                                                    */
/*  qinsert: append an item to the queue.                             */
/*                                                                    */
/*  Parameters:                                                       */
/*                                                                    */
/*    q         Pointer to a queue. It is assumed the queue has been  */
/*              initialized by a call to qinit.                       */
/*                                                                    */
/*    d         Item to be appended.                                  */
/*                                                                    */
/*  Return values:                                                    */
/*                                                                    */
/*    1         The item has been appended.                           */
/*                                                                    */
/*    0         The item could not be appended. Either the queue      */
/*              pointer provided was NULL, or the function was unable */
/*              to allocate the amount of memory needed for a new     */
/*              queue item.                                           */
/**********************************************************************/
int qinsert(QUEUE * q, void * d)
{
  if (!q || !(*q->end = malloc(sizeof(QENTRY)))) return 0;
  (*q->end)->d    = d;
  (*q->end)->next = NULL;
  q->end          = &((*q->end)->next);
  return 1;
}

/**********************************************************************/
/*  qremove: remove an item from the queue.                           */
/*                                                                    */
/*  Parameters:                                                       */
/*                                                                    */
/*    q         Pointer to a queue.                                   */
/*                                                                    */
/*    d         Pointer to the QDATUM variable that will hold the     */
/*              datum corresponding to the queue item.                */
/*                                                                    */
/*  Return values:                                                    */
/*                                                                    */
/*    non-NULL  An item has been removed. The variable that d points  */
/*              to now contains the datum associated with the item    */
/*              in question.                                          */
/*                                                                    */
/*    NULL      No item could be removed. Either the queue pointer    */
/*              provided was NULL, or the queue was empty. The memory */
/*              location that d points to has not been modified.      */
/**********************************************************************/
void * qremove(QUEUE * q, void ** d)
{
  QENTRY * tmp;

  if (!q || !q->begin) return NULL;
  tmp = q->begin;
  if (!(q->begin = q->begin->next)) q->end = &q->begin;
  *d = tmp->d;
  free(tmp);
  return d;
}

/**********************************************************************/
/*                                                                    */
/*  qpeek: access an item without removing it from the queue.         */
/*                                                                    */
/*  Parameters:                                                       */
/*                                                                    */
/*    q         Pointer to a queue.                                   */
/*                                                                    */
/*    d         Pointer to the QDATUM variable that will hold the datu*/
/*              associated with the first item in the queue, i. e.,   */
/*              the item that would be removed had qremove been called*/
/*              instead of qpeek.                                     */
/*                                                                    */
/*  Return values:                                                    */
/*                                                                    */
/*    See qremove.                                                    */
/*                                                                    */
/**********************************************************************/
void * qpeek(QUEUE * q, void **d)
{
  if (!q || !q->begin) return NULL;
  *d = q->begin->d;
  return d;
}
/**********************************************************************/
/* Function : CSFindThread                                            */
/* Purpose  : Return a pointer to the thread block                    */
/**********************************************************************/
static CSTHREAD * CSFindThread(int tid)
{
  int i=0;
  for (i=0; i<MAX_THREADS; i++)
  {
    if (CSThreads[i].InUse && CSThreads[i].tid == tid) return &CSThreads[i];
  }
  for (i=0; i<MAX_THREADS; i++)
  {
    if (!CSThreads[i].InUse)
    {
      CSThreads[i].InUse  = 1;
      CSThreads[i].tid    = tid;
      CSThreads[i].Indent = 0;
      return &CSThreads[i];
    }
  }

  return &CSThreads[MAX_THREADS-1];
}


/**********************************************************************/
/* Function : CSTraceHeader                                           */
/* Purpose  : Add the front part of each trace record                 */
/**********************************************************************/
static CSTHREAD * CSTraceHeader  (int adjust,BOOL WriteHeader)
{
  int        i;
  CSTHREAD * pThread = NULL;
#if defined (WIN32) || defined (WIN64)
  int        tid = __threadid();

  if (!TraceLock) TraceLock = CreateMutex(NULL,FALSE,NULL);

  if (TraceLockOwner == tid)
  {
    TraceLockUseCount++;
  }
  else
  {
    WaitForSingleObject(TraceLock,INFINITE);
    TraceLockOwner    = tid;
    TraceLockUseCount = 1;
  }

  pThread = CSFindThread(tid);
  pThread->Indent+=adjust;
  if (WriteHeader)
  {
    fprintf(CSTraceFile,"TID(%5d):",tid);
    for (i=0; i<=pThread->Indent; i++) fputc(' ',CSTraceFile);
  }
#endif
  return pThread;
}

/**********************************************************************/
/* Function : CSTraceRelease                                          */
/* Purpose  : Release access to the trace block                       */
/**********************************************************************/
static void CSTraceRelease(CSTHREAD * pThread)
{
#if defined (WIN32) || defined(WIN64)
  TraceLockUseCount--;
  if (!TraceLockUseCount)
  {
    TraceLockOwner = -1;
    ReleaseMutex(TraceLock);
  }
#endif
}

/**********************************************************************/
/* Function : CSTraceGroup                                            */
/* Purpose  : Start a group of trace records                          */
/**********************************************************************/
void CSTraceGroup()
{
  CSTraceHeader(0,FALSE);
}
/**********************************************************************/
/* Function : CSTraceGroupEnd                                         */
/* Purpose  : End of a group of trace records                         */
/**********************************************************************/
void CSTraceGroupEnd()
{
  CSTHREAD * pThread = CSTraceHeader(0,FALSE);
  CSTraceRelease(pThread);
  CSTraceRelease(pThread);
}
/**********************************************************************/
/* Function : CSTraceIn                                               */
/* Purpose  : Trace a function in                                     */
/**********************************************************************/
void      CSTraceIn      (char           * func)
{
  CSTHREAD * pThread;

#ifdef DEBUG
  if (CSDoCheckStorageTrace) CSCheckStorage();
#endif
  if (!CSTraceFile) goto MOD_EXIT;

  pThread = CSTraceHeader(0,func!=NULL);
  if (func)
  {
    fprintf(CSTraceFile,"--> %s\n",func);
    fflush(CSTraceFile);
  }
  pThread->Indent++;
  CSTraceRelease(pThread);
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : CSTraceOut                                              */
/* Purpose  : Trace a function out                                    */
/**********************************************************************/
void      CSTraceOut     (char           * func,
                          int              rc)
{
  CSTHREAD * pThread;

#ifdef DEBUG
  if (CSDoCheckStorageTrace) CSCheckStorage();
#endif
  if (!CSTraceFile) goto MOD_EXIT;

  pThread = CSTraceHeader(-1,func!=NULL);
  if (func)
  {
    fprintf(CSTraceFile,"<-- %s RC(0x%8.8X)\n",func,rc);
    fflush(CSTraceFile);
  }
  CSTraceRelease(pThread);
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : CSPrintf                                                */
/* Purpose  : Issue a printf                                          */
/**********************************************************************/
void      CSPrintf       (int              nl,
                          char           * format,
                          ...)
{
  CSTHREAD * pThread;
  va_list v;
#ifdef DEBUG
  if (CSDoCheckStorageTrace) CSCheckStorage();
#endif
  if (!CSTraceFile) goto MOD_EXIT;

  if (nl) pThread = CSTraceHeader(0,TRUE);
  va_start(v,format);
  vfprintf(CSTraceFile,format,v);
  va_end(v);
  fflush(CSTraceFile);
  if (nl) CSTraceRelease(pThread);
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : CSTraceStart                                            */
/* Purpose  : Trace a function out                                    */
/**********************************************************************/
int       CSTraceStart   (char * tracefile)
{
  int rc = 0;

  if (!CSTraceFile)
  {
    char FileName[200];

    sprintf(FileName,"%s%s",CSFileRoot,tracefile);
    CSTraceFile = fopen(FileName,"w");
    if (!CSTraceFile) rc = 99;
                 else CSTracing = 1;
  }
  return rc;
}

/**********************************************************************/
/* Function : CSTraceOut                                              */
/* Purpose  : Trace a function out                                    */
/**********************************************************************/
int       CSTraceStop    ()
{
  if (CSTraceFile) fclose(CSTraceFile);
  CSTracing   = 0;
  CSTraceFile = NULL;
  return 0;
}

/**********************************************************************/
/* Function : CSCheckOpts                                             */
/* Purpose  : Check the options seem reasonable                       */
/**********************************************************************/
void      CSCheckOpts    (CBOPT          * pOptions)
{
  int Gap = 5;
  int ScreenWidth;
  int blob,blobs;
  if (!pOptions -> ScreenWidth)     pOptions->ScreenWidth = 80;
  if (pOptions -> ScreenWidth < 20) pOptions->ScreenWidth = 20;
  if (!pOptions -> DataWidth)
  {
    ScreenWidth = pOptions->ScreenWidth;
    if (pOptions->Options & MF_OFFSET) ScreenWidth -= 10;

    blob = 8;
    if (pOptions->Options & MF_HEXSEP) blob ++;
    if (!(pOptions->Options & MF_NO_ASCII_COLUMN))
    {
      blob += 4;

      if (!pOptions->AsciiColumn) pOptions->AsciiColumn = 2;
      ScreenWidth -= pOptions->AsciiColumn;
    }

    blobs = ScreenWidth / blob;
    if (blobs < 1) blobs = 1;

    if (pOptions->Options & MF_NO_ASCII_COLUMN)
    {
      pOptions -> AsciiColumn = 0;
      pOptions -> DataGap     = 0;
    }
    else
    {
      pOptions -> DataGap = ScreenWidth - (blobs * blob);
      if (pOptions -> DataGap < pOptions->AsciiColumn)
        pOptions -> DataGap = pOptions->AsciiColumn;
    }
    pOptions -> DataWidth = blobs * 4;
  }
}

/**********************************************************************/
/* Function : CSDumpHex                                               */
/* Purpose  : Dump a buffer in HEX to the output function             */
/**********************************************************************/
int   CSDumpHex(int              (outfn)(void *,CBOPT *,char *,int ),
                void            * Parm,
                unsigned char   * pMsg,
                int               Len,
                CBOPT           * pOptions)
{
  int     rc = 0;
  char    Buffer[256];
  int     i,len;
  int     DataWidth;
  static  MQFIELD Field;
  int     hexopt;

  if (!pOptions->Checked) CSCheckOpts(pOptions);
  pOptions -> OutFormat = CSO_FORMATTED;

  DataWidth = pOptions ->DataWidth;

  if (Len < 0)
  {
    if (!pOptions->pData)
    {
      rc = 99;
      goto MOD_EXIT;
    }
    if ((char *)pMsg != pOptions->pData->Data)
    {
      rc = 99;
      goto MOD_EXIT;
    }
    Field.Length  = -1;
    Len           = pOptions->pData->Length;
  }
  else
  {
    Field.Length  = Len;
  }
  Field.Name       = "HEX";
  Field.FldOptions = 0;
  Field.Type       = MQFT_HEX;
  Field.Value      = NULL;

  pOptions -> LineMsgOffset    = 0;
/*pOptions -> FieldStartOfData = (char *)pMsg; */
  pOptions -> FieldMsgOffset   = 0;
  pOptions -> pField = &Field;

  for (i=0 ; i<Len ; i+=DataWidth)
  {
    len = Len-i;
    if (len>DataWidth) len=DataWidth;
    if (FORMATTING)
    {
      unsigned char * pData = (unsigned char *)Buffer;
      if (pOptions->Options & MF_OFFSET)
      {
        sprintf(Buffer,"+%8.8X ",i);
        pData += 10;
        pOptions -> LineDataOffset = 10;
      }
      else
        pOptions -> LineDataOffset = 0;

      pOptions -> LineMsgOffset     = i;
      pOptions -> LineMsgLength     = len;
      pOptions -> LineDataLength    = len*2;
      pOptions -> LineDataMaxLength = DataWidth*2;

      hexopt = 0;
      if (pOptions->Options & MF_HEXSEP)   hexopt |= HO_SEPARATORS;
      if (pOptions->Options & CSO_EBCDIC ) hexopt |= HO_TEXTISEBCDIC;

      HexStr( pMsg+i,
              len,
              DataWidth,
              pData,
              pOptions->AsciiColumn,
              hexopt);
    }
    CALLOUTFN(Parm,Buffer,(int )strlen(Buffer));
  }
MOD_EXIT:
  return rc;
}

/**********************************************************************/
/* Function : CSIssueCommand                                          */
/* Purpose  : Issue a command to the OS                               */
/**********************************************************************/
int CSIssueCommand (char           * Command,
                    char          ** ppErrorText)
{
  int                 Success    = 0;
  char              * pErrorText = CSGetMem(MAX_ERROR_TEXT_SIZE,"ErrorText");
  /* int                 InString   = 0; */
#if defined (WIN32) || defined (WIN64)
  STARTUPINFO         StartupInfo;
  PROCESS_INFORMATION ProcessInfo;
  HANDLE              fstdin     = INVALID_HANDLE_VALUE;
  HANDLE              fstdout    = INVALID_HANDLE_VALUE;
  /* char              * pstdin     = NULL; */
  /* char              * pstdout    = NULL; */
  int                 rc         = 0;
  /* int                 Append     = 0; */
  BOOL                Inherit = FALSE;
#endif

  if (!pErrorText) goto MOD_EXIT;
  *ppErrorText = pErrorText;
  strcpy(pErrorText,"Not enough memory");
  if (!(*Command))
  {
    strcpy(pErrorText,"No command supplied");
    goto MOD_EXIT;
  }
#if defined (WIN32) || defined(WIN64)
#ifdef NOTWORKING
  /********************************************************************/
  /* Now check for stdin/stdout redirectors                           */
  /********************************************************************/
  p = Command;
  while (*p)
  {
    if (*p == '"') InString = !InString;
    if (!InString)
    {
      if (*p == '<') { *p = 0; pstdin = p+1; }
      if (*p == '>')
      {
        *p = 0;
        if (*(p+1)=='>') { Append = 1; p++; };
        pstdout = p+1;
      }
    }
    p++;
  }
  /********************************************************************/
  /* Now do we have redirections                                      */
  /********************************************************************/
  if (pstdin)
  {
    fstdin = CreateFile( pstdin,
                                 GENERIC_READ,
                                                 FILE_SHARE_READ,
                                                 NULL,
                                 OPEN_EXISTING,
                                                 FILE_ATTRIBUTE_NORMAL,
                                                 NULL );

    if (fstdin==INVALID_HANDLE_VALUE)
    {
          rc = GetLastError();
      sprintf(pErrorText,"Can't open input file '%s' RC(%d)",pstdin,rc);
      goto MOD_EXIT;
    }
  }

  if (pstdout)
  {
    if (Append)
      fstdout = CreateFile( pstdout,
                                    GENERIC_WRITE,
                                                    0,
                                                    NULL,
                                    OPEN_ALWAYS,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    NULL );
        else
      fstdout = CreateFile( pstdout,
                                GENERIC_WRITE,
                                            0,
                                                    NULL,
                                    CREATE_ALWAYS,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    NULL );
    if (fstdout==INVALID_HANDLE_VALUE)
    {
      rc = GetLastError();
      sprintf(pErrorText,"Can't open output file '%s' RC(%d) ",pstdout,rc);
      goto MOD_EXIT;
    }
  }
#endif
  /********************************************************************/
  /* Ok, now start the process                                        */
  /********************************************************************/

  memset(&StartupInfo,0,sizeof(StartupInfo));
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.hStdInput  = fstdin;
  StartupInfo.hStdOutput = fstdout;
  StartupInfo.hStdError  = fstdout;
  if (fstdin  != INVALID_HANDLE_VALUE ||
      fstdout != INVALID_HANDLE_VALUE)
  {
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    Inherit = TRUE;
    if (fstdin  != INVALID_HANDLE_VALUE)
    {
      StartupInfo.hStdInput  = fstdin;
    }
    if (fstdout != INVALID_HANDLE_VALUE)
    {
      StartupInfo.hStdOutput = fstdout;
      StartupInfo.hStdError  = fstdout;
    }
  }
  StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_HIDE;

  if (!CreateProcess(NULL,                        /* Command NULL     */
                     Command,
                     NULL,                        /* Security         */
                     NULL,                        /* Security         */
                     Inherit,                     /* Inherit Handles  */
                     0, //DETACHED_PROCESS,
                     NULL,                        /* Environment      */
                     NULL,                        /* Current Directory*/
                     &StartupInfo,
                     &ProcessInfo))
  {
    rc = GetLastError();
    sprintf(pErrorText,"Command failed RC(%d) ",rc);
    goto MOD_EXIT;
  }

#endif
  Success = 1;

MOD_EXIT:
#if defined (WIN32) || defined(WIN64)
  if (!Success && rc)
  {
    int Len = strlen(pErrorText);

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  rc,
                  0,
                  &pErrorText[Len],
                  MAX_ERROR_TEXT_SIZE - Len,
                  NULL);
  }
  if (fstdin  != INVALID_HANDLE_VALUE) CloseHandle(fstdin);
  if (fstdout != INVALID_HANDLE_VALUE) CloseHandle(fstdout);
#endif
  if (Success && pErrorText)
  {
    CSFreeMem(pErrorText);
    *ppErrorText = NULL;
  }
  return Success;
}

/**********************************************************************/
/* Function : CSGetMem                                                */
/* Purpose  : Allocate OS/2 storage                                   */
/**********************************************************************/
void * CSGetMem(int  size,char * Name)
{
  CSSTORAGE * pCS = NULL;

  CSTRACEIN("CSGetMem");
  CSLock(0);

  if (!Name) Name="";

#ifdef CHECKSTORAGE
  if (size <= 0)
  {
    CSTrap(size);
  }
  if (CSDoCheckStorage) CSCheckStorage();
#endif
                                       /* Make room for storage       */
  size += sizeof(CSSTORAGE) + sizeof(int );
  /********************************************************************/
  /* Do we allocate from the heap or main memory ?                    */
  /********************************************************************/
  if (size < MAX_HEAP_SIZE)
  {
                                       /* Small....use the heap       */
    pCS = (CSSTORAGE *)malloc((int)size);
  }
  /********************************************************************/
  /* If we didn't get it use the main memory                          */
  /********************************************************************/
  if (!pCS)
  {
#if defined(WIN31)
    /******************************************************************/
    /* Windows 3.1 memory allocation                                  */
    /******************************************************************/
    HGLOBAL hArea;
    hArea = GlobalAlloc( GMEM_MOVEABLE, (int)size );
    if (!hArea)
    {
      pCS = NULL;
      goto MOD_EXIT;
    }

    pCS          = (CSSTORAGE *) GlobalLock( hArea );
    pCS -> hArea = hArea;
#else
    /******************************************************************/
    /* AIX (or other) memory allocation                               */
    /******************************************************************/
    pCS = (CSSTORAGE *)malloc(size);
    if (!pCS)
    {
      CSDebugTrap(0);
      goto MOD_EXIT;
    }
#endif
    pCS -> Type = ST_MAIN;
  }
  else
    pCS -> Type = ST_HEAP;

  /********************************************************************/
  /* Initialise storage header                                        */
  /********************************************************************/
  pCS -> Size = size;
#ifdef _DEBUG
  {
    char      * pEyeCatcher;

    SET_EYE(pCS->ID,STORAGE_EYECATCHERF);
    pEyeCatcher          = ((char *)pCS) + pCS -> Size - sizeof(int );
    SET_EYE(pEyeCatcher,STORAGE_EYECATCHER);
    if (pStorage) pStorage -> Prev = pCS;
    pCS -> Next          = pStorage;
    pCS -> Prev          = NULL;
    pStorage             = pCS;

    strncpy(pCS->Name,Name,sizeof(pCS->Name));
                                         /* Initialise to rubbish       */
    memset((pCS+1),'U',(int)size-sizeof(CSSTORAGE)-sizeof(int ));
  }
#endif
  CSMemBlocks++;
  CSMemCount++;
  CSMemAlloc += pCS -> Size;
  CSMemHigh   = max(CSMemAlloc,CSMemHigh);
  pCS++;

MOD_EXIT:

  CSUnlock(0);
  CSPRINTF((1,"CSGetMem Ptr:%p\n",pCS));
  CSTRACEOUT("CSGetMem",0);

  return pCS;
}

/**********************************************************************/
/* Function : CSFreeMem                                               */
/* Purpose  : Free the storage allocated by GetMem                    */
/**********************************************************************/
void CSFreeMem(void * p)
{
  CSSTORAGE * pCS = (CSSTORAGE *) p;

  CSTRACEIN("CSFreeMem");
  CSPRINTF((1,"CSFreeMem Ptr:%p\n",pCS));
  CSLock(0);

#ifdef CHECKSTORAGE
  {
    char      * pEyeCatcher;

    pCS--;
    pEyeCatcher = ((char *)pCS) + pCS -> Size - sizeof(int );

    if (!CHK_EYE(pEyeCatcher,STORAGE_EYECATCHER))
    {
      CSTrap(0);
    }

    {
      CSSTORAGE * pCSl = NULL;
      CSSTORAGE * pCSn = pStorage;

      if (CSDoCheckStorage)
      {
        CSCheckStorage();

        /**************************************************************/
        /* Check that we're in the chain                              */
        /**************************************************************/
        while (pCSn)
        {
          if (pCSn == pCS) break;
          pCSn = pCSn -> Next;
        }
        if (!pCSn)
        {
          CSTrap(0);
        }
      }
      /****************************************************************/
      /* If there was a previous update it to point to the next       */
      /****************************************************************/

      if (pCS -> Next)
      {
        pCS -> Next -> Prev = pCS -> Prev;
      }

      if (pCS -> Prev)
      {
        pCS -> Prev -> Next = pCS -> Next;
      }
      else
      {
        pStorage     = pCS -> Next;
        if (pStorage) pStorage -> Prev = NULL;
      }
    }
  }
#else
  pCS--;
#endif

#ifdef _DEBUG
                                       /* Scrub out previous data     */
                                       /* including eyecatcher        */
  memset((pCS+1),'F',(int)(pCS->Size-sizeof(CSSTORAGE)));
#endif

  CSMemAlloc -= pCS -> Size;
  CSMemBlocks--;
  if (pCS -> Type & ST_HEAP)
    free(pCS);
  else
#if defined(WIN31)
    GlobalFree(pCS -> hArea);
#else
    free(pCS);
#endif
  CSUnlock(0);
  CSTRACEOUT("CSFreeMem",0);
}
/**********************************************************************/
/* Function : CSInitialise                                            */
/* Purpose  : Initialise Common Services                              */
/**********************************************************************/
void CSInitialise(char * filename)
{
  strcpy(CSFileRoot,filename);
}

/**********************************************************************/
/* Function : CSTerminate                                             */
/* Purpose  : Free CS services                                        */
/**********************************************************************/
void CSTerminate(void)
{
  CSTreeDelete(&pStrings,NULL,NULL);

#ifdef DEBUG
  if (*CSFileRoot)
  {
    FILE * file;
    CBOPT  Opts;
    strcat(CSFileRoot,"CS.STG");
    file = fopen(CSFileRoot,"w");
    if (!file) goto MOD_EXIT;
    fprintf(file,"Unfreed storage >\n");
    memset(&Opts,0,sizeof(Opts));
    while (pStorage)
    {
      fprintf(file,"  %4.4s %6d bytes @ %p %.20s\n",
             (pStorage+1),
             pStorage->Size,
             pStorage,
             pStorage -> Name);

      CSDumpHex(&LineOut,
                file,
      (char *)  pStorage,
                pStorage->Size,
               &Opts);

      pStorage = pStorage -> Next;
    }
    fclose(file);
  }
MOD_EXIT:
#endif
#if defined (WIN32) || defined (WIN64)
  {
    int i;
    for (i=0; i<sizeof(Locks)/sizeof(Locks[0]);i++)
    {
      if (Locks[i].LockMutex) CloseHandle(Locks[i].LockMutex);
    }
  }
#endif
  ;
}

static int  LineOut(FILE    * file,
                    CBOPT   * CbOpt,
                    char    * pLine,
                    int       Length)
{
  fwrite( pLine , (int)Length, 1, file);
  fputc('\n',file);
  return 0;
}

/**********************************************************************/
/* Function : CSGetBlock                                              */
/* Purpose  : Get a Block of storage                                  */
/**********************************************************************/
CSBLOCK * CSGetBlock(int  BlockSize)
{
  CSBLOCK * pcsb = (CSBLOCK *)CSGetMem(BlockSize,"CSBLOCK");
  if (pcsb)
  {
    memset(pcsb,0,sizeof(CSBLOCK));
    pcsb -> Size = BlockSize;
    pcsb -> Used = sizeof(CSBLOCK);
  }
  return pcsb;
}

/**********************************************************************/
/* Function : CSFreeBlock                                             */
/* Purpose  : Free a block(s) of storage                              */
/**********************************************************************/
void CSFreeBlock(CSBLOCK * pcsb)
{
  if (pcsb -> Next) CSFreeBlock(pcsb -> Next);
  CSFreeMem(pcsb);
}

/**********************************************************************/
/* Function : CSGetSubBlock                                           */
/* Purpose  : Get a some storage from the block                       */
/**********************************************************************/
void * CSGetSubBlock(CSBLOCK * pcsb, int  Size)
{
  char       * p = NULL;
  CSBLOCK    * plcsb;
  CSSUBBLOCK * pSub;
  CSSUBBLOCK * plSub;
  CSSUBBLOCK * pBest;
  CSSUBBLOCK * pLargest;
  CSSUBBLOCK * pNextLargest = NULL;
  if (!pcsb) goto MOD_EXIT;
                                       /* Add suballoc overhead       */
  Size += sizeof(pSub -> Size);
  while (pcsb)
  {
    /******************************************************************/
    /* Check whether we have space in the free'd areas                */
    /******************************************************************/
    if (Size <= pcsb -> LargestSub)
    {
                                       /* Find a subblock             */
      pSub     = pcsb -> pSub;
      pBest    = pSub;
      pLargest = pSub;
      plSub    = NULL;
      while (pSub)
      {
                                       /* Big enough ?                */
        if (Size <= pSub -> Size)
        {
                                       /* Ok, it's big enough         */
          if (pSub -> Size < pBest -> Size)
          {
                                       /* a better match ?            */
            pBest = pSub;
          }
                                       /* This one larger ?           */
          if (pSub -> Size > pLargest -> Size)
          {
            pNextLargest = pLargest;
            pLargest     = pSub;
          }
        }
        plSub = pSub;
        pSub  = pSub -> Next;
      }
                                       /* Remove from the list        */
      if (plSub) plSub -> Next = pSub -> Next;
            else pcsb  -> pSub = pSub -> Next;
                                       /* Set new largest if we're    */
                                       /* taking this one             */
      if (pLargest == pSub)
      {
        if (pNextLargest && pNextLargest != pSub)
        {
          pcsb -> LargestSub = pNextLargest -> Size;
        }
        else
        {
                                       /* There can't be any more     */
          if (pcsb -> pSub) CSTrap(0);
          pcsb -> LargestSub = 0;
        }
      }
                                       /* Return this area            */
      p = ((char *)pSub) + sizeof(short);
      goto MOD_EXIT;
    }
                                       /* Return this area            */
    if (pcsb -> Size - pcsb -> Used >= Size) break;
    plcsb = pcsb;
    pcsb  = pcsb -> Next;
  }
                                       /* Did we find one big enough? */
  if (!pcsb)
  {
    pcsb = CSGetBlock(plcsb -> Size);
    if (!pcsb) goto MOD_EXIT;
    plcsb -> Next = pcsb;
  }
                                       /* Reserve the storage         */
  p             = (char *) pcsb;
  p            += pcsb -> Used;
  * (short *)p  = (short)Size;
  p            += sizeof(short);
  pcsb -> Used += Size;

MOD_EXIT:
  return p;
}

/**********************************************************************/
/* Function : CSStringAdd                                             */
/* Purpose  : Add to the list of stored strings                       */
/**********************************************************************/
char    * CSStringAdd    (char           * p,
                          int              len)
{
  char     * pStr;
  CSTREE   * pString;
  typedef struct
  {
    CSTREE Tree;
    char   Data[MAX_STRING];
  } STRINGELEM;

  STRINGELEM   Elem;
  STRINGELEM * pNewElem = NULL;
  STRINGELEM * pElem = &Elem;

  if (!p)
  {
    pStr = NULL;
    goto MOD_EXIT;
  }
  if (len < 0)
  {
#ifdef DEBUG
    CSTrap(len);
#endif
    pStr = NULL;
    goto MOD_EXIT;
  }

  if (len >= MAX_STRING)
  {
    pNewElem = CSGetMem(sizeof(CSTREE) + len + 5,"CSSTRING");
    if (!pNewElem)
    {
      pStr = NULL;
      goto MOD_EXIT;
    }
    pElem = pNewElem;
  }

  memcpy(pElem->Tree.Key,p,len);
  pElem->Tree.Key[len] = 0;
                                       /* Find string                 */
  pString = CSTreeFind(pStrings,pElem->Tree.Key,len+1);
  if (pString)
  {
    pString -> Type++;
    pStr = pString -> Key;
    goto MOD_EXIT;
  }

  memset(&Elem,0,CSOFFSETOF(CSTREE,Key));
  pString = CSTreeAdd(&pStrings,0,&Elem,sizeof(CSTREE)+len,len+1);
  if (!pString)
  {
    pStr = NULL;
    goto MOD_EXIT;
  }
  pStr      = pString->Key;
                                       /* Instance counter            */
  pString-> Type = 1;
MOD_EXIT:
  if (pNewElem) CSFreeMem(pNewElem);
  return pStr;
}

/**********************************************************************/
/* Function : CSStringDelete                                          */
/* Purpose  : Delete a string pointer                                 */
/**********************************************************************/
void      CSStringDelete (char           * p)
{
  CSTREE * pElem = (CSTREE *) (p - CSOFFSETOF(CSTREE,Key));

  if (!p) goto MOD_EXIT;
#ifdef DEBUG
  {
                                       /* Check that it looks right   */
    char      * pEyeCatcher;
    CSSTORAGE * pCS = (CSSTORAGE *)pElem;

    pCS--;
    pEyeCatcher = ((char *)pCS) + pCS -> Size - sizeof(int );

    if (!CHK_EYE(pEyeCatcher,STORAGE_EYECATCHER))
    {
      CSTrap(0);
    }
  }
#endif
  pElem -> Type --;

  if (!pElem -> Type)
  {
    CSTreeDeleteItem(&pStrings,NULL,NULL,pElem,strlen(p)+1);
  }
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : CSStringReplace                                         */
/* Purpose  : Replace a string pointer                                */
/**********************************************************************/
int       CSStringReplace(char          ** ppString,
                          char           * p,
                          int              len)
{
  int    Replaced = 0;
  char * pString  = *ppString;

  if (pString)
  {
                                       /* Did we have a previous ?    */
    if (pString[len] == 0 && p && !memcmp(pString,p,len))
    {
                                       /* These are the same - no op  */
      goto MOD_EXIT;
    }
                                       /* Ok, delete this one         */
    CSStringDelete(pString);
    *ppString = NULL;
    Replaced  = 1;
  }
                                       /* Now do we add something ?   */
  if (!p || !len)
  {
    goto MOD_EXIT;
  }
                                       /* Yes, go for it then         */
  pString   = CSStringAdd(p,len);
  *ppString = pString;
  Replaced  = 1;

MOD_EXIT:
  return Replaced;
}
/**********************************************************************/
/* Function : CSStringReplaceMax                                      */
/* Purpose  : Replace a string pointer given a strings maximum length */
/**********************************************************************/
int       CSStringReplaceMax(char          ** ppString,
                             char           * p,
                             int              len,
                             BOOL             NoEmpty)
{
  char * c        = p;
  int    actlen   = 0;

  if (p) while (*c && actlen < len) {c++; actlen++; }

  if (p && NoEmpty)
  {
                                       /* String must contain non-blnks*/
    int i;
    for (i=0;i<actlen;i++)
    {
      if (p[i] != ' ') break;
    }
                                       /* All blanks ?                */
    if (i==actlen) p = NULL;
  }

  return CSStringReplace(ppString,p,actlen);
}
/**********************************************************************/
/*  avlrotate:  perform a tree rotaion                                */
/**********************************************************************/
CSTREE * avlrotate(CSTREE *n,int dir)
{
  CSTREE         *t = n;
  unsigned  short hl,hr;

#ifdef AVL_DEBUG
  printf("avlrotate %s %.2s\n",dir?"left":"right",n->Key);
#endif

  n             = n->Sub[dir];
  t->Sub[dir]   = n->Sub[1-dir];
  n->Sub[1-dir] = t;

  hl = t->Sub[0] ? t->Sub[0]->Height : 0;
  hr = t->Sub[1] ? t->Sub[1]->Height : 0;
  t -> Height = max(hl,hr) + 1;

  hl = n->Sub[0] ? n->Sub[0]->Height : 0;
  hr = n->Sub[1] ? n->Sub[1]->Height : 0;
  n -> Height = max(hl,hr) + 1;

  return n;
}

/**********************************************************************/
/* Function : avlrebalance                                            */
/* Purpose  : Rebalance the tree                                      */
/**********************************************************************/
void avlrebalance(CSTREESTACK * pStack)
{
  CSTREE  * pNode;
  CSTREE  * pChild;
  unsigned  short hl,hr;
  int       index;
  int       childindex;
  while (pStack)
  {
                                       /* Get the node to rebalance   */
    pNode = pStack->pNode;
    if (pNode)
    {
                                       /* Set the height              */
      hl = pNode->Sub[0] ? pNode->Sub[0]->Height : 0;
      hr = pNode->Sub[1] ? pNode->Sub[1]->Height : 0;

      pNode -> Height = max(hl,hr) + 1;

      /****************************************************************/
      /* Now let's see what rebalancing should be done                */
      /****************************************************************/
      switch(hr-hl)
      {
        case  0:
        case -1:
        case  1:
                 /*****************************************************/
                 /* These are all normal....nothing to do here        */
                 /*****************************************************/
                 break;

        case -2:
        case  2:
                 /*****************************************************/
                 /* Ok, we need to rebalance                          */
                 /*****************************************************/
                 index = ((hr-hl) == -2) ? 0 : 1;

                 /*****************************************************/
                 /* Whats the balance of the child ?                  */
                 /*****************************************************/
                 pChild = pNode->Sub[index];

                 hl = pChild->Sub[0] ? pChild->Sub[0]->Height : 0;
                 hr = pChild->Sub[1] ? pChild->Sub[1]->Height : 0;
                 childindex = ((hr-hl) < 0) ? 0 : 1;
                 /*****************************************************/
                 /* Outside node ?                                    */
                 /*****************************************************/
                 if ((index == childindex) || (hr==hl))
                 {
                                       /* Outside node                */
#ifdef AVL_DEBUG
                   printf("Outside node %.2s\n",pNode->Key);
                   printtree();
#endif
                   pNode = avlrotate(pNode,index);
                 }
                 else
                 {
#ifdef AVL_DEBUG
                   printf("Inside node %.2s\n",pNode->Key);
                   printtree();
#endif
                                       /* Inside node                 */
                   pNode->Sub[index]=avlrotate(pNode->Sub[index],1-index);
                   pNode = avlrotate(pNode,index);
                   *(pStack -> pSub) = pNode;
                   pStack->pNode     = pNode;
                   continue;
                 }
                 *(pStack -> pSub) = pNode;
                 break;

        default:
#ifdef AVL_DEBUG
                 printf("Error in node %.2s\n",pNode->Key);
                 printtree();
#endif
                 CSTrap(hr-hl);
                 break;
      }
    }
    pStack = pStack->pNext;
  }
}
/**********************************************************************/
/* Function : SetIndexes                                              */
/* Purpose  : Scream through the collating list and update the        */
/*            indexes of the fields                                   */
/**********************************************************************/
void SetIndexes(CSTREE * pNode)
{
  int index = pNode->TreeIndex;
  while (pNode = pNode->Coll[1])
    pNode->TreeIndex = ++index;
}
/**********************************************************************/
/* Function : avlinsert                                               */
/* Purpose  : Insert an item into a binary tree                       */
/**********************************************************************/
void * avlinsert(CSTREESTACK * pStack,
                 int           Options,
                 CSTREE      * pElem,
                 int           Size,
                 int           KeySize)
{
  int             comp;
  CSTREESTACK     Stack;
  CSTREESTACK   * ParentStack;
  CSTREE        * pParent = pStack->pNode;
  CSTREE        * pNode = NULL;
                                     /* Didn't find it                */
  if (pParent)
  {
    Stack.pNext = pStack;
    switch(KeySize)
    {
          /* Case insensitive string pointers                         */
      case -2:
          {
            char * p1 = *(char **)pElem->Key;
            char * p2 = *(char **)pParent->Key;
            comp = stricmp(p1,p2);
            if (comp == 0) comp = strcmp(p1,p2);
          }
          break;
          /* Case sensitive string pointers                           */
      case -3:
          {
            char * p1 = *(char **)pElem->Key;
            char * p2 = *(char **)pParent->Key;
            comp = strcmp(p1,p2);
          }
          break;
      case -4:
          {
            int * p1 = (int *)&pElem->Key;
            int * p2 = (int *)&pParent->Key;
            comp = *p1 - *p2;
          }
          break;
      default:
#ifdef _DEBUG
          if (KeySize <= 0) CSDebugTrap(KeySize);
#endif
          /* Case insensitive fixed keysize                           */
          comp = memcmp(pElem->Key,pParent->Key,KeySize);
    }

    if (comp == 0)
    {
      if (Options & CSO_NOT_UNIQUE)
      {
                                       /* Ok, we're not unique        */
        comp = -1;
      }
      else
      {
        pNode = NULL;
        goto MOD_EXIT;
      }
    }

    if (comp < 0)
    {
      Stack.pNode =  pParent->Sub[0];
      Stack.pSub  = &pParent->Sub[0];
      Stack.dir   = 0;
      pNode = avlinsert(&Stack,
                        Options,
                        pElem,
                        Size,
                        KeySize);
      goto MOD_EXIT;
    }
    if (comp > 0)
    {
      Stack.pNode =  pParent->Sub[1];
      Stack.pSub  = &pParent->Sub[1];
      Stack.dir   = 1;
      pNode = avlinsert(&Stack,
                        Options,
                        pElem,
                        Size,
                        KeySize);
    }
  }
  else
  {
    /******************************************************************/
    /* Allocate an area to add to the tree                            */
    /******************************************************************/
    if (Options & CSO_NOALLOC)
    {
      pNode = pElem;
    }
    else
    {
      pNode = CSGetMem(Size,"CSTREE");
      if (!pNode) goto MOD_EXIT;
      memcpy(pNode,pElem,Size);
    }
    memset(pNode,0    ,CSOFFSETOF(CSTREE,Key));
    pNode->Height = 1;
    *(pStack->pSub) = pNode;
    /******************************************************************/
    /* Now, put it in the right collating sequence                    */
    /******************************************************************/
    ParentStack = pStack->pNext;
    if (ParentStack)
    {
      CSTREE * pGParent = ParentStack->pNode;
      int      dir      = pStack->dir;
                                       /* Add to the collating list   */
      pNode->Coll[dir]    = pGParent->Coll[dir];
      pNode->Coll[1-dir]  = pGParent;
      pGParent->Coll[dir] = pNode;
      if (pNode->Coll[dir]) pNode->Coll[dir]->Coll[1-dir] = pNode;
      /****************************************************************/
      /* Now we need to set the index accordingly                     */
      /****************************************************************/
      if (pNode->Coll[0]) SetIndexes(pNode->Coll[0]);
                     else SetIndexes(pNode);
    }
    /******************************************************************/
    /* Now, rebalance the stack                                       */
    /******************************************************************/
    avlrebalance(pStack);
  }

MOD_EXIT:
  return pNode;
}

/**********************************************************************/
/* Function : CSTreeAdd                                               */
/* Purpose  : Add an element to a binary tree                         */
/**********************************************************************/
void *  CSTreeAdd(void    *  ppRoot,
                  int        Options,
                  void    *  pElem,
                  int        Size,
                  int        KeySize)
{
  CSTREESTACK Stack;

  Stack.pNext =  NULL;
  Stack.pNode =  *(CSTREE **)ppRoot;
  Stack.pSub  =  ppRoot;

  return avlinsert(&Stack,
                   Options,
        (CSTREE  *)pElem,
                   Size,
                   KeySize);
}

/**********************************************************************/
/* Function : avlextreme                                              */
/* Purpose  : Find the extreme node                                   */
/**********************************************************************/
void avlextreme(CSTREESTACK * pStack,
                CSTREESTACK * pNode,   /* Node to replace             */
                int           index)
{
  CSTREESTACK   Stack;
  CSTREESTACK * ScanStack;
  CSTREE      * pParent = pStack->pNode;
  CSTREE      * pOld;

  if (pParent->Sub[index])
  {
    Stack.pNext =  pStack;
    Stack.pNode =  pParent->Sub[index];
    Stack.pSub  = &pParent->Sub[index];
    avlextreme(&Stack,pNode,index);
  }
  else
  {
    /******************************************************************/
    /* Reached the end of the chain...                                */
    /* Replace the parent.                                            */
    /******************************************************************/
    *pStack-> pSub    = pParent->Sub[1-index];
    pOld              = pNode -> pNode;
    pParent -> Sub[0] = pOld-> Sub[0];
    pParent -> Sub[1] = pOld-> Sub[1];
    ScanStack = pStack;
    while (ScanStack->pNext != pNode) ScanStack=ScanStack->pNext;
    ScanStack->pSub = (CSTREE **)(((char *)ScanStack->pSub) - ((char *)pOld) +
                                  ((char *)pParent));
    *pNode -> pSub = pParent;
    pNode -> pNode = pParent;
    avlrebalance(pStack->pNext);
  }
}
/**********************************************************************/
/* Function : avldelete                                               */
/* Purpose  : Delete an item from a binary tree                       */
/**********************************************************************/
int  avldelete(CSTREESTACK * pStack,
               int           (* fn)(void *,void *),
               void        * Parm,
               CSTREE      * pElem,
               int           KeySize)
{
  int             comp;
  int             rc = CSRC_NOT_FOUND;
  CSTREESTACK     Stack;
  CSTREE        * pParent = pStack->pNode;
  unsigned  short hl,hr;
                                     /* Didn't find it                */
  if (!pParent) goto MOD_EXIT;
  Stack.pNext = pStack;

  if (KeySize == -3)
  {
    char * p1 = *(char **)pElem->Key;
    char * p2 = *(char **)pParent->Key;
    comp = strcmp(p1,p2);
  }
  else if (KeySize == -2)
  {
    char * p1 = *(char **)pElem->Key;
    char * p2 = *(char **)pParent->Key;
    comp = stricmp(p1,p2);
    if (!comp) comp = strcmp(p1,p2);
  }
  else
    comp = memcmp(pElem->Key,pParent->Key,KeySize);
  if (comp < 0)
  {
    Stack.pNode =  pParent->Sub[0];
    Stack.pSub  = &pParent->Sub[0];
    rc = avldelete(&Stack,
                   fn,
                   Parm,
        (CSTREE  *)pElem,
                   KeySize);
    goto MOD_EXIT;
  }
  if (comp > 0)
  {
    Stack.pNode =  pParent->Sub[1];
    Stack.pSub  = &pParent->Sub[1];
    rc = avldelete(&Stack,
                   fn,
                   Parm,
        (CSTREE  *)pElem,
                   KeySize);
    goto MOD_EXIT;
  }
  /********************************************************************/
  /* Well if we dropped through to here we found our node             */
  /* We'll replace this node with either the highest or lowest        */
  /* node underneath ourselves it doesn't matter which but to keep    */
  /* things neat we'll try to address any balance problem underneath  */
  /* ourselves.                                                       */
  /********************************************************************/
  hl = pParent->Sub[0] ? pParent->Sub[0]->Height : 0;
  hr = pParent->Sub[1] ? pParent->Sub[1]->Height : 0;

  if (hr>hl)
  {
    Stack.pNode =  pParent->Sub[1];
    Stack.pSub  = &pParent->Sub[1];
    avlextreme(&Stack,pStack,0);
  }
  else if (hl>hr)
  {
    Stack.pNode =  pParent->Sub[0];
    Stack.pSub  = &pParent->Sub[0];
    avlextreme(&Stack,pStack,1);
  }
  else if (pParent->Sub[0])
  {
    Stack.pNode =  pParent->Sub[0];
    Stack.pSub  = &pParent->Sub[0];
    avlextreme(&Stack,pStack,1);
  }
  else if (pParent->Sub[1])
  {
    Stack.pNode =  pParent->Sub[1];
    Stack.pSub  = &pParent->Sub[1];
    avlextreme(&Stack,pStack,0);
  }
  else
  {
                                       /* We're a leaf node           */
    *(pStack->pSub) = NULL;
    avlrebalance(pStack->pNext);
  }
  /********************************************************************/
  /* Now...we need to remove ourselves from the collating sequence    */
  /********************************************************************/
  if (pParent->Coll[0]) pParent->Coll[0]->Coll[1] = pParent->Coll[1];
  if (pParent->Coll[1]) pParent->Coll[1]->Coll[0] = pParent->Coll[0];

  /********************************************************************/
  /* Now we need to set the index accordingly                         */
  /********************************************************************/
  if (pParent->Coll[0])
  {
    SetIndexes(pParent->Coll[0]);
  }
  else
  {
    if (pParent->Coll[1])
    {
      pParent->Coll[1]->TreeIndex = 0;
      SetIndexes(pParent->Coll[1]);
    }
  }
  /********************************************************************/
  /* Call the callers delete function if we have one                  */
  /********************************************************************/
  if (fn) (*fn)(pParent,Parm);

  /********************************************************************/
  /* Element removed from tree....remove storage                      */
  /********************************************************************/
  CSLinkRmvAll(pParent,0);
  CSFreeMem   (pParent);
  rc = 0;

MOD_EXIT:
  return rc;
}
/**********************************************************************/
/* Function : CSTreeDeleteItem                                        */
/* Purpose  : Delete one item from the tree                           */
/**********************************************************************/
int      CSTreeDeleteItem(void       * ppRoot,
                          int          (* fn)(void *,void *),
                          void       * Parm,
                          void       * pElem,
                          int          KeySize)

{
  CSTREESTACK Stack;

  if (!pElem) return CSRC_NOT_FOUND;

  Stack.pNext =  NULL;
  Stack.pNode =  *(CSTREE **)ppRoot;
  Stack.pSub  =  ppRoot;

  return avldelete(&Stack,
                   fn,
                   Parm,
        (CSTREE  *)pElem,
                   KeySize);
}

/**********************************************************************/
/* Function : CSTreeTraverse                                          */
/* Purpose  : Call function for each element of the tree              */
/**********************************************************************/
void      CSTreeTraverse (void       * pRoot,
                          int          (* fn)(void *,void *),
                          void       * Parm)
{
  CSTREE * pTree = (CSTREE *)pRoot;
  CSTREE * pNext;
  if (pTree)
  {
                                       /* Find the lowest element     */
    pTree = CSTreeFirst(pTree);

    while (pTree)
    {
      pNext = CSTreeNext(pTree);
      (fn)(pTree,Parm);
      pTree=pNext;
    }
  }
}

/**********************************************************************/
/* Function : CSTreeFirst                                             */
/* Purpose  : Return the first item in the tree                       */
/**********************************************************************/
void    * CSTreeFirst    (void       * pRoot)
{
  CSTREE * pTree = (CSTREE *)pRoot;
                                       /* Find the lowest element     */
  if (pTree)
  {
    while (pTree->Sub[0])
    {
      pTree = pTree->Sub[0];
    }
  }

  return pTree;
}
/**********************************************************************/
/* Function : CSTreeLast                                              */
/* Purpose  : Return the last item in the tree                        */
/**********************************************************************/
void    * CSTreeLast     (void       * pRoot)
{
  CSTREE * pTree = (CSTREE *)pRoot;
                                       /* Find the lowest element     */
  if (pTree)
  {
    while (pTree->Sub[1])
    {
      pTree = pTree->Sub[1];
    }
  }

  return pTree;
}
/**********************************************************************/
/* Function : CSTreeCount                                             */
/* Purpose  : Return the number of items in the tree                  */
/**********************************************************************/
int CSTreeCount(void          * pRoot)
{
  CSTREE *pLast = (CSTREE *)CSTreeLast(pRoot);
  if (pLast) return pLast->TreeIndex+1;
        else return 0;
}

/**********************************************************************/
/* Function : CSTreeGetIndex                                          */
/* Purpose  : Find the item with the right index                      */
/**********************************************************************/
void    * CSTreeGetIndex  (void          * pRoot,
                           int             Index)
{
  CSTREE * pTree = (CSTREE *)pRoot;
  while (pTree)
  {
    if (Index > pTree->TreeIndex)
    {
      pTree = pTree->Sub[1];
    }
    else
    if (Index < pTree->TreeIndex)
    {
      pTree = pTree->Sub[0];
    }
    else
      break;
  }
  return pTree;
}
/**********************************************************************/
/* Function : CSTreeTraverseB                                         */
/* Purpose  : Call function for each element of the tree              */
/**********************************************************************/
void      CSTreeTraverseB(void       * pRoot,
                          int          (* fn)(void *,void *),
                          void       * Parm)
{
  CSTREE * pTree = (CSTREE *)pRoot;
  QUEUE    q;
  CSTREE * c1,* c2;

  if (!pRoot) return;

  qinit  (&q);
  qinsert(&q, pTree);

  while (qremove(&q, (void **)&pTree))
  {
    c1 = pTree->Sub[0];
    c2 = pTree->Sub[1];

    (*fn)(pTree, Parm);

    if (c1) qinsert(&q, c1);
    if (c2) qinsert(&q, c2);
  }
}
/**********************************************************************/
/* Function : CSTreeDelete                                            */
/* Purpose  : Delete the entire tree                                  */
/**********************************************************************/
int      CSTreeDelete   (void       * ppRoot,
                         int         (* fn)(void *,void *),
                         void      * Parm)

{
  CSTREE * pTree = *(CSTREE **)ppRoot;
  if (pTree)
  {
                                       /* Call deletion routine       */
    if (fn) (*fn)(pTree,Parm);
    CSLinkRmvAll(pTree,0);
    if (pTree -> Sub[0]) CSTreeDelete(&pTree -> Sub[0],fn,Parm);
    if (pTree -> Sub[1]) CSTreeDelete(&pTree -> Sub[1],fn,Parm);
    CSFreeMem(pTree);
    *(CSTREE **)ppRoot = NULL;
  }
  return 0;
}

/**********************************************************************/
/* Function : CSListCount                                             */
/* Purpose  : Return how many element there are                       */
/**********************************************************************/
int       CSListCount  (CSLISTROOT * pRoot )
{
  return pRoot -> Count;
}

/**********************************************************************/
/* Function : CSListAdd                                               */
/* Purpose  : Add an element to a linked list                         */
/**********************************************************************/
void    * CSListAdd    (CSLISTROOT * pRoot,
                        int          Options,
                        void       * Elem,
                        int          Size)
{
  CSLIST     * pElem;
                                       /* Get an area                 */
  if (Options & CSO_NOHEADER)
  {
    pElem = (CSLIST *)CSGetMem(Size+sizeof(CSLIST),"CSLIST");
    if (!pElem) goto MOD_EXIT;
    memcpy(pElem+1,Elem,Size);
    pElem -> Options = 0;
  }
  else if (Options & CSO_NOALLOC)
  {
    pElem = (CSLIST *)Elem;
    Size -= sizeof(CSLIST);
  }
  else
  {
    pElem = (CSLIST *)CSGetMem(Size,"CSLIST");
    if (!pElem) goto MOD_EXIT;
    memcpy(pElem,Elem,Size);
    Size -= sizeof(CSLIST);
  }
  pElem -> Next         = NULL;
  pElem -> Prev         = (CSLIST *) pRoot -> Bottom;
  pElem -> Root         = pRoot;
  pElem -> Length       = (short)Size;
                                       /* Add to the end              */
  if (pRoot -> Bottom)
  {
    pRoot -> Count++;
    pRoot -> Bottom -> Next = (CSNEXTPREV *) pElem;
    pRoot -> Bottom         = (CSNEXTPREV *) pElem;
  }
  else
  {
    pRoot -> Count  = 1;
    pRoot -> Top    = (CSNEXTPREV *) pElem;
    pRoot -> Bottom = (CSNEXTPREV *) pElem;
  }

MOD_EXIT:
  return pElem;
}

/**********************************************************************/
/* Function : CSListAddSort                                           */
/* Purpose  : Add an element to a linked list in order                */
/**********************************************************************/
void    * CSListAddSort(CSLISTROOT * pRoot,
                        int          Options,
                        void       * Elem,
                        int          Size,
                        int          KeySize)
{
  CSLIST     * pElem;
  CSLIST     * p;
  CSLIST     * pl = NULL;
  CSLIST     * pn;
                                       /* Get an area                 */
  if (Options & CSO_NOHEADER)
  {
    pElem = (CSLIST *)CSGetMem(Size+sizeof(CSLIST),"CSLIST");
    if (!pElem) goto MOD_EXIT;
    memcpy(pElem+1,Elem,Size);
    pElem -> Options = 0;
  }
  else
  {
    pElem = (CSLIST *)CSGetMem(Size,"CSLIST");
    if (!pElem) goto MOD_EXIT;
    memcpy(pElem,Elem,Size);
    Size -= sizeof(CSLIST);
  }
  pElem -> Root         = pRoot;
  pElem -> Length       = (short)Size;

  /********************************************************************/
  /* Go through the list and add in the right place....               */
  /********************************************************************/
  p = (CSLIST *)pRoot -> Top;
  while (p)
  {
    char * p1 = (char *)(pElem+1);
    char * p2 = (char *)(p+1);
    if (memcmp(p1,p2,KeySize) <= 0) break;
    pl = p;
    p  = p -> Next;
  }

  /********************************************************************/
  /* Ok, add it to 'pl' if there                                      */
  /********************************************************************/
  if (pl)
  {
    pElem -> Next = pl -> Next;
    if (pl -> Next) pl -> Next -> Prev = pElem;
    pl    -> Next = pElem;
    pElem -> Prev = pl;
    if (pRoot -> Bottom == (CSNEXTPREV *)pl) pRoot->Bottom = (CSNEXTPREV *)pElem;
  }
  else
  {
    pn = (CSLIST *) pRoot -> Top;

    pElem -> Next = pn;
    pElem -> Prev = NULL;
    if (pn) pn ->Prev = pElem;
    pRoot -> Top = (CSNEXTPREV *)pElem;
  }

  if (!pElem -> Prev) pRoot -> Top    = (CSNEXTPREV *)pElem;
  if (!pElem -> Next) pRoot -> Bottom = (CSNEXTPREV *)pElem;

  pRoot -> Count++;

MOD_EXIT:
  return pElem;
}

/**********************************************************************/
/* Function : CSListDelete                                            */
/* Purpose  : Delete the entire list                                  */
/**********************************************************************/
void     CSListDelete   (CSLISTROOT * pRoot,
                         int          (* fn)(void *,void *),
                         void       * Parm)

{
  CSLIST     * pElem;
  CSLIST     * pnElem;

  pElem = (CSLIST *)pRoot -> Top;
  while (pElem)
  {
                                       /* Call deletion routine       */
    if (fn) (*fn)(pElem,Parm);
    pnElem = pElem -> Next;
    CSFreeMem(pElem);
    pElem = pnElem;
  }
  pRoot -> Count  = 0;
  pRoot -> Top    = NULL;
  pRoot -> Bottom = NULL;
}

/**********************************************************************/
/* Function : CSListDeleteItem                                        */
/* Purpose  : Remove an element from a linked list                    */
/**********************************************************************/
void      CSListDeleteItem(void       * Elem)
{
  CSLIST     * pElem = (CSLIST *) Elem;
  CSLISTROOT * pRoot = pElem -> Root;

  if (pElem -> Next)            pElem -> Next -> Prev = pElem -> Prev;
  if (pElem -> Prev)            pElem -> Prev -> Next = pElem -> Next;
  if (pRoot -> Top    == (CSNEXTPREV *) pElem)
    pRoot -> Top          = (CSNEXTPREV *) pElem -> Next;
  if (pRoot -> Bottom == (CSNEXTPREV *) pElem)
    pRoot -> Bottom       = (CSNEXTPREV *) pElem -> Prev;
  CSFreeMem(pElem);
  pRoot -> Count--;
}

/**********************************************************************/
/* Function : CSListLink                                              */
/* Purpose  : Link an element into a linked list                      */
/* Can either be added to the front or the back of the list           */
/**********************************************************************/
void    * CSListLink   (CSLISTROOT * pRoot,
                        int          Options,
                        CSLIST     * pElem)
{
  pElem -> Root         = pRoot;

  if (Options & CSO_FRONT)
  {
                                       /* Add to the front            */
    pElem -> Next         = (CSLIST *) pRoot -> Top;
    pElem -> Prev         = NULL;
    if (pRoot -> Top)
    {
      pRoot -> Top -> Prev = (CSNEXTPREV *) pElem;
      pRoot -> Top         = (CSNEXTPREV *) pElem;
    }
    else
    {
      pRoot -> Top    = (CSNEXTPREV *) pElem;
      pRoot -> Bottom = (CSNEXTPREV *) pElem;
    }
  }
  else
  {
                                       /* Add to the end              */
    pElem -> Next         = NULL;
    pElem -> Prev         = (CSLIST *) pRoot -> Bottom;
    if (pRoot -> Bottom)
    {
      pRoot -> Bottom -> Next = (CSNEXTPREV *) pElem;
      pRoot -> Bottom         = (CSNEXTPREV *) pElem;
    }
    else
    {
      pRoot -> Top    = (CSNEXTPREV *) pElem;
      pRoot -> Bottom = (CSNEXTPREV *) pElem;
    }
  }
  pRoot -> Count++;

  return pElem;
}

/**********************************************************************/
/* Function : CSListUnlink                                            */
/* Purpose  : Unlink an element from a linked list                    */
/* Can either be removed from the front or the back of the list       */
/**********************************************************************/
void    * CSListUnlink (CSLISTROOT * pRoot,
                        int          Options)
{
  CSLIST * pElem;

  if (Options & CSO_FRONT) pElem = (CSLIST *) pRoot -> Top;
                      else pElem = (CSLIST *) pRoot -> Bottom;
  if (!pElem) goto MOD_EXIT;

  if (pElem -> Next)            pElem -> Next -> Prev = pElem -> Prev;
  if (pElem -> Prev)            pElem -> Prev -> Next = pElem -> Next;
  if (pRoot -> Top    == (CSNEXTPREV *) pElem)
    pRoot -> Top          = (CSNEXTPREV *) pElem -> Next;
  if (pRoot -> Bottom == (CSNEXTPREV *) pElem)
    pRoot -> Bottom       = (CSNEXTPREV *) pElem -> Prev;

  pRoot -> Count--;

MOD_EXIT:
  return pElem;
}

/**********************************************************************/
/* Function : CSListFind                                              */
/* Purpose  : Check whether an entry is in the list                   */
/**********************************************************************/
void    * CSListFind(CSLISTROOT * pRoot,
                     void       * Elem)
{
  CSLIST * pElem = (CSLIST *) pRoot -> Top;

  while (pElem)
  {
    if (pElem == Elem) break;
    pElem = pElem -> Next;
  }
  return pElem;
}

/**********************************************************************/
/* Function : CSListFindEntry                                         */
/* Purpose  : Check whether an entry is in the list                   */
/**********************************************************************/
void    * CSListFindEntry(CSLISTROOT * pRoot,
                          int          Options,
                          void       * Elem,
                          int          Size)
{
  CSLIST * pElem = (CSLIST *) pRoot -> Top;
  char   * p;

  p = (char *) Elem;
  if (!(Options & CSO_NOHEADER))
  {
    p += sizeof(CSLIST);
  }

  while (pElem)
  {
    if (pElem->Length == Size)
    {
      if (!memcmp(pElem+1,p,Size)) break;
    }
    pElem = pElem -> Next;
  }
  return pElem;
}

/**********************************************************************/
/* Function : CSListTraverse                                          */
/* Purpose  : Call function for each element of the List              */
/**********************************************************************/
void      CSListTraverse(CSLISTROOT * pRoot,
                         int          (* fn)(void *,void *),
                         void       * Parm)
{
  CSLIST * pList;
  CSLIST * pNext;
  if (pRoot)
  {
    pList = (CSLIST *) pRoot -> Top;
    while(pList)
    {
      pNext = pList -> Next;
      (*fn)(pList,Parm);
      pList = pNext;
    }
  }
}

/**********************************************************************/
/* Function : CSLinkAdd                                               */
/* Purpose  : Add a link between two objects                          */
/**********************************************************************/
CSLINK  * CSLinkAdd    (void       * Source,
                        void       * Target,
                        short        Type,
                        int          Data)
{
  CSTREE * pSource = (CSTREE *) Source;
  CSTREE * pTarget = (CSTREE *) Target;
  CSLINK * pSlink;
  CSLINK * pTlink;

  pSlink = (CSLINK *)malloc(sizeof(CSLINK));
  if (!pSlink) goto MOD_EXIT;

  pTlink = (CSLINK *)malloc(sizeof(CSLINK));
  if (!pTlink) goto MOD_EXIT;

  pSlink -> Other  = pTlink;
  pSlink -> Source = pSource;
  pSlink -> Target = pTarget;
  pSlink -> Data   = Data;
  pSlink -> Type   = Type;
  pSlink -> Dir    = 0;

  pTlink -> Other  = pSlink;
  pTlink -> Source = pSource;
  pTlink -> Target = pTarget;
  pTlink -> Data   = Data;
  pTlink -> Type   = Type;
  pTlink -> Dir    = 1;

  pSlink -> Next   = pSource -> Links;
  pSlink -> Prev   = NULL;
  pSource -> Links = pSlink;
  if (pSlink -> Next) pSlink -> Next -> Prev = pSlink;

  pTlink -> Next   = pTarget -> Links;
  pTlink -> Prev   = NULL;
  pTarget -> Links = pTlink;
  if (pTlink -> Next) pTlink -> Next -> Prev = pTlink;

MOD_EXIT:
  return pSlink;
}

/**********************************************************************/
/* Function : CSLinkFind                                              */
/* Purpose  : Find a link between two onjects                         */
/**********************************************************************/
CSLINK  * CSLinkFind   (void       * Source,
                        void       * Target,
                        short        Type,
                        BOOL         Either)
{
  CSTREE * pSource = (CSTREE *) Source;
  CSLINK * pLink   = pSource -> Links;

  while (pLink)
  {
    if (Type == 0 || pLink->Type == Type)
    {
      if (((pLink -> Source == Source) && (pLink -> Target == Target)) ||
          (Either &&
           (pLink -> Source == Target) && (pLink -> Target == Source))) goto MOD_EXIT;
    }
    pLink = pLink -> Next;
  }

MOD_EXIT:
  return pLink;
}
/**********************************************************************/
/* Function : CSLinkRmv                                               */
/* Purpose  : Remove a link between two objects                       */
/**********************************************************************/
void      CSLinkRmv    (CSLINK     * pLink1)
{
  CSLINK * pLink2 = pLink1->Other;
  CSTREE * pObject;

  if (pLink1->Next) pLink1->Next->Prev = pLink1->Prev;
  if (pLink1->Prev) pLink1->Prev->Next = pLink1->Next;
  else
  {
    if (pLink1->Dir) pObject = (CSTREE *)pLink1->Target;
                else pObject = (CSTREE *)pLink1->Source;
    pObject -> Links = pLink1->Next;
  }
  if (pLink2->Next) pLink2->Next->Prev = pLink2->Prev;
  if (pLink2->Prev) pLink2->Prev->Next = pLink2->Next;
  else
  {
    if (pLink2->Dir) pObject = (CSTREE *)pLink2->Target;
                else pObject = (CSTREE *)pLink2->Source;
    pObject -> Links = pLink2->Next;
  }
  free(pLink1);
  free(pLink2);
}

/**********************************************************************/
/* Function : CSLinkRmvAll                                            */
/* Purpose  : Remove all the links for an object                      */
/**********************************************************************/
void CSLinkRmvAll (void       * Source,
                   short        Type)
{
  CSTREE * pSource = (CSTREE *) Source;
  CSLINK * pLink   = pSource -> Links;
  CSLINK * pNext;

  while(pLink)
  {
    pNext = pLink -> Next;
    if (Type == 0 || pLink->Type == Type) CSLinkRmv(pLink);
    pLink = pNext;
  }
}

/**********************************************************************/
/* Function : CSLinkNext                                              */
/* Purpose  : Return the next link if any  (NULL returns first)       */
/**********************************************************************/
CSLINK  * CSLinkNext   (void       * Object,
                        short        Type,
                        CSLINK     * pLink)
{
  CSTREE * pObject;

  if (!pLink)
  {
    pObject = (CSTREE *) Object;
    pLink   = pObject -> Links;
  }
  else
  {
    pLink   = pLink->Next;
  }
  while (pLink)
  {
    if (Type==0 || pLink->Type == Type) break;
    pLink = pLink->Next;
  }
  return pLink;
}

/**********************************************************************/
/* Function : CSByteSwapShort                                         */
/* Purpose  : ByteSwap an array of SHORTs                             */
/**********************************************************************/
void CSByteSwapShort(unsigned char * pShort, int array)
{
  unsigned char ch;

  while(array--)
  {
    ch             = * pShort;
    * pShort       = *( pShort + 1);
    * (pShort + 1) = ch;
    pShort        += sizeof(short int);
  }
}
/**********************************************************************/
/* Function : CSByteSwapLong                                          */
/* Purpose  : ByteSwap an array of LONGs                              */
/**********************************************************************/
void CSByteSwapLong(unsigned char * pLong, int array)
{
  unsigned char ch;

  while(array--)
  {
    ch             = * pLong;
    * pLong        = *( pLong  + 3);
    * (pLong  + 3) = ch;
    ch             = *( pLong  + 1);
    * (pLong  + 1) = *( pLong  + 2);
    * (pLong  + 2) = ch;
    pLong         += sizeof(int);
  }
}
/**********************************************************************/
/* Function : CSToAscii                                               */
/* Purpose  : Convert an Ebcdic string to ascii                       */
/**********************************************************************/
int  CSToAscii(unsigned char * pOut,
               unsigned char * pIn,
               int             Length)
{
  int i;
  int chars = 0;
  unsigned char * puOut,* puIn;
  puOut = pOut;
  puIn  = pIn;

  for (i=0 ; i<Length ; i++)
  {
    puOut[i] = CS_EBCDIC_TO_ASCII[puIn[i]];
    if (puOut[i] != '.') chars++;
  }

  return chars;
}

/**********************************************************************/
/* Function : CSToEdcdic                                              */
/* Purpose  : Convert to an ascii string to Ebcdic                    */
/**********************************************************************/
int CSToEbcdic(unsigned char * pOut,
               unsigned char * pIn,
               int    Length)
{
  int i;
  int chars = 0;
  unsigned char * puOut,* puIn;
  puOut = pOut;
  puIn  = pIn;
  for (i=0 ; i<Length ; i++)
  {
    puOut[i] = CS_ASCII_TO_EBCDIC[puIn[i]];
    if (puOut[i] != 75) chars++;
  }
  return chars;
}

/**********************************************************************/
/* Function : CSConvTest                                              */
/* Purpose  : Test conversion                                         */
/**********************************************************************/
void CSConvTest()
{
  unsigned char str[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz "
               "0123456789 <>{}/*+-?.:\"#@$œ|%";

  printf("Start:%s\n",str);

  CSToEbcdic(str,str,strlen((char *)str));
  CSToAscii (str,str,strlen((char *)str));

  printf("After:%s\n",str);
}

/**********************************************************************/
/* Function : CSLoadFile                                              */
/* Purpose  : Load a file into memory                                 */
/**********************************************************************/
char * CSLoadFile(FILE * file, int * BytesRead)
{
  int    len,ThisRead,TotalRead;
  char * pStartMsg = NULL;
  char * pMsg;

  fseek(file,0,SEEK_END);
  len = ftell(file);
  if (len < 0) goto MOD_EXIT;

  pStartMsg = CSGetMem(len+1,"FILELOAD");
  if (!pStartMsg) goto MOD_EXIT;

  rewind(file);
  pMsg      = pStartMsg;
  TotalRead = 0;
  while (ThisRead = fread(pMsg,1,len - TotalRead,file))
  {
    TotalRead += ThisRead;
    pMsg      += ThisRead;
    if (TotalRead >= len) break;
  }
  *BytesRead = TotalRead;

MOD_EXIT:
  return pStartMsg;
}

/**********************************************************************/
/* Function : CSConvGenerate                                          */
/* Purpose  : Gnerate the EBCDIC conversion table                     */
/**********************************************************************/
void CSConvGenerate()
{
  int Value[256],i,j,k;
  FILE * file;

  memset(&Value,0,sizeof(Value));
  for (i=0;i<16;i++)
  {
    for (j=0;j<16;j++)
    {
      k = i*16+j;
      Value[CS_EBCDIC_TO_ASCII[k]] = k;
    }
  }
  file = fopen("c:\\temp\\ebcdic.tbl","w");
  if (!file)
  {
    printf("Can't open file!\n");
    goto MOD_EXIT;
  }
  for (i=0;i<16;i++)
  {
    if (i==0) fprintf(file,"{");
         else fprintf(file," ");

    for (j=0;j<16;j++)
    {
      k = i*16+j;
      if (k==255) fprintf(file,"%3d}",Value[k]);
             else fprintf(file,"%3d,",Value[k]);
    }

    if (i==15) fprintf(file,";/*%c */\n",HEX[i]);
          else fprintf(file," /*%c */\n",HEX[i]);
  }
  printf("Done!\n");
  fclose(file);
MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : MonthDays                                               */
/* Purpose  : Return the number of days in the given month            */
/**********************************************************************/
static int MonthDays(int y,int m)
{
  if (m == 2)
  {
    BOOL leap;

         if (  y % 4  )  leap = FALSE;
    else if (!(y % 400)) leap = TRUE;
    else if (!(y % 100)) leap = FALSE;
    else                 leap = TRUE;
                                       /* February is odd             */
    if (leap) return 29;
         else return 28;
  }
  else
  {
    return DAYS[m];
  }
}
/**********************************************************************/
/* Function : GetTimeBit                                              */
/* Purpose  : Return the number pointed at                            */
/**********************************************************************/
static int GetTimeBit(char * p,int len)
{
  int val = 0;
  while(len--)
  {
    val *= 10;
    val += *p-'0';
    p++;
  }
  return val;
}
/**********************************************************************/
/* Function : CSAdjustTime                                            */
/* Purpose  : Adjust time                                             */
/*            DATE: YYYYMMDD   TIME:HHMMSShh                          */
/**********************************************************************/
void CSAdjustTime(char * pDate,char * pTime,int Adjust)
{
  int  i;
  int  hours,mins;
  char newtime[10];
  int  days = 0;
  int  d,m,y;

  if (!pTime)  goto MOD_EXIT;
  if (!Adjust) goto MOD_EXIT;

  /********************************************************************/
  /* Ok, check that the time is all digits                            */
  /********************************************************************/
  for (i=0; i<8; i++)
  {
    if (pTime[i] < '0' || pTime[i] > '9') goto MOD_EXIT;
  }

  /********************************************************************/
  /* Ok, get the contituent parts of the time                         */
  /********************************************************************/
  hours = GetTimeBit(&pTime[0],2);
  mins  = GetTimeBit(&pTime[2],2);

  mins += Adjust;

  /********************************************************************/
  /* Ok, use the minutes to adjust the hours                          */
  /********************************************************************/
  if (mins >= 60)
  {
    hours += mins/60;
    mins  %= 60;
  }
  if (mins < 0)
  {
    hours += (mins+1)/60 - 1;
    mins   = mins%60;
    if (mins < 0) mins += 60;
  }
  /********************************************************************/
  /* Now, let's check the hours are ok                                */
  /* Calculate whether the date has changed. We will apply this to    */
  /* date value later.                                                */
  /********************************************************************/
  if (hours >= 24)
  {
    days   = hours / 24;
    hours %= 24;
  }
  if (hours < 0)
  {
    days  = hours / 24 - 1;
    hours = hours%24 + 24;
  }

  sprintf(newtime,"%2.2d%2.2d",hours,mins);
  memcpy(pTime,newtime,4);

  if (!days || !pDate) goto MOD_EXIT;

  /********************************************************************/
  /* Ok, check that the date is all digits                            */
  /********************************************************************/
  for (i=0; i<8; i++)
  {
    if (pDate[i] < '0' || pDate[i] > '9') goto MOD_EXIT;
  }

  y  = GetTimeBit(&pDate[0],4);
  m  = GetTimeBit(&pDate[4],2);
  d  = GetTimeBit(&pDate[6],2);

  d += days;
  if (d < 0)
  {
    m--;
    if (m <= 0)  { y --; m += 12; }
    d = MonthDays(y,m) + d + 1;
  }
  else if (d > MonthDays(y,m))
  {
    d -= MonthDays(y,m);
    m++;
    if (m > 12) { y ++; m -= 12; }
  }

  sprintf(newtime,"%4.4d%2.2d%2.2d",y,m,d);
  memcpy(pDate,newtime,8);

MOD_EXIT:
  ;
}
/**********************************************************************/
/* Function : CSConvertParts                                          */
/* Purpose  : Convert amount into a string saying its distinct parts  */
/**********************************************************************/
void      CSConvertParts (int              Amount,
                          char           * p,
                          CSPARTS        * pParts,
                          BOOL             Parts)
{
  int n,t;

  if (Parts)
  {
    while (pParts->Value)
    {
      if (Amount >= pParts->Value || pParts->Value == 1)
      {
        n = Amount/pParts->Value;
        t = n*pParts->Value;

        if (pParts->sName)
        {
          if (n>1 && pParts->plural)
            sprintf(p,"%d %s ",n,pParts->plural);
          else
            sprintf(p,"%d %s%s ",n,pParts->sName,n>1?"s":"");
        }
        else
          sprintf(p,"%d %s%s ",n,pParts->Name,n>1?"s":"");
        p += strlen(p);
        Amount -= t;
        if (!Amount) goto MOD_EXIT;
      }
      pParts++;
    }
  }
  else
  {
    double val,amt;
    while (pParts->Value > Amount)
    {
      pParts++;
      if (pParts->Value == 0)
      {
        pParts--;
        break;
      }
    }
    val = pParts->Value;
    amt = Amount;
    val = amt/val;
    Amount = (int) val;
    if ((double)Amount == val)
      sprintf(p,"%.0f %s%s ",val,pParts->Name,val==1?"":"s");
    else
      sprintf(p,"%.1f %s%s ",val,pParts->Name,val==1?"":"s");
  }

MOD_EXIT:
  ;
}

/**********************************************************************/
/* Function : CSParseParts                                            */
/* Purpose  : Parse a string into the amounts                         */
/**********************************************************************/
int       CSParseParts   (char           * p,
                          CSPARTS        * pParts,
                          int              check)
{
  float     Amount = 0;
  float     n;
  CSPARTS * pp;

  while(*p)
  {
    /* Skip to a number                                               */
    while (*p < '0' || *p > '9')
    {
      if (!*p) goto MOD_EXIT;
      p++;
    }
                                       /* Ok, we've found a number    */
    n=0;
    sscanf(p,"%f",&n);
    while ((*p >= '0' && *p <= '9') || *p=='.') p++;
    while (*p == ' '                          ) p++;
    if (!*p)
    {
                                       /* Assume unit units           */
      Amount += n;
      goto MOD_EXIT;
    }
                                       /* Now check first n characters*/
    pp = pParts;
    while (pp->Value)
    {
      int i;
      BOOL Same = TRUE;

      for (i=0; i<check; i++)
      {
        if (tolower(*(p+i)) != tolower(pp->Name[i]))
        {
          Same = FALSE;
          break;
        }
      }
      if (Same)
      {
        Amount += n * pp->Value;
        break;
      }
      pp++;
    }
  }
MOD_EXIT:
  return (int)Amount;
}

/**********************************************************************/
/* Function : CSAtomicUpdate                                          */
/* Purpose  : Simple function to allow an atomic replacement of a     */
/*            global pointer.                                         */
/**********************************************************************/
void      CSAtomicUpdate (char           * pNew,
                          char          ** pTarget,
                          char          ** pOld)
{
  CSLock(0);
  *pOld    = *pTarget;
  *pTarget = pNew;
  CSUnlock(0);
}
/**********************************************************************/
/* Function : CSIsEbcdic                                              */
/* Purpose  : Return 1 if given codepage is ebcdic                    */
/**********************************************************************/
int CSIsEbcdic(int  CCSID)
{

  switch(CCSID)
  {
    case  37:
    case 256:
    case 273:
    case 277:
    case 278:
    case 280:
    case 284:
    case 285:
    case 286:
    case 290:
    case 297:
    case 300:
    case 420:
    case 421:
    case 424:
    case 425:
    case 500:
    case 803:
    case 833:
    case 834:
    case 835:
    case 836:
    case 837:
    case 838:
    case 870:
    case 871:
    case 875:
    case 880:
    case 918:
    case 924:
    case 930:
    case 931:
    case 933:
    case 935:
    case 937:
    case 939:
    case 1025:
    case 1026:
    case 1027:
    case 1047:
    case 1097:
    case 1112:
    case 1122:
    case 1123:
    case 1130:
    case 1132:
    case 1137:
    case 1140:
    case 1141:
    case 1142:
    case 1143:
    case 1144:
    case 1145:
    case 1146:
    case 1147:
    case 1148:
    case 1149:
    case 1279:
    case 1364:
    case 1388:
    case 1390:
    case 1399:
    case 4133:
    case 4325:
    case 4369:
    case 4370:
    case 4371:
    case 4372:
    case 4373:
    case 4374:
    case 4376:
    case 4378:
    case 4380:
    case 4381:
    case 4386:
    case 4393:
    case 4396:
    case 4516:
    case 4520:
    case 4596:
    case 4899:
    case 4929:
    case 4931:
    case 4932:
    case 4933:
    case 4934:
    case 4966:
    case 4967:
    case 4971:
    case 4976:
    case 5014:
    case 5026:
    case 5029:
    case 5031:
    case 5033:
    case 5035:
    case 5123:
    case 5143:
    case 5484:
    case 8229:
    case 8448:
    case 8478:
    case 8482:
    case 8489:
    case 8492:
    case 8612:
    case 8692:
    case 9025:
    case 9026:
    case 9028:
    case 9030:
    case 9122:
    case 9125:
    case 9127:
    case 9131:
    case 12325:
    case 12544:
    case 12588:
    case 12712:
    case 12788:
    case 13218:
    case 13219:
    case 13221:
    case 13223:
    case 16421:
    case 16684:
    case 16884:
    case 17314:
    case 20517:
    case 20980:
    case 24613:
    case 25076:
    case 29172:
    case 32805:
    case 33058:
    case 33268:
    case 33698:
    case 33699:
    case 37364:
    case 41460:
    case 45556:
    case 49652:
    case 53748:
    case 61696:
    case 61711:
    case 61712:
         return 1;
         break;
  default:
         return 0;
  }
}

/**********************************************************************/
/* Function : CSLinkNext                                              */
/* Purpose  : Return the next link if any  (NULL returns first)       */
/**********************************************************************/
#ifdef WINDOWSPRINTF
/**********************************************************************/
/* Function : printf                                                  */
/* Purpose  : Windows printf routine                                  */
/**********************************************************************/
int cdecl printf(const char * format,
                 ...)
{
  static FILE * File = NULL;
  va_list       v;
  int           n = 0;

  if (!File) File = fopen("stdout","rw");
  if (File)
  {
    va_start(v,format);
                                       /* Write to log (if any)       */
    n = vfprintf(File,format,v);
    va_end(v);
  }
  return n;
}
#endif


#if defined(WIN32) || defined(WIN64)

/**********************************************************************/
/* Function : CSItcCreate                                             */
/* Purpose  : Create an ITC object                                    */
/**********************************************************************/
int CSItcCreate(CSITC * itc)
{
  int rc=0;
  memset(itc,0,sizeof(CSITC));

#if defined(WIN32) || defined(WIN64)
  itc->EventSem = CreateSemaphore( NULL, 0, 1000, NULL );
  if (!itc -> EventSem)
  {
    rc = GetLastError();
    goto MOD_EXIT;
  }
#else
  rc = DosCreateEventSem(NULL,&itc->EventSem,0L,FALSE);
#endif
MOD_EXIT:
  return rc;
}

/**********************************************************************/
/* Function : CSItcDestroy                                            */
/* Purpose  : Destroy an ITC object                                   */
/**********************************************************************/
int CSItcDestroy(CSITC * itc)
{
  CSITCBLOCK * pb;
#if defined(WIN32) || defined(WIN64)
  CloseHandle(itc->EventSem);
#else
  DosCloseEventSem(itc->EventSem);
#endif
  while (1)
  {
    pb = CSListUnlink((CSLISTROOT*)&itc->List,FALSE);
    if (!pb) break;

    if (pb->p) CSFreeMem(pb->p);
    CSFreeMem(pb);
  }
  return 0;
}

/**********************************************************************/
/* Function : CSItcPost                                               */
/* Purpose  : Post a list item                                        */
/**********************************************************************/
int CSItcPost(CSITC * itc,
              void  * parms,
              int     ParmLength,
              void  * p)
{
  int          rc = 0;
  CSITCBLOCK * pb;
                                       /* Get an area                 */
  pb = (CSITCBLOCK *)CSGetMem(sizeof(CSITCBLOCK)+ParmLength,"CSITCBLOCK");
  if (!pb)
  {
    rc = -1;
    goto MOD_EXIT;
  }
  pb -> p          = p;
  pb -> ParmLength = ParmLength;
  memcpy(pb+1,parms,ParmLength);

  CSListLink((CSLISTROOT*)&itc->List,CSO_FRONT,&pb->Header);

#if defined(WIN32) || defined(WIN64)
  ReleaseSemaphore(itc->EventSem,1,NULL);
#else
  DosPostEventSem(itc->EventSem);
#endif

MOD_EXIT:
  return rc;
}

/**********************************************************************/
/* Function : CSItcWait                                               */
/* Purpose  : Wait for a list item                                    */
/**********************************************************************/
int CSItcWait(CSITC *  itc,
              void  *  parms,
              int      length,
              void  ** p,
              int      timeout)
{
  int          rc = 0;
  int          ntrc;
  CSITCBLOCK * pb = 0;

#if defined(WIN32) || defined(WIN64)
  ntrc=WaitForSingleObject(itc->EventSem, timeout);
  switch(ntrc)
  {
    case WAIT_FAILED:
    case WAIT_ABANDONED:
         return CSRC_FAILED;

    case WAIT_TIMEOUT:
         return CSRC_TIMEOUT;

    default:
         rc = 0;
         break;
  }
#else
  if (!itc->PostCount)
  {
    ntrc = DosWaitEventSem(itc->EventSem,timeout);
    switch(ntrc)
    {
      case ERROR_TIMEOUT:
      case ERROR_INTERRUPT:
           return CSRC_TIMEOUT;

      case ERROR_INVALID_HANDLE:
      case ERROR_NOT_ENOUGH_MEMORY:
           return CSRC_FAILED;

      default:
           DosResetEventSem(itc->EventSem,&itc->PostCount);
           break;
    }
  }
  itc->PostCount--;
#endif

  pb = CSListUnlink((CSLISTROOT*)&itc->List,0);
  if (pb)
  {
    *p = pb -> p;
    memcpy(parms,pb+1,pb->ParmLength);
    CSFreeMem(pb);
  }
  else
  {
    rc = CSRC_FAILED;
  }
  return rc;
}

#endif

/**********************************************************************/
/* Function : CSHeadRoom                                              */
/* Purpose  : Function to ensure sufficient headroom                  */
/**********************************************************************/
BOOL CSHeadRoom(CSHEADROOM * Head,
                void      ** ppCur,
                int          Room)
{
  BOOL   Changed = FALSE;
  char * pCur    = *ppCur;
  int    NewSize;
  if (Head->Base)
  {
    int used = (char *)pCur - Head->Base;

    if (used > Head->Size) CSTrap(0);
    if (used < (Head->Size - Room)) goto MOD_EXIT;
                                       /* Need new area               */
                                       /* Double it                   */
    NewSize = Head->Size * 2;
                                       /* Limit increment to 500K     */
    if ((NewSize - Head->Size) > 500*1024)
      NewSize = Head->Size + 500*1024;
                                       /* Must be at least 'room' tho */
    if ((NewSize - Head->Size) < Room)
      NewSize = Head->Size + Room;
    pCur = CSGetMem(NewSize,Head->Name);
    if (!pCur)
    {
      Changed = TRUE;
      *ppCur  = NULL;
      goto MOD_EXIT;
    }
    memcpy(pCur,Head->Base,used);
    CSFreeMem(Head->Base);
    Head->Base = pCur;
    Head->Size = NewSize;
    pCur       = Head->Base + used;
    Changed    = TRUE;
    *ppCur     = pCur;
  }
  else
  {
                                       /* No current area             */
    pCur       = CSGetMem(Room,Head->Name);
    Changed    = TRUE;
    *ppCur     = pCur;
    if (!pCur) goto MOD_EXIT;
    Head->Base = pCur;
    Head->Size = Room;
  }
MOD_EXIT:
  return Changed;
}
