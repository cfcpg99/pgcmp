#ifndef _H_TABLE_DIFF
#define _H_TABLE_DIFF

#include <pthread.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/resource.h>

#include "td_blocks.h"
#include "td_workers.h"

#define CONKEY_SZ                 2048
#define NOT_FOUND                   -1 
#define NOT_OK                       1
#define OK                           0
#define RELID_SZ                    16
#define INDRELID_SZ                 16
#define SMALL_TABLE_ROWS      SHRT_MAX 
#define TD_NAME_SZ                 128 // schema + table name length
#define BAD_TABLE_DEF    (EX__MAX + 1) // table structure issue

#define DEF_CPU_SEGMENTS             1 // segments per CPU
#define DEF_CPU_SET_SZ               1 // CPUs per target

#define GROUP_SEP                   29 // non-printable primary key and row data separator

// table description
typedef struct                          
{
  char *col_names;                     // column names
  char *col_types;                     // column types
  char conkey[CONKEY_SZ];              // primary key column numbers
  char *pk_col_names;                  // primary key column names
  char *pk_col_types;                  // primary key column types
  char pk_name[TD_NAME_SZ+1];          // primary key name
  char relid[RELID_SZ];                // table relation id
  char *t1_tab_col_alias;              // column names prefixed with table 1 alias
  char *t2_tab_col_alias;              // column names prefixed with table 2 alias
  char *tablesample;                   // method
  double tablesample_pct;              // sample percent
  int32_t ntile;                       // number of samples
  int32_t num_cols;                    // number columns
  int32_t num_pk_cols;                 // number of primary key columns
  uint64_t estimated_rows;             // aprroximate row count
} table_desc_t;

extern int32_t table_diff(void);
extern void free_table_desc(table_desc_t *p_table_desc);
extern void create_small_table_segment(void);
#endif
