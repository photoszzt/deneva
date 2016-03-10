/*
   Copyright 2015 Rachael Harding

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef _WORKERTHREAD_H_
#define _WORKERTHREAD_H_

#include "global.h"

class Workload;
class Message;

class WorkerThread : public Thread {
public:
	RC 			run();
  void setup();
  void send_init_done_to_all_nodes(); 
  void progress_stats(); 
  void process(Message * msg); 
  void commit(uint64_t txn_id); 
  void abort(uint64_t txn_id); 
  RC process_rfin(Message * msg);
  RC process_rack(Message * msg);
  RC process_rqry_rsp(Message * msg);
  RC process_rqry(Message * msg);
  RC process_rinit(Message * msg);
  RC process_rprepare(Message * msg);
  RC process_rpass(Message * msg);
  RC process_rtxn(Message * msg);
  RC process_rtxn_cont(Message * msg);
  RC process_log_msg(Message * msg);
  RC process_log_msg_rsp(Message * msg);
  RC init_phase(); 
  uint64_t get_next_txn_id(); 
  bool is_cc_new_timestamp(); 

private:
  uint64_t _thd_txn_id;
	ts_t 		_curr_ts;
	ts_t 		get_next_ts();
  uint64_t prog_time;


};

#endif