#include "pgcmp.h"
#include "td_setup.h"

static void add_table_alias(void);
static void get_td_approx_rows(target_t *p_target);
static void set_tablesample_pct(void);
static void mk_hash_blocks(void);

/*
   create a list of block start and end primary keys
   small tables are limited to 2 blocks for each target
*/
void create_blocks(void)
{

TRACE_IN(1)

 add_table_alias();
 get_td_approx_rows(&g_a_target);
 set_tablesample_pct();

 mk_hash_blocks();

 if (gv.table_desc.estimated_rows > SMALL_TABLE_ROWS)
 {
    create_segment();
 }
 else
 {
    create_small_table_segment();
 }

TRACE_OUT(1)
}

// //////////////////////////////////////////////////////////////////////////
// source:
//
// # ACE - Active Consistency Engine
//
// Copyright (C) 2023 - 2026, pgEdge (https://www.pgedge.com/)
//
// This software is released under the PostgreSQL License:
// https://opensource.org/license/postgresql
//
// //////////////////////////////////////////////////////////////////////////

/*
  set the percentage of rows scanned by tablesample 
*/
static void set_tablesample_pct(void)
{
TRACE_IN(1)

 // ACE
 gv.table_desc.tablesample     = "bernoulli";
 gv.table_desc.tablesample_pct = 0.0;

 if (gv.table_desc.estimated_rows > 1e8)
 {
    gv.table_desc.tablesample     = "system";
    gv.table_desc.tablesample_pct = 0.01;
 }
 else if (gv.table_desc.estimated_rows > 1e6)
 {
    gv.table_desc.tablesample     = "system";
    gv.table_desc.tablesample_pct = 0.1;
 }
 else if (gv.table_desc.estimated_rows > 1e5)
 {
    gv.table_desc.tablesample_pct = 1;
 }
 else if (gv.table_desc.estimated_rows > 1e4)
 {
    gv.table_desc.tablesample_pct = 10;
 }
 else
 {
    gv.table_desc.tablesample_pct = 100;
 }
 // ACE

TRACE_OUT(1)
}

/*
   create ntile block start and end primary keys
target a:

tablesample:     bernoulli
tablesample_pct: 10.00
iters:           1
cpu:             2
ntile:           2
td_approx_rows:     32767

    block_start | block_end  
-------------+------------
 1000000001  | 1000000009
 1000000009  | 1000016224
 1000016224  | 1000032768
 1000032768  | ¤
(4 rows)
*/

