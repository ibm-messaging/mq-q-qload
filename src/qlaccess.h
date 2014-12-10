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
/*  FILE   : QLACCESS.H                                               */
/*  PURPOSE: Give dynamic access to MQAPI                             */
/**********************************************************************/

#if defined(MVS) || defined (HP_NSK)

  #define SERVER_LIB   NULL
  #define CLIENT_LIB   NULL
                                       /* Datatypes                   */
#elif defined(WIN32)
  #define LOADLIBRARY
  #define SERVER_LIB   "mqm.dll"
  #define CLIENT_LIB   "mqic32.dll"
  #define MQ_DEFAULT_PATH ""

#elif defined(AMQ_UNIX) || defined(AMQ_AS400)
  #define LOADLIBRARY

#ifdef _AIX
  #ifdef _REENTRANT
    #define SERVER_LIB   "libmqm_r.a(libmqm_r.o)"
    #define CLIENT_LIB   "libmqic_r.a(mqic_r.o)"
  #else
    #define SERVER_LIB   "libmqm.a(libmqm.o)"
    #define CLIENT_LIB   "libmqic.a(mqic.o)"
  #endif
#endif

#ifdef _HPUX_SOURCE
  #ifdef HP_IPF
    #ifdef _REENTRANT
      #define SERVER_LIB   "libmqm_r.so"
      #define CLIENT_LIB   "libmqic_r.so"
    #else
      #define SERVER_LIB   "libmqm.so"
      #define CLIENT_LIB   "libmqic.so"
    #endif
  #else
    #ifdef _REENTRANT
      #define SERVER_LIB   "libmqm_r.sl"
      #define CLIENT_LIB   "libmqic_r.sl"
    #else
      #define SERVER_LIB   "libmqm.sl"
      #define CLIENT_LIB   "libmqic.sl"
    #endif
  #endif
#endif

#ifdef _SOLARIS_2
  #define SERVER_LIB   "libmqm.so"
  #define CLIENT_LIB   "libmqic.so"
#endif

#ifdef AMQ_AS400
  #ifdef _REENTRANT
    #define SERVER_LIB   "QMQM/LIBMQM_R"
    #define CLIENT_LIB   "QMQM/LIBMQIC_R"
  #else
    #define SERVER_LIB   "QMQM/LIBMQM"
    #define CLIENT_LIB   "QMQM/LIBMQIC"
  #endif
#endif

#if defined (_LINUX_2)
  #ifdef _REENTRANT
    #define SERVER_LIB   "libmqm_r.so"
    #define CLIENT_LIB   "libmqic_r.so"
  #else
    #define SERVER_LIB   "libmqm.so"
    #define CLIENT_LIB   "libmqic.so"
  #endif
#endif

#if defined (AMQ_DARWIN)
  #ifdef _REENTRANT
    #define SERVER_LIB   "libmqm_r.dylib"
    #define CLIENT_LIB   "libmqic_r.dylib"
  #else
    #define SERVER_LIB   "libmqm.dylib"
    #define CLIENT_LIB   "libmqic.dylib"
  #endif
#endif

#endif
                                       /* Datatypes                   */
#define PMQENTRY  MQENTRY *
                                       /* Indirect MQAPI macros       */
#ifdef NOLOAD
#define IMQBACK(m,Hconn,pC,pR) \
     MQBACK(Hconn,pC,pR)

#define IMQCLOSE(m,Hconn,pHobj,Options,pC,pR) \
     MQCLOSE(Hconn,pHobj,Options,pC,pR)

#define IMQCMIT(m,Hconn,pC,pR) \
     MQCMIT(Hconn,pC,pR)

#define IMQCONN(m,pName,pHconn,pC,pR) \
     MQCONN(pName,pHconn,pC,pR)

#define IMQCONNX(m,pName,pConnOpts,pHconn,pC,pR) \
     MQCONNX(pName,pConnOpts,pHconn,pC,pR)

#define IMQDISC(m,pHconn,pC,pR) \
     MQDISC(pHconn,pC,pR)

#define IMQGET(m,Hconn,Hobj,pMQMD,pMQGMO,BufLen,Buf,DatLen,pC,pR) \
     MQGET(Hconn,Hobj,pMQMD,pMQGMO,BufLen,Buf,DatLen,pC,pR)

