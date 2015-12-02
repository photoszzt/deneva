#include "row.h"
#include "txn.h"
#include "row_lock.h"
#include "mem_alloc.h"
#include "manager.h"
#include "helper.h"

void Row_lock::init(row_t * row) {
	_row = row;
  owners_size = 1;//1031;
	owners = NULL;
  owners = (LockEntry**) mem_allocator.alloc(sizeof(LockEntry*)*owners_size,0);
  for(uint64_t i = 0; i < owners_size; i++)
    owners[i] = NULL;
	waiters_head = NULL;
	waiters_tail = NULL;
	owner_cnt = 0;
	waiter_cnt = 0;
  max_owner_ts = 0;

	latch = new pthread_mutex_t;
	pthread_mutex_init(latch, NULL);
	
	lock_type = LOCK_NONE;
	blatch = false;
  own_starttime = 0;

}

RC Row_lock::lock_get(lock_t type, txn_man * txn) {
	uint64_t *txnids = NULL;
	int txncnt = 0;
	return lock_get(type, txn, txnids, txncnt);
}

RC Row_lock::lock_get(lock_t type, txn_man * txn, uint64_t* &txnids, int &txncnt) {
	assert (CC_ALG == DL_DETECT || CC_ALG == NO_WAIT || CC_ALG == WAIT_DIE || CC_ALG == CALVIN);
	RC rc;
	//int part_id =_row->get_part_id(); // This was for DL_DETECT
	if (g_central_man)
		glob_manager.lock_row(_row);
	else 
		pthread_mutex_lock( latch );
#if DEBUG_ASSERT
  /*
	if (owners[hash(txn->get_txn_id())] != NULL)
		assert(lock_type == owners[hash(txn->get_txn_id())]->type); 
	else 
		assert(lock_type == LOCK_NONE);

	LockEntry * tmp1 = owners[hash(txn->get_txn_id())];
	UInt32 cnt = 0;
	while (tmp1) {
		assert(tmp1->txn->get_txn_id() != txn->get_txn_id());
		cnt ++;
		tmp1 = tmp1->next;
	}
	assert(cnt == owner_cnt);

	LockEntry * tmp2 = waiters_head;
	cnt = 0;
	while (tmp2) {
		assert(tmp2->txn->get_txn_id() != txn->get_txn_id());
		cnt ++;
		tmp2 = tmp2->next;
	}
	assert(cnt == waiter_cnt);
  */
#endif

  if(owner_cnt > 0) {
    INC_STATS(txn->get_thd_id(),cc_busy_cnt,1);
  }
	bool conflict = conflict_lock(lock_type, type);
#if TWOPL_LITE
  conflict = owner_cnt > 0;
#endif
	if (CC_ALG == WAIT_DIE && !conflict) {
		if (waiters_head && txn->get_ts() < waiters_head->txn->get_ts()) {
			conflict = true;
      //printf("special ");
    }
	}
	if (CC_ALG == CALVIN && !conflict) {
    if(waiters_head)
      conflict = true;
  }
	// Some txns coming earlier is waiting. Should also wait.
	if (CC_ALG == DL_DETECT && waiters_head != NULL)
		conflict = true;
	
	if (conflict) { 
    //printf("conflict! rid%ld txnid%ld ",_row->get_primary_key(),txn->get_txn_id());
		// Cannot be added to the owner list.
		if (CC_ALG == NO_WAIT) {
			rc = Abort;
      DEBUG("abort %ld,%ld %ld %lx\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
      //printf("abort %ld %ld %lx\n",txn->get_txn_id(),_row->get_primary_key(),(uint64_t)_row);
			goto final;
		} else if (CC_ALG == DL_DETECT) {
			LockEntry * entry = get_entry();
			entry->txn = txn;
			entry->type = type;
      //txn->lock_ready = false;
      ATOM_CAS(txn->lock_ready,true,false);
      txn->incr_lr();
			LIST_PUT_TAIL(waiters_head, waiters_tail, entry);
			waiter_cnt ++;
            rc = WAIT;
            txn->rc = rc;
            txn->wait_starttime = get_sys_clock();
		} else if (CC_ALG == WAIT_DIE) {
            ///////////////////////////////////////////////////////////
            //  - T is the txn currently running
            //	IF T.ts > min ts of owners
            //		T can wait
            //  ELSE
            //      T should abort
            //////////////////////////////////////////////////////////

      //bool canwait = txn->get_ts() > max_owner_ts;
			bool canwait = true;
      LockEntry * en;
      for(uint64_t i = 0; i < owners_size; i++) {
        en = owners[i];
        while (en != NULL) {
          if (txn->get_ts() > en->txn->get_ts()) {
            //printf("abort %ld %ld -- %ld -- %f\n",txn->get_txn_id(),en->txn->get_txn_id(),_row->get_primary_key(),(float)(txn->get_ts() - en->txn->get_ts()) / BILLION);
            INC_STATS(txn->get_thd_id(),twopl_time_diff,(txn->get_ts() - en->txn->get_ts()));
            canwait = false;
            break;
          }
          en = en->next;
        }
        if(!canwait)
          break;
      }
			if (canwait) {
				// insert txn to the right position
				// the waiter list is always in timestamp order
				LockEntry * entry = get_entry();
        entry->start_ts = get_sys_clock();
				entry->txn = txn;
				entry->type = type;
        entry->start_ts = get_sys_clock();
				entry->txn = txn;
				entry->type = type;
        LockEntry * en;
        //txn->lock_ready = false;
        ATOM_CAS(txn->lock_ready,true,false);
        txn->incr_lr();
				en = waiters_head;
				while (en != NULL && txn->get_ts() < en->txn->get_ts()) 
					en = en->next;
				if (en) {
					LIST_INSERT_BEFORE(en, entry,waiters_head);
				} else 
					LIST_PUT_TAIL(waiters_head, waiters_tail, entry);

			  waiter_cnt ++;
        DEBUG("wait %ld,%ld %ld %lx\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
        //printf("wait %ld %ld %lx\n",txn->get_txn_id(),_row->get_primary_key(),(uint64_t)_row);
        rc = WAIT;
        txn->rc = rc;
        txn->cc_wait_cnt++;
        txn->wait_starttime = get_sys_clock();
        //printf("wait \n");
      } else {
        INC_STATS(txn->get_thd_id(),abort_from_ts,1);
        DEBUG("abort %ld,%ld %ld %lx\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
        //printf("abort %ld %ld %lx\n",txn->get_txn_id(),_row->get_primary_key(),(uint64_t)_row);
        rc = Abort;
        //printf("abort \n");
      }
    } else if (CC_ALG == CALVIN){
			LockEntry * entry = get_entry();
      entry->start_ts = get_sys_clock();
			entry->txn = txn;
			entry->type = type;
      DEBUG("wait %ld,%ld %ld\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key());
      //printf("wait %ld %ld\n",txn->get_txn_id(),_row->get_primary_key());
			LIST_PUT_TAIL(waiters_head, waiters_tail, entry);
			waiter_cnt ++;
      //txn->lock_ready = false;
      ATOM_CAS(txn->lock_ready,true,false);
      txn->incr_lr();
      rc = WAIT;
      txn->rc = rc;
      txn->wait_starttime = get_sys_clock();
    }
	} else { 
    DEBUG("1lock %ld,%ld %ld %lx\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
    //printf("1lock %ld %ld %lx\n",txn->get_txn_id(),_row->get_primary_key(),(uint64_t)_row);
#if DEBUG_TIMELINE
    printf("LOCK %ld %ld\n",entry->txn->get_txn_id(),entry->start_ts);
#endif
#if CC_ALG != NO_WAIT
		LockEntry * entry = get_entry();
		entry->type = type;
    entry->start_ts = get_sys_clock();
		entry->txn = txn;
		STACK_PUSH(owners[hash(txn->get_txn_id())], entry);
#endif
    if(txn->get_ts() > max_owner_ts)
      max_owner_ts = txn->get_ts();
		owner_cnt ++;
    if(lock_type == LOCK_NONE)
      own_starttime = get_sys_clock();
		lock_type = type;
		if (CC_ALG == DL_DETECT) 
			ASSERT(waiters_head == NULL);
    rc = RCOK;
	}
final:
	
  /*
	if (rc == WAIT && CC_ALG == DL_DETECT) {
		// Update the waits-for graph
		ASSERT(waiters_tail->txn == txn);
		txnids = (uint64_t *) mem_allocator.alloc(sizeof(uint64_t) * (owner_cnt + waiter_cnt), part_id);
		txncnt = 0;
		LockEntry * en = waiters_tail->prev;
		while (en != NULL) {
			if (conflict_lock(type, en->type)) 
				txnids[txncnt++] = en->txn->get_txn_id();
			en = en->prev;
		}
		en = owners;
		if (conflict_lock(type, lock_type)) 
			while (en != NULL) {
				txnids[txncnt++] = en->txn->get_txn_id();
				en = en->next;
			}
		ASSERT(txncnt > 0);
	}
  */

	if (g_central_man)
		glob_manager.release_row(_row);
	else
		pthread_mutex_unlock( latch );

	return rc;
}


RC Row_lock::lock_release(txn_man * txn) {	

  uint64_t thd_prof_start = get_sys_clock();
	if (g_central_man)
		glob_manager.lock_row(_row);
	else 
		pthread_mutex_lock( latch );

  INC_STATS(txn->get_thd_id(),thd_prof_cc0,get_sys_clock() - thd_prof_start);
  thd_prof_start = get_sys_clock();

  DEBUG("unlock %ld,%ld %ld %lx\n",txn->get_txn_id(),txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
  //printf("unlock %ld %ld\n",txn->get_txn_id(),_row->get_primary_key());

  // If CC is NO_WAIT or WAIT_DIE, txn should own this lock
  // What about Calvin?
#if CC_ALG == NO_WAIT
  assert(owner_cnt > 0);
  owner_cnt--;
  if (owner_cnt == 0) {
    INC_STATS(txn->get_thd_id(),owned_cnt,1);
    uint64_t endtime = get_sys_clock();
    INC_STATS(txn->get_thd_id(),owned_time,endtime - own_starttime);
    if(lock_type == LOCK_SH) {
      INC_STATS(txn->get_thd_id(),owned_time_rd,endtime - own_starttime);
      INC_STATS(txn->get_thd_id(),owned_cnt_rd,1);
    }
    else {
      INC_STATS(txn->get_thd_id(),owned_time_wr,endtime - own_starttime);
      INC_STATS(txn->get_thd_id(),owned_cnt_wr,1);
    }
    lock_type = LOCK_NONE;
  }

#else

	// Try to find the entry in the owners
	LockEntry * en = owners[hash(txn->get_txn_id())];
	LockEntry * prev = NULL;

	while (en != NULL && en->txn != txn) {
		prev = en;
		en = en->next;
	}

  INC_STATS(txn->get_thd_id(),thd_prof_cc1,get_sys_clock() - thd_prof_start);
  thd_prof_start = get_sys_clock();

	if (en) { // find the entry in the owner list
    en->txn->cc_hold_time = get_sys_clock() - en->start_ts;
		if (prev) prev->next = en->next;
		else owners[hash(txn->get_txn_id())] = en->next;
		return_entry(en);
		owner_cnt --;
    if(owner_cnt == 0){
      INC_STATS(txn->get_thd_id(),owned_cnt,1);
      uint64_t endtime = get_sys_clock();
      INC_STATS(txn->get_thd_id(),owned_time,endtime - own_starttime);
      if(lock_type == LOCK_SH) {
        INC_STATS(txn->get_thd_id(),owned_time_rd,endtime - own_starttime);
        INC_STATS(txn->get_thd_id(),owned_cnt_rd,1);
      }
      else {
        INC_STATS(txn->get_thd_id(),owned_time_wr,endtime - own_starttime);
        INC_STATS(txn->get_thd_id(),owned_cnt_wr,1);
      }
      lock_type = LOCK_NONE;
    }
  } else {
    assert(false);
		en = waiters_head;
		while (en != NULL && en->txn != txn)
			en = en->next;
		ASSERT(en);
    uint64_t t = get_sys_clock() - en->start_ts;
    // Stats
    en->txn->cc_wait_time += t;
    en->txn->txn_time_wait += t;

		LIST_REMOVE(en);
		if (en == waiters_head)
			waiters_head = en->next;
		if (en == waiters_tail)
			waiters_tail = en->prev;
		return_entry(en);
		waiter_cnt --;
	}
#endif

  INC_STATS(txn->get_thd_id(),thd_prof_cc2,get_sys_clock() - thd_prof_start);
  thd_prof_start = get_sys_clock();
	if (owner_cnt == 0)
		ASSERT(lock_type == LOCK_NONE);
#if DEBUG_ASSERT && CC_ALG == WAIT_DIE 
  for (en = waiters_head; en != NULL && en->next != NULL; en = en->next)
    assert(en->next->txn->get_ts() < en->txn->get_ts());
  for (en = waiters_head; en != NULL && en->next != NULL; en = en->next)
    assert(en->txn->get_txn_id() !=txn->get_txn_id());
#endif

	LockEntry * entry;
    ts_t t;
	// If any waiter can join the owners, just do it!
	while (waiters_head && !conflict_lock(lock_type, waiters_head->type)) {
		LIST_GET_HEAD(waiters_head, waiters_tail, entry);
#if DEBUG_TIMELINE
    printf("LOCK %ld %ld\n",entry->txn->get_txn_id(),get_sys_clock());
#endif
    DEBUG("2lock %ld,%ld %ld %lx\n",entry->txn->get_txn_id(),entry->txn->batch_id,_row->get_primary_key(),(uint64_t)_row);
    //printf("2lock %ld %ld %lx\n",entry->txn->get_txn_id(),_row->get_primary_key(),(uint64_t)_row);
    // Stats
    t = get_sys_clock() - entry->start_ts;
    //INC_STATS(0, time_wait_lock, t);
    entry->txn->cc_wait_time += t;
    entry->txn->txn_time_wait += t;

#if CC_ALG != NO_WAIT
		STACK_PUSH(owners[hash(entry->txn->get_txn_id())], entry);
#endif 
		owner_cnt ++;
		waiter_cnt --;
    if(entry->txn->get_ts() > max_owner_ts)
      max_owner_ts = entry->txn->get_ts();
		ASSERT(entry->txn->lock_ready == false);
    //if(entry->txn->decr_lr() == 0 && entry->txn->locking_done) {
    if(entry->txn->decr_lr() == 0) {
      if(ATOM_CAS(entry->txn->lock_ready,false,true))
        txn_table.restart_txn(entry->txn->get_txn_id(),entry->txn->batch_id);
    }
    if(lock_type == LOCK_NONE)
      own_starttime = get_sys_clock();
		lock_type = entry->type;
#if CC_AlG == NO_WAIT
		return_entry(entry);
#endif
	} 

	if (g_central_man)
		glob_manager.release_row(_row);
	else
		pthread_mutex_unlock( latch );

  INC_STATS(txn->get_thd_id(),thd_prof_cc3,get_sys_clock() - thd_prof_start);
	return RCOK;
}

bool Row_lock::conflict_lock(lock_t l1, lock_t l2) {
	if (l1 == LOCK_NONE || l2 == LOCK_NONE)
		return false;
    else if (l1 == LOCK_EX || l2 == LOCK_EX)
        return true;
	else
		return false;
}

LockEntry * Row_lock::get_entry() {
	LockEntry * entry = (LockEntry *) 
		mem_allocator.alloc(sizeof(LockEntry), _row->get_part_id());
  entry->type = LOCK_NONE;
  entry->txn = NULL;
  //DEBUG_M("row_lock::get_entry alloc %lx\n",(uint64_t)entry);
	return entry;
}
void Row_lock::return_entry(LockEntry * entry) {
  //DEBUG_M("row_lock::return_entry free %lx\n",(uint64_t)entry);
	mem_allocator.free(entry, sizeof(LockEntry));
}
