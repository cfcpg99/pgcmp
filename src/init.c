#include "pgcmp.h"
#include "td_patch.h"

static void connect_workers(target_t *p_target);
static void set_pg_vars(target_t *p_target);
static void alloc_worker(target_t *p_target); 

/*
   allocate memory, get and set variabless. connect to postgres.
   target 'a' is set once. target 'b' is set for each row in list
*/
void init(void)
{
 struct timeval l_start_tv;
 struct timeval l_end_tv;

TRACE_IN(1) 

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_start_tv, NULL);
 }

 if (gv.command & TABLE_DATA_CMD)
 {
    gv.td_stats.get_table_desc_et = 0;
    gv.td_stats.create_blocks_et  = 0;
    if ((g_flags & SMALL_TABLE_FLG) == 0)
    {
       // reset to runtime values
       gv.td_segment.cpus       = gv.td_cpus;
       gv.td_segment.particles  = gv.td_particles;
    }
    gv.td_error_count    = 0;
 }

 alloc_worker(&g_b_target);

 connect_workers(&g_b_target);

 set_pg_vars(&g_b_target);

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_end_tv, NULL);
    gv.misc_stats.init_et = get_elapsed_time(&l_start_tv, &l_end_tv);
 }

TRACE_OUT(1)
}

/*
  the a_target is initialized once per file and is the standard
  that b_targets are compared
*/
void init_a_target(void)
{    
 int32_t l_rc;

TRACE_IN(1)
      
 g_a_target.id = A_TARGET;
 g_b_target.id = B_TARGET;

 alloc_worker(&g_a_target);
      
 connect_workers(&g_a_target);

 set_pg_vars(&g_a_target);
   
 if (gv.command & TABLE_DATA_CMD)
 { 
    if (g_flags & CREATE_PATCH_FLG)
    {
       td_connect_patch(&g_a_target);
    }
      
    if ((l_rc = pthread_mutex_init(&gv.td_write_mutex, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_mutex_init failed %s", strerror(l_rc));
    } 
 } 

TRACE_OUT(1)
}

/*
  allocate worker array
*/
static void alloc_worker(target_t *p_target)
{
 int32_t i;

TRACE_IN(1)

 if ((gv.command & TABLE_DATA_CMD) == 0)
 {
    if ((p_target->ud_pipe_buf = calloc(PIPE_BUF_SZ+1, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc ud_pipe_buf failed %s\n", strerror(errno));
    }
 }

 if ((p_target->worker = calloc(gv.td_cpus, sizeof(worker_t))) == NULL)
 {
    bail(EX_OSERR, "calloc worker failed %s\n", strerror(errno));
 }

 for(i = 0; i < gv.td_cpus; i++)
 {
    if ((p_target->worker[i].sql = calloc(gv.sql_buf_sz, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc gv.sql_buf_sz %d '%c' worker[%d] failed %s", 
            gv.sql_buf_sz, p_target->id, i, strerror(errno));
    }
 }

TRACE_OUT(1)
}

/*
   set postgres vars
*/
static void set_pg_vars(target_t *p_target)
{
 char l_schema[PQ_NAME_SZ+1] = "";
 char *l_cptr;
 int32_t l_len;

TRACE_IN(1)

 // connection info from postgres point of view
 p_target->host   = PQhost(p_target->worker[0].pq.conn);
 p_target->dbname = PQdb(p_target->worker[0].pq.conn);
 p_target->port   = PQport(p_target->worker[0].pq.conn);

 if (gv.command & OBJECT_REQUIRED_CMD)
 { 
    if ((l_cptr = strchr(p_target->object, '.')) != NULL)
    {
       l_len = (int32_t) (l_cptr - p_target->object);
       memcpy(l_schema, p_target->object, l_len);
       l_schema[l_len] = '\0';
    }

    p_target->td_nspname  = strdup(l_schema);
 }
 
 if (gv.command & TABLE_DATA_CMD)
 {
    if ((l_cptr = strchr(p_target->object, '.')) != NULL)
    {
       l_cptr++;
 
       p_target->td_relname = strdup(l_cptr); 
    }
 }
 else if (gv.command & SOURCE_CODE_CMD)
 {
    if ((l_cptr = strchr(p_target->object, '.')) != NULL)
    {
       l_cptr++;
    
       p_target->ud_sourcename = strdup(l_cptr);
    }
 }

 TRACE_OUT(1)
}

/*
   create worker connections
*/
static void connect_workers(target_t *p_target)
{
 int32_t i;

TRACE_IN(1)

 for(i = 0; i < gv.td_cpus; i++)
 {
    p_target->worker[i].pq.conn_info  = p_target->conninfo;

    p_target->worker[i].pq.errfp = gv.errfp;
    if (pq_connect(&p_target->worker[i].pq) != 0)
    {
       bail(EX_NOHOST, "%c worker[%d] connection failed\n%s\nconninfo: '%s'",
            p_target->id, i, p_target->worker[i].pq.error_msg, p_target->worker[i].pq.conn_info);
    }
 }

TRACE_OUT(1)
}
