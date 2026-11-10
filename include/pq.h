#ifndef _H_PQ
#define _H_PQ

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/time.h>

#include <libpq-fe.h>

#define PQ_NAME_SZ             ((64*2)+1) // schemaname.tablename 

typedef struct 
{
  PGconn *conn;
  FILE *errfp;
  char *conn_info;
  char *error_msg;
  char *notice_msg;
  int32_t error;
  int32_t error_count;
  int32_t notice;
  int32_t notice_count;
  int32_t status;
} pq_t;

extern int32_t pq_connect(pq_t *p_pq);
extern PGresult *pq_exec(pq_t *p_pq, const char *p_sql);
extern void pq_finish(pq_t *p_pq);
#endif
