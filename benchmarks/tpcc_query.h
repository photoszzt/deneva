#ifndef _TPCC_QUERY_H_
#define _TPCC_QUERY_H_

#include "global.h"
#include "helper.h"
#include "query.h"
#include "remote_query.h"

class workload;

// items of new order transaction
struct Item_no {
	uint64_t ol_i_id;
	uint64_t ol_supply_w_id;
	uint64_t ol_quantity;
};

enum TPCCRemTxnType {
  TPCC_PAYMENT_S,
  TPCC_PAYMENT0,
  TPCC_PAYMENT1,
  TPCC_PAYMENT2,
  TPCC_PAYMENT3,
  TPCC_PAYMENT4,
  TPCC_PAYMENT5,
  TPCC_NEWORDER_S,
  TPCC_NEWORDER0,
  TPCC_NEWORDER1,
  TPCC_NEWORDER2,
  TPCC_NEWORDER3,
  TPCC_NEWORDER4,
  TPCC_NEWORDER5,
  TPCC_NEWORDER6,
  TPCC_NEWORDER7,
  TPCC_NEWORDER8,
  TPCC_NEWORDER9,
  TPCC_FIN,
  TPCC_RDONE};

class tpcc_client_query : public base_client_query {
public:
  void client_init(uint64_t thd_id, workload * h_wl); 
  void client_init(uint64_t thd_id, workload * h_wl, uint64_t node_id); 
private:
  void gen_payment(uint64_t thd_id); 
  void gen_new_order(uint64_t thd_id); 
public:
	TPCCTxnType txn_type;
	/**********************************************/	
	// common txn input for both payment & new-order
	/**********************************************/	
	uint64_t w_id;
	uint64_t d_id;
	uint64_t c_id;
	/**********************************************/	
	// txn input for payment
	/**********************************************/	
	uint64_t d_w_id;
	uint64_t c_w_id;
	uint64_t c_d_id;
	char c_last[LASTNAME_LEN];
	double h_amount;
	bool by_last_name;
	/**********************************************/	
	// txn input for new-order
	/**********************************************/
	Item_no * items;
	bool rbk;
	bool remote;
	uint64_t ol_cnt;
	uint64_t o_entry_d;
	// Input for delivery
	uint64_t o_carrier_id;
	uint64_t ol_delivery_d;
	// for order-status


	// Other
	uint64_t ol_i_id;
	uint64_t ol_supply_w_id;
	uint64_t ol_quantity;
	uint64_t ol_number;
	uint64_t ol_amount;
	uint64_t o_id;


};

class tpcc_query : public base_query {
public:
	void init(uint64_t thd_id, workload * h_wl);
  void reset();
	void remote_qry(base_query * query, int type,int dest_id);
	void remote_rsp(base_query * query);
	void unpack(base_query * query, void * d);
	void unpack_rsp(base_query * query, void * d);
	void pack(base_query * query, void ** data, int * sizes, int * num, RC rc);
    //void client_query(base_query * query, uint64_t dest_id);
    void unpack_client(base_query * query, void * d); 
  base_query * merge(base_query * query); 
  uint64_t participants(bool *& pps,workload * wl); 
  void deep_copy(base_query * qry); 
  bool readonly();
	//uint64_t rtn_node_id;
	TPCCTxnType txn_type;
	TPCCRemTxnType txn_rtype;
	/**********************************************/	
	// common txn input for both payment & new-order
	/**********************************************/	
	uint64_t w_id;
	uint64_t d_id;
	uint64_t c_id;
	/**********************************************/	
	// txn input for payment
	/**********************************************/	
	uint64_t d_w_id;
	uint64_t c_w_id;
	uint64_t c_d_id;
	char c_last[LASTNAME_LEN];
	double h_amount;
	bool by_last_name;
	/**********************************************/	
	// txn input for new-order
	/**********************************************/
	Item_no * items;
	bool rbk;
	bool remote;
	uint64_t ol_cnt;
	uint64_t o_entry_d;
	// Input for delivery
	uint64_t o_carrier_id;
	uint64_t ol_delivery_d;
	// for order-status


	// Other
	uint64_t ol_i_id;
	uint64_t ol_supply_w_id;
	uint64_t ol_quantity;
	uint64_t ol_number;
	uint64_t ol_amount;
	uint64_t o_id;
  uint64_t rqry_req_cnt;

};

#endif