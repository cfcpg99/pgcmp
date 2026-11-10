#include "pgcmp.h"
#include "td_patch.h"

static void free_target(target_t *p_target);
static void disconnect_workers(target_t *p_target);

/*
   close postgres connections and free memory
*/
void cleanup(int32_t p_done)
{
 int32_t i;
 struct timeval l_start_tv;
 struct timeval l_end_tv;
 int32_t l_rc;

TRACE_IN(1)

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_start_tv, NULL);
 }

 // input EOF
 if (p_done == 1)
 {
    if (g_flags & CREATE_PATCH_FLG)
    {
       td_disconnect_patch(&g_a_target);
       fclose(gv.patchfp);
    }

    free_target(&g_a_target);
    g_a_target.pq.conn_info = NULL; 
    free_table_desc(&gv.table_desc);
 }
 
 free_target(&g_b_target);

 if (gv.command == TABLE_DATA_CMD)
 {
    for(i = 0; i < gv.td_segment.block_count; i++)
    {
       gv.td_segment.block[i].a_block_rows = 0;
       gv.td_segment.block[i].a_not_found  = 0;
       gv.td_segment.block[i].b_block_rows = 0;
       gv.td_segment.block[i].b_not_found  = 0;
       gv.td_segment.block[i].errors       = 0;
       gv.td_segment.block[i].not_ok       = 0;
       gv.td_segment.block[i].ok           = 0;
    }

    if (gv.patchfp != NULL && p_done == 0)
    {
       fprintf(gv.patchfp, "rollback;\n");
       fprintf(gv.patchfp, "EOF\n");
       fprintf(gv.patchfp, "\n");
    }

    if (gv.td_segment.block_count > 0)
    {
       gv.td_segment.error_block_count = 0;
       memset(gv.td_segment.error_list, '\0', (gv.td_segment.block_count) * sizeof(int32_t));
    }

    // input EOF
    if (p_done == 1)
    {
       free_segment();

       if ((l_rc = pthread_mutex_destroy(&gv.td_write_mutex)) != 0) 
       {
          bail(EX_OSERR, "pthread_mutex_destroy failed %s", strerror(l_rc));
       }
    }
 }

 // first a and b targets have been compared
 if (g_flags & SET_A_TARGET_FLG)
 {
    // process remaining 'b' targets
    g_flags ^= SET_A_TARGET_FLG;
 }

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_end_tv, NULL);
    gv.misc_stats.cleanup_et = get_elapsed_time(&l_start_tv, &l_end_tv);
 }

TRACE_OUT(1)
}

/* 
  free target variables
*/
static void free_target(target_t *p_target)
{

TRACE_IN(1)

 if (p_target->td_nspname != NULL)
 {
    free(p_target->td_nspname);
    p_target->td_nspname = NULL;
 }

 if (p_target->td_relname != NULL)
 {
    free(p_target->td_relname);
    p_target->td_relname = NULL;
 }

 if (p_target->object != NULL) 
 {
    free(p_target->object);
    p_target->object  = NULL;
 }

 if (p_target->ud_sourcename != NULL)
 {
    free(p_target->ud_sourcename);
    p_target->ud_sourcename = NULL;
 }

 if (gv.command == TABLE_DATA_CMD)
 {
    gv.td_error_ix = 0;
 }

 if (p_target->worker != NULL)
 {
    disconnect_workers(p_target);
 }

 if (p_target->conninfo != NULL)
 {
    free(p_target->conninfo);
    p_target->conninfo = NULL;
 }

 if (gv.command != TABLE_DATA_CMD)
 {
    if (p_target->ud_pipe_buf != NULL) 
    {
       free(p_target->ud_pipe_buf);
       p_target->ud_pipe_buf = NULL;
    }
 }

 if (p_target->worker != NULL)
 {
    free(p_target->worker);
    p_target->worker = NULL;
 }

TRACE_OUT(1)
}

/*
   free table definition variables
*/
void free_table_desc(table_desc_t *p_table_desc)
{

TRACE_IN(1)

 if (p_table_desc->col_names != NULL)
 { 
    free(p_table_desc->col_names);
    p_table_desc->col_names = NULL;
 }

 if (p_table_desc->col_types != NULL)
 {
    free(p_table_desc->col_types);
    p_table_desc->col_types = NULL;
 }

 if (p_table_desc->pk_col_names != NULL) 
 {
    free(p_table_desc->pk_col_names);
    p_table_desc->pk_col_names = NULL;
 }

 if (p_table_desc->pk_col_types != NULL)
 {
    free(p_table_desc->pk_col_types);
    p_table_desc->pk_col_types = NULL;
 }

 if (p_table_desc->t1_tab_col_alias != NULL)
 {
    free(p_table_desc->t1_tab_col_alias);
    p_table_desc->t1_tab_col_alias = NULL;
 }

 if (p_table_desc->t2_tab_col_alias != NULL)
 {
    free(p_table_desc->t2_tab_col_alias);
    p_table_desc->t2_tab_col_alias = NULL;
 }

TRACE_OUT(1)
}

/*
   disconnect from postgres
*/
void disconnect_workers(target_t *p_target)
{
 int32_t i;

TRACE_IN(1) 

 for(i = 0; i < gv.td_cpus; i++)
 {
    if (p_target->worker[i].pq.conn != NULL)
    {
       pq_finish(&p_target->worker[i].pq);
       p_target->worker[i].pq.conn = NULL;
    }

    if (p_target->worker[i].sql != NULL)
    {
       free(p_target->worker[i].sql);
       p_target->worker[i].sql = NULL;
    }

    if (p_target->worker[i].pq.error_msg != NULL) 
    {
       free(p_target->worker[i].pq.error_msg);
       p_target->worker[i].pq.error_msg = NULL;
    }

    if (p_target->worker[i].pq.notice_msg != NULL)
    {
       free(p_target->worker[i].pq.notice_msg);
       p_target->worker[i].pq.notice_msg = NULL;      
    }
 }

TRACE_OUT(1)
}
