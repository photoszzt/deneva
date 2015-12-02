#ifndef _YCSB_QUERY_H_
#define _YCSB_QUERY_H_

#include "global.h"
#include "helper.h"
#include "query.h"
#include "remote_query.h"

class workload;

enum YCSBRemTxnType {
  YCSB_0,
  YCSB_1,
  YCSB_FIN,
  YCSB_RDONE
};
// Each ycsb_query contains several ycsb_requests, 
// each of which is a RD, WR or SCAN 
// to a single table

class ycsb_request {
public:
//	char table_name[80];
	access_t acctype; 
	uint64_t key;
	// for each field (string) in the row, shift the string to left by 1 character
	// and fill the right most character with value
	char value;
	// only for (qtype == SCAN)
	UInt32 scan_len;
};

class ycsb_client_query : public base_client_query {
  public:
	void client_init(uint64_t thd_id, workload * h_wl);
  void client_init(uint64_t thd_id, workload * h_wl, uint64_t node_id);
  void client_init();
  
  // calvin
  //void client_query(base_client_query * query, uint64_t dest_id, uint64_t batch_num,
//		  txnid_t txn_id);

	uint64_t request_cnt;
	ycsb_request * requests;

private:
	void gen_requests(uint64_t thd_id, workload * h_wl);
	void gen_requests2(uint64_t thd_id, workload * h_wl);
	void gen_requests3(uint64_t thd_id, workload * h_wl);
	// for Zipfian distribution
	double zeta(uint64_t n, double theta);
	uint64_t zipf(uint64_t n, double theta);
	
	myrand * mrand;
	static uint64_t the_n;
	static double denom;
	double zeta_2_theta;

};

// Beware diamond inheritance...
class ycsb_query : public base_query {
public:
	void init(uint64_t thd_id, workload * h_wl);
  void init();
  void reset();
  void unpack_rsp(base_query * query, void * d); 
  void client_query(base_query * query, uint64_t dest_id) {} 
  uint64_t participants(bool *& pps,workload * wl); 
  void deep_copy(base_query * qry); 
  bool readonly();
	
  base_query * merge(base_query * query); 
void unpack(base_query * query, void * d) ;
void remote_qry(base_query * query, int type, int dest_id) ;
void remote_rsp(base_query * query) ;
//void unpack_client(base_query * query, void * d) ;

  uint64_t access_cnt;
	uint64_t request_cnt;
	ycsb_request * requests;

	YCSBRemTxnType txn_rtype;
  uint64_t rid;
	uint64_t req_i;
  ycsb_request req;
  uint64_t rqry_req_cnt;

};

#endif