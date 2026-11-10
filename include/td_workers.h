#ifndef _H_TD_WORKERS
#define _H_TD_WORKERS

#include "pq.h"

typedef struct
{
  char *pk_data;                     // primary key and data
  int32_t ok;                        // comparision result
} key_data_t;

typedef struct                       // table_diff
{
  pthread_t pthread;                 // thread data
  pq_t pq;                           // postgres connection
  PGresult *rset;                    // query result set
  char *end_key;                     // query end block
  char *sql;                         // query
  char *start_key;                   // query start block
  double elapsed_time;               // run time
  int32_t error_ix;                  // thread error_list index
  int32_t id;                        // worker_id
  int32_t key_rows;                  // key rows per block
  int32_t num_fields;                // number of columns
  int32_t num_tuples;                // rows returned
  int32_t tid;                       // target id
  key_data_t *key_data;              // key prefixed data 
  struct timeval end_tv;             // job end time
  struct timeval start_tv;           // job start time
} worker_t;

extern int32_t g_max_key_len;

extern void run_query_workers(int32_t p_num_jobs);
extern void run_target_workers(void *p_target, int32_t p_num_jobs);
extern void run_get_error_workers(int32_t p_num_jobs);
extern void run_cmp_error_workers(int32_t p_num_jobs);
#endif
