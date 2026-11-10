#include "pgcmp.h"
#include "td_setup.h"

extern void mk_table_hash_sql(target_t *p_target);
extern void mk_block_hash_sql(target_t *p_target, int32_t p_range_ix);
static void compare_hash_blocks(void);
static int32_t handle_errors(void);
static void compare_error_rows(int32_t p_first_block_ix, int32_t p_last_block_ix);
static void free_b_worker_vars(int32_t p_num_workers);

/*
   create hash values from blocks of table data on the server
   compare hash values on client
   get error data blocks on hash failure
   compare and log bad or missing rows
*/
int32_t table_diff(void)
{
 int32_t l_rc = 1;
 struct timeval l_start_tv;
 struct timeval l_end_tv;

TRACE_IN(1) 

 if (g_flags & SET_A_TARGET_FLG)
 {
    get_table_desc(&g_a_target, &gv.table_desc);

    check_table_desc();

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_start_tv, NULL);
    }
 
    create_blocks();

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       gv.td_stats.create_blocks_et = get_elapsed_time(&l_start_tv, &l_end_tv);
    }
 }
 else
 {
    check_table_desc();
 }

 compare_hash_blocks();

 if (gv.td_segment.error_block_count == 0)
 {
TRACE_OUT(1)
    return(0);
 }

 l_rc = handle_errors();

TRACE_OUT(1)
  
 return(l_rc);
}

/*
  compare the a and b blocks hash values
*/
static void compare_hash_blocks(void)
{
 int32_t i;
 int32_t j;
 block_t *l_block;
 double l_get_block_hash_et = 0;
 double l_cmp_block_hash_et = 0;
 struct timeval l_start_tv;
 struct timeval l_end_tv;

TRACE_IN(1) 

 i = 0;

 while(i < gv.td_segment.block_count)
 {
    if (g_flags & DBG_TIMING_FLG)
    {  
       gettimeofday(&l_start_tv, NULL);
    }
 
    if (g_flags & SET_A_TARGET_FLG)
    {
       if ((g_flags & SMALL_TABLE_FLG) == 0)
       {
          mk_block_hash_sql(&g_a_target, i);
       }
       else
       {
          mk_table_hash_sql(&g_a_target);
       }
    }

    if ((g_flags & SMALL_TABLE_FLG) == 0)
    {
       mk_block_hash_sql(&g_b_target, i);
    }
    else
    {
       mk_table_hash_sql(&g_b_target);
    }

    if (g_flags & SET_A_TARGET_FLG)
    {
       run_query_workers(gv.td_segment.cpus);
    }
    else
    {
       run_target_workers(&g_b_target, gv.td_segment.cpus);
    }

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       l_get_block_hash_et += get_elapsed_time(&l_start_tv, &l_end_tv);
    }

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_start_tv, NULL);
    }

    l_block = &gv.td_segment.block[i];
    for(j = 0; j < gv.td_segment.cpus; j++)
    {
       if (g_flags & SET_A_TARGET_FLG)
       {
          strcpy(l_block->a_hash, PQgetvalue(g_a_target.worker[j].rset, 0, 0)); 
       }

       strcpy(l_block->b_hash, PQgetvalue(g_b_target.worker[j].rset, 0, 0)); 

       // compare 'a' and 'b' block hash values
       if (strcmp(l_block->a_hash, PQgetvalue(g_b_target.worker[j].rset, 0, 0)))
       {
          gv.td_segment.block[i+j].errors++; 
       }
       l_block++;
    }

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       l_cmp_block_hash_et += get_elapsed_time(&l_start_tv, &l_end_tv);
    }

    for(j = 0; j < gv.td_segment.cpus; j++)
    {
       if (g_flags & SET_A_TARGET_FLG)
       {
          PQclear(g_a_target.worker[j].rset);
       }

       PQclear(g_b_target.worker[j].rset);
    }

    // increment to next batch
    i += gv.td_segment.cpus;
 }

 l_block = gv.td_segment.block;
 for(i = 0, j = 0; i < gv.td_segment.block_count; i++)
 {
    if (l_block->errors != 0)
    {
       // add to list of error blocks
       gv.td_segment.error_list[j]  = i;
       j++;
    }
    l_block++;
 }
  
 gv.td_segment.error_block_count = j;

 if (g_flags & DBG_TIMING_FLG)
 {
    gv.td_stats.get_block_hash_et = l_get_block_hash_et;
    gv.td_stats.cmp_block_hash_et = l_cmp_block_hash_et;
 }

