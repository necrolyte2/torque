#include <pbs_config.h>   /* the master config generated by configure */

#include <stdio.h>
#include "sys/types.h"
#include "libpbs.h"
#include "server_limits.h"
#include "list_link.h"
#include "attribute.h"
#include "server.h"
#include "credential.h"
#include "batch_request.h"
#include "pbs_job.h"
#include "pbs_error.h"
#include "log.h"
#include "acct.h"
#include "svrfunc.h"
#include "array.h"
#include "csv.h"

extern int chk_hold_priv(long val, int perm);
extern int get_hold(tlist_head *, char **, attribute *);
extern int svr_authorize_req(struct batch_request *preq, char *owner,
                               char *submit_host);

extern char *msg_permlog;

void req_holdarray(struct batch_request *preq)
  {
  long *hold_val;
  int newstate;
  int newsub;
  long old_hold;

  job *pjob;
  char *pset;
  int rc;
  attribute temphold;
  attribute *pattr;
  char owner[PBS_MAXUSER + 1];
  job_array *pa;
  /* batch_request *preq_tmp; */

  pa = get_array(preq->rq_ind.rq_hold.rq_orig.rq_objname);

  if (pa == NULL)
    {
    /* this shouldn't happen since we verify that this is a valid array
       just prior to calling this function */
    req_reject(PBSE_UNKARRAYID, 0, preq, NULL, "unable to find array");
    }

  get_jobowner(pa->ai_qs.owner, owner);

  if (svr_authorize_req(preq, owner, pa->ai_qs.submit_host) == -1)
    {
    sprintf(log_buffer, msg_permlog,
            preq->rq_type,
            "Array",
            preq->rq_ind.rq_delete.rq_objname,
            preq->rq_user,
            preq->rq_host);

    log_event(
      PBSEVENT_SECURITY,
      PBS_EVENTCLASS_JOB,
      preq->rq_ind.rq_delete.rq_objname,
      log_buffer);

    req_reject(PBSE_PERM, 0, preq, NULL, "operation not permitted");
    return;
    }


  if ((rc = get_hold(&preq->rq_ind.rq_hold.rq_orig.rq_attr, &pset,
                     &temphold)) != 0)
    {
    req_reject(rc, 0, preq, NULL, NULL);
    return;
    }

  /* if other than HOLD_u is being set, must have privil */

  if ((rc = chk_hold_priv(temphold.at_val.at_long, preq->rq_perm)) != 0)
    {
    req_reject(rc, 0, preq, NULL, NULL);
    return;
    }

  pjob = (job*)GET_NEXT(pa->array_alljobs);

  while (pjob != NULL)
    {

    hold_val = &pjob->ji_wattr[(int)JOB_ATR_hold].at_val.at_long;
    old_hold = *hold_val;
    *hold_val |= temphold.at_val.at_long;
    pjob->ji_wattr[(int)JOB_ATR_hold].at_flags |= ATR_VFLAG_SET;

    pattr = &pjob->ji_wattr[(int)JOB_ATR_checkpoint];

    if ((pjob->ji_qs.ji_state == JOB_STATE_RUNNING) &&
        ((pattr->at_flags & ATR_VFLAG_SET) &&
         ((csv_find_string(pattr->at_val.at_str, "s") != NULL) ||
          (csv_find_string(pattr->at_val.at_str, "c") != NULL) ||
          (csv_find_string(pattr->at_val.at_str, "enabled") != NULL))))
      {
      /* TODO */
      /* preq_tmp = alloc_br(preq->rq_type); */

      }
    else if (old_hold != *hold_val)
      {
      /* indicate attributes changed  */

      pjob->ji_modified = 1;

      svr_evaljobstate(pjob, &newstate, &newsub, 0);

      svr_setjobstate(pjob, newstate, newsub);
      }


    pjob = (job*)GET_NEXT(pjob->ji_arrayjobs);
    }

  reply_ack(preq);

  }

