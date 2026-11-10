#ifndef _H_TD_BLOCKS
#define _H_TD_BLOCKS

#define TD_HASH_SZ          (64+1)   // length of block hash value

typedef struct                       // table diff
{
  char a_hash[TD_HASH_SZ];                   
  char b_hash[TD_HASH_SZ];                   
  char *end_key;                     // block end key
  char *start_key;                   // block start key
  int32_t a_block_rows;              // a row count
  int32_t a_not_found;               // exists in a not b
  int32_t b_block_rows;              // b row count
  int32_t b_not_found;               // exists in b not a
  int32_t errors;                    // error count
  int32_t not_ok;                    // fail count
  int32_t ok;                        // success count
} block_t;

typedef struct
{
  block_t *block;                    // array of blocks
  int32_t block_count;               // size of block list
  int32_t cpus;                      // table_diff initial number of cpus for each target
  int32_t error_block_count;         // blocks with errors
  int32_t *error_list;               // list of block containing errors
  int32_t particles;                 // table_diff initial blocks per cpu
} td_segment_t;                    

extern void create_blocks(void);
extern void create_segment(void);
extern void get_error_blocks(int32_t p_range_ix, int32_t p_jobs);
extern void get_small_table_error_blocks(int32_t p_first_block_ix, 
                                         int32_t p_last_block_ix);
extern void free_segment(void);
#endif
