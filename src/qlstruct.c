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
/*  FILE   : QLSTRUCT.C                                               */
/*  PURPOSE: MQSeries structure definitions                           */
/**********************************************************************/
#ifdef WINDOWS
#include <windows.h>
#endif

#include <string.h>
#include <cmqc.h>
#include <cmqcfc.h>
#include <cmqxc.h>
#define MQSTRUCT_DEFINITIONS
#include "qlvalues.h"
#include "qlstruct.h"
                                       /* External ValueSets          */
                                       /* In MQVALUES.C               */
extern MQVALUE ValMQRC[];
extern MQVALUE ValMQPARM[];
extern MQVALUE ValMQCMD[];

int       Initialised = 0;

/**********************************************************************/
/* These are the most likely structures to change across releases.    */
/* Make sure compilation fails if the file is not updated             */
/**********************************************************************/
#if MQGMO_CURRENT_VERSION > 4
#error Need to update MQGMO fields
#endif
#if MQPMO_CURRENT_VERSION > 3
#error Need to update MQPMO fields
#endif

/**********************************************************************/
/* Depend sets                                                        */
/**********************************************************************/
MQVALUE ValQueueStatus[] =
{
  0                         ,NULL                           ,NULL
};


MQVALUE ValRequestFlags[] =
{
  0                         ,NULL                           ,NULL
};

MQVALUE ValNotifyCode[] =
{
  0                         ,NULL                         ,NULL
};

MQVALUE ValSocketAction[] =
{
  0                         ,NULL                         ,NULL
};


MQVALUE ValMQOPENOptions[] =
{
         MQOO_ALTERNATE_USER_AUTHORITY ,"MQOO_ALTERNATE_USER_AUTHORITY","Alternate user authority",
         MQOO_BIND_AS_Q_DEF            ,"MQOO_BIND_AS_Q_DEF"           ,"Bind as Q Def",
         MQOO_BIND_NOT_FIXED           ,"MQOO_BIND_NOT_FIXED"          ,"Bind not fixed",
         MQOO_BIND_ON_GROUP            ,"MQOO_BIND_ON_GROUP"           ,"Bind on group",
         MQOO_BIND_ON_OPEN             ,"MQOO_BIND_ON_OPEN"            ,"Bind on open",
         MQOO_BROWSE                   ,"MQOO_BROWSE"                  ,"Browse",
         MQOO_CO_OP                    ,"MQOO_CO_OP"                   ,"Co Op",
         MQOO_FAIL_IF_QUIESCING        ,"MQOO_FAIL_IF_QUIESCING"       ,"Fail if quiescing",
         MQOO_INPUT_AS_Q_DEF           ,"MQOO_INPUT_AS_Q_DEF"          ,"Input as Queue Definition",
         MQOO_INPUT_EXCLUSIVE          ,"MQOO_INPUT_EXCLUSIVE"         ,"Input exlusive",
         MQOO_INPUT_SHARED             ,"MQOO_INPUT_SHARED"            ,"Input shared",
         MQOO_INQUIRE                  ,"MQOO_INQUIRE"                 ,"Inquire",
         MQOO_NO_MULTICAST             ,"MQOO_NO_MULTICAST"            ,"No Multicast",
         MQOO_NO_READ_AHEAD            ,"MQOO_NO_READ_AHEAD"           ,"No Read Ahead",
         MQOO_OUTPUT                   ,"MQOO_OUTPUT"                  ,"Output",
         MQOO_PASS_ALL_CONTEXT         ,"MQOO_PASS_ALL_CONTEXT"        ,"Pass all context",
         MQOO_PASS_IDENTITY_CONTEXT    ,"MQOO_PASS_IDENTITY_CONTEXT"   ,"Pass identity context",
         MQOO_RESOLVE_NAMES            ,"MQOO_RESOLVE_NAMES"           ,"Resolve names",
         MQOO_READ_AHEAD               ,"MQOO_READ_AHEAD"              ,"Read Ahead",
         MQOO_READ_AHEAD_AS_Q_DEF      ,"MQOO_READ_AHEAD_AS_Q_DEF"     ,"Read Ahead as Queue Definition",
         MQOO_RESOLVE_LOCAL_Q          ,"MQOO_RESOLVE_LOCAL_Q"         ,"Resolve Local Queue",
         MQOO_RESOLVE_LOCAL_TOPIC      ,"MQOO_RESOLVE_LOCAL_TOPIC"     ,"Resolve Local Topic",
         MQOO_SAVE_ALL_CONTEXT         ,"MQOO_SAVE_ALL_CONTEXT"        ,"Save all context",
         MQOO_SET                      ,"MQOO_SET"                     ,"Set",
         MQOO_SET_ALL_CONTEXT          ,"MQOO_SET_ALL_CONTEXT"         ,"Set all context",
         MQOO_SET_IDENTITY_CONTEXT     ,"MQOO_SET_IDENTITY_CONTEXT"    ,"Set identity context",
         0                             ,NULL                           ,NULL
};

MQVALUE ValMQCLOSEOptions[] =
{
         MQCO_NONE                     ,"MQCO_NONE"                    ,"None",
         MQCO_DELETE                   ,"MQCO_DELETE"                  ,"Delete",
         MQCO_DELETE_PURGE             ,"MQCO_DELETE_PURGE"            ,"Delete & Purge",
         MQCO_KEEP_SUB                 ,"MQCO_KEEP_SUB"                ,"Keep Sub",
         MQCO_REMOVE_SUB               ,"MQCO_REMOVE_SUB"              ,"Remove Sub",
         MQCO_QUIESCE                  ,"MQCO_QUIESCE"                 ,"Quiesce",
         0                             ,NULL                           ,NULL
};

MQVALUE ValMQSUBOptions[] =
{
         MQSO_CREATE                   ,"MQSO_CREATE"                  ,"Create",
         MQSO_RESUME                   ,"MQSO_RESUME"                  ,"Resume",
         MQSO_ALTER                    ,"MQSO_ALTER"                   ,"Alter",
         MQSO_DURABLE                  ,"MQSO_DURABLE"                 ,"Durable",
         MQSO_MANAGED                  ,"MQSO_MANAGED"                 ,"Managed",
         MQSO_GROUP_SUB                ,"MQSO_GROUP_SUB"               ,"Group sub",
         MQSO_ANY_USERID               ,"MQSO_ANY_USERID"              ,"Any user id",
         MQSO_FIXED_USERID             ,"MQSO_FIXED_USERID"            ,"Fixed user id",
         MQSO_PUBLICATIONS_ON_REQUEST  ,"MQSO_PUBLICATIONS_ON_REQUEST" ,"Publications on request",
         MQSO_NEW_PUBLICATIONS_ONLY    ,"MQSO_NEW_PUBLICATIONS_ONLY"   ,"New publications only",
         MQSO_WILDCARD_CHAR            ,"MQSO_WILDCARD_CHAR"           ,"Wildcard char",
         MQSO_WILDCARD_TOPIC           ,"MQSO_WILDCARD_TOPIC"          ,"Wildcard topic",
         MQSO_ALTERNATE_USER_AUTHORITY ,"MQSO_ALTERNATE_USER_AUTHORITY","Alternate user authority",
         MQSO_SET_CORREL_ID            ,"MQSO_SET_CORREL_ID"           ,"Set correl id",
         MQSO_FAIL_IF_QUIESCING        ,"MQSO_FAIL_IF_QUIESCING"       ,"Fail if quiescing",
         MQSO_NO_READ_AHEAD            ,"MQSO_NO_READ_AHEAD"           ,"No Read Ahead",
         MQSO_READ_AHEAD               ,"MQSO_READ_AHEAD"              ,"Read Ahead",
         MQSO_READ_AHEAD_AS_Q_DEF      ,"MQSO_READ_AHEAD_AS_Q_DEF"     ,"Read ahead as q def",
         MQSO_SET_IDENTITY_CONTEXT     ,"MQSO_SET_IDENTITY_CONTEXT"    ,"Set identity context",
         MQSO_NO_MULTICAST             ,"MQSO_NO_MULTICAST"            ,"No multicast",
         MQSO_SCOPE_QMGR               ,"MQSO_SCOPE_QMGR"              ,"Scope qmgr",
         0                             ,NULL                           ,NULL
};

MQVALUE ValMQSUBRQOptions[] =
{
         MQSRO_NONE                    ,"MQSRO_NONE"                   ,"None",
         MQSRO_FAIL_IF_QUIESCING       ,"MQSRO_FAIL_IF_QUIESCING"      ,"Fail if quiescing",
         0                             ,NULL                           ,NULL
};

MQVALUE ValMQGETOptions[] =
{
         MQGMO_ACCEPT_TRUNCATED_MSG    ,"MQGMO_ACCEPT_TRUNCATED_MSG"   ,"Accept truncated message",
         MQGMO_ALL_MSGS_AVAILABLE      ,"MQGMO_ALL_MSGS_AVAILABLE"     ,"All messages available",
         MQGMO_ALL_SEGMENTS_AVAILABLE  ,"MQGMO_ALL_SEGMENTS_AVAILABLE" ,"All segments available",
         MQGMO_BROWSE_FIRST            ,"MQGMO_BROWSE_FIRST"           ,"Browse first",
         MQGMO_BROWSE_MSG_UNDER_CURSOR ,"MQGMO_BROWSE_MSG_UNDER_CURSOR","Browse message under cursor",
         MQGMO_BROWSE_NEXT             ,"MQGMO_BROWSE_NEXT"            ,"Browse next",
         MQGMO_COMPLETE_MSG            ,"MQGMO_COMPLETE_MSG"           ,"Complete message",
         MQGMO_CONVERT                 ,"MQGMO_CONVERT"                ,"Convert",
         MQGMO_FAIL_IF_QUIESCING       ,"MQGMO_FAIL_IF_QUIESCING"      ,"Fail if quiescing",
         MQGMO_LOCK                    ,"MQGMO_LOCK"                   ,"Lock",
         MQGMO_LOGICAL_ORDER           ,"MQGMO_LOGICAL_ORDER"          ,"Logical order",
         MQGMO_MARK_BROWSE_CO_OP       ,"MQGMO_MARK_BROWSE_CO_OP"      ,"Mark Browse CoOp",
         MQGMO_MARK_BROWSE_HANDLE      ,"MQGMO_MARK_BROWSE_HANDLE"     ,"Mark Browse Handle",
         MQGMO_MARK_SKIP_BACKOUT       ,"MQGMO_MARK_SKIP_BACKOUT"      ,"Mark skip backout",
         MQGMO_MSG_UNDER_CURSOR        ,"MQGMO_MSG_UNDER_CURSOR"       ,"Get message under cursor",
         MQGMO_NO_PROPERTIES           ,"MQGMO_NO_PROPERTIES"          ,"No Properties",
         MQGMO_NO_SYNCPOINT            ,"MQGMO_NO_SYNCPOINT"           ,"No syncpoint",
         MQGMO_NO_WAIT                 ,"MQGMO_NO_WAIT"                ,"No wait",
         MQGMO_PROPERTIES_COMPATIBILITY,"MQGMO_PROPERTIES_COMPATIBILITY","Properties Conpatibility",
         MQGMO_PROPERTIES_FORCE_MQRFH2 ,"MQGMO_PROPERTIES_FORCE_MQRFH2","Properties Force MQRFH2",
         MQGMO_PROPERTIES_IN_HANDLE    ,"MQGMO_PROPERTIES_IN_HANDLE"   ,"Properties in Handle",
         MQGMO_SET_SIGNAL              ,"MQGMO_SET_SIGNAL"             ,"Set signal",
         MQGMO_SYNCPOINT               ,"MQGMO_SYNCPOINT"              ,"Syncpoint",
         MQGMO_SYNCPOINT_IF_PERSISTENT ,"MQGMO_SYNCPOINT_IF_PERSISTENT","Syncpoint if persistent",
         MQGMO_UNLOCK                  ,"MQGMO_UNLOCK"                 ,"Unlock",
         MQGMO_UNMARKED_BROWSE_MSG     ,"MQGMO_UNMARKED_BROWSE_MSG"    ,"Unmarked Browse Msg",
         MQGMO_UNMARK_BROWSE_CO_OP     ,"MQGMO_UNMARK_BROWSE_CO_OP"    ,"Unmark Browse CoOp",
         MQGMO_UNMARK_BROWSE_HANDLE    ,"MQGMO_UNMARK_BROWSE_HANDLE"   ,"Unmark Browse Handle",
         MQGMO_WAIT                    ,"MQGMO_WAIT"                   ,"Wait",
         MQGMO_NONE                    ,"MQGMO_NONE"                   ,"None",
         0                             ,NULL                           ,NULL
};

MQVALUE ValMQGETSignals[] =
{
  MQEC_MSG_ARRIVED              ,"MQEC_MSG_ARRIVED"             ,"Message has arrived",
  MQEC_WAIT_INTERVAL_EXPIRED    ,"MQEC_WAIT_INTERVAL_EXPIRED"   ,"Wait interval expired",
  MQEC_WAIT_CANCELED            ,"MQEC_WAIT_CANCELED"           ,"Wait cancelled",
  MQEC_Q_MGR_QUIESCING          ,"MQEC_Q_MGR_QUIESCING"         ,"QMgr quiescing",
  MQEC_CONNECTION_QUIESCING     ,"MQEC_CONNECTION_QUIESCING"    ,"Connection quiescing",
  0                             ,""                             ,"",
  0                             ,NULL                           ,NULL
};

MQVALUE ValMQPUTOptions[] =
{
         MQPMO_ALTERNATE_USER_AUTHORITY,"MQPMO_ALTERNATE_USER_AUTHORITY","Alternate user authority",
         MQPMO_ASYNC_RESPONSE          ,"MQPMO_ASYNC_RESPONSE"         ,"Async Response",
         MQPMO_DEFAULT_CONTEXT         ,"MQPMO_DEFAULT_CONTEXT"        ,"Default context",
         MQPMO_FAIL_IF_QUIESCING       ,"MQPMO_FAIL_IF_QUIESCING"      ,"Fail if quiescing",
         MQPMO_LOGICAL_ORDER           ,"MQPMO_LOGICAL_ORDER"          ,"Logical order",
         MQPMO_MD_FOR_OUTPUT_ONLY      ,"MQPMO_MD_FOR_OUTPUT_ONLY"     ,"MD for output only",
         MQPMO_NEW_CORREL_ID           ,"MQPMO_NEW_CORREL_ID"          ,"New correl id",
         MQPMO_NEW_MSG_ID              ,"MQPMO_NEW_MSG_ID"             ,"New message id",
         MQPMO_NONE                    ,"MQPMO_NONE"                   ,"None",
         MQPMO_NOT_OWN_SUBS            ,"MQPMO_NOT_OWN_SUBS"           ,"Not own Subs",
         MQPMO_NO_CONTEXT              ,"MQPMO_NO_CONTEXT"             ,"No context",
         MQPMO_NO_SYNCPOINT            ,"MQPMO_NO_SYNCPOINT"           ,"No syncpoint",
         MQPMO_PASS_ALL_CONTEXT        ,"MQPMO_PASS_ALL_CONTEXT"       ,"Pass all context",
         MQPMO_PASS_IDENTITY_CONTEXT   ,"MQPMO_PASS_IDENTITY_CONTEXT"  ,"Pass identity context",
         MQPMO_RESOLVE_LOCAL_Q         ,"MQPMO_RESOLVE_LOCAL_Q"        ,"Resolve Local Q",
         MQPMO_RETAIN                  ,"MQPMO_RETAIN"                 ,"Retain",
         MQPMO_SCOPE_QMGR              ,"MQPMO_SCOPE_QMGR"             ,"Scope Qmgr",
         MQPMO_SET_ALL_CONTEXT         ,"MQPMO_SET_ALL_CONTEXT"        ,"Set all context",
         MQPMO_SET_IDENTITY_CONTEXT    ,"MQPMO_SET_IDENTITY_CONTEXT"   ,"Set identity context",
         MQPMO_SUPPRESS_REPLYTO        ,"MQPMO_SUPPRESS_REPLYTO"       ,"Suppress reply to",
         MQPMO_SYNC_RESPONSE           ,"MQPMO_SYNC_RESPONSE"          ,"Sync Response",
         MQPMO_SYNCPOINT               ,"MQPMO_SYNCPOINT"              ,"Syncpoint",
         MQPMO_WARN_IF_NO_SUBS_MATCHED ,"MQPMO_WARN_IF_NO_SUBS_MATCHED","Warn if no subs matched",
         0                             ,NULL                           ,NULL
};