TRACE_OUT(1)
}

/*
  get and compare rows from error blocks
*/
static int32_t handle_errors(void)
{
 int32_t i;
 int32_t l_last = 0;
 double  l_get_error_rows_et  = 0;
 double  l_cmp_error_rows_et  = 0;
 struct timeval l_start_tv;
 struct timeval l_end_tv;
 int32_t l_rc = 1;  

TRACE_IN(1)  
 i = 0;

 // loop through error_blocks by cpu set_size
 while(i < gv.td_segment.block_count) 
 {
    l_last += gv.td_segment.cpus;

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_start_tv, NULL);
    }

    // get error block row data
    if ((g_flags & SMALL_TABLE_FLG) == 0)
    { 
       get_error_blocks(i, l_last );
    }
    else
    {
       get_small_table_error_blocks(i, l_last);
    }

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       l_get_error_rows_et += get_elapsed_time(&l_start_tv, &l_end_tv);
       gettimeofday(&l_start_tv, NULL);
    }

    // block row data check
    compare_error_rows(i, l_last); 

    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       l_cmp_error_rows_et += get_elapsed_time(&l_start_tv, &l_end_tv);
    }

    free_b_worker_vars(l_last - i);

    gv.td_error_ix += gv.td_segment.cpus;

    i += gv.td_segment.cpus; 
 }

 l_rc = 1;

 if (g_flags & DBG_TIMING_FLG)
 {
    gv.td_stats.get_error_rows_et = l_get_error_rows_et;
    gv.td_stats.cmp_error_rows_et = l_cmp_error_rows_et;
 }

TRACE_OUT(1)  

 return(l_rc);
}

/*
   compare a and b error blocks row values
*/
static void compare_error_rows(int32_t p_first_block_ix, int32_t p_last_block_ix)
{
 int32_t l_cpus = p_last_block_ix - p_first_block_ix;
 int32_t i;
 int32_t j;
 
TRACE_IN(1)

 run_get_error_workers(l_cpus);

 gv.td_error_ix = p_first_block_ix;

 run_cmp_error_workers(l_cpus);

 for(i = 0; i < l_cpus; i++)
 {
    if (g_a_target.worker[i].rset != NULL)
    {
        PQclear(g_a_target.worker[i].rset);
    }

    if (g_b_target.worker[i].rset != NULL)
    { 
       PQclear(g_b_target.worker[i].rset);
    }

    g_a_target.worker[i].rset = NULL;
    g_b_target.worker[i].rset = NULL;
 }

 // free key vars
 for(i = 0; i < l_cpus; i++)
 {
    if (g_a_target.worker[i].key_data != NULL)
    {
       for(j = 0; j < g_a_target.worker[i].key_rows; j++)
       {
          if (g_a_target.worker[i].key_data[j].pk_data != NULL)
          {
             free(g_a_target.worker[i].key_data[j].pk_data);
             g_a_target.worker[i].key_data[j].pk_data = NULL;
          }
       }
     
       free(g_a_target.worker[i].key_data);

       g_a_target.worker[i].key_data = NULL;
       g_a_target.worker[i].key_rows = 0;
    }

    if (g_b_target.worker[i].key_data != NULL)
    {
       for(j = 0; j < g_b_target.worker[i].key_rows; j++)
       {
          if (g_b_target.worker[i].key_data[j].pk_data != NULL)
          {
             free(g_b_target.worker[i].key_data[j].pk_data);
             g_b_target.worker[i].key_data[j].pk_data = NULL;
          }
       }

       free(g_b_target.worker[i].key_data);

       g_b_target.worker[i].key_data = NULL;
       g_b_target.worker[i].key_rows = 0;
    }
 }

TRACE_OUT(1)
}

/*
 * free worker 'b' key_data variables
*/
static void free_b_worker_vars(int32_t p_num_workers)
{
 key_data_t *l_b_key_data;
 int32_t i;
 int32_t j;

TRACE_IN(1)

 for(i = 0; i < p_num_workers; i++)
 {
    if ((l_b_key_data = g_b_target.worker[i].key_data) != NULL)
    {
       for(j = 0; j < g_b_target.worker[i].key_rows; j++)
       {
          if (l_b_key_data->pk_data != NULL)
          {
             free(l_b_key_data->pk_data);
          }
          l_b_key_data++;
       }
    }

    free(g_b_target.worker[i].key_data);
    g_b_target.worker[i].key_data = NULL;
 }

 TRACE_OUT(1)
}