#define IMQINQ(m,Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR) \
     MQINQ(Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR)

#define IMQOPEN(m,Hconn,pMQOD,Opt,pHobj,pC,pR) \
     MQOPEN(Hconn,pMQOD,Opt,pHobj,pC,pR)

#define IMQPUT(m,Hconn,Hobj,pMQMD,pMQPMO,BufLen,Buf,pC,pR) \
     MQPUT(Hconn,Hobj,pMQMD,pMQPMO,BufLen,Buf,pC,pR)

#define IMQPUT1(m,Hconn,pMQOD,pMQMD,pMQPMO,BufLen,Buf,pC,pR) \
     MQPUT1(Hconn,pMQOD,pMQMD,pMQPMO,BufLen,Buf,pC,pR)

#define IMQSET(m,Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR) \
     MQSET(Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR)

#define IMQSUB(m,Hconn,pSD,Hobj,Hsub,pC,pR) \
     MQSUB(Hconn,pSD,Hobj,Hsub,pC,pR)

#define IMQSUBRQ(m,Hconn,Hsub,Action,pMQSRO,pC,pR) \
     MQSUBRQ(Hconn,Hsub,Action,pMQSRO,pC,pR)

#define IMQCB(m,Hconn,Operation,pMQCBD,Hobj,pMQMD,pMQGMO,pC,pR) \
     MQCB(Hconn,Operation,pMQCBD,Hobj,pMQMD,pMQGMO,pC,pR)

#define IMQCTL(m,Hconn,Operation,pCTLO,pC,pR) \
     MQCTL(Hconn,Operation,pCTLO,pC,pR)

#define IMQCRTMH(m,Hconn,pCMHO,pHmsg,pC,pR) \
     MQCRTMH(Hconn,pCMHO,pHmsg,pC,pR)

#define IMQDLTMH(m,Hconn,pHmsg,pDMHO,pC,pR) \
     MQDLTMH(Hconn,pHmsg,pDMHO,pC,pR)

#define IMQINQMP(m,Hconn,Hmsg,pIPO,pName,pPD,pType,ValLen,pValue,DatLen,pC,pR) \
     MQINQMP(Hconn,Hmsg,pIPO,pName,pPD,pType,ValLen,pValue,DatLen,pC,pR)

#define IMQSETMP(m,Hconn,Hmsg,pSPO,pName,pPD,Type,ValLen,pValue,pC,pR) \
     MQSETMP(Hconn,Hmsg,pSPO,pName,pPD,Type,ValLen,pValue,pC,pR)

#define IMQDLTMP(m,Hconn,Hmsg,pDPO,pName,pC,pR) \
     MQDLTMP(Hconn,Hmsg,pDPO,pName,pC,pR)

#define IMQBUFMH(m,Hconn,Hmsg,pBMHO,pMQMD,BufLen,Buf,DatLen,pC,pR) \
     MQBUFMH(Hconn,Hmsg,pBMHO,pMQMD,BufLen,Buf,DatLen,pC,pR)

#define IMQMHBUF(m,Hconn,Hmsg,pMHBO,pName,pMQMD,BufLen,Buf,DatLen,pC,pR) \
     MQMHBUF(Hconn,Hmsg,pMHBO,pName,pMQMD,BufLen,Buf,DatLen,pC,pR)

#define IMQSTAT(m,Hconn,Type,pStatus,pC,pR) \
     MQSTAT(Hconn,Type,pStatus,pC,pR)

#else  /* not NOLOAD */
#define IMQBACK(m,Hconn,pC,pR) \
     (*(m).MQBACK)(Hconn,pC,pR)

#define IMQCLOSE(m,Hconn,pHobj,Options,pC,pR) \
     (*(m).MQCLOSE)(Hconn,pHobj,Options,pC,pR)

#define IMQCMIT(m,Hconn,pC,pR) \
     (*(m).MQCMIT)(Hconn,pC,pR)

#define IMQCONN(m,pName,pHconn,pC,pR) \
     (*(m).MQCONN)(pName,pHconn,pC,pR)

