#ifndef _H_PGCMP
#define _H_PGCMP

#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/resource.h>

#include "pq.h"
#include "table_diff.h"
#include "util_diff.h"
#include "misc.h"

// commands
#define ADHOC_SQL_CMD        0x00000001
#define GLOBAL_VARS_CMD      0x00000002
#define GLOBAL_PRIVS_CMD     0x00000004
#define PG_HBA_RULES_CMD     0x00000008
#define PG_LS_DIR_CMD        0x00000010
#define PG_READ_FILE_CMD     0x00000020
#define PG_SETTINGS_CMD      0x00000040
#define PG_ROLES_CMD         0x00000080
#define SCHEMA_DEF_CMD       0x00000100
#define SCHEMA_PRIVS_CMD     0x00000200
#define SOURCE_CODE_CMD      0x00000400
#define TABLE_DATA_CMD       0x00000800

#define OBJECT_REQUIRED_CMD \
(TABLE_DATA_CMD | SCHEMA_DEF_CMD | SCHEMA_PRIVS_CMD | SOURCE_CODE_CMD | PG_LS_DIR_CMD | PG_READ_FILE_CMD | ADHOC_SQL_CMD)

#define NO_OBJECT_REQUIRED_CMD \
(GLOBAL_VARS_CMD | GLOBAL_PRIVS_CMD | PG_SETTINGS_CMD | PG_HBA_RULES_CMD | PG_ROLES_CMD)

// processing flags
#define SET_A_TARGET_FLG     0x00000001 // once per file
#define SMALL_TABLE_FLG      0x00000002 // bypass tablesample
#define PRIVS_ONLY_FLG       0x00000004 // dump owners and grants only
#define CREATE_PATCH_FLG     0x00000008 // create table patch

// debug flags
#define DBG_RUSAGE_FLG       0x00010000
#define DBG_TIMING_FLG       0x00020000
#define DBG_VERBOSE_FLG      0x00040000
#define DBG_TRACE_ON_FLG     0x00080000

#define DBG_REPORT_FLG       (DBG_RUSAGE_FLG | DBG_TIMING_FLG | DBG_VERBOSE_FLG) 

#define DEF_SQL_BUF_SZ       2048       // sql statement size
#define DEF_TD_CPUS          2          // cpus for each target
#define DEF_TD_SEGMENTS      1          // number of segments
#define A_TARGET             'a'        // a_target identifier
#define B_TARGET             'b'        // b_target identifier
#define SMALL_BUF_SZ         1024       // generic buffer size 

#define EX_FAIL              1          // compare failed exit code

#define INDENT_LEVEL_SZ      16         // trace function depth depth

// number of array elements
#define ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))  

// logging routines
#define bail(exit_code, ...) \
  fprintf(gv.errfp, __VA_ARGS__); \
  fprintf(gv.errfp, " in file %s near line %d\n", basename(__FILE__), __LINE__); \
exit(exit_code);

#define notice(...) \
  fprintf(gv.errfp, __VA_ARGS__); 

// function trace
#define TRACE_IN(p_indent) \
if (g_flags & DBG_TRACE_ON_FLG) \
{ \
   fprintf(gv.errfp, "%sin  ---> %s %s %5d\n", \
           gv.indent, basename(__FILE__), __func__, __LINE__); \
   if (p_indent != 0) \
   { \
      gv.indent[gv.indent_level++] = '\t'; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
}

#define TRACE_OUT(p_indent) \
if (g_flags & DBG_TRACE_ON_FLG) \
{ \
   if (p_indent != 0) \
   { \
      gv.indent_level--; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
   fprintf(gv.errfp, "%sout ---> %s %s %5d\n", \
           gv.indent, basename(__FILE__), __func__, __LINE__); \
}

// utility stats
typedef struct 
{
  int32_t read_count;                  // total number of reads
  int32_t bytes_read;                  // total bytes read
  int32_t write_count;                 // total number of writes
  int32_t bytes_written;               // total bytes written
  double get_data_et;                  // get data and create a and b files
} ud_stats_t;

// a and b targets
typedef struct
{
  int32_t id;                          // 
  char *conninfo;                      // connection parameters
  char *host;                          // postgres host
  char *port;                          // database port 
  char *dbname;                        // database name
  char *object;                        // schema qualified object name
  pthread_t pthread;                   // thread data
  pq_t pq;
  PGresult *rset;      
  char *buf;
  int32_t rc;                          // return code  
  char *td_nspname;                    // schemaname
  char *td_relname;                    // tablename
  char ud_filename[FILENAME_MAX+1];    // schema and source OS diffs 
  char *ud_sourcename;                 // schema qualified function or procedure name
  char *ud_pipe_buf;                   // pipe buffer
  int32_t ud_err_pipe[2];              // error messages
  int32_t ud_req_pipe[2];              // request pipe
  int32_t ud_rsp_pipe[2];              // response pipe
  ud_stats_t ud_stats;                 // utility stats
  worker_t *worker;                    // worker thread list
} target_t;  

// misc stats
typedef struct
{
  double init_et;                      // total initialization time
  double cleanup_et;                   // total cleanup time
  double diff_et;                      // total diff time
  double response_et;                  // total response time
  double request_et;                   // total request time
} misc_stats_t;

// table stats
typedef struct
{
  double get_table_desc_et;            // pkey and column info
  double create_blocks_et;             // create hash block keys
  double get_block_hash_et;            // get the block hash time
  double cmp_block_hash_et;            // compare targets block hash values
  double get_error_rows_et;            // get target block error rows
  double cmp_error_rows_et;            // compare and locate error rows
} td_stats_t;

// process variables
typedef struct
{
  misc_stats_t misc_stats;
  table_desc_t table_desc;             // 'a' table description
  td_stats_t td_stats;                 // td stats
  char *diff_cmd;                      // OS diff process for utility programs
  char *cfgfile_name;                  // stdin or input filename 
  char indent[INDENT_LEVEL_SZ];        // trace indentation
  char *tmpdir;                        // intermediate files
  char *progname;                      // process name
  FILE *errfp;                         // error output
  FILE *outfp;                         // out file pointer
  FILE *patchfp;                       // table diff patch file
  uint32_t command;                    // command id
  int32_t indent_level;                // trace indentation 
  int32_t a_lineno;                    // config file a_target lineno
  int32_t b_lineno;                    // config file b_target lineno
  int32_t pid;                         // process id
  int32_t sql_buf_sz;                  // sql buffer size
  int32_t td_cpus;                     // cpus per target
  int32_t td_particles;                // additional loops per cpu 
  int32_t td_max_key_len;              // debug report
  int32_t td_error_ix;                 // current 
  td_segment_t td_segment;             // list of blocks
  uint64_t td_error_count;             // number of errors;
  uint64_t td_max_errors;              // stop after count errors
  pthread_mutex_t td_write_mutex;      // for table diff log
  sig_atomic_t signum;                 // interrupt number
} global_var_t;

// global variables
extern target_t g_a_target;            // a_target variables
extern target_t g_b_target;            // b target variables
extern global_var_t gv;                // global variables
extern uint32_t g_flags;               // conditions

// pgcmp routines
extern void init(void);
extern int32_t process(void);
extern void cleanup(int32_t p_done);
#endif