void mk_hash_blocks(void)
{
// ACE
 char *l_block_sql =
"with sampled_data \n"
"as \n"
"(\n"
"  select %s\n"  // pkey
"  from   %s\n"
"         tablesample %s(%f)\n"
"  order by %s\n" 
"),\n"
"first_row \n"
"as \n"
"(\n"
"  select %s\n"
"  from   %s\n"
"  order by %s\n"
"  limit 1\n" 
"),\n" 
"last_row \n"
"as \n"
"(\n"
"  select %s\n"
"  from   %s\n"
"  order by %s desc\n"
"  limit 1\n"
"),\n"
"sample_boundaries \n"
"as \n"
"(\n"
"  select %s,\n"
"         ntile(%d) over (order by %s) as bucket\n" 
"  from   sampled_data\n"
"),\n"
"block_starts\n"
"as \n"
"(\n"
"  select distinct on(bucket) %s\n"
"  from   sample_boundaries\n" 
"  order by bucket, %s\n"
"),\n"
"all_bounds \n"
"as \n"
"(\n"
"  select %s,\n"
"         0 as seq\n"
"  from first_row\n" 
" union all\n"
"  select %s,\n"
"         1 as seq\n"
"  from   block_starts t1,\n"
"         first_row t2\n"
"         where row(%s) > row(%s)\n"
" union all\n"
"  select %s,\n" 
"         2 seq\n"
"  from   last_row\n"
"),\n"
"blocks \n"
"as \n"
"(\n"
"  select %s,\n"
"         row(%s) as block_start,\n"
"         lead(row(%s)) over (order by seq, row(%s)) as block_end,\n" 
"         seq\n"
"  from   all_bounds\n"
")\n"
"select replace(replace(replace(block_start::text, '\"', ''''), '(', ''), ')', '') block_start,\n"
"       replace(replace(replace(block_end::text, '\"', ''''), '(', ''), ')', '') block_end\n"
"from   blocks\n"
"order by seq";
// END ACE

TRACE_IN(1)

 gv.table_desc.ntile = gv.td_segment.cpus * gv.td_segment.particles;

 sprintf(g_a_target.worker[0].sql, l_block_sql, 
         // sampled_data
         gv.table_desc.pk_col_names,
         g_a_target.object,
         gv.table_desc.tablesample,
         gv.table_desc.tablesample_pct,
         gv.table_desc.pk_col_names,
         // first_row
         gv.table_desc.pk_col_names,
         g_a_target.object,
         gv.table_desc.pk_col_names,
         // last_row
         gv.table_desc.pk_col_names,
         g_a_target.object,
         gv.table_desc.pk_col_names,
         // sample_boundries
         gv.table_desc.pk_col_names,
         // ntile
         gv.table_desc.ntile,
         gv.table_desc.pk_col_names,
         // block_starts
         gv.table_desc.pk_col_names, 
         gv.table_desc.pk_col_names, 
         // all_bounds
         gv.table_desc.pk_col_names,
         gv.table_desc.t1_tab_col_alias,
         gv.table_desc.t1_tab_col_alias,
         gv.table_desc.t2_tab_col_alias,
         gv.table_desc.pk_col_names,
         // blocks
         gv.table_desc.pk_col_names,
         gv.table_desc.pk_col_names,
         gv.table_desc.pk_col_names,
         gv.table_desc.pk_col_names);

 if ((g_a_target.worker[0].rset = pq_exec(&g_a_target.worker[0].pq, 
                                          g_a_target.worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec failed\n%s\n%s",
         g_a_target.worker[0].pq.error_msg,
         g_a_target.worker[0].sql);
 } 

TRACE_OUT(1)
}

/*
   return hash value of block data
*/
void mk_block_hash_sql(target_t *p_target, int32_t p_block_ix)
{
int32_t i;
int32_t l_block_ix = p_block_ix;

TRACE_IN(1)

 for(i = 0; i < gv.td_segment.cpus; i++)
 {
     if (l_block_ix == 0)
     {
        // first
        sprintf(p_target->worker[i].sql,
                //"select encode(digest(coalesce(string_agg(_tbl_::text, '|' order by %s), 'EMPTY_BLOCK'), 'sha256'), 'hex')\n"
                "select encode(sha256(coalesce(string_agg(_tbl_::text, e'|' order by %s), 'EMPTY_BLOCK')::bytea), 'hex')\n"
                "from   %s as _tbl_\n"
                "where  row(%s) <= row(%s)\n",
                gv.table_desc.pk_col_names,
                p_target->object,
                gv.table_desc.pk_col_names,
                gv.td_segment.block[l_block_ix].end_key);
     }
     else if (l_block_ix +1 == gv.td_segment.block_count)
     {
        // last
        sprintf(p_target->worker[i].sql,
                // "select encode(digest(coalesce(string_agg(_tbl_::text, '|' order by %s), 'EMPTY_BLOCK'), 'sha256'), 'hex')\n"
                "select encode(sha256(coalesce(string_agg(_tbl_::text, e'|' order by %s), 'EMPTY_BLOCK')::bytea), 'hex')\n"
                "from   %s as _tbl_\n"
                "where  row(%s) > row(%s)\n",
                gv.table_desc.pk_col_names,
                p_target->object,
                gv.table_desc.pk_col_names,
                gv.td_segment.block[l_block_ix].start_key);
     }
     else
     {
        // next
        sprintf(p_target->worker[i].sql,
                //"select encode(digest(coalesce(string_agg(_tbl_::text, '|' order by %s), 'EMPTY_BLOCK'), 'sha256'), 'hex')\n"
                "select encode(sha256(coalesce(string_agg(_tbl_::text, e'|' order by %s), 'EMPTY_BLOCK')::bytea), 'hex')\n"
                "from   %s as _tbl_\n"
                "where  row(%s) >= row(%s)\n"
                "and    row(%s) <= row(%s)\n",
                gv.table_desc.pk_col_names,
                p_target->object,
                gv.table_desc.pk_col_names,
                gv.td_segment.block[l_block_ix].start_key,
                gv.table_desc.pk_col_names,
                gv.td_segment.block[l_block_ix].end_key);
     }
     l_block_ix++;
 }

TRACE_OUT(1)
}

/*
   return small table data hash values 
*/
void mk_table_hash_sql(target_t *p_target)
{
TRACE_IN(1)

 sprintf(p_target->worker[0].sql,
         "select encode(sha256(coalesce(string_agg(_tbl_::text, e'|' order by %s), 'EMPTY_BLOCK')::bytea), 'hex')\n"
         "from  %s as _tbl_",
         gv.table_desc.pk_col_names,
         p_target->object);

 TRACE_OUT(1);
}

/*
   create and execute sql to return primary key and row data from error blocks 
*/
void get_error_blocks(int32_t p_first_block_ix, int32_t p_last_block_ix)
{
 int32_t i;
 int32_t j = 0;
 int32_t l_reps;

TRACE_IN(1)

 for(i = p_first_block_ix; i < p_last_block_ix; i++)
 {
   if (i == 0)
   {
      // first  block
      sprintf(g_a_target.worker[j].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) <= row(%s)", 
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_a_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].end_key);
  
      sprintf(g_b_target.worker[j++].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) <= row(%s)",
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_b_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].end_key);
   }
   else if (i +1 == gv.td_segment.block_count) 
   {
      // last block
      sprintf(g_a_target.worker[j].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) > row(%s)",
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_a_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].start_key);
      
      sprintf(g_b_target.worker[j++].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) > row(%s)",
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_b_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].start_key);
   }
   else
   {  
      // next_block
      sprintf(g_a_target.worker[j].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) >= row(%s) and row(%s) < row(%s)",
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_a_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].start_key,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].end_key);
 
      sprintf(g_b_target.worker[j++].sql, 
              "select row(%s) pk, row(%s) data from %s where row(%s) >= row(%s) and row(%s) < row(%s)",
              gv.table_desc.pk_col_names,
              gv.table_desc.col_names,
              g_b_target.object,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].start_key,
              gv.table_desc.pk_col_names,
              gv.td_segment.block[i].end_key);
   }
 }

 l_reps = p_last_block_ix - p_first_block_ix;

 run_query_workers(l_reps);

 j = 0;
 for(i = p_first_block_ix; i < p_last_block_ix; i++)
 {
    gv.td_segment.block[gv.td_segment.error_list[i]].a_block_rows = g_a_target.worker[j].num_tuples;
    gv.td_segment.block[gv.td_segment.error_list[i]].b_block_rows = g_b_target.worker[j].num_tuples;

    j++;
 }

 TRACE_OUT(1)
}

