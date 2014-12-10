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
/*  FILE   : QLACCESS.C                                               */
/*  PURPOSE: Give dynamic link access to MQAPI                        */
/**********************************************************************/
                                       /* Include files               */
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#if defined(WIN32)
  #include "windows.h"
#endif
#if defined(AMQ_UNIX)
  #include <errno.h>
  #include <dlfcn.h>
#endif
#include <cmqc.h>
#include "qlcs.h"
#include "qlaccess.h"

#ifndef CHAR
  #define CHAR char
#endif


#if defined(NOLOAD)
  #undef LOADLIBRARY
#endif
                                       /* Defines                     */
#define DO_NOT_LOAD_MQM_DLL 10         /* Error conditions            */
#define ERROR_LOADING_DLL   11

/**********************************************************************/
/* UNSUPPORTED FUNCTIONS                                              */
/**********************************************************************/
static void MQENTRY NotSupportedMQCONNX (PMQCHAR   pQMgrName,
                                         PMQCNO    pConnectOpts,
                                         PMQHCONN  pHconn,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

#ifndef HP_NSK
static void MQENTRY NotSupportedMQSUB (MQHCONN  Hconn,
                                       PMQVOID  pSubDesc,
                                       PMQHOBJ  pHobj,
                                       PMQHOBJ  pHsub,
                                       PMQLONG  pCompCode,
                                       PMQLONG  pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQSUBRQ (MQHCONN  Hconn,
                                         MQHOBJ   Hsub,
                                         MQLONG   Action,
                                         PMQVOID  pSubRqOpts,
                                         PMQLONG  pCompCode,
                                         PMQLONG  pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQCB (MQHCONN  Hconn,
                                      MQLONG   Operation,
                                      PMQVOID  pCallBackDesc,
                                      MQHOBJ   Hobj,
                                      PMQVOID  pMsgDesc,
                                      PMQVOID  pGetMsgOpts,
                                      PMQLONG  pCompCode,
                                      PMQLONG  pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQCTL (MQHCONN  Hconn,
                                       MQLONG   Operation,
                                       PMQVOID  pControlOpts,
                                       PMQLONG  pCompCode,
                                       PMQLONG  pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQCRTMH (MQHCONN   Hconn,
                                         PMQVOID   pCrtMsgHOpts,
                                         PMQHMSG   pHmsg,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQDLTMH (MQHCONN   Hconn,
                                         PMQHMSG   pHmsg,
                                         PMQVOID   pDltMsgHOpts,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQINQMP (MQHCONN   Hconn,
                                         MQHMSG    Hmsg,
                                         PMQVOID   pInqPropOpts,
                                         PMQVOID   pName,
                                         PMQVOID   pPropDesc,
                                         PMQLONG   pType,
                                         MQLONG    ValueLength,
                                         PMQVOID   pValue,
                                         PMQLONG   pDataLength,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQSETMP (MQHCONN   Hconn,
                                         MQHMSG    Hmsg,
                                         PMQVOID   pSetPropOpts,
                                         PMQVOID   pName,
                                         PMQVOID   pPropDesc,
                                         MQLONG    Type,
                                         MQLONG    ValueLength,
                                         PMQVOID   pValue,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQDLTMP (MQHCONN   Hconn,
                                         MQHMSG    Hmsg,
                                         PMQVOID   pDltPropOpts,
                                         PMQVOID   pName,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQBUFMH (MQHCONN   Hconn,
                                         MQHMSG    Hmsg,
                                         PMQVOID   pBufMsgHOpts,
                                         PMQVOID   pMsgDesc,
                                         MQLONG    BufferLength,
                                         PMQVOID   pBuffer,
                                         PMQLONG   pDataLength,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQMHBUF (MQHCONN   Hconn,
                                         MQHMSG    Hmsg,
                                         PMQVOID   pMsgHBufOpts,
                                         PMQVOID   pName,
                                         PMQVOID   pMsgDesc,
                                         MQLONG    BufferLength,
                                         PMQVOID   pBuffer,
                                         PMQLONG   pDataLength,
                                         PMQLONG   pCompCode,
                                         PMQLONG   pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}

static void MQENTRY NotSupportedMQSTAT (MQHCONN  Hconn,
                                        MQLONG   Type,
                                        PMQVOID  pStatus,
                                        PMQLONG  pCompCode,
                                        PMQLONG  pReason)
{
  *pCompCode = MQCC_FAILED;
  *pReason   = MQRC_FUNCTION_NOT_SUPPORTED;
}
#endif


#ifdef LOADLIBRARY

static char * GetDefaultPath(void)
{
  BOOL Bit64 = sizeof(char *) == 8;

  /* printf("Bit64 is %d\n",Bit64); */

#if defined(_LINUX_2)
  return Bit64 ? "/opt/mqm/lib64" : "/opt/mqm/lib";
#elif defined(_SOLARIS_2)
  return Bit64 ? "/opt/mqm/lib64" : "/opt/mqm/lib";
#elif defined(_AIX)
  return Bit64 ? "/usr/mqm/lib64" : "/usr/mqm/lib";
#elif defined(_HPUX_SOURCE)
  return Bit64 ? "/opt/mqm/lib64" : "/opt/mqm/lib";
#elif defined(WIN32) || defined(WIN64)
  return "";
#elif defined(AMQ_AS400)
  return "";
#else
  #error Please add default path to GetDefaultPath
#endif
}

static char * GetClientPath(void)
{
  char * pPath = getenv("MQIC_DLL_PATH");
  if (!pPath) pPath = GetDefaultPath();
  return pPath;
}

static char * GetServerPath(void)
{
  char * pPath = getenv("MQM_DLL_PATH");
  if (!pPath) pPath = GetDefaultPath();
  return pPath;
}
/**********************************************************************/
/* Function : MQAccess                                                */
/* Purpose  : Dynamically load the supplied library. Resolve the MQI  */
/*            function adresses and place them in the given MQAPI     */
/*            control block.                                          */
/*            MQI can then be called using the indirect MQ macros     */
/*            defined in MQACCESS.H.                                  */
/**********************************************************************/
int MQAccess(char * Library, MQAPI * MQApi)
{
  int         rc    = 0;
#ifdef AMQ_AS400
  CSLIBHANDLE lib   = 0;
#else
  CSLIBHANDLE lib   = NULL;
#endif
  CHAR      * pPath = NULL;

  CSLock(0);

  if (Library && *Library)
  {
                                       /* Is this a client ?          */
    if (strstr(Library,"mqic"))
    {
      pPath = GetClientPath();
                                       /* Is it EXACTLY the client    */
      if (!strcmp(Library,"mqic"  )) Library = CLIENT_LIB;
      if (!strcmp(Library,"mqic32")) Library = CLIENT_LIB;
    }
    else
      pPath = GetServerPath();

    rc = CSAccessLibrary(Library,pPath,NULL,0,&lib);
  }
  else
  {
    rc = CSAccessLibrary(SERVER_LIB,GetServerPath(),NULL,0,&lib);
    if (rc) rc = CSAccessLibrary(CLIENT_LIB,GetClientPath(),NULL,0,&lib);
  }
  /********************************************************************/
  /* Do we have a library loaded then ?                               */
  /********************************************************************/
  if (rc) goto MOD_EXIT;
                                       /* Load the MQI entry points   */
  if ((MQApi->MQCONN  = (void *) CSAccessProc(lib, "MQCONN" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQDISC  = (void *) CSAccessProc(lib, "MQDISC" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQCMIT  = (void *) CSAccessProc(lib, "MQCMIT" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQBACK  = (void *) CSAccessProc(lib, "MQBACK" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQPUT   = (void *) CSAccessProc(lib, "MQPUT"  )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQGET   = (void *) CSAccessProc(lib, "MQGET"  )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQPUT1  = (void *) CSAccessProc(lib, "MQPUT1" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQINQ   = (void *) CSAccessProc(lib, "MQINQ"  )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQSET   = (void *) CSAccessProc(lib, "MQSET"  )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQOPEN  = (void *) CSAccessProc(lib, "MQOPEN" )) == NULL)
  {
    rc = ERROR_LOADING_DLL;
    goto MOD_EXIT;
  }
  if ((MQApi->MQCLOSE = (void *) CSAccessProc(lib, "MQCLOSE")) == NULL)
    rc = ERROR_LOADING_DLL;
  /********************************************************************/
  /* The following APIs might not be there. We try to load them       */
  /* but if we fail we point it at a 'dummy' function                 */
  /********************************************************************/
  MQApi->MQCONNX  = (void *) CSAccessProc(lib, "MQCONNX" );
  if (!MQApi->MQCONNX)
  {
    MQApi->MQCONNX  = NotSupportedMQCONNX;
  }

  MQApi->MQSUB    = (void *) CSAccessProc(lib, "MQSUB" );
  if (!MQApi->MQSUB)
  {
    MQApi->MQSUB = NotSupportedMQSUB;
  }

  MQApi->MQSUBRQ  = (void *) CSAccessProc(lib, "MQSUBRQ" );
  if (!MQApi->MQSUBRQ)
  {
    MQApi->MQSUBRQ = NotSupportedMQSUBRQ;
  }

  MQApi->MQCB  = (void *) CSAccessProc(lib, "MQCB" );
  if (!MQApi->MQCB)
  {
    MQApi->MQCB = NotSupportedMQCB;
  }

  MQApi->MQCTL  = (void *) CSAccessProc(lib, "MQCTL" );
  if (!MQApi->MQCTL)
  {
    MQApi->MQCTL = NotSupportedMQCTL;
  }

  MQApi->MQCRTMH  = (void *) CSAccessProc(lib, "MQCRTMH" );
  if (!MQApi->MQCRTMH)
  {
    MQApi->MQCRTMH = NotSupportedMQCRTMH;
  }

  MQApi->MQDLTMH  = (void *) CSAccessProc(lib, "MQDLTMH" );
  if (!MQApi->MQDLTMH)
  {
    MQApi->MQDLTMH = NotSupportedMQDLTMH;
  }

  MQApi->MQINQMP  = (void *) CSAccessProc(lib, "MQINQMP" );
  if (!MQApi->MQINQMP)
  {
    MQApi->MQINQMP = NotSupportedMQINQMP;
  }

  MQApi->MQSETMP  = (void *) CSAccessProc(lib, "MQSETMP" );
  if (!MQApi->MQSETMP)
  {
    MQApi->MQSETMP = NotSupportedMQSETMP;
  }

  MQApi->MQDLTMP  = (void *) CSAccessProc(lib, "MQDLTMP" );
  if (!MQApi->MQDLTMP)
  {
    MQApi->MQDLTMP = NotSupportedMQDLTMP;
  }

  MQApi->MQBUFMH  = (void *) CSAccessProc(lib, "MQBUFMH" );
  if (!MQApi->MQBUFMH)
  {
    MQApi->MQBUFMH = NotSupportedMQBUFMH;
  }

  MQApi->MQMHBUF  = (void *) CSAccessProc(lib, "MQMHBUF" );
  if (!MQApi->MQMHBUF)
  {
    MQApi->MQMHBUF = NotSupportedMQMHBUF;
  }

  MQApi->MQSTAT  = (void *) CSAccessProc(lib, "MQSTAT" );
  if (!MQApi->MQSTAT)
  {
    MQApi->MQSTAT = NotSupportedMQSTAT;
  }

MOD_EXIT:
  CSUnlock(0);
  return rc;
}
#endif

#ifndef LOADLIBRARY
/**********************************************************************/
/* Function : MQAccess                                                */
/* Purpose  : Non dynamically loaded version.                         */
/**********************************************************************/
int MQAccess(char * Library, MQAPI * MQApi)
{
  MQApi->MQCONN  = MQCONN;
  MQApi->MQCONNX = MQCONNX;
  MQApi->MQDISC  = MQDISC;
  MQApi->MQCMIT  = MQCMIT;
  MQApi->MQBACK  = MQBACK;
  MQApi->MQPUT   = MQPUT;
  MQApi->MQGET   = MQGET;
  MQApi->MQPUT1  = MQPUT1;
  MQApi->MQINQ   = MQINQ;
  MQApi->MQSET   = MQSET;
  MQApi->MQOPEN  = MQOPEN;
  MQApi->MQCLOSE = MQCLOSE;

#ifndef HP_NSK
  MQApi->MQSUB   = MQSUB;
  MQApi->MQSUBRQ = MQSUBRQ;
  MQApi->MQCB    = MQCB;
  MQApi->MQCTL   = MQCTL;
  MQApi->MQCRTMH = MQCRTMH;
  MQApi->MQDLTMH = MQDLTMH;
  MQApi->MQINQMP = MQINQMP;
  MQApi->MQSETMP = MQSETMP;
  MQApi->MQDLTMP = MQDLTMP;
  MQApi->MQBUFMH = MQBUFMH;
  MQApi->MQMHBUF = MQMHBUF;
  MQApi->MQSTAT  = MQSTAT;
#endif

  return 0;
}
#endif