#define IMQCONNX(m,pName,pConnOpts,pHconn,pC,pR) \
     (*(m).MQCONNX)(pName,pConnOpts,pHconn,pC,pR)

#define IMQDISC(m,pHconn,pC,pR) \
     (*(m).MQDISC)(pHconn,pC,pR)

#define IMQGET(m,Hconn,Hobj,pMQMD,pMQGMO,BufLen,Buf,DatLen,pC,pR) \
     (*(m).MQGET)(Hconn,Hobj,pMQMD,pMQGMO,BufLen,Buf,DatLen,pC,pR)

#define IMQINQ(m,Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR) \
     (*(m).MQINQ)(Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR)

#define IMQOPEN(m,Hconn,pMQOD,Opt,pHobj,pC,pR) \
     (*(m).MQOPEN)(Hconn,pMQOD,Opt,pHobj,pC,pR)

#define IMQPUT(m,Hconn,Hobj,pMQMD,pMQPMO,BufLen,Buf,pC,pR) \
     (*(m).MQPUT)(Hconn,Hobj,pMQMD,pMQPMO,BufLen,Buf,pC,pR)

#define IMQPUT1(m,Hconn,pMQOD,pMQMD,pMQPMO,BufLen,Buf,pC,pR) \
     (*(m).MQPUT1)(Hconn,pMQOD,pMQMD,pMQPMO,BufLen,Buf,pC,pR)

#define IMQSET(m,Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR) \
     (*(m).MQSET)(Hconn,Hobj,Sc,pS,Ia,pI,Cha,pCh,pC,pR)

#ifndef HP_NSK

#define IMQSUB(m,Hconn,pSD,Hobj,Hsub,pC,pR) \
     (*(m).MQSUB)(Hconn,pSD,Hobj,Hsub,pC,pR)

#define IMQSUBRQ(m,Hconn,Hsub,Action,pMQSRO,pC,pR) \
     (*(m).MQSUBRQ)(Hconn,Hsub,Action,pMQSRO,pC,pR)

#define IMQCB(m,Hconn,Operation,pMQCBD,Hobj,pMQMD,pMQGMO,pC,pR) \
     (*(m).MQCB)(Hconn,Operation,pMQCBD,Hobj,pMQMD,pMQGMO,pC,pR)

#define IMQCTL(m,Hconn,Operation,pCTLO,pC,pR) \
     (* (m).MQCTL)(Hconn,Operation,pCTLO,pC,pR)

#define IMQCRTMH(m,Hconn,pCMHO,pHmsg,pC,pR) \
     (* (m).MQCRTMH)(Hconn,pCMHO,pHmsg,pC,pR)

#define IMQDLTMH(m,Hconn,pHmsg,pDMHO,pC,pR) \
     (* (m).MQDLTMH)(Hconn,pHmsg,pDMHO,pC,pR)

#define IMQINQMP(m,Hconn,Hmsg,pIPO,pName,pPD,pType,ValLen,pValue,DatLen,pC,pR) \
     (* (m).MQINQMP)(Hconn,Hmsg,pIPO,pName,pPD,pType,ValLen,pValue,DatLen,pC,pR)

#define IMQSETMP(m,Hconn,Hmsg,pSPO,pName,pPD,Type,ValLen,pValue,pC,pR) \
     (* (m).MQSETMP)(Hconn,Hmsg,pSPO,pName,pPD,Type,ValLen,pValue,pC,pR)

#define IMQDLTMP(m,Hconn,Hmsg,pDPO,pName,pC,pR) \
     (* (m).MQDLTMP)(Hconn,Hmsg,pDPO,pName,pC,pR)

#define IMQBUFMH(m,Hconn,Hmsg,pBMHO,pMQMD,BufLen,Buf,DatLen,pC,pR) \
     (* (m).MQBUFMH)(Hconn,Hmsg,pBMHO,pMQMD,BufLen,Buf,DatLen,pC,pR)

#define IMQMHBUF(m,Hconn,Hmsg,pMHBO,pName,pMQMD,BufLen,Buf,DatLen,pC,pR) \
     (* (m).MQMHBUF)(Hconn,Hmsg,pMHBO,pName,pMQMD,BufLen,Buf,DatLen,pC,pR)

