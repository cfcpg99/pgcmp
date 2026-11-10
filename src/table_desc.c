#include "pgcmp.h"
#include "td_setup.h"

static void get_pkey_info(target_t *p_target, 
                          table_desc_t *p_table_desc, 
                          char *p_indrelid);
static void get_num_pkey_cols(target_t *p_target, 
                              table_desc_t *p_table_desc, 
                              const char *p_indrelid);
static void get_pkey_col_names(target_t *p_target,
                               table_desc_t *p_table_desc, 
                               const char *p_indrelid);
static void get_pkey_col_types(target_t *p_target,
                               table_desc_t *p_table_desc,
                               const char *p_indrelid);
static void get_data_col_names(target_t *p_target,
                               table_desc_t *p_table_desc,
                               const char *p_indrelid);
static void get_data_col_types(target_t *p_target,
                               table_desc_t *p_table_desc,
                               const char *p_indrelid);

/*
   get primary key and table columns
*/
void get_table_desc(target_t *p_target, table_desc_t *p_table_desc)
{
 char l_indrelid[INDRELID_SZ];
 struct timeval l_start_tv;
 struct timeval l_end_tv;

TRACE_IN(1)

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_start_tv, NULL);
 }

 get_pkey_info(p_target, p_table_desc, l_indrelid);
 get_num_pkey_cols(p_target, p_table_desc, l_indrelid);
 get_pkey_col_names(p_target, p_table_desc, l_indrelid);
 get_pkey_col_types(p_target, p_table_desc, l_indrelid);
 get_data_col_names(p_target, p_table_desc, l_indrelid);
 get_data_col_types(p_target, p_table_desc, l_indrelid);

 if (g_flags & DBG_TIMING_FLG) 
 {
    gettimeofday(&l_end_tv, NULL);
    gv.td_stats.get_table_desc_et += get_elapsed_time(&l_start_tv, &l_end_tv);
 }

TRACE_OUT(1)
}

/* 
  check for matching table structures 
*/
void check_table_desc(void)
{
 table_desc_t l_b_table_desc;

TRACE_IN(1);

 memset(&l_b_table_desc, '\0', sizeof(table_desc_t));

 get_table_desc(&g_b_target, &l_b_table_desc);

 if (strcmp(gv.table_desc.pk_col_types, l_b_table_desc.pk_col_types) != 0)
 {
    notice("inconsistent primary key types: a '%s' b '%s' at line %d in config file %s\n", 
           gv.table_desc.pk_col_types, l_b_table_desc.pk_col_types, gv.b_lineno, gv.cfgfile_name);
    exit(BAD_TABLE_DEF);
 }

 if (strcmp(gv.table_desc.col_types, l_b_table_desc.col_types) != 0)
 {
    notice("inconsistent column types: a '%s' b '%s' at line %d in file %s\n", 
           gv.table_desc.col_types, l_b_table_desc.col_types, gv.b_lineno, gv.cfgfile_name);
    exit(BAD_TABLE_DEF);
 }

 free_table_desc(&l_b_table_desc);

TRACE_OUT(1);
}

/*
  get primary key info
*/

static void get_pkey_info(target_t *p_target, 
                          table_desc_t *p_table_desc,
                          char *p_indrelid)
{
 char *l_pk_sql =
"select c2.relname,\n"
"       c2.oid relid,\n"
"       con.conkey,\n"
"       i.indrelid\n"
"from   pg_catalog.pg_namespace t1,\n"
"       pg_catalog.pg_class c,\n"
"       pg_catalog.pg_class c2,\n"
"       pg_catalog.pg_index i\n"
"join   pg_catalog.pg_constraint con on (conrelid = i.indrelid\n"
"                                        and conindid = i.indexrelid\n"
"                                        and contype = 'p')\n"
"where t1.nspname      = '%s'\n"
"and   c.relnamespace  = t1.oid\n"
"and   c.relname       = '%s'\n"
"and   c.oid           = i.indrelid\n"
"and   i.indexrelid    = c2.oid";

 sprintf(p_target->worker[0].sql, l_pk_sql, p_target->td_nspname, p_target->td_relname);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec %c failed\n%s\n%s",
         p_target->id,
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 p_target->worker[0].num_tuples = PQntuples(p_target->worker[0].rset);

 if (p_target->worker[0].num_tuples == 0)
 {
    // a_target required
    if (p_target->id == A_TARGET)
    {
       bail(BAD_TABLE_DEF, "target_a %s.%s: primary key not found at line %d in file %s\n%s",
            p_target->td_nspname, p_target->td_relname, gv.b_lineno, gv.cfgfile_name, p_target->worker[0].sql);
    }

    bail(BAD_TABLE_DEF, "target_b %s.%s: primary key not found at line %d in file %s\n",
         p_target->td_nspname, p_target->td_relname, gv.b_lineno, gv.cfgfile_name);
 }

 p_target->worker[0].num_tuples = PQntuples(p_target->worker[0].rset);

 strcpy(p_table_desc->pk_name, PQgetvalue(p_target->worker[0].rset, 0, 0));
 strcpy(p_table_desc->relid, PQgetvalue(p_target->worker[0].rset, 0, 1));
 strcpy(p_table_desc->conkey, PQgetvalue(p_target->worker[0].rset, 0, 2)+1);
 p_table_desc->conkey[strlen(p_table_desc->conkey)-1] = '\0';
 strcpy(p_indrelid, PQgetvalue(p_target->worker[0].rset, 0, 3));

 PQclear(p_target->worker[0].rset);
}

