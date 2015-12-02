#include "global.h"
#include "ycsb.h"
#include "tpcc.h"
#include "test.h"
#include "thread.h"
#include "mem_alloc.h"
#include "transport.h"
#include "sequencer.h"

void * nn_worker(void *);
void * recv_worker(void *);
void * send_worker(void *);
void network_test();
void network_test_recv();

Seq_thread_t * m_thds;

// defined in parser.cpp
void parser(int argc, char * argv[]);

int main(int argc, char* argv[])
{
  printf("Running sequencer...\n\n");
  assert(false);
	// 0. initialize global data structure
  /*
	parser(argc, argv);
	assert(CC_ALG == CALVIN);
    assert(g_node_id == g_node_cnt + g_client_node_cnt);
	//assert(g_txn_inflight > 

	uint64_t seed = get_sys_clock();
	srand(seed);
	printf("Random seed: %ld\n",seed);

	int64_t starttime;
	int64_t endtime;
    starttime = get_server_clock();
	// per-partition malloc
  printf("Initializing memory allocator... ");
  fflush(stdout);
	mem_allocator.init(g_part_cnt, MEM_SIZE / g_part_cnt); 
  printf("Done\n");
  printf("Initializing stats... ");
  fflush(stdout);
	stats.init();
  printf("Done\n");
	workload * m_wl;
	switch (WORKLOAD) {
		case YCSB :
			m_wl = new ycsb_wl; break;
		case TPCC :
			m_wl = new tpcc_wl; break;
		case TEST :
			m_wl = new TestWorkload; 
			((TestWorkload *)m_wl)->tick();
			break;
		default:
			assert(false);
	}
	m_wl->workload::init();
	printf("Workload initialized!\n");
#if NETWORK_TEST
	tport_man.init(g_node_id,m_wl);
	sleep(3);
	if(g_node_id == 0)
		network_test();
	else if(g_node_id == 1)
		network_test_recv();

	return 0;
#endif

  printf("Initializing remote query manager... ");
  fflush(stdout);
	rem_qry_man.init(g_node_id,m_wl);
  printf("Done\n");
  printf("Initializing transport manager... ");
  fflush(stdout);
	tport_man.init(g_node_id,m_wl);
  printf("Done\n");
  fflush(stdout);
  printf("Initializing work queue... ");
  work_queue.init();
  printf("Done\n");
  fflush(stdout);
  printf("Initializing sequence manager... ");
	seq_man.init(m_wl);
  printf("Done\n");
  fflush(stdout);
  printf("Initializing query pool... ");
  qry_pool.init(m_wl,g_inflight_max);
  printf("Done\n");
  printf("Initializing msg pool... ");
  msg_pool.init(m_wl,g_inflight_max);
  printf("Done\n");

	// 2. spawn multiple threads
	uint64_t thd_cnt = g_seq_thread_cnt;
  uint64_t all_thd_cnt = thd_cnt + 2;

	pthread_t * p_thds = 
		(pthread_t *) malloc(sizeof(pthread_t) * (all_thd_cnt));
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	cpu_set_t cpus __attribute__ ((unused));
	m_thds = new Seq_thread_t[all_thd_cnt];
	pthread_barrier_init( &warmup_bar, NULL, all_thd_cnt );
	for (uint32_t i = 0; i < all_thd_cnt; i++) 
		m_thds[i].init(i, g_node_id, m_wl);
  endtime = get_server_clock();
  printf("Initialization Time = %ld\n", endtime - starttime);
	warmup_finish = true;

	uint64_t cpu_cnt __attribute__ ((unused));
  cpu_cnt = 0;
	// spawn and run txns again.
	starttime = get_server_clock();

	//uint32_t numCPUs = sysconf(_SC_NPROCESSORS_ONLN);
  uint64_t vid = 0;
	//printf("num cpus: %u\n",numCPUs);
	for (uint32_t i = 0; i < thd_cnt; i++) {
#if !TPORT_TYPE_IPC
		CPU_ZERO(&cpus);
    CPU_SET(cpu_cnt, &cpus);
		cpu_cnt++;
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpus);
#endif
    pthread_create(&p_thds[vid], &attr, nn_worker, (void *)vid);
		vid++;
  }

  pthread_create(&p_thds[vid], NULL, send_worker, (void *)vid);
  vid++;
  recv_worker((void*)vid);

	for (uint32_t i = 0; i < thd_cnt+1; i++) 
		pthread_join(p_thds[i], NULL);

	endtime = get_server_clock();
	
	if (WORKLOAD != TEST) {
		printf("SEQUENCER PASS! SimTime = %f\n", (float)(endtime - starttime) / BILLION);
		if (STATS_ENABLE)
			stats.print_sequencer(false);
	} else {
		((TestWorkload *)m_wl)->summarize();
	}
  */

	//tport_man.shutdown();

	return 0;
}

/*
void * recv_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_recv();
	return NULL;
}

void * send_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_send();
	return NULL;
}

void * nn_worker(void * id) {
	uint64_t tid = (uint64_t)id;
	m_thds[tid].run_remote();
	return NULL;
}
*/

void network_test() {

	ts_t start;
	ts_t end;
	double time;
	int bytes;
	for(int i=4; i < 257; i+=4) {
		time = 0;
		for(int j=0;j < 1000; j++) {
			start = get_sys_clock();
			tport_man.simple_send_msg(i);
			while((bytes = tport_man.simple_recv_msg()) == 0) {}
			end = get_sys_clock();
			assert(bytes == i);
			time += end-start;
		}
		time = time/1000;
		printf("Network Bytes: %d, s: %f\n",i,time/BILLION);
        fflush(stdout);
	}
}

void network_test_recv() {
	int bytes;
	while(1) {
		if( (bytes = tport_man.simple_recv_msg()) > 0)
			tport_man.simple_send_msg(bytes);
	}
}