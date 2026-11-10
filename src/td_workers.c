#include "pgcmp.h"
#include "td_patch.h"

// worker trace functions
#define WTRACE_IN(p_indent) \
if (g_flags & DBG_TRACE_ON_FLG) \
{ \
   if (p_indent == 2) \
   { \
      gv.indent[gv.indent_level++] = '\t'; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
   fprintf(gv.errfp, "%sin  ---> %c[%d] %s %s %5d\n", \
           gv.indent, l_worker->tid, l_worker->id, basename(__FILE__), __func__, __LINE__); \
   if (p_indent > 0 ) \
   { \
      gv.indent[gv.indent_level++] = '\t'; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
}

#define WTRACE_OUT(p_indent) \
if (g_flags & DBG_TRACE_ON_FLG) \
{ \
   if (p_indent != 0) \
   { \
      gv.indent_level--; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
   fprintf(gv.errfp, "%sout ---> %c[%d] %s %s %5d\n", \
           gv.indent, l_worker->tid, l_worker->id, basename(__FILE__), __func__, __LINE__); \
   if (p_indent == 2) \
   { \
      gv.indent_level--; \
      gv.indent[gv.indent_level] = '\0'; \
   } \
}

static void *query(void *p_arg);
static void *get_a_key_data(void *p_arg);
static void *get_b_key_data(void *p_arg);
static void *cmp_a_key_data(void *p_arg);
static void *cmp_b_key_data(void *p_arg);
static void write_report(int32_t p_target_id,
                         worker_t *p_worker, 
                         const char *p_keydata1, 
                         const char *p_keydata2);
static void write_line(const char *p_keydata);
static void heading(void);
static int32_t pkey_cmp(const void *p_1, const void *p_2);

/*
  run same task on a and b
*/
void run_query_workers(int32_t p_num_jobs)
{
 int32_t i;
 int32_t l_rc = -1;

TRACE_IN(1)

 for(i = 0; i < p_num_jobs; i++)
 {
    g_a_target.worker[i].tid = g_a_target.id;
    g_a_target.worker[i].id = i;

    g_a_target.worker[i].error_ix = gv.td_error_ix + i;
    
    if ((l_rc = pthread_create(&g_a_target.worker[i].pthread,
                               NULL,
                               &query,
                               &g_a_target.worker[i])) != 0)
    {
       bail(EX_OSERR, "pthread_create thread a failed %s", strerror(l_rc));
    }
   
    g_b_target.worker[i].tid = g_b_target.id;
    g_b_target.worker[i].id = i;

    g_b_target.worker[i].error_ix = gv.td_error_ix + i;
    if ((l_rc = pthread_create(&g_b_target.worker[i].pthread,
                               NULL,
                               &query,
                               &g_b_target.worker[i])) != 0)
    { 
       bail(EX_OSERR, "pthread_create thread b failed %s", strerror(l_rc));
    }
 }

 for(i = 0; i < p_num_jobs; i++)
 {
     if ((l_rc = pthread_join(g_a_target.worker[i].pthread, NULL)) != 0)
     {
        bail(EX_OSERR, "pthread_join a worker[%d] failed %s", i, strerror(l_rc));
     }

     if ((l_rc = pthread_join(g_b_target.worker[i].pthread, NULL)) != 0)
     {
        bail(EX_OSERR, "pthread_join b worker[%d] failed %s", i, strerror(l_rc));
     }
 }

TRACE_OUT(1)
}

/*
  run task on target_b using target 'a' variables 
*/
void run_target_workers(void *p_target, int32_t p_num_jobs)
{
 target_t *l_target = (target_t *) p_target;
 worker_t *l_worker = l_target->worker;
 int32_t i;
 int32_t l_rc = -1;

TRACE_IN(1)

 for(i = 0; i < p_num_jobs; i++)
 {
    /* 
    l_target->worker[i].tid = p_target->id;
    l_target->worker[i].id = i;

    l_target->worker[i].error_ix = gv.td_error_ix + i;
    */

    l_worker->tid = l_target->id;
    l_worker->id = i;

    l_worker->error_ix = gv.td_error_ix + i;

    if ((l_rc = pthread_create(&l_target->worker[i].pthread,
                               NULL,
                               &query, 
                               l_worker)) != 0)
                               //&l_target->worker[i])) != 0)
    {
       bail(EX_OSERR, "pthread_create target '%c' worker[%d] failed %s", l_target->id, i, strerror(l_rc));
    }
    l_worker++;
 }

 l_worker = l_target->worker;
 for(i = 0; i < p_num_jobs; i++)
 {
     //if ((l_rc = pthread_join(l_target->worker[i].pthread, NULL)) != 0)
     if ((l_rc = pthread_join(l_worker->pthread, NULL)) != 0)
     {
        bail(EX_OSERR, "pthread_join target '%c' worker[%d] failed %s", l_target->id, i, strerror(l_rc));
     }
     l_worker++;
 }

 l_rc = EX_OK;

TRACE_OUT(1)
}

/* 
   run workers concurrently
*/
void run_get_error_workers(int32_t p_num_jobs)
{
 worker_t *l_a_worker;
 worker_t *l_b_worker;
 int32_t i;
 int32_t l_rc = -1;

TRACE_IN(1)

 l_a_worker = g_a_target.worker;
 l_b_worker = g_b_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    l_a_worker->tid = g_a_target.id;
    l_a_worker->id = i;
    l_a_worker->error_ix = gv.td_error_ix + i;

    if ((l_rc = pthread_create(&l_a_worker->pthread,
                               NULL,
                               &get_a_key_data,
                               l_a_worker)) != 0)
    {
       bail(EX_OSERR, "pthread_create thread a failed %s", strerror(l_rc));
    }

    l_b_worker->tid = g_b_target.id;
    l_b_worker->id = i;
    l_b_worker->error_ix = gv.td_error_ix + i; 
    if ((l_rc = pthread_create(&l_b_worker->pthread,
                               NULL,
                               &get_b_key_data,
                               l_b_worker)) != 0)
    {
       bail(EX_OSERR, "pthread_create thread b failed %s", strerror(l_rc));
    }
   
    l_a_worker++;
    l_b_worker++;
 }

 l_a_worker = g_a_target.worker;
 l_b_worker = g_b_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    if ((l_rc = pthread_join(l_a_worker->pthread, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_join a worker[%d] failed %s", i, strerror(l_rc));
    }

    if ((l_rc = pthread_join(l_b_worker->pthread, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_join b worker[%d] failed %s", i, strerror(l_rc));
    }

    l_a_worker++;
    l_b_worker++;
 }

TRACE_OUT(1)
}


/*
  run diffs on 'a 'target then 'b' target consecutively
*/
void run_cmp_error_workers(int32_t p_num_jobs)
{
 worker_t *l_worker;
 int32_t i;
 int32_t l_rc = 0;

TRACE_IN(1)

 l_worker = g_a_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    l_worker->tid = g_a_target.id;
    l_worker->id = i;
                        
    if ((l_rc = pthread_create(&l_worker->pthread,
                               NULL,
                               &cmp_a_key_data,
                               l_worker)) != 0)
    {
       bail(EX_OSERR, "pthread_create thread a failed %s", strerror(l_rc));
    }
    l_worker++;
 }

 l_worker = g_a_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    if ((l_rc = pthread_join(l_worker->pthread, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_join a worker[%d] failed %s", i, strerror(l_rc));
    }

    l_worker++;
 }

 l_worker = g_b_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    l_worker->tid = g_b_target.id;
    l_worker->id = i;
                        
    if ((l_rc = pthread_create(&l_worker->pthread,
                               NULL,
                               &cmp_b_key_data,
                               l_worker)) != 0)
    {
       bail(EX_OSERR, "pthread_create thread a failed %s", strerror(l_rc));
    }
     
    l_worker++;
 }

 l_worker = g_b_target.worker;
 for(i = 0; i < p_num_jobs; i++)
 {
    if ((l_rc = pthread_join(l_worker->pthread, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_join a worker[%d] failed %s", i, strerror(l_rc));
    }
    l_worker++;
 }

TRACE_OUT(1)
}

/*
   postgres query threada
*/
static void *query(void *p_arg)
{
  worker_t *l_worker = (worker_t *) p_arg;

WTRACE_IN(0)

  gettimeofday(&l_worker->start_tv, NULL);

  if ((l_worker->rset = pq_exec(&l_worker->pq, l_worker->sql)) == NULL)
  {
     bail(EX_SOFTWARE, "query_worker[%d] pq_exec failed\n%s\n'%s'",
          l_worker->id,
          l_worker->pq.error_msg,
          l_worker->sql);
  }

  l_worker->num_tuples = PQntuples(l_worker->rset);
  l_worker->num_fields = PQnfields(l_worker->rset);

  gettimeofday(&l_worker->end_tv, NULL);
  l_worker->elapsed_time += get_elapsed_time(&l_worker->start_tv, &l_worker->end_tv);

WTRACE_OUT(0)

  pthread_exit(NULL);
}

/*
   create key_data from target a worker results
*/
static void *get_a_key_data(void *p_arg)
{
 worker_t *l_worker = (worker_t *) p_arg;
 key_data_t *l_a_key_data;
 char *l_cptr;
 int32_t l_len;
 int32_t i;

WTRACE_IN(0)

 if ((l_worker->key_data = calloc(l_worker->num_tuples, sizeof(key_data_t))) == NULL)
 {
    bail(EX_OSERR, "calloc worker %d a_key rows %d failed %s", 
         l_worker->id, l_worker->num_tuples, strerror(errno));
 }

 l_a_key_data = l_worker->key_data;
 for(i = 0; i < l_worker->num_tuples; i++)
 {
    l_len = PQgetlength(l_worker->rset, i, 0) + PQgetlength(l_worker->rset, i, 1) +2;

    if ((l_a_key_data->pk_data = calloc(l_len, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc l_a_key_data->pk_data failed %s", strerror(errno));
    }

    memcpy(l_a_key_data->pk_data,  PQgetvalue(l_worker->rset, i, 0), PQgetlength(l_worker->rset, i, 0));
    l_cptr = l_a_key_data->pk_data + PQgetlength(l_worker->rset, i, 0);
    *l_cptr = GROUP_SEP;
    l_cptr++;
    memcpy(l_cptr, PQgetvalue(l_worker->rset, i, 1), PQgetlength(l_worker->rset, i, 1));

    l_a_key_data++;
 }

 l_worker->key_rows = l_worker->num_tuples;

WTRACE_OUT(0)

 pthread_exit(NULL);
}

/*
   create key_data from target b worker results
*/
static void *get_b_key_data(void *p_arg)
{
 worker_t *l_worker = (worker_t *) p_arg;
 key_data_t *l_b_key_data;
 char *l_cptr;
 int32_t l_len;
 int32_t i;

WTRACE_IN(0) 

 if ((l_worker->key_data = calloc(l_worker->num_tuples, sizeof(key_data_t))) == NULL)
 {
    bail(EX_OSERR, "calloc worker %d b_key rows %d failed %s",  
         l_worker->id, l_worker->num_tuples, strerror(errno));
 }
    
 l_b_key_data = l_worker->key_data;
 for(i = 0; i < l_worker->num_tuples; i++)
 {
    l_len = PQgetlength(l_worker->rset, i, 0) + PQgetlength(l_worker->rset, i, 1) +2;

    if ((l_b_key_data->pk_data = calloc(l_len, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc l_b_key_data->pk_data failed %s", strerror(errno));
    }

    memcpy(l_b_key_data->pk_data,  PQgetvalue(l_worker->rset, i, 0), PQgetlength(l_worker->rset, i, 0));
    l_cptr = l_b_key_data->pk_data + PQgetlength(l_worker->rset, i, 0);
    *l_cptr = GROUP_SEP;
    l_cptr++;
    memcpy(l_cptr,  PQgetvalue(l_worker->rset, i, 1), PQgetlength(l_worker->rset, i, 1));

    l_b_key_data++;
 }

 l_worker->key_rows = l_worker->num_tuples;

WTRACE_OUT(0)

 pthread_exit(NULL);
}

/*
  compare a to b
*/
static void *cmp_a_key_data(void *p_arg)
{
 worker_t *l_worker = (worker_t *) p_arg;
 block_t *l_error_block;
 key_data_t *l_a_key_data;
 key_data_t *l_b_key_data;
 key_data_t l_key_data;

 int32_t i;

WTRACE_IN(0)

 l_error_block = &gv.td_segment.block[gv.td_segment.error_list[l_worker->error_ix]];
 l_a_key_data = l_worker->key_data;

 qsort(g_b_target.worker[l_worker->id].key_data,
       g_b_target.worker[l_worker->id].key_rows,
       sizeof(key_data_t), 
       pkey_cmp);

 //fprintf(stderr, "A l_worker->num_tuples  %d gv.td_error_count %lu gv.td_max_errors+1 %lu\n",
 //        l_worker->num_tuples,  gv.td_error_count,  gv.td_max_errors);

 for(i = 0; i < l_worker->num_tuples && gv.td_error_count < gv.td_max_errors; i++)
 {
    l_key_data.pk_data = l_a_key_data->pk_data;

    if ((l_b_key_data = bsearch(&l_key_data, 
                                g_b_target.worker[l_worker->id].key_data,
                                g_b_target.worker[l_worker->id].key_rows, 
                                sizeof(key_data_t), 
                                pkey_cmp)) != NULL)
    {
 
       if (!strcmp(l_a_key_data->pk_data, l_b_key_data->pk_data))
       { 
          l_a_key_data->ok = OK;
          l_error_block->ok++;
       } 
       else
       {
          l_a_key_data->ok = NOT_OK;
          l_error_block->not_ok++;
          write_report(A_TARGET, l_worker, l_a_key_data->pk_data, l_b_key_data->pk_data);
       }
    }
    else
    {
       l_a_key_data->ok = NOT_FOUND;
       l_error_block->a_not_found++;
       write_report(A_TARGET, l_worker, l_a_key_data->pk_data, NULL);
    }
    
    l_a_key_data++;
 }
   
WTRACE_OUT(0)

 pthread_exit(NULL);
}

/*
  compare b to a 
*/
static void *cmp_b_key_data(void *p_arg)
{
 worker_t *l_worker = (worker_t *) p_arg;
 block_t *l_error_block;
 key_data_t *l_b_key_data;
 key_data_t l_key_data;
 int32_t i;

WTRACE_IN(0)

 // only worker->id 0 used when SMALL_TABLE_FLG
 if (g_flags & SMALL_TABLE_FLG && g_a_target.worker[l_worker->id].key_rows == 0)
 {
WTRACE_OUT(0)
    pthread_exit(NULL);
 }

 l_error_block = &gv.td_segment.block[gv.td_segment.error_list[l_worker->error_ix]];
 l_b_key_data  = l_worker->key_data; 

 qsort(g_a_target.worker[l_worker->id].key_data,
       g_a_target.worker[l_worker->id].key_rows,
       sizeof(key_data_t),    
       pkey_cmp);

//  fprintf(stderr, "B l_worker->num_tuples  %d gv.td_error_count %lu gv.td_max_errors+1 %lu\n",
//         l_worker->num_tuples,  gv.td_error_count,  gv.td_max_errors);
 
 for(i = 0; i < l_worker->num_tuples && gv.td_error_count < gv.td_max_errors; i++)
 {
    l_key_data.pk_data = l_b_key_data->pk_data;

    if (bsearch(&l_key_data,
                g_a_target.worker[l_worker->id].key_data, 
                g_a_target.worker[l_worker->id].key_rows,
                sizeof(key_data_t), 
                pkey_cmp) != NULL)
    {
       l_b_key_data->ok = OK;
    }
    else 
    {
       l_b_key_data->ok = NOT_FOUND;
       l_error_block->b_not_found++;
       write_report(B_TARGET, l_worker, l_b_key_data->pk_data, NULL);
    }
   
    l_b_key_data++;
 }
    
WTRACE_OUT(0)

  pthread_exit(NULL);
}

/*
   compare a and b primary keys 
*/
static int32_t pkey_cmp(const void *p_1, const void *p_2)
{
 key_data_t *l_a_key_data = (key_data_t *)p_1;
 key_data_t *l_b_key_data = (key_data_t *)p_2;
 char *l_cptr;
 int32_t l_len;
 int32_t l_rc;

 if ((l_cptr = strchr(l_a_key_data->pk_data, GROUP_SEP)) != NULL)
 {
    l_len = (int32_t) (l_cptr - l_a_key_data->pk_data);
    l_rc = memcmp(l_a_key_data->pk_data, l_b_key_data->pk_data, l_len);
 }
 else
 {
   bail(EX_SOFTWARE, "pkey_cmp failed - pkey data separator not found '%s' '%s'", 
        l_a_key_data->pk_data, l_b_key_data->pk_data);
 }

 return(l_rc);
}

/*
   write a line to report.  bails on max_errors
*/
static void write_report(int32_t p_target_id,
                         worker_t *p_worker,
                         const char *p_keydata1, 
                         const char *p_keydata2)
{
 worker_t *l_worker = p_worker; 
 int32_t l_rc;

 if ((l_rc = pthread_mutex_lock(&gv.td_write_mutex)) != 0)
 {
    bail(EX_OSERR, "pthread_mutex_lock failed %s", strerror(l_rc));
 }

WTRACE_IN(2)

 gv.td_error_count++;

 // show heading on first error
 if (gv.td_error_count == 1)
 {
    heading();
 }

 fprintf(gv.outfp, "%c ", p_target_id);
 
 write_line(p_keydata1);

 if (p_keydata2 != NULL)
 {
    fprintf(gv.outfp, " b ");

    write_line(p_keydata2);
 }
 
 fprintf(gv.outfp, "\n");

 if (g_flags & CREATE_PATCH_FLG)
 {
    td_patch(p_target_id, p_keydata1, p_keydata2);
 }

WTRACE_OUT(2)

 if ((l_rc = pthread_mutex_unlock(&gv.td_write_mutex)) != 0)
 {
    bail(EX_OSERR, "pthread_mutex_unlock failed %s", strerror(l_rc));
 }

}

static void write_line(const char *p_keydata)
{
 char *l_cptr;
 int32_t l_len;

 // remove delimiters from error rows
 l_cptr = strchr(p_keydata+1, GROUP_SEP);
 l_cptr -= 3;
 l_len = l_cptr - p_keydata+1;
 fprintf(gv.outfp, "%.*s,", l_len, p_keydata+1);
 l_cptr += 5;
 fprintf(gv.outfp, "%.*s", (int32_t) strlen(l_cptr) -1, l_cptr);
}

/*
   write target connection info
*/
static void heading(void)
{
 fprintf(gv.outfp, "# a %d %s_%s_%s", 
         gv.a_lineno, g_a_target.host, g_a_target.port, g_a_target.dbname);

 if (g_a_target.object != NULL)
 {
    fprintf(gv.outfp, "_%s", g_a_target.object);
 }

 fprintf(gv.outfp, " != ");

 fprintf(gv.outfp, "b %d %s_%s_%s", 
         gv.b_lineno, g_b_target.host, g_b_target.port, g_b_target.dbname);

 if (g_a_target.object != NULL)
 {
    fprintf(gv.outfp, "_%s", g_b_target.object);
 }

 fprintf(gv.outfp, "\n");

 fprintf(gv.outfp, "# file: %s\n", gv.cfgfile_name);

 fprintf(gv.outfp, "# column names: %s", gv.table_desc.pk_col_names);
 if (gv.table_desc.col_names != NULL)
 {
    fprintf(gv.outfp, ",");
    fprintf(gv.outfp, "%s", gv.table_desc.col_names);
 }

 fprintf(gv.outfp, "\n");

 fprintf(gv.outfp, "# column types: %s", gv.table_desc.pk_col_types);
 if (gv.table_desc.col_types != NULL)
 {
    fprintf(gv.outfp, ",");
    fprintf(gv.outfp, "%s", gv.table_desc.col_types);
 }

 fprintf(gv.outfp, "\n");
}
