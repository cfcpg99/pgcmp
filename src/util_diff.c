#include "pgcmp.h"

static int32_t util_diff(void *(*p_reqent)(void *), void *(*p_server)(void *));
static int32_t user_diff_func(const char *p_a_filename, const char *p_b_filename);

/*
   psql query results
*/
int32_t ud_adhoc_sql(void)
{
 int32_t l_rc = -1;
 
TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    strcpy(g_a_target.worker[0].sql, g_a_target.object);
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_adhoc.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port);
 }

 strcpy(g_b_target.worker[0].sql, g_b_target.object);
 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_adhoc.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port);
 
 l_rc = util_diff(&psql_req, &psql_rsp);
 
 TRACE_OUT(1)
   
 return(l_rc);
}

/*
   psql runtime postgressql settings
*/
int32_t ud_settings(void)
{
 char *l_pg_hba_sql =
"select * \n"
"from   pg_settings\n"
"order by category, name;";
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    strcpy(g_a_target.worker[0].sql, l_pg_hba_sql);
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_pg_settings.txt",
           gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port);
 }

 strcpy(g_b_target.worker[0].sql, l_pg_hba_sql);
 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_pg_settings.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port);

 l_rc = util_diff(&psql_req, &psql_rsp);

TRACE_OUT(1)

 return(l_rc);
}

/*
   psql runtime pg_hba_rules settings
*/
int32_t ud_hba_rules(void)
{
 char *l_pg_hba_rules_sql =
"select type,\n"
"       database,\n"
"       user_name,\n"
"       address,\n"
"       netmask,\n"
"       auth_method,\n"
"       options,error\n"
"from   pg_hba_file_rules\n"
"order  by rule_number;";
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    strcpy(g_a_target.worker[0].sql, l_pg_hba_rules_sql);
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_pg_hba_rules.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port);
 } 

 strcpy(g_b_target.worker[0].sql, l_pg_hba_rules_sql);
 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_pg_hba_rules.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port);

 l_rc = util_diff(&psql_req, &psql_rsp);

TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_dump schema definitions
*/
int32_t ud_schema_def(void)
{
 int32_t l_rc = -1;
    
TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_%s.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port, g_a_target.object);
 }

 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_%s.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port, g_b_target.object);

 l_rc = util_diff(&pg_dump_req, &pg_dump_rsp);

TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_dump schema priv
*/
int32_t ud_schema_privs(void)
{
 int32_t l_rc = -1;

TRACE_IN(1)


 if (g_flags & SET_A_TARGET_FLG)
 {
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_%s.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port, g_a_target.object);
 }

 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_%s.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port, g_b_target.object);

 g_flags |= PRIVS_ONLY_FLG;
 l_rc = util_diff(&pg_dump_req, &pg_dump_rsp);
 g_flags ^= PRIVS_ONLY_FLG;

TRACE_OUT(1)

 return(l_rc);
}

/*
   psql function and procedure source code
*/
int32_t ud_source_code(void)
{
 int32_t l_rc = -1;

TRACE_IN(1)


 if (g_flags & SET_A_TARGET_FLG)
 {
    sprintf(g_a_target.worker[0].sql, "\\sf %s\n", g_a_target.object);

    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_%s.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port, g_a_target.object);
 }

 sprintf(g_b_target.worker[0].sql, "\\sf %s\n", g_b_target.object);

 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_%s.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port, g_b_target.object);

 l_rc = util_diff(&psql_req, &psql_rsp);

TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_dumpall postgressql global variables
*/
int32_t ud_global_vars(void)
{
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 { 
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_global_vars.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port);
 }

 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_global_vars.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port);

 l_rc = util_diff(&pg_dumpall_req, &pg_dumpall_rsp);
 
TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_dumpall postgressql global privs
*/
int32_t ud_global_privs(void)
{
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_global_privs.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port);
 }

 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_global_privs.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port);

 g_flags |= PRIVS_ONLY_FLG;
 l_rc = util_diff(&pg_dumpall_req, &pg_dumpall_rsp);
 g_flags ^= PRIVS_ONLY_FLG;

TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_roles
*/
int32_t ud_roles(void)
{
 char *l_pg_roles_sql =
"select rolname,\n"
"       rolsuper,\n"
"       rolinherit,\n"
"       rolcreaterole,\n"
"       rolcreatedb,\n"
"       rolcanlogin,\n"
"       rolreplication,\n"
"       rolconnlimit,\n"
"       rolpassword,\n"
"       rolvaliduntil,\n"
"       rolbypassrls,\n"
"       rolconfig\n"
"from   pg_roles\n"
"order by 1;";
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    strcpy(g_a_target.worker[0].sql, l_pg_roles_sql);
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_%c.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port, g_a_target.id);
 }

 strcpy(g_b_target.worker[0].sql, l_pg_roles_sql);
 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_%c.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port, g_b_target.id);

 l_rc = util_diff(&psql_req, &psql_rsp);

 TRACE_OUT(1)

 return(l_rc);
}

