#include <fcntl.h>
#include <sys/stat.h>

#include "pgcmp.h"
#include "td_patch.h"

#define COMMENT                       1
#define BLANK_LINE                    2
#define SYNTAX_ERROR                  3
#define MAX_LINE_SZ               16384 

extern void init_a_target(void);

static int32_t read_config_file(FILE *p_info, int32_t *p_processed_a_target, char *p_buf);
static int32_t run_diff(void);
static int32_t check_line(char *p_buf);

int32_t process(void)
{
 FILE *l_infp;
 char l_buf[MAX_LINE_SZ+1];
 int32_t l_processed_a_target = 0;
 int32_t l_opened_patch = 0;
 struct timeval l_start_tv;
 struct timeval l_end_tv;
 int32_t l_rc = 0;

TRACE_IN(1)

 // open config file
 if (!strcmp(gv.cfgfile_name, "stdin"))
 {
    l_infp = stdin;
 }
 else
 {
    if ((l_infp = fopen(gv.cfgfile_name, "r")) == NULL)
    {
       bail(EX_OSFILE, "fopen '%s' failed %s\n", gv.cfgfile_name, strerror(errno));
    }
 }

 // process a single 'a' and multiple 'b' target lines
 while(read_config_file(l_infp, &l_processed_a_target, l_buf) == 0)
 {
    g_b_target.conninfo = strdup(l_buf);
 
    if (g_flags & CREATE_PATCH_FLG)
    {
       if (l_opened_patch == 0)
       { 
          td_open_patch();
          l_opened_patch++;
       }
 
       td_connect_patch(&g_b_target);
    }
 
    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_start_tv, NULL);
    }
 
    init();
 
    // compare targets
    l_rc = run_diff();
  
    if (g_flags & DBG_TIMING_FLG)
    {
       gettimeofday(&l_end_tv, NULL);
       gv.misc_stats.diff_et = get_elapsed_time(&l_start_tv, &l_end_tv);
    }

    if (g_flags & DBG_REPORT_FLG)
    {
       debug();
    }

    // cleanup the last b_target processed
    cleanup(0);

    if (g_flags & CREATE_PATCH_FLG)
    { 
       td_disconnect_patch(&g_b_target);
    }
 }

 // close config file
 fclose(l_infp);

 // cleanup a_target
 cleanup(1);

TRACE_OUT(1)

 return(l_rc);
}

static int32_t read_config_file(FILE *p_infp, int32_t *p_processed_a_target, char *p_buf)
{
 char *l_cptr;
 char *l_object;
 int32_t l_len = 0;

TRACE_IN(1)

 fgets(p_buf, MAX_LINE_SZ, p_infp);
 while(!feof(p_infp))
 {
    gv.b_lineno++;
 
    // remove '\n'
    l_len = strlen(p_buf) -1;
    p_buf[l_len] = '\0';
 
    // blank line
    if (p_buf[0] == '\0')
    {
       fgets(p_buf, MAX_LINE_SZ, p_infp);
       continue;
    }
 
    // end file processing
    if (!strcmp(p_buf, "\\q"))
    {
 TRACE_OUT(1)
       return(EOF);
    }
    
    l_object = NULL;
    // set object if it exists
    if ((l_object = strchr(p_buf, '|')) != NULL)
    { 
       // null terminate connection string
       *l_object = '\0';
 
       // start of object
       l_object++; 
    
       // remove possible inline comment
       if ((l_cptr = strchr(l_object, '#')) != NULL)
       {
          *l_cptr = '\0';
       }
   
       trim(l_object); 
    }
 
    trim(p_buf);
 
    // user exit
    if (!strcmp(p_buf, "\\q"))
    {
 TRACE_OUT(1)
       return(EOF);
    }
 
    // read next line on parse error 
    if (check_line(p_buf) != 0)
    {
       fgets(p_buf, MAX_LINE_SZ, p_infp);
       continue;
    }
 
    if (gv.command & OBJECT_REQUIRED_CMD)
    {
       if (l_object == NULL || l_object[0] == '\0')
       {
          // a_target object is required
          if (*p_processed_a_target == 0)
          {
             notice("a_target object is missing\n");
             exit(EX_CONFIG);
          }
            
          l_object = g_a_target.object; 
       }
 
       // remove inline comment
       if ((l_cptr = strchr(l_object, '#')) != NULL)
       {
          *l_cptr = '\0';
       }
  
       // set a_target object 
       if (*p_processed_a_target == 0)
       { 
          g_a_target.object  = strdup(l_object);
       }
       else // set b_target object
       {
          g_b_target.object  = strdup(l_object);
       }
    }
 
    // a_target line is in the buffer
    if (*p_processed_a_target == 0)
    {
       // initialize a_target
       g_a_target.conninfo = strdup(p_buf);
       init_a_target();
       *p_processed_a_target = 1;
       gv.a_lineno = gv.b_lineno;     

       // read first b_target line
       fgets(p_buf, MAX_LINE_SZ, p_infp);
       continue;
    }
    break;
 }

TRACE_OUT(1)

 return(feof(p_infp));
}
/*
   compare a_target and b_targets
*/
static int32_t run_diff(void)
{
 int32_t l_rc = 0;

TRACE_IN(1)

 switch(gv.command)
 { 
   case ADHOC_SQL_CMD:    l_rc = ud_adhoc_sql();    break;
   case GLOBAL_VARS_CMD:  l_rc = ud_global_vars();  break;
   case GLOBAL_PRIVS_CMD: l_rc = ud_global_privs(); break;
   case PG_HBA_RULES_CMD: l_rc = ud_hba_rules();    break;
   case PG_LS_DIR_CMD:
   case PG_READ_FILE_CMD: l_rc = ud_file();         break;
   case PG_SETTINGS_CMD:  l_rc = ud_settings();     break;
   case PG_ROLES_CMD:     l_rc = ud_roles();        break;
   case SCHEMA_DEF_CMD:   l_rc = ud_schema_def();   break;
   case SCHEMA_PRIVS_CMD: l_rc = ud_schema_privs(); break;
   case SOURCE_CODE_CMD:  l_rc = ud_source_code();  break;
   case TABLE_DATA_CMD:   l_rc = table_diff();      break;
 }

 if ((gv.command & TABLE_DATA_CMD) && gv.td_error_count >= gv.td_max_errors)
 {
    if (g_flags & CREATE_PATCH_FLG)
    {
       td_disconnect_patch(&g_b_target);
    }

    l_rc = EX_FAIL;
    notice("exceeded max errors %lu > %lu\n", gv.td_error_count, gv.td_max_errors);
 }

 // targets are unequal
 if (l_rc == EX_DATAERR)
 {
    l_rc = EX_FAIL;
 }

TRACE_OUT(1)

 return(l_rc);
}

/*
   handle comments and blank lines
*/
static int32_t check_line(char *p_buf)
{
 char *l_cptr; 
 int32_t l_rc = 0;

 if (p_buf[0] == '#')
 {
    return(COMMENT);
 }

 // line data and inline comment
 if ((l_cptr = strchr(p_buf, '#')) != NULL)
 {
    trim(l_cptr+1);
    *l_cptr = '\0';
    l_rc = 0;
 }

 if (strlen(trim(p_buf)) == 0)
 {
    return(BLANK_LINE); 
 }

 return(l_rc); 
} 