MQVALUE ValEncoding[]      = {
       { 1                             ,""                             ,"BIG_ENDIAN"                  },
       { 2                             ,""                             ,"LITTLE_ENDIAN"               },
       { 0                             ,""                             ,NULL                          }};

MQVALUE ValErrorDataRC[]   = {
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValPersistence[]   = {
       { MQPER_NOT_PERSISTENT          ,"MQPER_NOT_PERSISTENT"         , "Not Persistent"              },
       { MQPER_PERSISTENT              ,"MQPER_PERSISTENT"             , "Persistent"                  },
       { MQPER_PERSISTENCE_AS_Q_DEF    ,"MQPER_PERSISTENCE_AS_Q_DEF"   , "As Q Definition"             },
       { MQPER_PERSISTENCE_AS_PARENT   ,"MQPER_PERSISTENCE_AS_PARENT"  , "As Parent"                   },
       { 0                             ,NULL                           , NULL                          }};

                                       /* Field arrays                */
MQVALUE ValVerbId[]        = {
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValSyncAction[]    = {
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValGetStatus[]     = {
       { 0                             ,NULL                           , NULL                          }};

/* Compcode fields: */
MQVALUE ValCompCode[]      = {
       { MQCC_OK                       ,"MQCC_OK"                      ,"OK"                           },
       { MQCC_WARNING                  ,"MQCC_WARNING"                 ,"Warning"                      },
       { MQCC_FAILED                   ,"MQCC_FAILED"                  ,"Failed"                       },
       { MQCC_UNKNOWN                  ,"MQCC_UNKNOWN"                 ,"Unknown"                      },
       { 0                             ,NULL                           , NULL                          }};

/* MQBO fields: */
#ifndef MVS
MQVALUE ValMQBOOpts[]      = {
       { MQBO_NONE                     ,"MQBO_NONE"                    ,"No options"                   },
       { 0                             ,NULL                           , NULL                          }};
#endif

/* MQCFH fields */
MQVALUE ValCFHControl[] =    {
       { MQCFC_NOT_LAST                ,"MQCFC_NOT_LAST"               ,"Not last"                     },
       { MQCFC_LAST                    ,"MQCFC_LAST"                   ,"Last"                         },
       { 0                             ,NULL                           , NULL                          }};

/* MQCIH fields: */
MQVALUE ValCIHADSDescr[]   = {
       { MQCADSD_NONE                  ,"MQCADSD_NONE"                 ,"Dont send or recv descriptor" },
       { MQCADSD_SEND                  ,"MQCADSD_SEND"                 ,"Send descriptor"              },
       { MQCADSD_RECV                  ,"MQCADSD_RECV"                 ,"Recv descriptor"              },
       { MQCADSD_MSGFORMAT             ,"MQCADSD_MSGFORMAT"            ,"Use msg format for descriptor"},
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValCIHConvTask[]   = {
       { MQCCT_YES                     ,"MQCCT_YES"                    ,"Conversational"               },
       { MQCCT_NO                      ,"MQCCT_NO"                     ,"Non conversational"           },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValCIHLinkType[]   = {
       { MQCLT_PROGRAM                 ,"MQCLT_PROGRAM"                ,"DPL Program"                  },
       { MQCLT_TRANSACTION             ,"MQCLT_TRANSACTION"            ,"3270 Transaction"             },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValCIHRetCode[]    = {
       { MQCRC_OK                      ,"MQCRC_OK"                     ,"No Error"                     },
       { MQCRC_CICS_EXEC_ERROR         ,"MQCRC_CICS_EXEC_ERROR"        ,"EXEC CICS statement detected an error"},
       { MQCRC_MQ_API_ERROR            ,"MQCRC_MQ_API_ERROR"           ,"MQ call detected an error"    },
       { MQCRC_BRIDGE_ERROR            ,"MQCRC_BRIDGE_ERROR"           ,"CICS Bridge detected an error"},
       { MQCRC_BRIDGE_ABEND            ,"MQCRC_BRIDGE_ABEND"           ,"CICS Bridge ended abnormally" },
       { MQCRC_APPLICATION_ABEND       ,"MQCRC_APPLICATION_ABEND"      ,"Application ended abnormally" },
       { MQCRC_SECURITY_ERROR          ,"MQCRC_SECURITY_ERROR"         ,"Security Error"               },
       { MQCRC_PROGRAM_NOT_AVAILABLE   ,"MQCRC_PROGRAM_NOT_AVAILABLE"  ,"Program not available"        },
       { MQCRC_BRIDGE_TIMEOUT,"MQCRC_BRIDGE_TIMEOUT","Message within this UOW not recvd within specified time"},
       { MQCRC_TRANSID_NOT_AVAILABLE   ,"MQCRC_TRANSID_NOT_AVAILABLE"  ,"Transaction not available"    },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValCIHTaskEnd[]    = {
       { MQCTES_ENDTASK                ,"MQCTES_ENDTASK"               ,"End task"                     },
       { MQCTES_BACKOUT                ,"MQCTES_BACKOUT"               ,"Backout unit of work"         },
       { MQCTES_COMMIT                 ,"MQCTES_COMMIT"                ,"Commit unit of work"          },
       { MQCTES_NOSYNC                 ,"MQCTES_NOSYNC"                ,"Not synchronized"             },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValUOWControl[]    = {
       { MQCUOWC_CONTINUE              ,"MQCUOWC_CONTINUE"             ,"Additional data for current UOW (3270 only)"},
       { MQCUOWC_BACKOUT               ,"MQCUOWC_BACKOUT"              ,"Backout the UOW (DPL only)"   },
       { MQCUOWC_ONLY                  ,"MQCUOWC_ONLY"                 ,"Start UOW, perform fn, commit UOW (DPL only)"},
       { MQCUOWC_LAST                  ,"MQCUOWC_LAST"                 ,"Perform fn, commit UOW (DPL only)"},
       { MQCUOWC_COMMIT                ,"MQCUOWC_COMMIT"               ,"Commit the UOW (DPL only)"    },
       { MQCUOWC_FIRST                 ,"MQCUOWC_FIRST"                ,"Start UOW and perform fn (DPL only)"},
       { MQCUOWC_MIDDLE                ,"MQCUOWC_MIDDLE"               ,"Perform fn in current UOW (DPL only)"},
       { 0                             ,NULL                           , NULL                          }};

/* MQCNO fields: */
MQVALUE ValMQCNOOpts[]     = {
       { MQCNO_NONE                    ,"MQCNO_NONE"                   ,"None"                         },
       { MQCNO_STANDARD_BINDING        ,"MQCNO_STANDARD_BINDING/MQCNO_NONE","Standard (default) binding"},
       { MQCNO_FASTPATH_BINDING        ,"MQCNO_FASTPATH_BINDING"       ,"Fastpath binding"             },
       { MQCNO_SERIALIZE_CONN_TAG_Q_MGR,"MQCNO_SERIALIZE_CONN_TAG_Q_MGR","Serialize conn tag qmgr"     },
       { MQCNO_SERIALIZE_CONN_TAG_QSG  ,"MQCNO_SERIALIZE_CONN_TAG_QSG" ,"Serialize conn tag qsg"       },
       { MQCNO_RESTRICT_CONN_TAG_Q_MGR ,"MQCNO_RESTRICT_CONN_TAG_Q_MGR","Restrict conn tag qmgr"       },
       { MQCNO_RESTRICT_CONN_TAG_QSG   ,"MQCNO_RESTRICT_CONN_TAG_QSG"  ,"Restrict conn tag qsg"        },
       { MQCNO_HANDLE_SHARE_NONE       ,"MQCNO_HANDLE_SHARE_NONE"      ,"NOT shared hcon"              },
       { MQCNO_HANDLE_SHARE_BLOCK      ,"MQCNO_HANDLE_SHARE_BLOCK"     ,"Shared hcon+block"            },
       { MQCNO_HANDLE_SHARE_NO_BLOCK   ,"MQCNO_HANDLE_SHARE_NO_BLOCK"  ,"Shared hcon+non-block"        },
       { MQCNO_SHARED_BINDING          ,"MQCNO_SHARED_BINDING"         ,"Shared Binding"               },
       { MQCNO_ISOLATED_BINDING        ,"MQCNO_ISOLATED_BINDING"       ,"Isolated Bindings"            },
       { MQCNO_ACCOUNTING_MQI_ENABLED  ,"MQCNO_ACCOUNTING_MQI_ENABLED" ,"MQI Accounting Enabled"       },
       { MQCNO_ACCOUNTING_MQI_DISABLED ,"MQCNO_ACCOUNTING_MQI_DISABLED","MQI Accounting Disabled"      },
       { MQCNO_ACCOUNTING_Q_ENABLED    ,"MQCNO_ACCOUNTING_Q_ENABLED"   ,"Q Accounting Enabled"         },
       { MQCNO_ACCOUNTING_Q_DISABLED   ,"MQCNO_ACCOUNTING_Q_DISABLED"  ,"Q Accounting Disables"        },
       { MQCNO_NO_CONV_SHARING         ,"MQCNO_NO_CONV_SHARING"        ,"No conv sharing"              },
       { MQCNO_ALL_CONVS_SHARE         ,"MQCNO_ALL_CONVS_SHARE"        ,"All convs share"              },
       { MQCNO_CD_FOR_OUTPUT_ONLY      ,"MQCNO_CD_FOR_OUTPUT_ONLY"     ,"CD for output only"           },
       { MQCNO_USE_CD_SELECTION        ,"MQCNO_USE_CD_SELECTION"       ,"Use CD selection"             },
       { MQCNO_RECONNECT_AS_DEF        ,"MQCNO_RECONNECT_AS_DEF"       ,"Reconnect as def"             },
       { MQCNO_RECONNECT               ,"MQCNO_RECONNECT"              ,"Reconnect"                    },
       { MQCNO_RECONNECT_DISABLED      ,"MQCNO_RECONNECT_DISABLED"     ,"Reconnect Disabled"           },
       { MQCNO_RECONNECT_Q_MGR         ,"MQCNO_RECONNECT_Q_MGR"        ,"Reconnect Same QMgr"          },
       { MQCNO_LOCAL_BINDING           ,"MQCNO_LOCAL_BINDING"          ,"Local binding",               },
       { MQCNO_CLIENT_BINDING          ,"MQCNO_CLIENT_BINDING"         ,"Client binding",              },
       { MQCNO_ACTIVITY_TRACE_ENABLED  ,"MQCNO_ACTIVITY_TRACE_ENABLED" ,"Activity trace enabled",      },
       { MQCNO_ACTIVITY_TRACE_DISABLED ,"MQCNO_ACTIVITY_TRACE_DISABLED","Activity trace disabled",     },
       { 0                             ,NULL                           , NULL                          }};

/* MQDH fields: */
MQVALUE ValMQDHFlags[]     = {
       { MQDHF_NONE                    ,"MQDHF_NONE"                   ,"None"                         },
       { MQDHF_NEW_MSG_IDS             ,"MQDHF_NEW_MSG_IDS"            ,"Generate new message identifiers"},
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValMQPMRFlds[]     = {
       { MQPMRF_ACCOUNTING_TOKEN       ,"MQPMRF_ACCOUNTING_TOKEN"      ,"Accounting token is present"  },
       { MQPMRF_FEEDBACK               ,"MQPMRF_FEEDBACK"              ,"Feedback field is present"    },
       { MQPMRF_GROUP_ID               ,"MQPMRF_GROUP_ID"              ,"Group id is preesent"         },
       { MQPMRF_CORREL_ID              ,"MQPMRF_CORREL_ID"             ,"Correlation id is present"    },
       { MQPMRF_MSG_ID                 ,"MQPMRF_MSG_ID"                ,"Message-idis present"         },
       { MQPMRF_NONE                   ,"MQPMRF_NONE"                  ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

/* MQDLH fields: */
MQVALUE ValAppType[]       = {
       { MQAT_UNKNOWN                  ,"MQAT_UNKNOWN"                 ,"Unknown"                      },
       { MQAT_NO_CONTEXT               ,"MQAT_NO_CONTEXT"              ,"No Context"                   },
       { MQAT_CICS                     ,"MQAT_CICS"                    ,"CICS"                         },
       { MQAT_OS390                    ,"MQAT_OS390"                   ,"OS/390"                       },
       { MQAT_IMS                      ,"MQAT_IMS"                     ,"IMS"                          },
       { MQAT_OS2                      ,"MQAT_OS2"                     ,"OS2"                          },
       { MQAT_DOS                      ,"MQAT_DOS"                     ,"DOS"                          },
       { MQAT_UNIX                     ,"MQAT_UNIX"                    ,"UNIX"                         },
       { MQAT_QMGR                     ,"MQAT_QMGR"                    ,"Queue Manager"                },
       { MQAT_OS400                    ,"MQAT_OS400"                   ,"AS/400"                       },
       { MQAT_WINDOWS                  ,"MQAT_WINDOWS"                 ,"Windows 3.1"                  },
       { MQAT_CICS_VSE                 ,"MQAT_CICS_VSE"                ,"CICS VSE"                     },
       { MQAT_WINDOWS_NT               ,"MQAT_WINDOWS_NT"              ,"Windows NT"                   },
       { MQAT_VMS                      ,"MQAT_VMS"                     ,"Digital OpenVMS"              },
       { MQAT_GUARDIAN                 ,"MQAT_GUARDIAN"                ,"Tandem"                       },
       { MQAT_VOS                      ,"MQAT_VOS"                     ,"Stratus VOS"                  },
       { MQAT_IMS_BRIDGE               ,"MQAT_IMS_BRIDGE"              ,"IMB Bridge"                   },
       { MQAT_XCF                      ,"MQAT_XCF"                     ,"XCF"                          },
       { MQAT_CICS_BRIDGE              ,"MQAT_CICS_BRIDGE"             ,"CICS Bridge"                  },
       { MQAT_NOTES_AGENT              ,"MQAT_NOTES_AGENT"             ,"Lotus Notes Agent"            },
       { MQAT_USER                     ,"MQAT_USER"                    ,"User"                         },
       { MQAT_BROKER                   ,"MQAT_BROKER"                  ,"Broker"                       },
       { MQAT_JAVA                     ,"MQAT_JAVA"                    ,"Java"                         },
       { MQAT_DQM                      ,"MQAT_DQM"                     ,"Distributed QMgr agent"       },
       { MQAT_CHANNEL_INITIATOR        ,"MQAT_CHANNEL_INITIATOR"       ,"Channel Initiator"            },
       { MQAT_DEFAULT                  ,"MQAT_DEFAULT"                 ,"Default"                      },
       { MQAT_USER_FIRST               ,"MQAT_USER_FIRST"              ,"User first"                   },
       { MQAT_USER_LAST                ,"MQAT_USER_LAST"               ,"User last"                    },
       { 0                             ,NULL                           , NULL                          }};

/* MQGMO fields: */
MQVALUE ValMatchOptions[]  = {
       { MQMO_MATCH_MSG_ID             ,"MQMO_MATCH_MSG_ID"            ,"Match MsgId"                  },
       { MQMO_MATCH_CORREL_ID          ,"MQMO_MATCH_CORREL_ID"         ,"Match CorrelId"               },
       { MQMO_MATCH_GROUP_ID           ,"MQMO_MATCH_GROUP_ID"          ,"Match GroupId"                },
       { MQMO_MATCH_MSG_SEQ_NUMBER     ,"MQMO_MATCH_MSG_SEQ_NUMBER"    ,"Match Msg Sequence No"        },
       { MQMO_MATCH_OFFSET             ,"MQMO_MATCH_OFFSET"            ,"Match Offset"                 },
       { MQMO_MATCH_MSG_TOKEN          ,"MQMO_MATCH_MSG_TOKEN"         ,"Match Msg Token"              },
       { MQMO_NONE                     ,"MQMO_NONE"                    ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValGroupStatus[]   = {
       { MQGS_NOT_IN_GROUP             ,"MQGS_NOT_IN_GROUP"            ,"Not in a group"               },
       { MQGS_MSG_IN_GROUP             ,"MQGS_MSG_IN_GROUP"            ,"In a group (not last)"        },
       { MQGS_LAST_MSG_IN_GROUP        ,"MQGS_LAST_MSG_IN_GROUP"       ,"Last in a group"              },
       { 0                             ,"None"                         ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValSegStatus[]     = {
       { MQSS_NOT_A_SEGMENT            ,"MQSS_NOT_A_SEGMENT"           ,"Not a segment"                },
       { MQSS_SEGMENT                  ,"MQSS_SEGMENT"                 ,"Segment (but not last)"       },
       { MQSS_LAST_SEGMENT             ,"MQSS_LAST_SEGMENT"            ,"Last segment"                 },
       { 0                             ,"None"                         ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValSegmentation[]  = {
       { MQSEG_INHIBITED               ,"MQSEG_INHIBITED"              ,"Segmentation inhibited"       },
       { MQSEG_ALLOWED                 ,"MQSEG_ALLOWED"                ,"Segmentation allowed"         },
       { 0                             ,"None"                         ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};


/* MQIIH fields: */
MQVALUE ValIIHCommitMode[] = {
       { MQICM_COMMIT_THEN_SEND        ,"MQICM_COMMIT_THEN_SEND"       ,"Commit then send"             },
       { MQICM_SEND_THEN_COMMIT        ,"MQICM_SEND_THEN_COMMIT"       ,"Send then commit"             },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValIIHFlags[]      = {
       { MQIIH_NONE                    ,"MQIIH_NONE"                   ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValIIHSecurityScope[]={
       { MQISS_CHECK                  ,"MQISS_CHECK"                  ,"Check security scope"         },
       { MQISS_FULL                   ,"MQISS_FULL"                   ,"Full security scope"          },
       { 0                            , NULL                          , NULL                         }};

MQVALUE ValIIHTransState[] = {
       { MQITS_IN_CONVERSATION         ,"MQITS_IN_CONVERSATION"        ,"In conversation"              },
       { MQITS_NOT_IN_CONVERSATION     ,"MQITS_NOT_IN_CONVERSATION"    ,"Not in conversation"          },
       { MQITS_ARCHITECTED             ,"MQITS_ARCHITECTED"            ,"Returned transaction state in architected form"},
       { 0                             ,NULL                           , NULL                          }};

/* MQMD fields: */
MQVALUE ValMDReport[] =      {
       { MQRO_DISCARD_MSG              ,"MQRO_DISCARD_MSG"             ,"Discard the message"         },
       { MQRO_EXCEPTION_WITH_FULL_DATA ,"MQRO_EXCEPTION_WITH_FULL_DATA",
                                                    "Exception reports with full data required (full app msg)"},
       { MQRO_EXCEPTION_WITH_DATA      ,"MQRO_EXCEPTION_WITH_DATA"     ,
                                                 "Exception reports with data required (100 bytes of app msg)"},
       { MQRO_EXCEPTION                ,"MQRO_EXCEPTION"               ,"Exception reports required"  },
       { MQRO_EXPIRATION_WITH_FULL_DATA,"MQRO_EXPIRATION_WITH_FULL_DATA",
                                                   "Expiration reports with full data required (full app msg)"},
       { MQRO_EXPIRATION_WITH_DATA     ,"MQRO_EXPIRATION_WITH_DATA"    ,
                                                "Expiration reports with data required (100 bytes of app msg)"},
       { MQRO_EXPIRATION               ,"MQRO_EXPIRATION"              ,"Expiration reports required" },
       { MQRO_COD_WITH_FULL_DATA       ,"MQRO_COD_WITH_FULL_DATA"      ,
                                          "Confirm-on-delivery reports with full data required (full app msg)"},
       { MQRO_COD_WITH_DATA            ,"MQRO_COD_WITH_DATA"           ,
                                       "Confirm-on-delivery reports with data required (100 bytes of app msg)"},
       { MQRO_COD                      ,"MQRO_COD"                     ,"Confirm-on-delivery reports required"},
       { MQRO_COA_WITH_FULL_DATA       ,"MQRO_COA_WITH_FULL_DATA"      ,
                                           "Confirm-on-arrival reports with full data required (full app msg)"},
       { MQRO_COA_WITH_DATA            ,"MQRO_COA_WITH_DATA"           ,
                                        "Confirm-on-arrival reports with data required (100 bytes of app msg)"},
       { MQRO_COA                      ,"MQRO_COA"                     ,"Confirm-on-arrival reports required"},
       { MQRO_PASS_MSG_ID              ,"MQRO_PASS_MSG_ID"             ,
                                                          "Put msgid of this message into report or reply msg"},
       { MQRO_PASS_CORREL_ID           ,"MQRO_PASS_CORREL_ID"          ,
                                                         "Put correl of this message into report or reply msg"},
       { MQRO_NAN                      ,"MQRO_NAN"                     ,"Negative action notification reports required"},
       { MQRO_PAN                      ,"MQRO_PAN"                     ,"Positive action notification reports required"},
       { MQRO_NONE                     ,"MQRO_NONE"                    ,"None"                         },
       { MQRO_DEAD_LETTER_Q            ,"MQRO_DEAD_LETTER_Q"           ,"Place message on dead letter q"},
       { MQRO_COPY_MSG_ID_TO_CORREL_ID ,"MQRO_COPY_MSG_ID_TO_CORREL_ID",
                                              "Put msgid of this message into correlid of report or reply msg"},
       { MQRO_NEW_MSG_ID               ,"MQRO_NEW_MSG_ID"              ,"Generate new msgid"           },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValMQFeedback[] =    {
       { MQFB_NONE                     ,"MQFB_NONE"                    ,"None"                         },
       { MQFB_QUIT                     ,"MQFB_QUIT"                    ,"Application should end"       },
       { MQFB_EXPIRATION               ,"MQFB_EXPIRATION"              ,"Message Expired"              },
       { MQFB_COA                      ,"MQFB_COA"                     ,"Confirmation of arrival"      },
       { MQFB_COD                      ,"MQFB_COD"                     ,"Confirmation of delivery"     },
       { MQFB_CHANNEL_COMPLETED        ,"MQFB_CHANNEL_COMPLETED"       ,"Channel completed"            },
       { MQFB_CHANNEL_FAIL_RETRY       ,"MQFB_CHANNEL_FAIL_RETRY"      ,"Channel failed but will retry"},
       { MQFB_CHANNEL_FAIL             ,"MQFB_CHANNEL_FAIL"            ,"Channel failed"               },
       { MQFB_APPL_CANNOT_BE_STARTED   ,"MQFB_APPL_CANNOT_BE_STARTED"  ,"Application cant be started"  },
       { MQFB_TM_ERROR                 ,"MQFB_TM_ERROR"                ,"TM Error"                     },
       { MQFB_APPL_TYPE_ERROR          ,"MQFB_APPL_TYPE_ERROR"         ,"Appl. type error"             },
       { MQFB_STOPPED_BY_MSG_EXIT      ,"MQFB_STOPPED_BY_MSG_EXIT"     ,"Stopped by msg exit"          },
       { MQFB_XMIT_Q_MSG_ERROR         ,"MQFB_XMIT_Q_MSG_ERROR"        ,"XMITQ QMsgError"              },
       { MQFB_PAN                      ,"MQFB_PAN"                     ,"Positive action notification" },
       { MQFB_NAN                      ,"MQFB_NAN"                     ,"Negative action notification" },
       { MQFB_STOPPED_BY_CHAD_EXIT     ,"MQFB_STOPPED_BY_CHAD_EXIT"    ,"Stopped by CHAD exit"         },
       { MQFB_STOPPED_BY_PUBSUB_EXIT   ,"MQFB_STOPPED_BY_PUBSUB_EXIT"  ,"Stopped by PUBSUB exit"       },
       { MQFB_NOT_A_REPOSITORY_MSG     ,"MQFB_NOT_A_REPOSITORY_MSG"    ,"Not a repository msg"         },
       { MQFB_BIND_OPEN_CLUSRCVR_DEL   ,"MQFB_BIND_OPEN_CLUSRCVR_DEL"  ,""                             },
       { MQFB_DATA_LENGTH_ZERO         ,"MQFB_DATA_LENGTH_ZERO"        ,"A segment length was 0"       },
       { MQFB_DATA_LENGTH_NEGATIVE     ,"MQFB_DATA_LENGTH_NEGATIVE"    ,"A segment length was negative"},
       { MQFB_DATA_LENGTH_TOO_BIG      ,"MQFB_DATA_LENGTH_TOO_BIG"     ,"A segment length was too big" },
       { MQFB_BUFFER_OVERFLOW          ,"MQFB_BUFFER_OVERFLOW"         ,"Buffer overflow"              },
       { MQFB_LENGTH_OFF_BY_ONE        ,"MQFB_LENGTH_OFF_BY_ONE"       ,"Length of field one byte short"},
       { MQFB_IIH_ERROR                ,"MQFB_IIH_ERROR"               ,"MQIIH structure not valid/missing"},
       { MQFB_NOT_AUTHORIZED_FOR_IMS   ,"MQFB_NOT_AUTHORIZED_FOR_IMS"  ,"Userid not auth for use in IMS"},
       { MQFB_IMS_ERROR                ,"MQFB_IMS_ERROR"               ,"Unexpected error from IMS"    },
       { MQFB_CICS_INTERNAL_ERROR      ,"MQFB_CICS_INTERNAL_ERROR"     ,"Unexpected error from CICS Bridge" },
       { MQFB_CICS_NOT_AUTHORIZED      ,"MQFB_CICS_NOT_AUTHORIZED"     ,"Userid not auth/pwd invalid"  },
       { MQFB_CICS_BRIDGE_FAILURE      ,"MQFB_CICS_BRIDGE_FAILURE"     ,"CICS Bridge term. abnormally" },
       { MQFB_CICS_CORREL_ID_ERROR     ,"MQFB_CICS_CORREL_ID_ERROR"    ,"Correlid invalid"             },
       { MQFB_CICS_CCSID_ERROR         ,"MQFB_CICS_CCSID_ERROR"        ,"CICS CCSID invalid"           },
       { MQFB_CICS_ENCODING_ERROR      ,"MQFB_CICS_ENCODING_ERROR"     ,"Encoding not valid"           },
       { MQFB_CICS_CIH_ERROR           ,"MQFB_CICS_CIH_ERROR"          ,"MQCIH structure invalid/missing"},
       { MQFB_CICS_UOW_ERROR           ,"MQFB_CICS_UOW_ERROR"          ,"UOWControl field invalid"     },
       { MQFB_CICS_COMMAREA_ERROR      ,"MQFB_CICS_COMMAREA_ERROR"     ,"Length of commarea invalid"   },
       { MQFB_CICS_APPL_NOT_STARTED    ,"MQFB_CICS_APPL_NOT_STARTED"   ,"Appl. cant be started"        },
       { MQFB_CICS_APPL_ABENDED        ,"MQFB_CICS_APPL_ABENDED"       ,"Appl. abended"                },
       { MQFB_CICS_DLQ_ERROR           ,"MQFB_CICS_DLQ_ERROR"          ,"DLQ not available"            },
       { MQFB_CICS_UOW_BACKED_OUT      ,"MQFB_CICS_UOW_BACKED_OUT"     ,"UOW has been backed out"      },
       { MQFB_ACTIVITY                 ,"MQFB_ACTIVITY"                ,"Activity",                    },
       { MQFB_MAX_ACTIVITIES           ,"MQFB_MAX_ACTIVITIES"          ,"Max activities",              },
       { MQFB_NOT_FORWARDED            ,"MQFB_NOT_FORWARDED"           ,"Not forwarded",               },
       { MQFB_NOT_DELIVERED            ,"MQFB_NOT_DELIVERED"           ,"Not delivered",               },
       { MQFB_UNSUPPORTED_FORWARDING   ,"MQFB_UNSUPPORTED_FORWARDING"  ,"Unsupported forwarding",      },
       { MQFB_UNSUPPORTED_DELIVERY     ,"MQFB_UNSUPPORTED_DELIVERY"    ,"Unsupported delivery",        },
       { MQFB_PUBLICATIONS_ON_REQUEST  ,"MQFB_PUBLICATIONS_ON_REQUEST" ,"Publications on request",     },
       { MQFB_SUBSCRIBER_IS_PUBLISHER  ,"MQFB_SUBSCRIBER_IS_PUBLISHER" ,"Subscriber is publisher",     },
       { MQFB_MSG_SCOPE_MISMATCH       ,"MQFB_MSG_SCOPE_MISMATCH"      ,"Msg scope mismatch",          },
       { MQFB_SELECTOR_MISMATCH        ,"MQFB_SELECTOR_MISMATCH"       ,"Selector mismatch",           },
       { MQFB_NOT_A_GROUPUR_MSG        ,"MQFB_NOT_A_GROUPUR_MSG"       ,"Not a GROUPUR msg",           },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValMDMsgFlags[] =    {
       { MQMF_LAST_MSG_IN_GROUP        ,"MQMF_LAST_MSG_IN_GROUP"       ,"Last message in group"        },
       { MQMF_MSG_IN_GROUP             ,"MQMF_MSG_IN_GROUP"            ,"Message in a group"           },
       { MQMF_LAST_SEGMENT             ,"MQMF_LAST_SEGMENT"            ,"Last segment"                 },
       { MQMF_SEGMENT                  ,"MQMF_SEGMENT"                 ,"Segment"                      },
       { MQMF_SEGMENTATION_ALLOWED     ,"MQMF_SEGMENTATION_ALLOWED"    ,"Segmentation allowed"         },
       { MQMF_NONE                     ,"MQMF_NONE"                    ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValMDMsgType[] =     {
       { MQMT_REQUEST                  ,"MQMT_REQUEST"                 ,"Request msg"                  },
       { MQMT_REPLY                    ,"MQMT_REPLY"                   ,"Reply msg"                    },
       { MQMT_DATAGRAM                 ,"MQMT_DATAGRAM"                ,"Datagram"                     },
       { MQMT_REPORT                   ,"MQMT_REPORT"                  ,"Report"                       },
       { MQMT_MQE_FIELDS_FROM_MQE      ,"MQMT_MQE_FIELDS_FROM_MQE"     ,"MQE Fields from MQE"          },
       { MQMT_MQE_FIELDS               ,"MQMT_MQE_FIELDS"              ,"MQE Fields"                   },
       { 0                             ,"MQMT_NONE"                    ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

/* MQMD fields: */
MQVALUE ValMDEFlags[] =      {
       { MQMDEF_NONE                   ,"MQMDEF_NONE"                  ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};

/* MQOD fields: */
MQVALUE ValObjectType[]    = {
       { MQOT_Q                        ,"MQOT_Q"                       ,"Queue"                        },
       { MQOT_NAMELIST                 ,"MQOT_NAMELIST"                ,"Namelist"                     },
       { MQOT_PROCESS                  ,"MQOT_PROCESS"                 ,"Process"                      },
       { MQOT_STORAGE_CLASS            ,"MQOT_STORAGE_CLASS"           ,"Storage Class"                },
       { MQOT_Q_MGR                    ,"MQOT_Q_MGR"                   ,"QueueManager"                 },
       { MQOT_CHANNEL                  ,"MQOT_CHANNEL"                 ,"Channel"                      },
       { MQOT_AUTH_INFO                ,"MQOT_AUTH_INFO"               ,"Auth Info"                    },
       { MQOT_TOPIC                    ,"MQOT_TOPIC"                   ,"Topic"                        },
       { MQOT_CF_STRUC                 ,"MQOT_CF_STRUC"                ,"CF Struc"                     },
       { MQOT_RESERVED_1               ,"MQOT_RESERVED_1"              ,"Reserved"                     },
       { MQOT_ALL                      ,"MQOT_ALL"                     ,"All"                          },
       { MQOT_ALIAS_Q                  ,"MQOT_ALIAS_Q"                 ,"Alias Queue"                  },
       { MQOT_MODEL_Q                  ,"MQOT_MODEL_Q"                 ,"Model Queue"                  },
       { MQOT_LOCAL_Q                  ,"MQOT_LOCAL_Q"                 ,"Local Queue"                  },
       { MQOT_REMOTE_Q                 ,"MQOT_REMOTE_Q"                ,"Remote Queue"                 },
       { MQOT_SENDER_CHANNEL           ,"MQOT_SENDER_CHANNEL"          ,"Sender Channel"               },
       { MQOT_SERVER_CHANNEL           ,"MQOT_SERVER_CHANNEL"          ,"Server Channel"               },
       { MQOT_REQUESTER_CHANNEL        ,"MQOT_REQUESTER_CHANNEL"       ,"Requester Channel"            },
       { MQOT_RECEIVER_CHANNEL         ,"MQOT_RECEIVER_CHANNEL"        ,"Receiver Channel"             },
       { MQOT_CURRENT_CHANNEL          ,"MQOT_CURRENT_CHANNEL"         ,"Current Channel"              },
       { MQOT_SAVED_CHANNEL            ,"MQOT_SAVED_CHANNEL"           ,"Saved Channel"                },
       { MQOT_SVRCONN_CHANNEL          ,"MQOT_SVRCONN_CHANNEL"         ,"Svrconn Channel"              },
       { MQOT_CLNTCONN_CHANNEL         ,"MQOT_CLNTCONN_CHANNEL"        ,"Clntconn Channel"             },
       { MQOT_COMM_INFO                ,"MQOT_COMM_INFO"               ,"Comm info",                   },
       { MQOT_LISTENER                 ,"MQOT_LISTENER"                ,"Listener",                    },
       { MQOT_SERVICE                  ,"MQOT_SERVICE"                 ,"Service",                     },
       { MQOT_SHORT_CHANNEL            ,"MQOT_SHORT_CHANNEL"           ,"Short channel",               },
       { MQOT_CHLAUTH                  ,"MQOT_CHLAUTH"                 ,"Chlauth",                     },
       { MQOT_REMOTE_Q_MGR_NAME        ,"MQOT_REMOTE_Q_MGR_NAME"       ,"Remote Q Mgr name",           },
       { MQOT_PROT_POLICY              ,"MQOT_PROT_POLICY"             ,"Prot policy",                 },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValRMHFlags[]      = {
       { MQRMHF_LAST                   ,"MQRMHF_LAST"                  ,"Last part of object"          },
       { MQRMHF_NOT_LAST               ,"MQRMHF_NOT_LAST"              ,"Not last part of object"      },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValWIHFlags[]      = {
       { MQWIH_NONE                    ,"MQWIH_NONE"                   ,"None"                         },
       { 0                             ,NULL                           , NULL                          }};


MQVALUE ValCFHType[]       = {
       { MQCFT_COMMAND                 ,"MQCFT_COMMAND"                ,"Command"                      },
       { MQCFT_RESPONSE                ,"MQCFT_RESPONSE"               ,"Response"                     },
       { MQCFT_INTEGER                 ,"MQCFT_INTEGER"                ,"Integer"                      },
       { MQCFT_STRING                  ,"MQCFT_STRING"                 ,"String"                       },
       { MQCFT_INTEGER_LIST            ,"MQCFT_INTEGER_LIST"           ,"Integer List"                 },
       { MQCFT_STRING_LIST             ,"MQCFT_STRING_LIST"            ,"String List"                  },
       { MQCFT_EVENT                   ,"MQCFT_EVENT"                  ,"Event"                        },
       { MQCFT_USER                    ,"MQCFT_USER"                   ,"User"                         },
       { MQCFT_BYTE_STRING             ,"MQCFT_BYTE_STRING"            ,"Byte String"                  },
       { MQCFT_TRACE_ROUTE             ,"MQCFT_TRACE_ROUTE"            ,"Trace route"                  },
       { MQCFT_REPORT                  ,"MQCFT_REPORT"                 ,"Report"                       },
       { MQCFT_INTEGER_FILTER          ,"MQCFT_INTEGER_FILTER"         ,"Integer Filter"               },
       { MQCFT_STRING_FILTER           ,"MQCFT_STRING_FILTER"          ,"String Filter"                },
       { MQCFT_BYTE_STRING_FILTER      ,"MQCFT_BYTE_STRING_FILTER"     ,"Byte String Filter"           },
       { MQCFT_COMMAND_XR              ,"MQCFT_COMMAND_XR"             ,"XR Command"                   },
       { MQCFT_XR_MSG                  ,"MQCFT_XR_MSG"                 ,"XR Message"                   },
       { MQCFT_XR_ITEM                 ,"MQCFT_XR_ITEM"                ,"XR Item"                      },
       { MQCFT_XR_SUMMARY              ,"MQCFT_XR_SUMMARY"             ,"XR Summary"                   },
       { MQCFT_GROUP                   ,"MQCFT_GROUP"                  ,"Group"                        },
       { MQCFT_STATISTICS              ,"MQCFT_STATISTICS"             ,"Statistics"                   },
       { MQCFT_ACCOUNTING              ,"MQCFT_ACCOUNTING"             ,"Accounting"                   },
       { MQCFT_INTEGER64               ,"MQCFT_INTEGER64"              ,"64 bit Integer"               },
       { MQCFT_INTEGER64_LIST          ,"MQCFT_INTEGER64_LIST"         ,"64 bit Integer List"          },
       { MQCFT_APP_ACTIVITY            ,"MQCFT_APP_ACTIVITY"           ,"App activity",                },
       { 0                             ,NULL                           , NULL                          }};

MQVALUE ValMQCompress[]    = {
       /* Cast to byte to Map -1 to 255 */
       { MQCOMPRESS_NOT_AVAILABLE      ,"MQCOMPRESS_NOT_AVAILABLE"     ,"Not available"         },
       { MQCOMPRESS_NONE               ,"MQCOMPRESS_NONE"              ,"None"                         },
       { MQCOMPRESS_RLE                ,"MQCOMPRESS_RLE"               ,"RLE"                          },
       { MQCOMPRESS_ZLIBFAST           ,"MQCOMPRESS_ZLIBFAST"          ,"ZLIB Fast Compression"        },
       { MQCOMPRESS_ZLIBHIGH           ,"MQCOMPRESS_ZLIBHIGH"          ,"ZLIB High Compression"        },
       { MQCOMPRESS_SYSTEM             ,"MQCOMPRESS_SYSTEM"            ,"System"                       },
       { MQCOMPRESS_ANY                ,"MQCOMPRESS_ANY"               ,"Any"                          },
       { 0                             ,NULL                           , NULL                          }};

/* @@@ Insert your value fields below */

                                       /* Field arrays                */
static MQFIELD SSLPeerNameLength = { "SSLPeerLength:", MQFT_LONG   , 4, NULL };
static MQFIELD Exit              = { "Exit         :", MQFT_CHAR   ,128,NULL };
static MQFIELD ExitData          = { "Exit Data    :", MQFT_CHAR   , 32,NULL };
static MQFIELD Cluster           = { "Cluster      :", MQFT_CHAR   , 48,NULL };
static MQFIELD SSLPeerName       = { "SSLPeerName  :", MQFT_CHARN  , 0 ,&SSLPeerNameLength };
static MQFIELDS ExitFields[] = { {0,&Exit             ,NULL           , 1},
                                 {0,NULL              ,NULL           , 0}};
static MQSTRUCT DefExit      = { "!Exit               ",0,ExitFields, NULL };

static MQFIELDS DataFields[] = { {0,&ExitData         ,NULL           , 1},
                                 {0,NULL              ,NULL           , 0}};
static MQSTRUCT DefData      = { "!Exit Data          ",0,DataFields, NULL };

static MQFIELDS ClusterFields[] = { {0,&Cluster          ,NULL           , 1},
                                    {0,NULL              ,NULL           , 0}};
static MQSTRUCT DefCluster   = { "!Cluster            ",0,ClusterFields, NULL };

static MQFIELDS SSLPeerFields[] = { {0,&SSLPeerName      ,NULL           , 1},
                                    {0,NULL              ,NULL           , 0}};
static MQSTRUCT DefSSLPeer   = { "!SSL Peer           ",0,SSLPeerFields, NULL };

                                       /* Common Fields               */
static MQFIELD Align4            = { "!Align       :", MQFT_ALIGN  , 4, NULL };
static MQFIELD Align8            = { "!Align       :", MQFT_ALIGN  , 8, NULL };
static MQFIELD Eyecatcher        = { "StrucId      :", MQFT_CHAR   , 4, NULL };
static MQFIELD Version           = { "Version      :", MQFT_LONG   , 4, NULL };
static MQFIELD MQMDVersion       = { "MQMD Version :", MQFT_LONG   , 4, NULL };
static MQFIELD CCSID4            = { "CCSID        :", MQFT_LONG   , 4, NULL };
static MQFIELD Reserved1         = { "Reserved     :", MQFT_HEX    , 1, NULL };
static MQFIELD Reserved2         = { "Reserved     :", MQFT_HEX    , 2, NULL };
static MQFIELD Reserved4         = { "Reserved     :", MQFT_HEX    , 4, NULL };
       MQFIELD FieldMessage      = { "Message Data :", MQFT_MSG    ,-1, NULL };
static MQFIELD DumpStr           = { "String Dump  :", MQFT_DUMPSTR,-1, NULL };
static MQFIELD QName             = { "Queue Name   :", MQFT_CHAR   ,48, NULL };
static MQFIELD ProcessName       = { "Process Name :", MQFT_CHAR   ,48, NULL };
static MQFIELD TriggerData       = { "Trigger Data :", MQFT_CHAR   ,64, NULL };
static MQFIELD ApplType          = { "Appl Type    :", MQFT_LONG   , 4, ValAppType };
static MQFIELD ApplId            = { "Appl Id      :", MQFT_CHAR   ,256, NULL };
static MQFIELD EnvData           = { "Env. Data    :", MQFT_CHAR   ,128, NULL };
static MQFIELD UserData          = { "User Data    :", MQFT_CHAR   ,128, NULL };
static MQFIELD MsgSequenceNumber = { "Sequence No. :", MQFT_LONG   ,  4, NULL };

                                       /* MQBO Fields                 */
#ifndef MVS
static MQFIELD BOOptions         = { "Options      :", MQFT_FLAG4  , 4, ValMQBOOpts };
#endif

                                       /* MQCIH fields                */
static MQFIELD AbendCode         = { "Abend Code   :", MQFT_CHAR   , 4, NULL};
static MQFIELD ADSDescriptor     = { "ADS Descrptor:", MQFT_FLAG4  , 4, ValCIHADSDescr};
static MQFIELD AttentionId       = { "Attn Id      :", MQFT_CHAR   , 4, NULL};
static MQFIELD Authenticator     = { "Authenticator:", MQFT_CHAR   , 8, NULL};
static MQFIELD CancelCode        = { "Cancel Code  :", MQFT_CHAR   , 4, NULL};
static MQFIELD CIHCompCode       = { "CIH CompCode :", MQFT_LONG   , 4, NULL /*ValCIHCompCode*/};
static MQFIELD ConversationalTask= { "Conver. Task?:", MQFT_LONG   , 4, ValCIHConvTask};
static MQFIELD CursorPosition    = { "Cursor Posn  :", MQFT_LONG   , 4, NULL };
static MQFIELD ErrorOffset       = { "Error Offset :", MQFT_LONG   , 4, NULL };
static MQFIELD Facility          = { "Facility     :", MQFT_CHAR   , 8, NULL};
static MQFIELD FacilityKeepTime  = { "FacKeepTime  :", MQFT_LONG   , 4, NULL };
static MQFIELD FacilityLike      = { "FacilityLike :", MQFT_CHAR   , 4, NULL};
static MQFIELD CIHFlags          = { "Flags        :", MQFT_LONG   , 4, NULL };
static MQFIELD Function          = { "Function     :", MQFT_CHAR   , 4, NULL /*ValCIHFunction*/};
static MQFIELD GetWaitInterval   = { "GetWaitIntval:", MQFT_LONG   , 4, NULL };
static MQFIELD InputItem         = { "Input Item   :", MQFT_LONG   , 4, NULL };
static MQFIELD LinkType          = { "Link Type    :", MQFT_LONG   , 4, ValCIHLinkType};
static MQFIELD NextTransactionId = { "Next TransId :", MQFT_CHAR   , 4, NULL };
static MQFIELD OutputDataLength  = { "OutputDataLen:", MQFT_LONG   , 4, NULL };
static MQFIELD CIHReason         = { "CIH Reason   :", MQFT_LONG   , 4, NULL /*ValCIHReason*/};
static MQFIELD CIHReturnCode     = { "CIH RetCode  :", MQFT_LONG   , 4, ValCIHRetCode};
static MQFIELD RemoteSysId       = { "Remote SysId :", MQFT_CHAR   , 4, NULL };
static MQFIELD RemoteTransId     = { "Remote TranId:", MQFT_CHAR   , 4, NULL };
static MQFIELD ReplyToFormat     = { "Reply To Fmt :", MQFT_CHAR   , 8, NULL };
static MQFIELD Reserved8         = { "Reserved     :", MQFT_CHAR   , 8, NULL };
static MQFIELD StartCode         = { "StartCode    :", MQFT_CHAR   , 4, NULL /*ValCIHStartCode*/};
static MQFIELD TaskEndStatus     = { "TaskEndStatus:", MQFT_LONG   , 4, ValCIHTaskEnd};
static MQFIELD TransactionId     = { "TransId      :", MQFT_CHAR   , 4, NULL };
static MQFIELD UOWControl        = { "UOWControl   :", MQFT_LONG   , 4, ValUOWControl};

                                       /* MQIIH fields                */
static MQFIELD MFSMapName        = { "MFSMapName   :", MQFT_CHAR   , 8, NULL };
static MQFIELD IIHFlags          = { "IIHFlags     :", MQFT_FLAG1  , 1, ValIIHFlags};
static MQFIELD LTermOverride     = { "LTermOverride:", MQFT_CHAR   , 8, NULL };
static MQFIELD SecurityScope     = { "SecurityScope:", MQFT_CHAR1F , 1, ValIIHSecurityScope };
static MQFIELD TranInstanceId    = { "TranInst.Id  :", MQFT_HEX    ,16, NULL };
static MQFIELD TranState         = { "TranState    :", MQFT_CHAR1F , 1, ValIIHTransState };
static MQFIELD CommitMode        = { "CommitMode   :", MQFT_CHAR1F , 1, ValIIHCommitMode };

                                       /* MQRMH fields                */
static MQFIELD RMHFlags          = { "Flags        :", MQFT_LONG   , 4, ValRMHFlags };
static MQFIELD RMHObjectType     = { "Object Type  :", MQFT_CHAR   , 8, NULL };
static MQFIELD ObjectInstanceId  = { "Obj.Inst.Id  :", MQFT_HEX    ,24, NULL };
static MQFIELD SrcEnvLength      = { "SrcEnvLength :", MQFT_LONG   , 4, NULL };
static MQFIELD SrcEnvOffset      = { "SrcEnvOffset :", MQFT_LONG   , 4, NULL };
static MQFIELD SrcNameLength     = { "SrcNameLength:", MQFT_LONG   , 4, NULL };
static MQFIELD SrcNameOffset     = { "SrcNameOffset:", MQFT_LONG   , 4, NULL };
static MQFIELD DestEnvLength     = { "DestEnvLength:", MQFT_LONG   , 4, NULL };
static MQFIELD DestEnvOffset     = { "DestEnvOffset:", MQFT_LONG   , 4, NULL };
static MQFIELD DestNameLength    = { "DestNameLen  :", MQFT_LONG   , 4, NULL };
static MQFIELD DestNameOffset    = { "DestNameOff. :", MQFT_LONG   , 4, NULL };
static MQFIELD DataLogicalLength = { "DataLog.Len  :", MQFT_LONG   , 4, NULL };
static MQFIELD DataLogicalOffset = { "DataLog.Off  :", MQFT_LONG   , 4, NULL };
static MQFIELD DataLogicalOffset2= { "DataLog.Off2 :", MQFT_LONG   , 4, NULL };

                                       /* MQWIH fields                */
static MQFIELD WIHFlags          = { "Flags        :", MQFT_LONG   , 4, ValWIHFlags };
static MQFIELD ServiceName       = { "ServiceName  :", MQFT_CHAR   ,32, NULL };
static MQFIELD ServiceStep       = { "ServiceStep  :", MQFT_CHAR   , 8, NULL };
static MQFIELD Reserved32        = { "Reserved     :", MQFT_HEX    ,32, NULL };


static MQFIELD Encoding          = { "Encoding     :", MQFT_BYTE   , 1, ValEncoding };
static MQFIELD MQEncoding        = { "MQEncoding   :", MQFT_HEX4   , 4, NULL };
static MQFIELD BatchSize         = { "BatchSize    :", MQFT_SHORT  , 2, NULL };
static MQFIELD QueueManager      = { "QueueManager :", MQFT_CHAR   ,48, NULL };

                                       /* XQH Definitions             */
static MQFIELD RemoteQName       = { "Remote Q     :", MQFT_CHAR   ,48, NULL };
static MQFIELD RemoteQMgrName    = { "Remote QMgr  :", MQFT_CHAR   ,48, NULL };


static MQFIELD Userid            = { "Userid       :", MQFT_CHAR   ,12, NULL };
static MQFIELD Password          = { "Password     :", MQFT_CHAR   ,12, NULL };
static MQFIELD CompCode          = { "CompCode     :", MQFT_LONG   , 4, ValCompCode };

                                       /* Initialised Dynamically     */
static MQFIELD Reason            = { "Reason       :", MQFT_LONG   , 4, ValMQRC };
static MQFIELD Handle            = { "Handle       :", MQFT_LONG   , 4, NULL };

                                       /* MQOD Fields                 */
static MQFIELD ObjectType        = { "Object Type  :", MQFT_LONG   , 4, ValObjectType };
static MQFIELD ObjectName        = { "Object Name  :", MQFT_CHAR   ,48, NULL };
static MQFIELD ObjectQMgrName    = { "Object QMgr  :", MQFT_CHAR   ,48, NULL };
static MQFIELD DynamicQName      = { "DynamicQName :", MQFT_CHAR   ,48, NULL };
static MQFIELD AlternateUserId   = { "Alt. UserId  :", MQFT_CHAR   ,12, NULL };
static MQFIELD RecsPresent       = { "Recs Present :", MQFT_LONG   , 4, NULL };
static MQFIELD KnownDestCount    = { "Known Dest   :", MQFT_LONG   , 4, NULL };
static MQFIELD UnknownDestCount  = { "Unknown Dest :", MQFT_LONG   , 4, NULL };
static MQFIELD InvalidDestCount  = { "Invalid Dest :", MQFT_LONG   , 4, NULL };
static MQFIELD AlternateSecurity = { "Alt. Security:", MQFT_HEX    ,40, NULL };

                                       /* MQSD Fields                 */
static MQFIELD ResObjectString   = { "Rslv Obj Str :", MQFT_CHARV  ,20, NULL };
static MQFIELD ResolvedType      = { "Resolved Type:", MQFT_LONG   , 4, NULL };
static MQFIELD SubOptions        = { "SD Options   :", MQFT_FLAG4  , 4, ValMQSUBOptions };
static MQFIELD SubExpiry         = { "Sub Expiry   :", MQFT_LONG   , 4, NULL };
static MQFIELD ObjectString      = { "Object String:", MQFT_CHARV  , 0, NULL };
static MQFIELD SubName           = { "Sub Name     :", MQFT_CHARV  , 0, NULL };
static MQFIELD SubUserData       = { "Sub User Data:", MQFT_CHARV  , 0, NULL };
static MQFIELD SubCorrelId       = { "Sub CorrelId :", MQFT_HEX    ,24, NULL };
static MQFIELD PubPriority       = { "Pub Priority :", MQFT_LONG   , 4, NULL };
static MQFIELD PubAccountingToken= { "Pub Acct Tkn :", MQFT_HEX    ,32, NULL };
static MQFIELD PubApplIdentityData={ "Pub Appl Iden:", MQFT_CHAR   ,32, NULL };
static MQFIELD SelectionString   = { "Selection Str:", MQFT_CHARV  , 0, NULL };
static MQFIELD SubLevel          = { "Sub Level    :", MQFT_LONG   , 4, NULL };

                                       /* MQSRO Fields                */
static MQFIELD SubROptions       = { "SRO Options  :", MQFT_FLAG4  , 4, NULL };
static MQFIELD NumPubs           = { "Num Pubs     :", MQFT_LONG   , 4, NULL };

                                       /* MQGMO Fields                */
static MQFIELD GetOptions        = { "GMO Options  :", MQFT_FLAG4  , 4, ValMQGETOptions };
static MQFIELD WaitInterval      = { "Wait Interval:", MQFT_LONG   , 4, NULL };
static MQFIELD Signal1           = { "Signal 1     :", MQFT_LONG   , 4, ValMQGETSignals };
static MQFIELD Signal2           = { "Signal 2     :", MQFT_LONG   , 4, NULL };
static MQFIELD ResolvedQName     = { "Resolved Q   :", MQFT_CHAR   ,48, NULL };
static MQFIELD MatchOptions      = { "Match Options:", MQFT_FLAG4  , 4, ValMatchOptions };
static MQFIELD GroupStatus       = { "Group Status :", MQFT_CHAR1F , 1, ValGroupStatus };
static MQFIELD SegmentStatus     = { "Segment Stat.:", MQFT_CHAR1F , 1, ValSegStatus };
static MQFIELD Segmentation      = { "Segmentation :", MQFT_CHAR1F , 1, ValSegmentation };
static MQFIELD MsgToken          = { "Message Token:", MQFT_HEX    ,16, NULL };
static MQFIELD ReturnedLength    = { "Returned Len :", MQFT_LONG   , 4, NULL };
static MQFIELD MsgHandle         = { "MsgHandle    :", MQFT_LONGH64, 8, NULL };
                                       /* MQPMO Fields                */
static MQFIELD PutOptions        = { "PMO Options  :", MQFT_FLAG4  , 4, &ValMQPUTOptions };
static MQFIELD Timeout           = { "Timeout (res):", MQFT_LONG   , 4, NULL };
static MQFIELD Context           = { "Context      :", MQFT_LONG   , 4, NULL };
static MQFIELD ResolvedQMgrName  = { "Resolved Qmgr:", MQFT_CHAR   ,48, NULL };
static MQFIELD PutMsgRecFields   = { "PutMsgRecFlds:", MQFT_FLAG4  , 4, &ValMQPMRFlds };
static MQFIELD OriginalMsgHandle = { "OrigMsgHandle:", MQFT_LONGH64, 8, NULL };
static MQFIELD NewMsgHandle      = { "NewMsgHandle :", MQFT_LONGH64, 8, NULL };
static MQFIELD Action            = { "Action       :", MQFT_LONG   , 4, NULL };
static MQFIELD PubLevel          = { "Pub Level    :", MQFT_LONG   , 4, NULL };

                                       /* MQMD Fields                 */
static MQFIELD Report            = { "Report       :", MQFT_FLAG4  , 4, &ValMDReport };
static MQFIELD MsgType           = { "Message Type :", MQFT_LONG   , 4, &ValMDMsgType };
static MQFIELD Expiry            = { "Expiry       :", MQFT_LONG   , 4, NULL };
static MQFIELD Feedback          = { "Feedback     :", MQFT_LONG   , 4, &ValMQFeedback };
static MQFIELD MQFormat          = { "Format       :", MQFT_FORMAT , 8, NULL, FO_DEPENDED };
static MQFIELD Priority          = { "Priority     :", MQFT_LONG   , 4, NULL };
static MQFIELD Persistence       = { "Persistence  :", MQFT_LONG   , 4, &ValPersistence };
static MQFIELD MsgId             = { "Message Id   :", MQFT_HEXCHAR,24, NULL };
static MQFIELD CorrelId          = { "Correl. Id   :", MQFT_HEXCHAR,24, NULL };
static MQFIELD BackoutCount      = { "Backout Cnt. :", MQFT_LONG   , 4, NULL };
static MQFIELD ReplyToQ          = { "ReplyToQ     :", MQFT_CHAR   ,48, NULL };
static MQFIELD ReplyToQMgr       = { "ReplyToQMgr  :", MQFT_CHAR   ,48, NULL };
static MQFIELD UserIdentifier    = { "UserId       :", MQFT_CHAR   ,12, NULL };
static MQFIELD AccountingToken   = { "AccountingTkn:", MQFT_HEX    ,32, NULL };
static MQFIELD ApplIdentityData  = { "ApplIdentity :", MQFT_CHAR   ,32, NULL };
static MQFIELD PutApplType       = { "PutApplType  :", MQFT_LONG   , 4, &ValAppType };
static MQFIELD PutApplName       = { "PutApplName  :", MQFT_CHAR   ,28, NULL };
static MQFIELD PutDate           = { "Put Date     :", MQFT_DATE   , 8, NULL };
static MQFIELD PutTime           = { "Put Time     :", MQFT_TIME   , 8, NULL };
static MQFIELD ApplOriginData    = { "ApplOriginDat:", MQFT_CHAR   , 4, NULL };
static MQFIELD GroupId           = { "Group Id     :", MQFT_HEX    ,24, NULL };
static MQFIELD MsgSeqNumber      = { "Msg Seq No.  :", MQFT_LONG   , 4, NULL };
static MQFIELD Offset            = { "Offset       :", MQFT_LONG   , 4, NULL };
static MQFIELD MsgFlags          = { "MsgFlags     :", MQFT_FLAG4  , 4, &ValMDMsgFlags };
static MQFIELD OriginalLength    = { "Original Len.:", MQFT_LONG   , 4, NULL };
                                       /* MQMDE Fields                */
static MQFIELD StrucLength       = { "Struc Length :", MQFT_LONG   , 4, NULL };
static MQFIELD MDEFlags          = { "Flags        :", MQFT_FLAG4  , 4, &ValMDEFlags };

                                       /* DLH Definitions             */
static MQFIELD DestQName         = { "Dest. Queue  :", MQFT_CHAR   ,48, NULL };
static MQFIELD DestQMgrName      = { "Dest. QMgr   :", MQFT_CHAR   ,48, NULL };

                                       /* DH Definitions              */
static MQFIELD DHFlags           = { "Flags        :", MQFT_FLAG4  , 4, &ValMQDHFlags };

                                       /* CFH Definitions             */
static MQFIELD Type              = { "Type         :", MQFT_LONG   , 4, &ValCFHType };
static MQFIELD Command           = { "Command      :", MQFT_LONG   , 4, &ValMQCMD };
static MQFIELD Control           = { "Control      :", MQFT_LONG   , 4, &ValCFHControl };
static MQFIELD ParmCount         = { "Parm Count   :", MQFT_LONG   , 4, NULL };
       MQFIELD ParameterId       = { "Parameter Id :", MQFT_LONG   , 4, ValMQPARM };
static MQFIELD IntParm           = { "Value        :", MQFT_LONGH  , 4, NULL };
static MQFIELD IntParms          = { "Values       :", MQFT_ARRAY  , 4, &ParmCount };
static MQFIELD StringLength      = { "String Length:", MQFT_LONG   , 4, NULL };
static MQFIELD StrParm           = { "Value        :", MQFT_CHARN  , 0, &StringLength };
static MQFIELD StrParms          = { "Values       :", MQFT_ARRAY  , 4, &ParmCount };

static MQFIELD IntParm64         = { "Value        :", MQFT_LONGH64, 8, NULL };

static MQFIELD HexLength         = { "Bytes Length :", MQFT_LONG   , 4, NULL };
static MQFIELD HexParm           = { "Value        :", MQFT_HEXN   , 0, &HexLength };
static MQFIELD IntFilter         = { "Filter Value :", MQFT_LONGH  , 4, NULL };
static MQFIELD Operator          = { "Operator     :", MQFT_LONG   , 4, NULL };
static MQFIELD FilterLength      = { "Filter Length:", MQFT_LONG   , 4, NULL };
static MQFIELD CharFilter        = { "String Filter:", MQFT_CHARN  , 0, &FilterLength };


/* MQI parameters that may be reused elsewhere in these structure decoders */
static MQFIELD QMgrName          = { "QMgr Name    :", MQFT_CHAR   ,48, NULL };
static MQFIELD ApplName          = { "Appl. Name   :", MQFT_CHAR   ,28, NULL };
static MQFIELD MQCONNOptions     = { "Client Opts  :", MQFT_FLAG4  , 4, NULL };
static MQFIELD ConnectionId      = { "Connection Id:", MQFT_HEX    ,128, NULL };
static MQFIELD CloseOptions      = { "Close Options:", MQFT_FLAG4  , 4, ValMQCLOSEOptions };

       MQFIELD OpenOptions       = { "Open Options :", MQFT_FLAG4  , 4, &ValMQOPENOptions };
static MQFIELD PropertyControl   = { "Property Ctl :", MQFT_LONG   , 4, NULL };

static MQFIELD MsgLength         = { "Msg Length   :", MQFT_LONG   , 4, NULL };

static MQFIELD Selector          = { "Selector     :", MQFT_LONG   , 4, ValMQPARM };

                                       /* MQCD Definitions            */
static MQFIELD Desc              = { "Description  :", MQFT_CHAR   ,64, NULL };
static MQFIELD MsgExit           = { "Message  Ex. :", MQFT_CHAR   ,128,NULL };
static MQFIELD SendExit          = { "Send     Ex. :", MQFT_CHAR   ,128,NULL };
static MQFIELD ReceiveExit       = { "Receive  Ex. :", MQFT_CHAR   ,128,NULL };
static MQFIELD MsgUserData       = { "Msg      Data:", MQFT_CHAR   ,32, NULL };
static MQFIELD SendUserData      = { "Send     Data:", MQFT_CHAR   ,32, NULL };
static MQFIELD ReceiveUserData   = { "Receive  Data:", MQFT_CHAR   ,32, NULL };
static MQFIELD MsgExitsDefined   = { "Msg  Exits   :", MQFT_LONG   , 4,NULL };
static MQFIELD SendExitsDefined  = { "Send Exits   :", MQFT_LONG   , 4,NULL };
static MQFIELD ReceiveExitsDefined = { "Rcv  Exits   :", MQFT_LONG   , 4,NULL };

static MQFIELD RFHFlags              = { "RFH Flags    :", MQFT_FLAG4  , 4, NULL };
static MQFIELD NameValueCCSID        = { "NameVal CCSID:", MQFT_LONG   , 4, NULL };
static MQFIELD FieldXml              = { "Xml          :", MQFT_XML    ,-1, NULL };
       MQFIELD FieldXmlMsg           = { "Xml Message  :", MQFT_XMLMSG ,-1, NULL };
static MQFIELD EPHFlags              = { "EPH Flags    :", MQFT_FLAG4  , 4, NULL };
static MQFIELD RFHString             = { "RFH String   :", MQFT_CHARN  , 0 ,NULL };



static MQFIELD Length            = { "Length       :", MQFT_LONG   , 4, NULL };
static MQFIELD GetStatus         = { "Status       :", MQFT_FLAG4  , 4, ValGetStatus  };
static MQFIELD Inherited         = { "Inherited    :", MQFT_LONG   , 4, NULL          };
static MQFIELD SegmentIndex      = { "Segment Index:", MQFT_SHORT  , 2, NULL          };
static MQFIELD RequestFlags      = { "Req Flags    :", MQFT_FLAG4  , 4, ValRequestFlags};
static MQFIELD QueueStatus       = { "Queue Status :", MQFT_FLAG4  , 4, ValQueueStatus};
static MQFIELD NotificationCode  = { "Notify Code  :", MQFT_LONG   , 4, ValNotifyCode };
static MQFIELD NotificationValue = { "Notify Value :", MQFT_LONG   , 4, NULL          };

static MQFIELD ResolvedQNameLen  = { "ResolvedQNLen:", MQFT_BYTE   , 1, NULL };
static MQFIELD PSPropertyStyle   = { "Prop Style   :", MQFT_LONG   , 4, NULL          };
static MQFIELD SubScope          = { "Sub Scope    :", MQFT_LONG   , 4, NULL          };
static MQFIELD SubType           = { "Sub Type     :", MQFT_LONG   , 4, NULL          };

/**********************************************************************/
/* Dependency definitions                                             */
/**********************************************************************/
MQDEPEND DepVersion2[]    = {{ &Version        , 2                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion3[]    = {{ &Version        , 3                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion4[]    = {{ &Version        , 4                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion5[]    = {{ &Version        , 5                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion6[]    = {{ &Version        , 6                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion7[]    = {{ &Version        , 7                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion8[]    = {{ &Version        , 8                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion9[]    = {{ &Version        , 9                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion10[]    = {{ &Version        ,10                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion11[]    = {{ &Version        ,11                  , 1000                 },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMsgId[]       = {{ &PutMsgRecFields, MQPMRF_MSG_ID      , MQPMRF_MSG_ID        },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCorrelId[]    = {{ &PutMsgRecFields, MQPMRF_CORREL_ID   , MQPMRF_CORREL_ID     },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepAccount[]     = {{ &PutMsgRecFields, MQPMRF_ACCOUNTING_TOKEN, MQPMRF_ACCOUNTING_TOKEN },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepGroupId[]     = {{ &PutMsgRecFields, MQPMRF_GROUP_ID    , MQPMRF_GROUP_ID },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepFeedback[]    = {{ &PutMsgRecFields, MQPMRF_FEEDBACK    , MQPMRF_FEEDBACK },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQDLH[]       = {{ &MQFormat         , 0x4d514445, 0x41442020, MQFMT_DEAD_LETTER_HEADER  }, /* MQDEAD__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQRFH[]       = {{ &MQFormat         , 0x4d514852, 0x46202020, MQFMT_RF_HEADER}, /* MQHRF */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQRFH2[]      = {{ &MQFormat         , 0x4d514852, 0x46322020, MQFMT_RF_HEADER_2}, /* MQHRF2 */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQXQH[]       = {{ &MQFormat         , 0x4d51584d, 0x49542020, MQFMT_XMIT_Q_HEADER  }, /* MQXMIT__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQMDE[]       = {{ &MQFormat         , 0x4d51484d, 0x44452020, MQFMT_MD_EXTENSION  }, /* MQHMDE__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQDH[]        = {{ &MQFormat         , 0x4d514844, 0x49535420, MQFMT_DIST_HEADER  }, /* MQHDIST_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQPCF[]       = {{ &MQFormat         , 0x4d515043, 0x46202020, MQFMT_PCF  }, /* MQPCF___ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQAdmin[]     = {{ &MQFormat         , 0x4d514144, 0x4d494e20, MQFMT_ADMIN  }, /* MQADMIN_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQCmd1[]      = {{ &MQFormat         , 0x4d51434d, 0x44312020, MQFMT_COMMAND_1  }, /* MQCMD1__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQCmd2[]      = {{ &MQFormat         , 0x4d51434d, 0x44322020, MQFMT_COMMAND_2  }, /* MQCMD2__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQSTR[]       = {{ &MQFormat         , 0x4d515354, 0x52202020, MQFMT_STRING  }, /* MQSTR___ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQCICS[]      = {{ &MQFormat         , 0x6D716369, 0x63732020, MQFMT_CICS  }, /* MQCIH__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQEvent[]     = {{ &MQFormat         , 0x4d514556, 0x454e5420, MQFMT_EVENT  }, /* MQEVENT_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQTM[]        = {{ &MQFormat         , 0x4d515452, 0x49472020, MQFMT_TRIGGER  }, /* MQTRIG__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQDLHEBC[]    = {{ &MQFormat         , (long)0xD4D8C4C5, 0xc1c44040  },                   /* MQDEAD__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepVersion2RFH[] = {{ &Version        , 2                  , 1000                 },     /* Chained RFH's*/
                             { &Eyecatcher     , 0x52464820         , 0x52464820,          },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQRFHEBC[]    = {{ &MQFormat         , 0xd4d8c4c6, 0xc8404040, }, /* MQHRF */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQRFH2EBC[]   = {{ &MQFormat         , 0xd4d8c4c6, 0xc8f24040, }, /* MQHRF2 */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQXQHEBC[]       = {{ &MQFormat         , (long)0xD4D8E7D4, 0xc9e34040  },                   /* MQXMIT__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQMDEEBC[]       = {{ &MQFormat         , (long)0xD4D8C8D4, 0xc4c54040  },                   /* MQHMDE__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQDHEBC[]        = {{ &MQFormat         , (long)0xD4D8C8C4, 0xc9e2e340  },                   /* MQHDIST_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQPCFEBC[]       = {{ &MQFormat         , (long)0xD4D8D7C3, 0xc6404040  },                   /* MQPCF___ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQAdminEBC[]     = {{ &MQFormat         , (long)0xD4D8C1C4, 0xd4c9d540  },                   /* MQADMIN_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQCmd1EBC[]      = {{ &MQFormat         , (long)0xD4D8C3D4, 0xc4f14040  },                   /* MQCMD1__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQCmd2EBC[]      = {{ &MQFormat         , (long)0xD4D8C3D4, 0xc4f24040  },                   /* MQCMD2__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQSTREBC[]       = {{ &MQFormat         , (long)0xD4D8E2E3, 0xd9404040  },                   /* MQSTR___ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQEventEBC[]     = {{ &MQFormat         , (long)0xD4D8C5E5, 0xc5d5e340  },                   /* MQEVENT_ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQTMEBC[]        = {{ &MQFormat         , (long)0xD4D8E3D9, 0xc9c74040  },                   /* MQTRIG__ */
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepMQEPH[]       = {{ &MQFormat         , 0,0,MQFMT_EMBEDDED_PCF},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepAcc[]         = {{ &Type           , MQCFT_ACCOUNTING   , MQCFT_ACCOUNTING},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepStats[]       = {{ &Type           , MQCFT_STATISTICS   , MQCFT_STATISTICS},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFGR[]        = {{ &Type           , MQCFT_GROUP        , MQCFT_GROUP          },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFIF[]        = {{ &Type           , MQCFT_INTEGER_FILTER,MQCFT_INTEGER_FILTER },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFIL64[]      = {{ &Type           , MQCFT_INTEGER64_LIST,MQCFT_INTEGER64_LIST },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFIN64[]      = {{ &Type           , MQCFT_INTEGER64     ,MQCFT_INTEGER64      },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFSF[]        = {{ &Type           , MQCFT_STRING_FILTER ,MQCFT_STRING_FILTER  },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFH[]         = {{ &Type           , MQCFT_COMMAND      , MQCFT_RESPONSE},
                             { NULL            , 0                  , 0                    }};
MQDEPEND DepCFHXR[]       = {{ &Type           , MQCFT_COMMAND_XR   , MQCFT_XR_SUMMARY},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepTRTE[]        = {{ &Type           , MQCFT_TRACE_ROUTE  , MQCFT_REPORT},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepEvent[]       = {{ &Type           , MQCFT_EVENT        , MQCFT_EVENT},
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFIL[]        = {{ &Type           , MQCFT_INTEGER_LIST , MQCFT_INTEGER_LIST },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFIN[]        = {{ &Type           , MQCFT_INTEGER      , MQCFT_INTEGER },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFSL[]        = {{ &Type           , MQCFT_STRING_LIST  , MQCFT_STRING_LIST },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFST[]        = {{ &Type           , MQCFT_STRING       , MQCFT_STRING },
                             { NULL            , 0                  , 0                    }};

MQDEPEND DepCFBS[]        = {{ &Type           , MQCFT_BYTE_STRING  , MQCFT_BYTE_STRING },
                             { NULL            , 0                  , 0                    }};


/**********************************************************************/
/* Structure definitions                                              */
/**********************************************************************/
#ifndef MVS
MQFIELDS MQBOFields[]={   {0,&Eyecatcher       ,NULL           , 1},
                          {0,&Version          ,NULL           , 2 + SF_VERSION},
                          {0,&BOOptions        ,NULL           , 2},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQBO     = { "Begin Options (MQBO)", 0, MQBOFields, NULL };
MQFIELD BO            = { "!MQBO"            , MQFT_STRUCT, 0, &DefMQBO };
#endif

MQFIELDS MQCIHFields[]={      {0,&Eyecatcher         ,NULL           , 1},
                              {0,&Version            ,NULL           , 2 + SF_VERSION},
                              {0,&StrucLength        ,NULL           , 3 + SF_STRUCLENGTH},
                              {0,&MQEncoding         ,NULL           , 2 + SF_ENCODING},
                              {0,&CCSID4             ,NULL           , 2 + SF_CCSID},
                              {0,&MQFormat           ,NULL           , 1},
                              {0,&CIHFlags           ,NULL           , 3},
                              {0,&CIHReturnCode      ,NULL           , 1},
                              {0,&CIHCompCode        ,NULL           , 1},
                              {0,&CIHReason          ,NULL           , 1},
                              {0,&UOWControl         ,NULL           , 2},
                              {0,&GetWaitInterval    ,NULL           , 2},
                              {0,&LinkType           ,NULL           , 2},
                              {0,&OutputDataLength   ,NULL           , 2},
                              {0,&FacilityKeepTime   ,NULL           , 2},
                              {0,&ADSDescriptor      ,NULL           , 2},
                              {0,&ConversationalTask ,NULL           , 2},
                              {0,&TaskEndStatus      ,NULL           , 2},
                              {0,&Facility           ,NULL           , 2},
                              {0,&Function           ,NULL           , 2},
                              {0,&AbendCode          ,NULL           , 2},
                              {0,&Authenticator      ,NULL           , 2},
                              {0,&Reserved8          ,NULL           , 2},
                              {0,&ReplyToFormat      ,NULL           , 2},
                              {0,&RemoteSysId        ,NULL           , 2},
                              {0,&RemoteTransId      ,NULL           , 2},
                              {0,&TransactionId      ,NULL           , 2},
                              {0,&FacilityLike       ,NULL           , 2},
                              {0,&AttentionId        ,NULL           , 2},
                              {0,&StartCode          ,NULL           , 2},
                              {0,&CancelCode         ,NULL           , 2},
                              {0,&NextTransactionId  ,NULL           , 2},
                              {0,&Reserved8          ,NULL           , 3},
                              {0,&Reserved8          ,NULL           , 3},
                              {0,&CursorPosition     ,DepVersion2    , 2},
                              {0,&ErrorOffset        ,DepVersion2    , 2},
                              {0,&InputItem          ,DepVersion2    , 2},
                              {0,&Reserved4          ,DepVersion2    , 3},
                              {0,NULL                ,NULL           , 0}};
MQSTRUCT DefMQCIH    = { "CICS Information Header (MQCIH)", 0, MQCIHFields, NULL };
MQFIELD CIH           = { "!MQCIH"           , MQFT_STRUCT, 0, &DefMQCIH };


MQFIELDS MQIIHFields[]={      {0,&Eyecatcher         ,NULL           , 3},
                              {0,&Version            ,NULL             , 2 + SF_VERSION},
                              {0,&StrucLength        ,NULL           , 2},
                              {0,&MQEncoding         ,NULL           , 3},
                              {0,&CCSID4             ,NULL           , 3},
                              {0,&MQFormat           ,NULL           , 1},
                              {0,&IIHFlags           ,NULL           , 3},
                              {0,&LTermOverride      ,NULL           , 1},
                              {0,&MFSMapName         ,NULL           , 1},
                              {0,&ReplyToFormat      ,NULL           , 1},
                              {0,&Authenticator      ,NULL           , 1},
                              {0,&TranInstanceId     ,NULL           , 1},
                              {0,&TranState          ,NULL           , 1},
                              {0,&CommitMode         ,NULL           , 1},
                              {0,&SecurityScope      ,NULL           , 1},
                              {0,&Reserved1          ,NULL           , 3},
                              {0,NULL                ,NULL           , 0}};
MQSTRUCT DefMQIIH    = { "IMS Information Header (MQIIH)", 0, MQIIHFields, NULL };
MQFIELD IIH           = { "!MQIIH"           , MQFT_STRUCT, 0, &DefMQIIH };

MQFIELDS MQRMHFields[]={      {0,&Eyecatcher         ,NULL           , 3},
                              {0,&Version            ,NULL             , 2 + SF_VERSION},
                              {0,&StrucLength        ,NULL           , 2},
                              {0,&MQEncoding         ,NULL           , 1},
                              {0,&CCSID4             ,NULL           , 2},
                              {0,&MQFormat           ,NULL           , 1},
                              {0,&RMHFlags           ,NULL           , 1},
                              {0,&RMHObjectType      ,NULL           , 1},
                              {0,&ObjectInstanceId   ,NULL           , 1},
                              {0,&SrcEnvLength       ,NULL           , 1},
                              {0,&SrcEnvOffset       ,NULL           , 1},
                              {0,&SrcNameLength      ,NULL           , 1},
                              {0,&SrcNameOffset      ,NULL           , 1},
                              {0,&DestEnvLength      ,NULL           , 1},
                              {0,&DestEnvOffset      ,NULL           , 1},
                              {0,&DestNameLength     ,NULL           , 1},
                              {0,&DestNameOffset     ,NULL           , 1},
                              {0,&DataLogicalLength  ,NULL           , 1},
                              {0,&DataLogicalOffset  ,NULL           , 1},
                              {0,&DataLogicalOffset2 ,NULL           , 1},
                              {0,NULL                ,NULL           , 0}};
MQSTRUCT DefMQRMH    = { "Reference Message Header (MQRMH)", 0, MQRMHFields, NULL };
MQFIELD RMH           = { "!MQRMH"           , MQFT_STRUCT, 0, &DefMQRMH };

MQFIELDS MQWIHFields[]={      {0,&Eyecatcher         ,NULL           , 3},
                              {0,&Version            ,NULL             , 2 + SF_VERSION},
                              {0,&StrucLength        ,NULL           , 2},
                              {0,&MQEncoding         ,NULL           , 1},
                              {0,&CCSID4             ,NULL           , 1},
                              {0,&MQFormat           ,NULL           , 1},
                              {0,&WIHFlags           ,NULL           , 1},
                              {0,&ServiceName        ,NULL           , 1},
                              {0,&ServiceStep        ,NULL           , 1},
                              {0,&MsgToken           ,NULL           , 1},
                              {0,&Reserved32         ,NULL           , 3},
                              {0,NULL                ,NULL           , 0}};
MQSTRUCT DefMQWIH    = { "Work Information Header (MQWIH)", 0, MQWIHFields, NULL };
MQFIELD WIH           = { "!MQWIH"           , MQFT_STRUCT, 0, &DefMQWIH };

MQFIELDS MQORFields[] = { {0,&ObjectName       ,NULL           , 1},
                          {0,&ObjectQMgrName   ,NULL           , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQOR      = { "Object Record (MQOR)",0,MQORFields, NULL };
MQFIELD OR            = { "!MQOR"              , MQFT_STRUCT, 0, &DefMQOR };

MQFIELDS MQRRFields[] = { {0,&CompCode         ,NULL           , 1},
                          {0,&Reason           ,NULL           , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQRR      = { "Response Record (MQRR)",0,MQRRFields, NULL };
MQFIELD RR            = { "!RR"                , MQFT_STRUCT, 0, &DefMQRR };

MQFIELDS MQPMRFields[]= { {0,&MsgId            ,DepMsgId       , 1},
                          {0,&CorrelId         ,DepCorrelId    , 1},
                          {0,&GroupId          ,DepGroupId     , 1},
                          {0,&Feedback         ,DepFeedback    , 1},
                          {0,&AccountingToken  ,DepAccount     , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQPMR     = { "Put Message Record (MQPMR)",0,MQPMRFields, NULL };
MQFIELD PMR           = { "!PMR"              , MQFT_STRUCT, 0, &DefMQPMR };

MQFIELD ObjectRecOffset   = { "ObjectRec Off:", MQFT_OSTR , 4, &DefMQOR };
MQFIELD DistArray         = { ""              , MQFT_COUNT, 0, &RecsPresent };
MQFIELD ObjectRecPtr      = { "ObjectRec Ptr:", MQFT_PSTR , 4, &DefMQOR };
MQFIELD ResponseRecOffset = { "Resp. Rec Off:", MQFT_OSTR , 4, &DefMQRR };
MQFIELD ResponseRecPtr    = { "Resp. Rec Ptr:", MQFT_PSTR , 4, &DefMQRR };
MQFIELD PutMsgRecOffset   = { "PutMsgRecOff :", MQFT_OSTR , 4, &DefMQPMR};
MQFIELD PutMsgRecPtr      = { "PutMsgRecPtr :", MQFT_PSTR , 4, &DefMQPMR};
MQFIELD DistLArray        = { ""              , MQFT_COUNT, 0, &RecsPresent };

MQFIELDS MQPMOFields[]= { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL             , 2 + SF_VERSION},
                          {0,&PutOptions       ,NULL           , 1},
                          {0,&Timeout          ,NULL           , 3},
                          {0,&Context          ,NULL           , 2},
                          {0,&KnownDestCount   ,NULL           , 3},
                          {0,&UnknownDestCount ,NULL           , 3},
                          {0,&InvalidDestCount ,NULL           , 3},
                          {0,&ResolvedQName    ,NULL           , 1},
                          {0,&ResolvedQMgrName ,NULL           , 1},
                          {0,&RecsPresent      ,DepVersion2    , 1},
                          {0,&PutMsgRecFields  ,DepVersion2    , 3},
                          {0,&PutMsgRecOffset  ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&ResponseRecOffset,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&PutMsgRecPtr     ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&ResponseRecPtr   ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&OriginalMsgHandle,DepVersion3    , 3},
                          {0,&NewMsgHandle     ,DepVersion3    , 3},
                          {0,&Action           ,DepVersion3    , 3},
                          {0,&PubLevel         ,DepVersion3    , 3},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQPMO     = { "Put Options (MQPMO)",0,MQPMOFields    , NULL };
MQFIELD PMO           = { "!MQPMO"             , MQFT_STRUCT, 0, &DefMQPMO };

MQFIELDS MQGMOFields[]= { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL             , 2 + SF_VERSION},
                          {0,&GetOptions       ,NULL           , 1},
                          {0,&WaitInterval     ,NULL           , 1},
                          {0,&Signal1          ,NULL           , 3},
                          {0,&Signal2          ,NULL           , 3},
                          {0,&ResolvedQName    ,NULL           , 1},
                          {0,&MatchOptions     ,DepVersion2    , 1},
                          {0,&GroupStatus      ,DepVersion2    , 2},
                          {0,&SegmentStatus    ,DepVersion2    , 2},
                          {0,&Segmentation     ,DepVersion2    , 2},
                          {0,&Reserved1        ,DepVersion2    , 3},
                          {0,&MsgToken         ,DepVersion3    , 1},
                          {0,&ReturnedLength   ,DepVersion3    , 2},
                          {0,&Reserved4        ,DepVersion4    , 3},
                          {0,&MsgHandle        ,DepVersion4    , 3},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQGMO     = { "Get Options (MQGMO)",0,MQGMOFields, NULL };
MQFIELD GMO           = { "!MQGMO"             , MQFT_STRUCT, 0, &DefMQGMO };

MQFIELDS MQODFields[] = { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL             , 2 + SF_VERSION},
                          {0,&ObjectType       ,NULL           , 1},
                          {0,&ObjectName       ,NULL           , 1},
                          {0,&ObjectQMgrName   ,NULL           , 1},
                          {0,&DynamicQName     ,NULL           , 2},
                          {0,&AlternateUserId  ,NULL           , 2},
                          {0,&RecsPresent      ,DepVersion2    , 3},
                          {0,&KnownDestCount   ,DepVersion2    , 3},
                          {0,&UnknownDestCount ,DepVersion2    , 3},
                          {0,&InvalidDestCount ,DepVersion2    , 3},
                          {0,&ObjectRecOffset  ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&ResponseRecOffset,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&ObjectRecPtr     ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&ResponseRecPtr   ,DepVersion2    , 3},
                          {0,&DistLArray       ,DepVersion2    , 3},
                          {0,&AlternateSecurity,DepVersion3    , 3},
                          {0,&ResolvedQName    ,DepVersion3    , 3},
                          {0,&ResolvedQMgrName ,DepVersion3    , 3},
                          {0,&ObjectString     ,DepVersion4    , 1},
                          {0,&SelectionString  ,DepVersion4    , 1},
                          {0,&ResObjectString  ,DepVersion4    , 1},
                          {0,&ResolvedType     ,DepVersion4    , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQOD      = { "Object Descriptor (MQOD)",0,MQODFields, NULL };
MQFIELD OD            = { "!MQOD"              , MQFT_STRUCT, 0, &DefMQOD };

MQFIELDS MQSDFields[] = { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL           , 3 + SF_VERSION},
                          {0,&SubOptions       ,NULL           , 1},
                          {0,&ObjectName       ,NULL           , 1},
                          {0,&AlternateUserId  ,NULL           , 2},
                          {0,&AlternateSecurity,NULL           , 3},
                          {0,&SubExpiry        ,NULL           , 3},
                          {0,&ObjectString     ,NULL           , 1},
                          {0,&SubName          ,NULL           , 1},
                          {0,&SubUserData      ,NULL           , 2},
                          {0,&SubCorrelId      ,NULL           , 2},
                          {0,&PubPriority      ,NULL           , 3},
                          {0,&PubAccountingToken,NULL          , 3},
                          {0,&PubApplIdentityData,NULL         , 3},
                          {0,&SelectionString  ,NULL           , 1},
                          {0,&SubLevel         ,NULL           , 1},
                          {0,&ResObjectString  ,NULL           , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQSD      = { "Subscription Descriptor (MQSD)",0,MQSDFields, NULL };
MQFIELD SD            = { "!MQSD"              , MQFT_STRUCT, 0, &DefMQSD };

MQFIELDS SubPropsFields[]={{0,&SubCorrelId      ,NULL           , 1},
                           {0,&DestQName        ,NULL           , 1},
                           {0,&DestQMgrName     ,NULL           , 1},
                           {0,&PSPropertyStyle  ,NULL           , 1},
                           {0,&SubScope         ,NULL           , 1},
                           {0,&SubType          ,NULL           , 1},
                           {0,&Align8           ,NULL           , 1},
                           {0,NULL              ,NULL           , 0}};
MQSTRUCT DefSubProps   = { "Subscription Properties (SDSubProps)",0,SubPropsFields, NULL };
MQFIELD SubProps       = { "!SubProps"          , MQFT_STRUCT, 0, &DefSubProps };


MQFIELDS MQSROFields[]= { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL           , 3 + SF_VERSION},
                          {0,&SubROptions      ,NULL           , 1},
                          {0,&NumPubs          ,NULL           , 1},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQSRO     = { "Sub Request Options (MQSRO)",0,MQSROFields, NULL };
MQFIELD SRO           = { "!MQSRO"             , MQFT_STRUCT, 0, &DefMQSRO };

MQFIELDS MQMDFields[] = { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL             , 2 + SF_VERSION},
                          {0,&Report           ,NULL           , 1},
                          {0,&MsgType          ,NULL           , 1},
                          {0,&Expiry           ,NULL           , 2},
                          {0,&Feedback         ,NULL           , 2},
                          {0,&MQEncoding       ,NULL           , 2 + SF_ENCODING},
                          {0,&CCSID4           ,NULL           , 2 + SF_CCSID},
                          {0,&MQFormat         ,NULL           , 1},
                          {0,&Priority         ,NULL           , 1},
                          {0,&Persistence      ,NULL           , 1},
                          {0,&MsgId            ,NULL           , 1},
                          {0,&CorrelId         ,NULL           , 2},
                          {0,&BackoutCount     ,NULL           , 3},
                          {0,&ReplyToQ         ,NULL           , 1},
                          {0,&ReplyToQMgr      ,NULL           , 1},
                          {0,&UserIdentifier   ,NULL           , 2},
                          {0,&AccountingToken  ,NULL           , 3},
                          {0,&ApplIdentityData ,NULL           , 3},
                          {0,&PutApplType      ,NULL           , 3},
                          {0,&PutApplName      ,NULL           , 3},
                          {0,&PutDate          ,NULL           , 3},
                          {0,&PutTime          ,NULL           , 3},
                          {0,&ApplOriginData   ,NULL           , 3},
                          {0,&GroupId          ,DepVersion2    , 2},
                          {0,&MsgSeqNumber     ,DepVersion2    , 2},
                          {0,&Offset           ,DepVersion2    , 2},
                          {0,&MsgFlags         ,DepVersion2    , 2},
                          {0,&OriginalLength   ,DepVersion2    , 3},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQMD      = { "Message Descriptor (MQMD)",0,MQMDFields, NULL };
MQFIELD MD            = { "!MQMD"              , MQFT_STRUCT, 0, &DefMQMD };

MQFIELDS MQMDEFields[]= { {0,&Eyecatcher       ,NULL           , 3},
                          {0,&Version          ,NULL             , 2 + SF_VERSION},
                          {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                          {0,&MQEncoding       ,NULL           , 2 + SF_ENCODING},
                          {0,&CCSID4           ,NULL           , 2 + SF_CCSID},
                          {0,&MQFormat         ,NULL           , 1},
                          {0,&MDEFlags         ,NULL           , 1},
                          {0,&GroupId          ,NULL           , 1},
                          {0,&MsgSeqNumber     ,NULL           , 1},
                          {0,&Offset           ,NULL           , 1},
                          {0,&MsgFlags         ,NULL           , 1},
                          {0,&OriginalLength   ,NULL           , 3},
                          {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQMDE     = { "Message Descriptor Extension (MQMDE)",0,MQMDEFields, NULL };
MQFIELD MDE           = { "!MQMDE"            , MQFT_STRUCT, 0, &DefMQMDE };

MQFIELDS XQHFields[]  = {{0,&Eyecatcher       ,NULL           , 3},
                         {0,&Version          ,NULL             , 2 + SF_VERSION},
                         {0,&RemoteQName      ,NULL           , 1},
                         {0,&RemoteQMgrName   ,NULL           , 1},
                         {0,&MD               ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefXQH       = { "Transmission Header (XQH)",0, XQHFields , NULL };
MQFIELD  XQH          = { "!MQXQH"           , MQFT_STRUCT, 0, &DefXQH };

MQFIELDS MQDLHFields[]={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&Reason           ,NULL           , 1},
                        {0,&DestQName        ,NULL           , 1},
                        {0,&DestQMgrName     ,NULL           , 1},
                        {0,&MQEncoding       ,NULL           , 2 + SF_ENCODING},
                        {0,&CCSID4           ,NULL           , 2 + SF_CCSID},
                        {0,&MQFormat         ,NULL           , 1},
                        {0,&PutApplType      ,NULL           , 2},
                        {0,&PutApplName      ,NULL           , 2},
                        {0,&PutDate          ,NULL           , 3},
                        {0,&PutTime          ,NULL           , 3},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQDLH     = { "Dead Letter Queue Header (MQDLH)",0,MQDLHFields, NULL };
MQFIELD DLH           = { "!MQDLH"           , MQFT_STRUCT, 0, &DefMQDLH };

MQFIELDS MQDHFields[] ={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&MQEncoding       ,NULL           , 2 + SF_ENCODING},
                        {0,&CCSID4           ,NULL           , 2 + SF_CCSID},
                        {0,&MQFormat         ,NULL           , 1},
                        {0,&DHFlags          ,NULL           , 1},
                        {0,&PutMsgRecFields  ,NULL           , 3},
                        {0,&RecsPresent      ,NULL           , 3},
                        {0,&ObjectRecOffset  ,NULL           , 3},
                        {0,&DistLArray       ,NULL           , 3},
                        {0,&PutMsgRecOffset  ,NULL           , 3},
                        {0,&DistLArray       ,NULL           , 3},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQDH      = { "Distribution List Header (MQDH)",0,MQDHFields, NULL};
MQFIELD DH            = { "!MQDH"            , MQFT_STRUCT, 0, &DefMQDH };

MQFIELDS RFHFields[]  ={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&MQEncoding       ,NULL           , 3 + SF_ENCODING},
                        {0,&CCSID4           ,NULL           , 3 + SF_CCSID},
                        {0,&MQFormat         ,NULL           , 3},
                        {0,&RFHFlags         ,NULL           , 3},
                        {0,&RFHString        ,NULL           , 1},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQRFH     = { "Rules and Formatting (MQRFH)",3,RFHFields, NULL};
MQFIELD RFH           = { "!RFH"            , MQFT_STRUCT, 0, &DefMQRFH };

MQFIELD RFH2;
MQFIELDS RFH2Fields[] ={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&MQEncoding       ,NULL           , 3 + SF_ENCODING},
                        {0,&CCSID4           ,NULL           , 3 + SF_CCSID},
                        {0,&MQFormat         ,NULL           , 3},
                        {0,&RFHFlags         ,NULL           , 3},
                        {0,&NameValueCCSID   ,DepVersion2    , 3},
                        {0,&FieldXml         ,DepVersion2    , 3},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQRFH2    = { "Rules and Formatting (MQRFH2)",3,RFH2Fields, NULL};
MQFIELD RFH2          = { "!RFH"            , MQFT_STRUCT, 0, &DefMQRFH2 };

MQFIELDS MQCFHFields[]={{0,&Type             ,DepCFH         , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFH     = { "PCF Header (MQCFH)",0,MQCFHFields, NULL};
MQFIELD CFH           = { "!MQCFH"            , MQFT_STRUCT, 0, &DefMQCFH};

MQFIELDS MQCFHXRFields[]={{0,&Type           ,DepCFHXR       , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL           , 2 + SF_VERSION},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFHXR     = { "PCF Header (MQCFH XR)",0,MQCFHXRFields, NULL};
MQFIELD CFHXR           = { "!MQCFH"            , MQFT_STRUCT, 0, &DefMQCFHXR};

MQFIELDS MQTRTEFields[]={{0,&Type             ,DepTRTE       , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL           , 3},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQTRTE    = { "Trace Route (MQCFH)",0,MQTRTEFields, NULL};
MQFIELD TRTE          = { "!MQTRTE"           , MQFT_STRUCT, 0, &DefMQTRTE};

MQFIELDS MQEPHFields[]={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL           , 3},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&MQEncoding       ,NULL           , 3 + SF_ENCODING},
                        {0,&CCSID4           ,NULL           , 3 + SF_CCSID},
                        {0,&MQFormat         ,NULL           , 3},
                        {0,&EPHFlags         ,NULL           , 3},
                        {0,&TRTE             ,NULL           , 3},
                        {0,&CFH              ,NULL           , 3},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQEPH     = { "Embedded PCF Header (MQEPH)",3,MQEPHFields, NULL};
MQFIELD EPH           = { "!EPH"            , MQFT_STRUCT, 0, &DefMQEPH };

MQFIELDS EventFields[]={{0,&Type             ,DepEvent       , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefEvent     = { "Event Header (MQCFH)",0,EventFields, NULL};
MQFIELD Event         = { "!MQCFH"            , MQFT_STRUCT, 0, &DefEvent};

MQFIELDS AccFields[]  ={{0,&Type             ,DepAcc         , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL           , 3},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefAcc       = { "Accounting Header (MQCFH)",0,AccFields, NULL};
MQFIELD Acc           = { "!MQCFH"            , MQFT_STRUCT, 0, &DefAcc};

MQFIELDS StatsFields[]={{0,&Type             ,DepStats       , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&Version          ,NULL           , 3},
                        {0,&Command          ,NULL           , 1},
                        {0,&MsgSequenceNumber,NULL           , 2},
                        {0,&Control          ,NULL           , 3},
                        {0,&CompCode         ,NULL           , 3},
                        {0,&Reason           ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefStats     = { "Statistics Header (MQCFH)",0,StatsFields, NULL};
MQFIELD Stats         = { "!MQCFH"            , MQFT_STRUCT, 0, &DefStats};

MQFIELDS MQCFGRFields[]={{0,&Type             ,DepCFGR        , 3 + SF_DEPSTRUCT},
                        {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                        {0,&ParameterId      ,NULL           , 1},
                        {0,&ParmCount        ,NULL           , 2},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFGR    = { "PCF Group (MQCFGR)",0,MQCFGRFields, NULL};
MQFIELD CFGR          = { "!MQCFGR"          , MQFT_STRUCT, 0, &DefMQCFGR};

MQFIELDS MQCFIL64Fields[]={{0,&Type             ,DepCFIL64      , 3 + SF_DEPSTRUCT},
                           {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                           {0,&ParameterId      ,NULL           , 1},
                           {0,&ParmCount        ,NULL           , 3},
                           {0,&IntParms         ,NULL           , 3},
                           {0,&IntParm64        ,NULL           , 1},
                           {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFIL64  = { "Integer List (MQCFIL64)",0,MQCFIL64Fields, NULL};
MQFIELD CFIL64        = { "!MQCFIL64"         , MQFT_STRUCT, 0, &DefMQCFIL64};

MQFIELDS MQCFIFFields[]={{0,&Type             ,DepCFIF        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&Operator         ,NULL           , 1},
                         {0,&IntFilter        ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFIF    = { "Integer Filter (MQCFIF)",0,MQCFIFFields, NULL};
MQFIELD CFIF          = { "!MQCFIF"           , MQFT_STRUCT, 0, &DefMQCFIF};

MQFIELDS MQCFSFFields[]={{0,&Type             ,DepCFSF        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&Operator         ,NULL           , 1},
                         {0,&CCSID4           ,NULL           , 3},
                         {0,&FilterLength     ,NULL           , 1},
                         {0,&CharFilter       ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFSF    = { "String Filter (MQCFSF)",0,MQCFSFFields, NULL};
MQFIELD CFSF          = { "!MQCFSF"           , MQFT_STRUCT, 0, &DefMQCFSF};

MQFIELDS MQCFIN64Fields[]={{0,&Type             ,DepCFIN64      , 3 + SF_DEPSTRUCT},
                           {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                           {0,&ParameterId      ,NULL           , 1},
                           {0,&Reserved4        ,NULL           , 3},
                           {0,&IntParm64        ,NULL           , 1},
                           {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFIN64  = { "Integer 64 (MQCFIN)",0,MQCFIN64Fields, NULL};
MQFIELD CFIN64        = { "!MQCFIN64"         , MQFT_STRUCT, 0, &DefMQCFIN64};

MQFIELDS MQCFILFields[]={{0,&Type             ,DepCFIL        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&ParmCount        ,NULL           , 3},
                         {0,&IntParms         ,NULL           , 3},
                         {0,&IntParm          ,NULL           , 1 + SF_PARMVALUE},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFIL    = { "Integer List (MQCFIL)",0,MQCFILFields, NULL};
MQFIELD CFIL          = { "!MQCFIL"           , MQFT_STRUCT, 0, &DefMQCFIL};


MQFIELDS MQCFINFields[]={{0,&Type             ,DepCFIN        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&IntParm          ,NULL           , 1 + SF_PARMVALUE},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFIN    = { "Integer (MQCFIN)",0,MQCFINFields, NULL};
MQFIELD CFIN          = { "!MQCFIN"           , MQFT_STRUCT, 0, &DefMQCFIN};


MQFIELDS MQCFSLFields[]={{0,&Type             ,DepCFSL        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&CCSID4           ,NULL           , 3},
                         {0,&ParmCount        ,NULL           , 3},
                         {0,&StringLength     ,NULL           , 2},
                         {0,&StrParms         ,NULL           , 3},
                         {0,&StrParm          ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFSL    = { "String List (MQCFSL)",0,MQCFSLFields, NULL};
MQFIELD CFSL          = { "!MQCFSL"           , MQFT_STRUCT, 0, &DefMQCFSL};

MQFIELDS MQCFSTFields[]={{0,&Type             ,DepCFST        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&CCSID4           ,NULL           , 3},
                         {0,&StringLength     ,NULL           , 2},
                         {0,&StrParm          ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFST    = { "String (MQCFST)",0,MQCFSTFields, NULL};
MQFIELD CFST          = { "!MQCFST"           , MQFT_STRUCT, 0, &DefMQCFST};

MQFIELDS MQCFBSFields[]={{0,&Type             ,DepCFBS        , 3 + SF_DEPSTRUCT},
                         {0,&StrucLength      ,NULL           , 3 + SF_STRUCLENGTH},
                         {0,&ParameterId      ,NULL           , 1},
                         {0,&HexLength        ,NULL           , 2},
                         {0,&HexParm          ,NULL           , 1},
                         {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQCFBS    = { "Bytes (MQCFBS)",0,MQCFBSFields, NULL};
MQFIELD CFBS          = { "!MQCFBS"           , MQFT_STRUCT, 0, &DefMQCFBS};

MQFIELDS MQPCFFields[]={{0,&CFH              ,NULL           , 1},
                        {0,&CFIN             ,NULL           , 1},
                        {0,&CFIL             ,NULL           , 1},
                        {0,&CFSL             ,NULL           , 1},
                        {0,&CFST             ,NULL           , 1},
                        {0,&CFBS             ,NULL           , 1},
                        {0,&Event            ,NULL           , 1},
                        {0,&CFIF             ,NULL           , 1},
                        {0,&CFSF             ,NULL           , 1},
                        {0,&CFGR             ,NULL           , 1},
                        {0,&CFIN64           ,NULL           , 1},
                        {0,&CFIL64           ,NULL           , 1},
                        {0,&Acc              ,NULL           , 1},
                        {0,&Stats            ,NULL           , 1},
                        {0,&TRTE             ,NULL           , 1},
                        {0,&CFHXR            ,NULL           , 1},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQPCF     = { "!PCF Header (MQPCF)",0,MQPCFFields, NULL};
MQFIELD PCF           = { "!MQPCF"           , MQFT_REPEAT, 0, &DefMQPCF};

/* Dummy PCF definition for formatting out PCF standalone */
MQFIELDS MQPCFFields2[]={{0,&PCF             ,NULL           , 0},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQPCF2    = { "!PCF Header (MQPCF)",0,MQPCFFields2, NULL};
MQFIELD PCF2           = { "!MQPCF"           , MQFT_REPEAT, 0, &DefMQPCF2};

MQFIELDS MQTMFields[] ={{0,&Eyecatcher       ,NULL           , 3},
                        {0,&Version          ,NULL             , 2 + SF_VERSION},
                        {0,&QName            ,NULL           , 1},
                        {0,&ProcessName      ,NULL           , 2},
                        {0,&TriggerData      ,NULL           , 1},
                        {0,&ApplType         ,NULL           , 2},
                        {0,&ApplId           ,NULL           , 1},
                        {0,&EnvData          ,NULL           , 1},
                        {0,&UserData         ,NULL           , 1},
                        {0,&QMgrName         ,DepVersion2    , 1},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQTM      = { "Trigger Message (MQTM)"         ,0,MQTMFields, NULL};
MQFIELD TM            = { "!MQDH"            , MQFT_STRUCT, 0, &DefMQTM };

MQFIELDS MQHDRFields[]={{0,&DLH              ,DepMQDLH       , 0},
                        {0,&DLH              ,DepMQDLHEBC    , 0},
                        {0,&XQH              ,DepMQXQH       , 0},
                        {0,&XQH              ,DepMQXQHEBC    , 0},
                        {0,&MDE              ,DepMQMDE       , 0},
                        {0,&MDE              ,DepMQMDEEBC    , 0},
                        {0,&DH               ,DepMQDH        , 0},
                        {0,&DH               ,DepMQDHEBC     , 0},
                        {0,&RFH2             ,DepMQRFH2      , 0},
                        {0,&RFH2             ,DepMQRFH2EBC   , 0},
                        {0,&PCF              ,DepMQAdmin     , 0},
                        {0,&PCF              ,DepMQAdminEBC  , 0},
                        {0,&DumpStr          ,DepMQCmd1      , 0},
                        {0,&DumpStr          ,DepMQCmd1EBC   , 0},
                        {0,&DumpStr          ,DepMQCmd2      , 0},
                        {0,&DumpStr          ,DepMQCmd2EBC   , 0},
                        {0,&DumpStr          ,DepMQSTR       , 0},
                        {0,&DumpStr          ,DepMQSTREBC    , 0},
                        {0,&PCF              ,DepMQEvent     , 0},
                        {0,&PCF              ,DepMQEventEBC  , 0},
                        {0,&PCF              ,DepMQPCF       , 0},
                        {0,&PCF              ,DepMQPCFEBC    , 0},
                        {0,&TM               ,DepMQTM        , 0},
                        {0,&TM               ,DepMQTMEBC     , 0},
                        {0,&RFH              ,DepMQRFH       , 0},
                        {0,&RFH              ,DepMQRFHEBC    , 0},
                        {0,&CIH              ,DepMQCICS      , 0},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQHDR     = { "",0            ,MQHDRFields, NULL };
MQFIELD MQHDR         = { "!MQHDR"           , MQFT_REPEAT, 0, &DefMQHDR };

MQFIELDS MQMSGFields[]={{0,&MQHDR            ,NULL           , 0},
                        {0,&RFH2             ,DepMQRFH2      , 0},
                        {0,&RFH2             ,DepMQRFH2EBC   , 0},
                        {0,&FieldMessage     ,NULL           , 0},
                        {0,NULL              ,NULL           , 0}};
MQSTRUCT DefMQMSG     = { "Message Content",0            ,MQMSGFields, NULL };
MQFIELD MQMSG         = { "!MQMSG"            , MQFT_STRUCT, 0, &DefMQMSG };

/* Required for trace formatter */
MQFIELDS REASONFields[]={{0,&Reason   ,NULL           , 1},
                         {0,NULL            ,NULL           , 0}};
MQSTRUCT DefReason    = { "Reason",0,REASONFields, NULL };

MQFIELDS COMPCODEFields[]={{0,&CompCode   ,NULL           , 1},
                         {0,NULL          ,NULL           , 0}};
MQSTRUCT DefCompcode  = { "Compcode",0,COMPCODEFields, NULL };

MQFIELDS LongFields[]={{0,&Version        ,NULL           , 1},
                         {0,NULL          ,NULL           , 0}};
MQSTRUCT DefLong  = { " ",0,LongFields, NULL };

MQFIELDS HexFields[]={{0,&MQEncoding      ,NULL           , 1},
                         {0,NULL          ,NULL           , 0}};
MQSTRUCT DefHex   = { " ",0,HexFields, NULL };

MQFIELDS OpenFields[]={{0,&OpenOptions    ,NULL           , 1},
                       {0,NULL          ,NULL           , 0}};
MQSTRUCT DefOpenOpts  = { "OpenOptions",0,OpenFields, NULL };

MQFIELDS CloseFields[]={{0,&CloseOptions    ,NULL           , 1},
                        {0,NULL          ,NULL           , 0}};
MQSTRUCT DefCloseOpts  = { "CloseOptions",0,CloseFields, NULL };

MQFIELDS ODTypeFields[]={{0,&ObjectType   ,NULL           , 1},
                         {0,NULL          ,NULL           , 0}};
MQSTRUCT DefODType = { "ObjType",0,ODTypeFields, NULL };

MQFIELDS ODNameFields[]={{0,&ObjectName   ,NULL           , 1},
                         {0,NULL          ,NULL           , 0}};
MQSTRUCT DefODName = { "ObjName",0,ODNameFields, NULL };

MQFIELDS ODQmgrFields[]={{0,&ObjectQMgrName ,NULL           , 1},
                         {0,NULL            ,NULL           , 0}};
MQSTRUCT DefODQmgr = { "ObjQmgr",0,ODQmgrFields, NULL };


/**********************************************************************/
/* All Structures MUST BE IN HERE TO GET OFFSETS INITIALIZED          */
/**********************************************************************/
MQSTRUCTS   MQStructs[] = {
   /**********************************************************************/
   /* Structures defined in cmqc.h follow (externalized)                 */
   /**********************************************************************/
#ifndef MVS
                           { &DefMQBO,    MQBO_STRUC_ID     ,0},
#endif
                           { &DefMQCIH,   MQCIH_STRUC_ID    ,0},
                           { &DefMQDH,    MQDH_STRUC_ID     ,0},
                           { &DefMQDLH,   MQDLH_STRUC_ID    ,0},
                           { &DefMQGMO,   MQGMO_STRUC_ID    ,0},
                           { &DefMQIIH,   MQIIH_STRUC_ID    ,0},
                           { &DefMQMD,    MQMD_STRUC_ID     ,0},
                           { &DefMQMDE,   MQMDE_STRUC_ID    ,0},
                           { &DefMQOD,    MQOD_STRUC_ID     ,0},
                           { &DefMQOR,    NULL              ,0},
                           { &DefMQPMO,   MQPMO_STRUC_ID    ,0},
                           { &DefMQPMR,   NULL              ,0},
                           { &DefMQRFH,   NULL              ,0},
                           { &DefMQRFH2,  MQRFH_STRUC_ID    ,0},
                           { &DefMQRMH,   MQRMH_STRUC_ID    ,0},
                           { &DefMQRR,    NULL              ,0},
                           { &DefMQSD,    MQSD_STRUC_ID     ,0},
                           { &DefMQSRO,   MQSRO_STRUC_ID    ,0},
                           { &DefMQTM,    MQTM_STRUC_ID     ,0},
                           { &DefMQTM,    MQTMC_STRUC_ID    ,0},
                           { &DefMQWIH,   MQWIH_STRUC_ID    ,0},
                           { &DefXQH,     MQXQH_STRUC_ID    ,0},

   /**********************************************************************/
   /* Structures defined in cmqcfc.h follow (externalized)               */
   /**********************************************************************/
                           { &DefMQPCF,   NULL              ,0}, /* Internal to this pgm */
                           { &DefMQPCF2,  NULL              ,0}, /* Internal to this pgm */
                           { &DefMQCFH,   NULL              ,0},
                           { &DefMQCFHXR, NULL              ,0},
                           { &DefEvent,   NULL              ,0},
                           { &DefMQCFIL,  NULL              ,0},
                           { &DefMQCFIN,  NULL              ,0},
                           { &DefMQCFSL,  NULL              ,0},
                           { &DefMQCFST,  NULL              ,0},
                           { &DefMQCFBS,  NULL              ,0},
                           { &DefMQCFGR,  NULL              ,0},
                           { &DefMQCFIF,  NULL              ,0},
                           { &DefMQCFIL64,NULL              ,0},
                           { &DefMQCFIN64,NULL              ,0},
                           { &DefMQCFSF,  NULL              ,0},
                           { &DefMQEPH,   NULL              ,0},
                           { &DefMQHDR,   NULL              ,0},
                           { &DefMQMSG,   NULL              ,0},

       /* Others here just for offset initialization */
                           { &DefReason,   NULL              ,0},
                           { &DefCompcode, NULL              ,0},
                           { &DefLong,     NULL              ,0},
                           { &DefHex,      NULL              ,0},
                           { &DefOpenOpts, NULL              ,0},
                           { &DefCloseOpts,NULL              ,0},

                           { NULL,        NULL              ,0}};

/**********************************************************************/
/* Function : InitialiseOffsets                                       */
/* Purpose  : Initialise the structure offset values                  */
/**********************************************************************/
void InitialiseOffsets(void)
{
  int         i = 0;
  int         Offset;
  MQSTRUCT  * pStruct;
  MQFIELDS  * pFields;
  MQDEPEND  * pDepend;

  Initialised = 1;
  while(pStruct = MQStructs[i++].Struct)
  {
    Offset  = 0;
    pFields = pStruct -> Fields;
    while (pFields->Field)
    {
      pFields -> Offset = Offset;
      Offset += pFields -> Field -> Length;
      if (pFields->Depend)
      {
        pDepend = pFields->Depend;
        while (pDepend)
        {
          if (!pDepend->Field) break;
          pDepend->Field->FldOptions |= FO_DEPENDED;
          pDepend++;
        }
      }
      pFields++;
    }
  }
  /********************************************************************/
  /* Initialise structure fields that can't be done statically        */
  /********************************************************************/
  Reason.Value       = ValMQRC;
  Selector.Value     = ValMQPARM;
  OpenOptions.Value  = ValMQOPENOptions;
  CloseOptions.Value = ValMQCLOSEOptions;
  PutOptions.Value   = ValMQPUTOptions;
  GetOptions.Value   = ValMQGETOptions;
  SubOptions.Value   = ValMQSUBOptions;
  SubROptions.Value  = ValMQSUBRQOptions;
  ParameterId.Value  = ValMQPARM;
  Command.Value      = ValMQCMD;
}


