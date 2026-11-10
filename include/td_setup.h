#ifndef _H_TD_SETUP
#define _H_TD_SETUP

#include "pgcmp.h"

// table_diff setup routines
extern void get_table_desc(target_t *p_target, table_desc_t *p_table_desc);
extern void check_table_desc(void);
extern void mk_block_hash_sql(target_t *p_target, int32_t p_block_ix);
extern void mk_table_hash_sql(target_t *p_target);
#endif
