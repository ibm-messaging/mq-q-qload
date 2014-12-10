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
/*  FILE   : QLPARSE.H                                                */
/*  PURPOSE: MQSeries message parse header file                       */
/**********************************************************************/
#include "qlcs.h"   /* Include definition of CBOPT */
                                       /* Macros                      */
#define DETAILOK   ((pOptions->Options & MF_DETAIL)>=(pFields -> Attrs & SF_DETAIL))

                                       /* Prototypes                  */

int  PrintStruct(MQSTRUCT       * pStruct,
                 MQSCHAIN       * pChain,
                 int             (outfn)(void *,CBOPT *,char *,int ),
                 void           * Parm,
                 CBOPT          * pOptions,
                 MQLONG         * pMsgLen,
                 unsigned char ** pp);
