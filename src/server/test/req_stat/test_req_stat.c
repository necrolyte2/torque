#include "license_pbs.h" /* See here for the software license */
#include "req_stat.h"
#include "test_req_stat.h"
#include <stdlib.h>
#include <stdio.h>
#include "pbs_error.h"


extern int abort_called;
extern time_t last_report;


START_TEST(test_stat_update)
  {
  batch_request preq;
  stat_cntl     cntl;

  memset(&preq, 0, sizeof(preq));
  memset(&cntl, 0, sizeof(cntl));
  abort_called = 0;

  preq.rq_reply.brp_choice = BATCH_REPLY_CHOICE_Queue;
  preq.rq_reply.brp_un.brp_txt.brp_str = strdup("MSG=1.napali");

  // Make sure that the job isn't aborted.
  stat_update(&preq, &cntl);
  fail_unless(abort_called == 0);

  preq.rq_reply.brp_choice = BATCH_REPLY_CHOICE_Text;
  preq.rq_reply.brp_code = PBSE_UNKJOBID;
  strcpy(preq.rq_ind.rq_status.rq_id, "1.napali");

  // If we've never reported make sure we don't abort
  last_report = 0;
  stat_update(&preq, &cntl);
  fail_unless(abort_called == 0);

  // Make sure we won't abort if we're within the job reported abort delta
  last_report = time(NULL) - (JOB_REPORTED_ABORT_DELTA / 2);
  stat_update(&preq, &cntl);
  fail_unless(abort_called == 0);

  // Make sure we abort if we're past the delta
  last_report = time(NULL) - (JOB_REPORTED_ABORT_DELTA + 1);
  stat_update(&preq, &cntl);
  fail_unless(abort_called == 1);
  }
END_TEST

START_TEST(test_two)
  {


  }
END_TEST

Suite *req_stat_suite(void)
  {
  Suite *s = suite_create("req_stat_suite methods");
  TCase *tc_core = tcase_create("test_stat_update");
  tcase_add_test(tc_core, test_stat_update);
  suite_add_tcase(s, tc_core);

  tc_core = tcase_create("test_two");
  tcase_add_test(tc_core, test_two);
  suite_add_tcase(s, tc_core);

  return s;
  }

void rundebug()
  {
  }

int main(void)
  {
  int number_failed = 0;
  SRunner *sr = NULL;
  rundebug();
  sr = srunner_create(req_stat_suite());
  srunner_set_log(sr, "req_stat_suite.log");
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return number_failed;
  }