#define IMQSTAT(m,Hconn,Type,pStatus,pC,pR) \
     (* (m).MQSTAT)(Hconn,Type,pStatus,pC,pR)

#endif

#endif /* not NOLOAD */
                                       /* Typedefs                    */
typedef struct _MQAPI
{
 void (PMQENTRY MQBACK) (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQCLOSE) (
   MQHCONN  Hconn,      /* Connection handle */
   PMQHOBJ  pHobj,      /* Object handle */
   MQLONG   Options,    /* Options that control the action of MQCLOSE */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQCMIT) (
   MQHCONN  Hconn,      /* Connection handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQCONN) (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQCONNX) (
   PMQCHAR   pName,      /* Name of queue manager */
   PMQCNO    pConnOpts,  /* Connect Options */
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQDISC) (
   PMQHCONN  pHconn,     /* Connection handle */
   PMQLONG   pCompCode,  /* Completion code */
   PMQLONG   pReason);   /* Reason code qualifying CompCode */

 void (PMQENTRY MQGET) (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pGetMsgOpts,   /* Options that control the action of
                              MQGET */
   MQLONG   BufferLength,  /* Length in bytes of the Buffer area */
   PMQVOID  pBuffer,       /* Area to contain the message data */
   PMQLONG  pDataLength,   /* Length of the message */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason);      /* Reason code qualifying CompCode */

 void (PMQENTRY MQINQ) (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason);        /* Reason code qualifying CompCode */

 void (PMQENTRY MQOPEN) (
   MQHCONN  Hconn,      /* Connection handle */
   PMQVOID  pObjDesc,   /* Object descriptor */
   MQLONG   Options,    /* Options that control the action of MQOPEN */
   PMQHOBJ  pHobj,      /* Object handle */
   PMQLONG  pCompCode,  /* Completion code */
   PMQLONG  pReason);   /* Reason code qualifying CompCode */


 void (PMQENTRY MQPUT) (
   MQHCONN  Hconn,         /* Connection handle */
   MQHOBJ   Hobj,          /* Object handle */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason);      /* Reason code qualifying CompCode */


 void (PMQENTRY MQPUT1) (
   MQHCONN  Hconn,         /* Connection handle */
   PMQVOID  pObjDesc,      /* Object descriptor */
   PMQVOID  pMsgDesc,      /* Message descriptor */
   PMQVOID  pPutMsgOpts,   /* Options that control the action of
                              MQPUT1 */
   MQLONG   BufferLength,  /* Length of the message in Buffer */
   PMQVOID  pBuffer,       /* Message data */
   PMQLONG  pCompCode,     /* Completion code */
   PMQLONG  pReason);      /* Reason code qualifying CompCode */


 void (PMQENTRY MQSET) (
   MQHCONN  Hconn,           /* Connection handle */
   MQHOBJ   Hobj,            /* Object handle */
   MQLONG   SelectorCount,   /* Count of selectors */
   PMQLONG  pSelectors,      /* Array of attribute selectors */
   MQLONG   IntAttrCount,    /* Count of integer attributes */
   PMQLONG  pIntAttrs,       /* Array of integer attributes */
   MQLONG   CharAttrLength,  /* Length of character attributes buffer */
   PMQCHAR  pCharAttrs,      /* Character attributes */
   PMQLONG  pCompCode,       /* Completion code */
   PMQLONG  pReason);        /* Reason code qualifying CompCode */

#ifndef HP_NSK

 void (PMQENTRY MQSUB) (
   MQHCONN  Hconn,      /* I: Connection handle */
   PMQVOID  pSubDesc,   /* IO: Subscription descriptor */
   PMQHOBJ  pHobj,      /* IO: Object handle for queue */
   PMQHOBJ  pHsub,      /* O: Subscription object handle */
   PMQLONG  pCompCode,  /* OC: Completion code */
   PMQLONG  pReason);   /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQSUBRQ) (
   MQHCONN  Hconn,      /* I: Connection handle */
   MQHOBJ   Hsub,       /* I: Subscription handle */
   MQLONG   Action,     /* I: Action requested on the subscription */
   PMQVOID  pSubRqOpts, /* IO: Subscription Request Options */
   PMQLONG  pCompCode,  /* O: Completion code */
   PMQLONG  pReason);   /* O: Reason code qualifying CompCode */

 void (PMQENTRY MQCB) (
   MQHCONN  Hconn,         /* I: Connection handle */
   MQLONG   Operation,     /* I: Operation */
   PMQVOID  pCallbackDesc, /* I: Callback descriptor */
   MQHOBJ   Hobj,          /* I: Object handle */
   PMQVOID  pMsgDesc,      /* I: Message Descriptor */
   PMQVOID  pGetMsgOpts,   /* I: Get options */
   PMQLONG  pCompCode,     /* O: Completion code */
   PMQLONG  pReason);      /* O: Reason code qualifying CompCode */

 void (PMQENTRY MQCTL) (
   MQHCONN  Hconn,        /* I: Connection handle */
   MQLONG   Operation,    /* I: Operation */
   PMQVOID  pControlOpts, /* I: Control options */
   PMQLONG  pCompCode,    /* O: Completion code */
   PMQLONG  pReason);     /* O: Reason code qualifying CompCode */



 void (PMQENTRY MQCRTMH) (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQVOID   pCrtMsgHOpts,  /* I: Options that control the action of MQCRTMH */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQDLTMH) (
   MQHCONN   Hconn,         /* I: Connection handle */
   PMQHMSG   pHmsg,         /* IO: Message handle */
   PMQVOID   pDltMsgHOpts,  /* I: Options that control the action of MQDLTMH */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQINQMP) (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pInqPropOpts,  /* IO: Options that control the action of MQINQMP */
   PMQVOID   pName,         /* I: Property name */
   PMQVOID   pPropDesc,     /* O: Property descriptor */
   PMQLONG   pType,         /* IO: Property data type */
   MQLONG    ValueLength,   /* IL: Length in bytes of the Value area */
   PMQVOID   pValue,        /* OB: Property value */
   PMQLONG   pDataLength,   /* O: Length of the property value */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQSETMP) (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pSetPropOpts,  /* I: Options that control the action of MQSETMP */
   PMQVOID   pName,         /* I: Property name */
   PMQVOID   pPropDesc,     /* IO: Property descriptor */
   MQLONG    Type,          /* I: Property data type */
   MQLONG    ValueLength,   /* IL: Length of the Value area */
   PMQVOID   pValue,        /* IB: Property value */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQDLTMP) (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pDltPropOpts,  /* I: Options that control the action of MQDLTMP */
   PMQVOID   pName,         /* I: Property name */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQBUFMH) (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pBufMsgHOpts,  /* I: Options that control the action of MQBUFMH */
   PMQVOID   pMsgDesc,      /* IO: Message descriptor */
   MQLONG    BufferLength,  /* IL: Length in bytes of the Buffer area */
   PMQVOID   pBuffer,       /* IOB: Area to contain the message buffer */
   PMQLONG   pDataLength,   /* O: Length of the output buffer */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQMHBUF) (
   MQHCONN   Hconn,         /* I: Connection handle */
   MQHMSG    Hmsg,          /* I: Message handle */
   PMQVOID   pMsgHBufOpts,  /* I: Options that control the action of MQMHBUF */
   PMQVOID   pName,         /* I: Property name */
   PMQVOID   pMsgDesc,      /* IO: Message descriptor */
   MQLONG    BufferLength,  /* IL: Length in bytes of the Buffer area */
   PMQVOID   pBuffer,       /* OB: Area to contain the properties */
   PMQLONG   pDataLength,   /* O: Length of the properties */
   PMQLONG   pCompCode,     /* OC: Completion code */
   PMQLONG   pReason);      /* OR: Reason code qualifying CompCode */

 void (PMQENTRY MQSTAT) (
   MQHCONN  Hconn,      /* I: Connection handle */
   MQLONG   Type,       /* I: Status information type */
   PMQVOID  pStatus,    /* O: Status information */
   PMQLONG  pCompCode,  /* O: Completion code */
   PMQLONG  pReason);   /* O: Reason code qualifying CompCode */

#endif

} MQAPI;
                                       /* Prototypes                  */
int MQAccess   (char * Library,MQAPI * MQApi);
