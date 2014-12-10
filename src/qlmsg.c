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
/*  FILE   : QLMSG.C                                                  */
/*  PURPOSE: Various functions operating on MQI objects               */
/**********************************************************************/
#ifdef MVS
  #pragma csect(code,"MSGCODE")
  #pragma csect(static,"MSGSTAT")
  #pragma options(RENT)
#endif
                                       /* Include files               */
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "cmqc.h"
#ifdef MVS
  #define NOLOAD                       /* Can't use MQAccess on MVS   */
#endif
#include "qlaccess.h"
#include "qlvalues.h"
#include "q.h"

/**********************************************************************/
/* Function: QueryQMName                                              */
/* Purpose : Get the name of the Queue Manager                        */
/**********************************************************************/
MQLONG QueryQMName(MQ     * pMQ,
                   MQCHAR48 QM)
{
  MQOD   ObjDesc      = { MQOD_DEFAULT };
  MQLONG flQueueFlags = MQOO_INQUIRE;
  MQHOBJ QmHobj       = MQHO_UNUSABLE_HOBJ;
  MQLONG CompCode, Reason;
  MQLONG Selectors[1];
                                       /* Open QM Object              */
  ObjDesc.Version     = MQOD_VERSION_1;
  ObjDesc.ObjectType  = MQOT_Q_MGR;

  IMQOPEN( pMQ->Api,
           pMQ->hQm,
          &ObjDesc,
           flQueueFlags,
          &QmHobj,
          &CompCode,
          &Reason );
  if (CompCode == MQCC_FAILED)
  {
    MQError("MQOPEN",ObjDesc.ObjectName,Reason);
    goto MOD_EXIT;
  }
  if (verbose >= VL_MQAPI) MQError("MQOPEN",ObjDesc.ObjectName,Reason);
  if (verbose >= VL_MQDETAILS) fprintf(stderr,"MQHOBJ = %d\n",QmHobj);
                                       /* Now issue query             */
  Selectors[0] = MQCA_Q_MGR_NAME;

  IMQINQ(pMQ->Api,
         pMQ->hQm,                  /* Connection handle              */
         QmHobj,                    /* Object handle                  */
         1,                         /* Count of selectors             */
         Selectors,                 /* Array of attribute selectors   */
         0,                         /* Count of integer attributes    */
         NULL,                      /* Array of integer attributes    */
         MQ_Q_MGR_NAME_LENGTH,      /* Length of character-attributes */
                                    /* buffer                         */
         QM,                        /* Character attributes           */
         &CompCode,
         &Reason);
  if (CompCode == MQCC_FAILED)
  {
    MQError("MQINQ",ObjDesc.ObjectName,Reason);
    goto MOD_EXIT;
  }

  if (verbose >= VL_MQAPI) MQError("MQINQ",ObjDesc.ObjectName,Reason);

MOD_EXIT:

  if (QmHobj != MQHO_UNUSABLE_HOBJ)
  {
                                   /* Close Queue Manager Desc.     */
    IMQCLOSE(pMQ->Api,
             pMQ->hQm,
            &QmHobj,
             MQCO_NONE,
            &CompCode,
            &Reason);

    if (verbose >= VL_MQAPI) MQError("MQCLOSE",ObjDesc.ObjectName,Reason);
  }
  return Reason;
}
/**********************************************************************/
/* End of function QueryQMName                                        */
/**********************************************************************/

/**********************************************************************/
/* Function: OpenQueue                                                */
/* Purpose : Open a MQ Queue                                          */
/**********************************************************************/
MQLONG OpenQueue    (MQ        * pMQ,
                     char      * Q,
                     MQLONG      OpenFlags,
                     MQLONG      Version,
                     MQHOBJ    * hObj)
{
  MQLONG CompCode,Reason;
  MQOD   ObjDesc = { MQOD_DEFAULT };
  char * p;

  ObjDesc.Version = Version;
  /********************************************************************/
  /* Parse the input Queue name                                       */
  /********************************************************************/
                                       /* Is there a QM name ?        */
          p = strchr(Q,'/');
  if (!p) p = strchr(Q,'#');
  if (!p) p = strchr(Q,'\\');
  if (!p) p = strchr(Q,',');

  if (p)
  {
    memcpy(ObjDesc.ObjectQMgrName,Q, p-Q);
    Q = p+1;
  }
                                       /* Copy Queue Name             */
  strncpy(ObjDesc.ObjectName,Q,(int)MQ_Q_NAME_LENGTH);

  IMQOPEN( pMQ->Api,
           pMQ->hQm,
          &ObjDesc,
           OpenFlags,
           hObj,
          &CompCode,
          &Reason );

  if (verbose >= VL_MQAPI || CompCode == MQCC_FAILED)
    MQError("MQOPEN",Q,Reason);

  return Reason;
}
