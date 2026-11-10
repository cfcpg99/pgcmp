#ifndef _H_UTIL_DIFF
#define _H_UTIL_DIFF

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define PIPE_BUF_SZ  2048 // IPC request/response buffer size
#define READ_PIPE       0 // stdin fileno
#define WRITE_PIPE      1 // stdout fileno
#define ERROR_PIPE      2 // stderr fileno

#define DEF_CMP_PROG "diff --ignore-all-space" // default diff program

extern int32_t ud_adhoc_sql(void);
extern int32_t ud_file(void);
extern int32_t ud_global_privs(void);
extern int32_t ud_global_vars(void);
extern int32_t ud_hba_rules(void);
extern int32_t ud_roles(void);
extern int32_t ud_schema_def(void);
extern int32_t ud_schema_privs(void);
extern int32_t ud_settings(void);
extern int32_t ud_source_code(void);

extern void *pg_dumpall_req(void *p_arg);
extern void *pg_dumpall_rsp(void *p_arg);
extern void *pg_dump_req(void *p_arg);
extern void *pg_dump_rsp(void *p_arg);
extern void *psql_req(void *p_arg);
extern void *psql_rsp(void *p_arg);

extern void pg_get_data(void *(*p_routine)(void *));
extern void sig_pipe(int32_t p_signo);
#endif