static void get_num_pkey_cols(target_t *p_target, 
                              table_desc_t *p_table_desc, 
                              const char *p_indrelid)
{
 char *l_att_count_sql = 
"select count(*)\n"
"from   pg_attribute\n"
"where  attrelid = %s\n"
"and    attnum = any('{%s}')";

 // get number of primary key columns
 sprintf(p_target->worker[0].sql, l_att_count_sql,  p_indrelid, p_table_desc->conkey);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec failed\n%s\n%s",
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 p_table_desc->num_pk_cols  = atoi(PQgetvalue(p_target->worker[0].rset, 0, 0));

 PQclear(p_target->worker[0].rset);
}

static void get_pkey_col_names(target_t *p_target, 
                               table_desc_t *p_table_desc, 
                               const char *p_indrelid)
{
 char *l_pk_col_names_sql =
"select array(select t1.attname\n"
"             from   pg_attribute t1\n"
"             where  t1.attrelid = %s\n"
"             and    t1.attnum = any('{%s}')\n"
"             order by t1.attnum)";

 if ((p_table_desc->pk_col_names = calloc(p_table_desc->num_pk_cols, PQ_NAME_SZ+1)) == NULL)
 {
    bail(EX_OSERR, "calloc a.table_desc.pk_col_names failed %s", strerror(errno));
 }

 // get primary key column names
 sprintf(p_target->worker[0].sql, l_pk_col_names_sql,  p_indrelid, p_table_desc->conkey);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec %c failed\n%s\n%s",
         p_target->id,
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 strcpy(p_table_desc->pk_col_names, PQgetvalue(p_target->worker[0].rset, 0, 0)+1);
 p_table_desc->pk_col_names[strlen(p_table_desc->pk_col_names)-1] = '\0';

 PQclear(p_target->worker[0].rset);
}

static void get_pkey_col_types(target_t *p_target, 
                               table_desc_t *p_table_desc,
                               const char *p_indrelid)
{
 char *l_pk_col_types_sql =
"select array(select t2.typname\n"
"             from  pg_attribute t1,\n"
"                   pg_type t2\n"
"             where t1.attrelid  = %s\n"
"             and   t1.attnum    = any('{%s}')\n"
"             and   t2.oid       = t1.atttypid\n"
"             order by t1.attnum)::text as pk_type";

 sprintf(p_target->worker[0].sql, l_pk_col_types_sql,  p_indrelid, p_table_desc->conkey);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec %c failed\n%s\n%s",
         p_target->id,
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 if ((p_table_desc->pk_col_types = calloc(p_table_desc->num_pk_cols, PQ_NAME_SZ+1)) == NULL)
 {
    bail(EX_OSERR, "calloc a.table_desc.pk_col_types failed %s", strerror(errno));
 }

 strcpy(p_table_desc->pk_col_types, PQgetvalue(p_target->worker[0].rset, 0, 0)+1);
 p_table_desc->pk_col_types[strlen(p_table_desc->pk_col_types)-1] = '\0';

 PQclear(p_target->worker[0].rset);
}

static void get_data_col_names(target_t *p_target, 
                               table_desc_t *p_table_desc,
                               const char *p_indrelid)
{
 char *l_col_names_sql = 
"select array(select t1.attname\n"
"             from   pg_attribute t1\n"
"             where  t1.attrelid = %s\n"
"             and    t1.attnum > -1\n"
"             and    t1.attnum not in (%s)\n"
"             order by t1.attnum)::text as col";

// get data column names not in pkey 
 sprintf(p_target->worker[0].sql, l_col_names_sql,  p_indrelid,  p_table_desc->conkey);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec %c failed\n%s\n%s",
         p_target->id,
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 // all pkey no data columns
 if (PQntuples(p_target->worker[0].rset) > 0)
 {
    if ((p_table_desc->col_names = calloc(strlen(PQgetvalue(p_target->worker[0].rset, 0, 0))+1, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc a.table_desc.col_names failed %s", strerror(errno));
    }

    strcpy(p_table_desc->col_names, PQgetvalue(p_target->worker[0].rset, 0, 0)+1);

    p_table_desc->col_names[strlen(p_table_desc->col_names)-1] = '\0';

    gv.table_desc.num_cols = PQntuples(p_target->worker[0].rset) + gv.table_desc.num_pk_cols;

    PQclear(p_target->worker[0].rset);
 }
}

static void get_data_col_types(target_t *p_target,
                               table_desc_t *p_table_desc,
                               const char *p_indrelid)
{
 char *l_col_types_sql =
"select array(select t2.typname\n"
"             from   pg_attribute t1,\n"
"                    pg_type t2\n"
"             where  t1.attrelid = %s\n"
"             and    t1.attnum > -1\n"
"             and    t1.attnum not in (%s)\n"
"             and    t2.oid  = t1.atttypid\n"
"             order by t1.attnum)::text as col_type";

 // all pkey no data columns
 if (p_table_desc->col_names == NULL)
 {
    return;
 }

 sprintf(p_target->worker[0].sql, l_col_types_sql,  p_indrelid,  p_table_desc->conkey);

 if ((p_target->worker[0].rset = pq_exec(&p_target->worker[0].pq, p_target->worker[0].sql)) == NULL)
 {
    bail(EX_SOFTWARE, "pq_exec failed\n%s\n%s",
         p_target->worker[0].pq.error_msg,
         p_target->worker[0].sql);
 }

 if ((p_table_desc->col_types = calloc(strlen(PQgetvalue(p_target->worker[0].rset, 0, 0))+1, sizeof(char))) == NULL)
 {
    bail(EX_OSERR, "calloc a.table_desc.col_types failed %s", strerror(errno));
 }

 strcpy(p_table_desc->col_types, PQgetvalue(p_target->worker[0].rset, 0, 0)+1);

 p_table_desc->col_types[strlen(p_table_desc->col_types)-1] = '\0';

 PQclear(p_target->worker[0].rset);
}
