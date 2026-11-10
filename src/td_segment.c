#include "pgcmp.h"

/*
   intialize segment
*/
void create_segment(void)
{
 int32_t l_len;
 int32_t i;

TRACE_IN(1)

 gv.td_segment.block_count = gv.td_segment.cpus * gv.td_segment.particles;

 if ((gv.td_segment.block = calloc(gv.td_segment.block_count, sizeof(block_t))) == NULL) 
 {
    bail(EX_OSERR, "calloc block failed %s", strerror(errno));
 }

 if ((gv.td_segment.error_list = calloc(gv.td_segment.block_count, sizeof(int32_t))) == NULL)
 {
    bail(EX_OSERR, "calloc error_list failed %s", strerror(errno));
 } 

 for(i = 0; i < gv.td_segment.block_count; i++)
 {
    if (i == 0)
    {
       // first
       gv.td_segment.block[i].start_key = strdup(" ");
       gv.td_segment.block[i].end_key   = strdup(PQgetvalue(g_a_target.worker[0].rset, i, 1));
    }
    else if (i + 1 == gv.td_segment.block_count)
    {
       // last
       gv.td_segment.block[i].start_key = strdup(PQgetvalue(g_a_target.worker[0].rset, i, 0));
       gv.td_segment.block[i].end_key   = strdup(" ");
    } 
    else
    {
       // next
       gv.td_segment.block[i].start_key = strdup(PQgetvalue(g_a_target.worker[0].rset, i, 0)); 
       gv.td_segment.block[i].end_key   = strdup(PQgetvalue(g_a_target.worker[0].rset, i, 1));
    }

    if (g_flags & DBG_VERBOSE_FLG)
    {
       if ((l_len = strlen(gv.td_segment.block[i].start_key)) > gv.td_max_key_len)
       {
          gv.td_max_key_len = l_len;
       }

       if ((l_len = strlen(gv.td_segment.block[i].end_key)) > gv.td_max_key_len)
       {
          gv.td_max_key_len = l_len;
       }
    }
 }
 PQclear(g_a_target.worker[0].rset);
 
TRACE_OUT(1)
}

/* 
  small table segment is 1 block
*/
void create_small_table_segment(void)
{

TRACE_IN(1)
  
 // free failed mk_segments query
 PQclear(g_a_target.worker[0].rset);

 g_flags |= SMALL_TABLE_FLG;
 gv.td_segment.cpus      = 1;
 gv.td_segment.particles = 1;

 // same blocks for a and b targets
 gv.td_segment.block_count = 1; 
 if ((gv.td_segment.block = calloc(gv.td_segment.block_count, sizeof(block_t))) == NULL) 
 {
    bail(EX_OSERR, "calloc block failed %s", strerror(errno));
 }

 if ((gv.td_segment.error_list = calloc(gv.td_segment.block_count, sizeof(int32_t))) == NULL)
 {
    bail(EX_OSERR, "calloc error_list failed %s", strerror(errno));
 }
 
TRACE_OUT(1)
}

/* 
  dealloc segment memory
*/
void free_segment(void)
{
int32_t i;

TRACE_IN(1)

 for(i = 0; i < gv.td_segment.block_count; i++)
 {
    if (gv.td_segment.block[i].start_key != NULL)
    {
       free(gv.td_segment.block[i].start_key);
       gv.td_segment.block[i].start_key = NULL;
    }

    if (gv.td_segment.block[i].end_key != NULL)
    {
       free(gv.td_segment.block[i].end_key);
       gv.td_segment.block[i].end_key = NULL;
    }
 }

 if (gv.td_segment.error_list != NULL)
 {
    free(gv.td_segment.error_list);
    gv.td_segment.error_list = NULL;
 }

 if (gv.td_segment.block != NULL)
 {
    free(gv.td_segment.block);
    gv.td_segment.block  = NULL;
 }

TRACE_OUT(1)
}
