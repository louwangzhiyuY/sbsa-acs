/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/
#include "val/include/sbsa_avs_val.h"
#include "val/include/val_interface.h"

#include "val/include/sbsa_avs_timer.h"
#include "val/include/sbsa_avs_pe.h"

#define TEST_NUM   (AVS_TIMER_TEST_NUM_BASE + 7)
#define TEST_DESC  "CNTCTLBase & CNTBaseN access      "

#define ARBIT_VALUE 0xA000

static
void
payload()
{

  uint64_t cnt_ctl_base, cnt_base_n;
  uint32_t data;
  uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());
  uint64_t timer_num = val_timer_get_info(TIMER_INFO_NUM_PLATFORM_TIMERS, 0);

  if (!timer_num) {
      val_print(AVS_PRINT_WARN, "\n       No System timers are defined      ", 0);
      val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 01));
      return;
  }

  while(timer_num){
      --timer_num;  //array index starts from 0, so subtract 1 from count

      if (val_timer_get_info(TIMER_INFO_IS_PLATFORM_TIMER_SECURE, timer_num))
          continue;    //Skip Secure Timer

      cnt_ctl_base = val_timer_get_info(TIMER_INFO_SYS_CNTL_BASE, timer_num);
      cnt_base_n   = val_timer_get_info(TIMER_INFO_SYS_CNT_BASE_N, timer_num);

      if (cnt_ctl_base == 0) {
          val_print(AVS_PRINT_WARN, "\n       CNTCTL BASE_N is zero             ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 02));
          return;
      }

      data = val_mmio_read(cnt_ctl_base + 0xFD0);
      if ((data == 0x0) || ((data & 0xFFFF) == 0xFFFF)) {
          val_print(AVS_PRINT_ERR, "\n       Unxepected value for CNTCTLBase.CounterID %x ", data);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 01));
          return;
      }

      if (cnt_base_n == 0) {
          val_print(AVS_PRINT_WARN, "\n      CNT_BASE_N is zero                 ", 0);
          val_set_status(index, RESULT_SKIP(g_sbsa_level, TEST_NUM, 03));
          return;
      }

      data = val_mmio_read(cnt_base_n + 0x0);
      val_mmio_write(cnt_base_n + 0x0, data - ARBIT_VALUE);  // Writes to Read-Only registers should be ignored
      if(val_mmio_read(cnt_base_n + 0x0) < data) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 02));
          val_print(AVS_PRINT_ERR, "\n    CNTBaseN offset 0 should be read-only ", 0);
          return;
      }

      data = val_mmio_read(cnt_base_n + 0x10);
      if (!data) {
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 03));
          val_print(AVS_PRINT_ERR, "\n   CNTBaseN.CNTFRQ should not be 0   ", 0);
          return;
      }

      data = val_mmio_read(cnt_base_n + 0xFD0);
      if ((data == 0x0) || ((data & 0xFFFF) == 0xFFFF)) {
          val_print(AVS_PRINT_ERR, "\n      Unxepected value for CNTBaseN.CounterID %x  ", data);
          val_set_status(index, RESULT_FAIL(g_sbsa_level, TEST_NUM, 04));
          return;
      }

      val_set_status(index, RESULT_PASS(g_sbsa_level, TEST_NUM, 01));
  }

}

uint32_t
t007_entry(uint32_t num_pe)
{
  uint32_t status = AVS_STATUS_FAIL;

  num_pe = 1;  //This Timer test is run on single processor

  status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe, g_sbsa_level);

  if (status != AVS_STATUS_SKIP)
      val_run_test_payload(TEST_NUM, num_pe, payload, 0);

  /* get the result from all PE and check for failure */
  status = val_check_for_error(TEST_NUM, num_pe);

  val_report_status(0, SBSA_AVS_END(g_sbsa_level, TEST_NUM));

  return status;
}