/*
   create and execute sql to return primary key and row data from small table error blocks 
*/
void get_small_table_error_blocks(int32_t p_first_block_ix, int32_t p_last_block_ix)
{
 int32_t l_reps = p_last_block_ix - p_first_block_ix;
 int32_t i;
 int32_t j = 0;

TRACE_IN(1)

 // first block
 sprintf(g_a_target.worker[0].sql, "select row(%s) pk, row(%s) data from %s",
         gv.table_desc.pk_col_names,
         gv.table_desc.col_names,
         g_a_target.object);

 sprintf(g_b_target.worker[0].sql, "select row(%s) pk, row(%s) data from %s",
         gv.table_desc.pk_col_names,
         gv.table_desc.col_names,
         g_b_target.object);

 run_query_workers(gv.td_segment.cpus);

 j = 0;
 for(i = p_first_block_ix; i < p_first_block_ix + l_reps; i++)
 {
    gv.td_segment.block[gv.td_segment.error_list[i]].a_block_rows = g_a_target.worker[j].num_tuples;
    gv.td_segment.block[gv.td_segment.error_list[i]].b_block_rows = g_b_target.worker[j++].num_tuples;
 }
 
TRACE_OUT(1)

 return;
}  

/*
   prefix columns with table aliases t1 and t2
*/
static void add_table_alias(void)
{
 char l_buf[SMALL_BUF_SZ];
 char *l_cptr;
 char *l_cptr2;
 int32_t l_len;

TRACE_IN(1)

 if ((gv.table_desc.t1_tab_col_alias = calloc(gv.table_desc.num_pk_cols, PQ_NAME_SZ+4)) == NULL)
 {
    bail(EX_OSERR, "calloc table_desc.t1_tab_col_alias failed %s", strerror(errno));
 }

 if ((gv.table_desc.t2_tab_col_alias = calloc(gv.table_desc.num_pk_cols, PQ_NAME_SZ+4)) == NULL)
 {
    bail(EX_OSERR, "calloc table_desc.t2_tab_col_alias failed %s", strerror(errno));
 }

 // add table 1 alias to a
 strcpy(l_buf, gv.table_desc.pk_col_names);
 if (strchr(l_buf, ',') == NULL)
 {
    sprintf(gv.table_desc.t1_tab_col_alias, "t1.%s", l_buf);
 }
 else
 {
    l_len = 0;
    l_cptr = l_buf;
    while(l_cptr != NULL)
    {
       if ((l_cptr2 = strchr(l_cptr, ',')) != NULL)
       {
          *l_cptr2 = '\0';
          l_len += sprintf(gv.table_desc.t1_tab_col_alias+l_len, "t1.%s,", l_cptr);
          l_cptr2++;
          l_cptr = l_cptr2;
       }
       else
       {
          sprintf(gv.table_desc.t1_tab_col_alias+l_len, "t1.%s", l_cptr);
          break;
       }
    }
 }

 // add table 2 alias to b
 strcpy(l_buf, gv.table_desc.pk_col_names);
 if (strchr(l_buf, ',') == NULL)
 {
    sprintf(gv.table_desc.t2_tab_col_alias, "t2.%s", l_buf);
 }
 else
 {
    l_len = 0;
    l_cptr = l_buf;
    while(l_cptr != NULL)
       {
       if ((l_cptr2 = strchr(l_cptr, ',')) != NULL)
       {
          *l_cptr2 = '\0';
          l_len += sprintf(gv.table_desc.t2_tab_col_alias+l_len, "t2.%s,", l_cptr);
          l_cptr2++;
          l_cptr = l_cptr2;
       }
       else
       {
          sprintf(gv.table_desc.t2_tab_col_alias+l_len, "t2.%s", l_cptr);
          break;
       }
    }
 }

TRACE_OUT(1)
}

/*
  get analyzed row counts or make one up
*/
static void get_td_approx_rows(target_t *p_target)
{

 char *l_explain_sql =
"explain\n"
"select 1\n"
"from %s.%s";
 char *l_cptr;
 char *l_stop;

TRACE_IN(1)

 gv.table_desc.estimated_rows = 0;
 sprintf(p_target->worker[0].sql, l_explain_sql, p_target->td_nspname, p_target->td_relname);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec failed '%s'\n%s\n%s",
         p_target->worker[0].pq.conn_info,
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 if ((l_cptr = strstr(PQgetvalue(p_target->worker[0].rset, 0, 0), "rows=")) != NULL)
 {
    l_cptr += 5;
    gv.table_desc.estimated_rows  = strtoul(l_cptr, &l_stop, 10);
 }
 
 PQclear(p_target->worker[0].rset);

 if (gv.table_desc.estimated_rows <= 0)
 {
    notice("a %s.%s td_approx_rows == %s\n",
           p_target->td_nspname, p_target->td_relname, PQgetvalue(p_target->worker[0].rset, 0, 0));
    exit(EX_SOFTWARE);
 }

TRACE_OUT(1)
}