/*
   pg_ls_dir compares directories and pg_read_file compares files 
*/
int32_t ud_file(void)
{
 char *l_name;
 char *l_pg_ls_dir_sql =
"select pg_ls_dir('%s') order by 1;";
 char *l_pg_read_file_sql =
"select pg_read_file('%s') order by 1;";
 char *l_sql;
 int32_t l_rc = -1;

TRACE_IN(1)

 if (gv.command == PG_LS_DIR_CMD)
 {
    l_name = "pg_ls_dir";
    l_sql  =  l_pg_ls_dir_sql;
 }
 else 
 {
    l_name = "pg_read_file";
    l_sql  =  l_pg_read_file_sql;
 }

 if (g_flags & SET_A_TARGET_FLG)
 {
    sprintf(g_a_target.worker[0].sql, l_sql, g_a_target.object);
    sprintf(g_a_target.ud_filename, "%s/a_%s_%s_%s_%s.txt",
            gv.tmpdir, g_a_target.host, g_a_target.dbname, g_a_target.port, l_name);
 }

 sprintf(g_b_target.worker[0].sql, l_sql, g_b_target.object);
 sprintf(g_b_target.ud_filename, "%s/b_%s_%s_%s_%s.txt",
         gv.tmpdir, g_b_target.host, g_b_target.dbname, g_b_target.port, l_name);

 l_rc = util_diff(&psql_req, &psql_rsp);
 
 TRACE_OUT(1)
   
 return(l_rc);
}   

/*
   get and compare target a and b output
*/
static int32_t util_diff(void *(*p_reqent)(void *), void *(*p_server)(void *))
{
 struct timeval l_start_tv;
 struct timeval l_end_tv;
 int32_t l_rc = 0;

TRACE_IN(1)

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_start_tv, NULL);
 }

 pg_get_data(p_server);

 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_end_tv, NULL);
    gv.misc_stats.response_et = get_elapsed_time(&l_start_tv, &l_end_tv);
    gettimeofday(&l_start_tv, NULL);
 }

 pg_get_data(p_reqent);

 
 if (g_flags & DBG_TIMING_FLG)
 {
    gettimeofday(&l_end_tv, NULL);
    gv.misc_stats.request_et = get_elapsed_time(&l_start_tv, &l_end_tv);
 }
 
 if (g_a_target.rc != 0)
 {
    l_rc = g_a_target.rc;
 }

 if (g_b_target.rc != 0)
 {
    l_rc = g_b_target.rc;
 } 

 if (l_rc == 0)
 {
    l_rc = user_diff_func(g_a_target.ud_filename, g_b_target.ud_filename);
 }

TRACE_OUT(1)

 return(l_rc);
}

/*
   run user diff process
*/
static int32_t user_diff_func(const char *p_a_filename, const char *p_b_filename)
{
 char l_buf[FILENAME_MAX*3];
 FILE *l_fp;
 int32_t l_status;
 int32_t l_line_count = 0;
 int32_t l_rc = EX_OK;

TRACE_IN(1)

 sprintf(l_buf, "%s \"%s\" \"%s\" 2>&1", gv.diff_cmd, p_a_filename, p_b_filename); 
 if ((l_fp = popen(l_buf, "r")) == NULL)
 {
    bail(EX_OSERR, "popen '%s' failed %s", l_buf, strerror(errno));
 }

 fgets(l_buf, sizeof(l_buf), l_fp);
 while(!feof(l_fp))
 {
    l_line_count++;

    fprintf(gv.outfp, "%s", l_buf);

    fgets(l_buf, sizeof(l_buf), l_fp);
 }

 if ((l_status = pclose(l_fp)) != 0)
 {
    if ((l_rc = WIFEXITED(l_status)) != 0)
    {
       if (l_rc != 1)  // exit_code 1 is data error
       {
          bail(EX_OSERR, "popen failed exit status %d exit code %d", l_status, l_rc);
       }
    }
    else if ((l_rc =  WIFSIGNALED(l_status)) != 0)
    {
       bail(EX_OSERR, "popen failed signaled status %d exit code %d", l_status, l_rc);
    }
 }

TRACE_OUT(1)

 return(l_rc);
}

/*
   handle sig PIPE for utility programs
*/
void sig_pipe(int32_t p_signo)
{
 bail(EX_OSERR, "SIGPIPE %d", p_signo);
}
