#include "pgcmp.h"

#define CMD_LEFT_OFFSET    38
#define MIN_IX_NAME_LEN    12
#define MIN_REL_NAME_LEN    7
#define TITLE_GAP           8
#define TITLE_SZ          512

static void show_rusage(void);
static void show_timings(void);
static void target_info(void);
static void td_block_hash_values(void);
static void td_error_details(void);

/*
   run reports
*/
void debug(void)
{
 time_t l_time;
 struct tm *l_tm;
 char l_buf[80];
 char *l_title = "Unknown Command";

TRACE_IN(1)

 switch(gv.command)
 {
   case GLOBAL_VARS_CMD:
     l_title = "Global Variables";
   break;
   case GLOBAL_PRIVS_CMD:
     l_title = "Global Privileges";
   break;
   case PG_HBA_RULES_CMD:
     l_title = "PG_HBA Rules";
   break;
   case PG_SETTINGS_CMD:
     l_title = "PG Settings";
   break;
   case SCHEMA_DEF_CMD:
     l_title = "Schema Definition";
   break;
   case SCHEMA_PRIVS_CMD:
     l_title = "Schema Privileges";
   break;
   case SOURCE_CODE_CMD:
     l_title = "Source Code";
   break;
   case PG_ROLES_CMD:
     l_title = "PG Roles";
   break;
   case ADHOC_SQL_CMD:
     l_title = "Adhoc SQL";
   break;
   case PG_LS_DIR_CMD:
     l_title = "pg_ls_dir()";
   break;
   case PG_READ_FILE_CMD:
     l_title = "pg_read_file()";
   break;
   case TABLE_DATA_CMD:
     l_title = "Table Data";
 }

 time(&l_time);
 l_tm = localtime(&l_time);
 strftime(l_buf, sizeof(l_buf),"%Y-%m-%d %H:%M:%S", l_tm);

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "%*s %s %*s %s\n", CMD_LEFT_OFFSET, " ", l_title, TITLE_GAP, " ", l_buf);
 
 fprintf(gv.errfp, "-- Settings\n\n");
 target_info();

 // print only on first compare
 if (gv.command == TABLE_DATA_CMD && g_flags & SET_A_TARGET_FLG)
 {
    fprintf(gv.errfp, "tablesample:       %s\n", gv.table_desc.tablesample);
    fprintf(gv.errfp, "estimated_rows:    %9lu\n", gv.table_desc.estimated_rows);
    fprintf(gv.errfp, "cpus:              %9d\n", gv.td_segment.cpus);
    fprintf(gv.errfp, "segments:          %9d\n", gv.td_segment.particles);
    fprintf(gv.errfp, "ntile:             %9d\n", gv.table_desc.ntile);
    fprintf(gv.errfp, "sample_pct:        %9.2f\n", gv.table_desc.tablesample_pct);
 }

 if (gv.command == TABLE_DATA_CMD)
 {
    if (g_flags & SMALL_TABLE_FLG)
    {
       fprintf(gv.errfp, "small table overide\n");
    }

    if (g_flags & DBG_VERBOSE_FLG)
    {
       td_block_hash_values();
       if (gv.td_segment.error_block_count > 0)
       {
          td_error_details();
       }
    }
 }

 if (g_flags & DBG_RUSAGE_FLG)
 {
    show_rusage();
 }

 if (g_flags & DBG_TIMING_FLG)
 {
    show_timings();
 }

 fprintf(gv.errfp, "\n");

TRACE_OUT(1)
}

/*
   show table and utility timings
*/
static void show_timings(void)
{
TRACE_IN(1)

 fprintf(gv.errfp, "\n-- Timings\n\n");

 if (gv.command == TABLE_DATA_CMD)
 {
    if (gv.misc_stats.init_et > 0)
    {
       fprintf(gv.errfp, "init:                   %11.6f\n", gv.misc_stats.init_et);
    }

    if (gv.misc_stats.cleanup_et > 0)
    {
       fprintf(gv.errfp, "cleanup:                %11.6f\n", gv.misc_stats.cleanup_et);
    }

    if (gv.td_stats.get_table_desc_et > 0)
    {
       fprintf(gv.errfp, "get_table_desc:         %11.6f\n", gv.td_stats.get_table_desc_et);
    }
    if (gv.td_stats.create_blocks_et > 0)
    {
       fprintf(gv.errfp, "create_blocks:          %11.6f\n", gv.td_stats.create_blocks_et);
    }

    if (gv.td_stats.get_block_hash_et > 0)
    {
       fprintf(gv.errfp, "get_block_hash:         %11.6f\n", gv.td_stats.get_block_hash_et);
    }

    if (gv.td_stats.cmp_block_hash_et > 0)
    {
       fprintf(gv.errfp, "cmp_block_hash:         %11.6f\n", gv.td_stats.cmp_block_hash_et);
    }

    if (gv.td_stats.get_error_rows_et > 0)
    {
       fprintf(gv.errfp, "get_error_rows:         %11.6f\n", gv.td_stats.get_error_rows_et);
    }

    if (gv.td_stats.cmp_error_rows_et > 0)
    {
       fprintf(gv.errfp, "cmp_error_rows:         %11.6f\n", gv.td_stats.cmp_error_rows_et);
    }

    gv.td_stats.get_error_rows_et  = 0;
    gv.td_stats.cmp_error_rows_et  = 0;
 }
 else
 {
    fprintf(gv.errfp, "init_et:              %11.6f\n", gv.misc_stats.init_et);
    fprintf(gv.errfp, "response_et:          %11.6f\n", gv.misc_stats.response_et);
    fprintf(gv.errfp, "request_et:           %11.6f\n", gv.misc_stats.request_et);
    fprintf(gv.errfp, "cleanup_et:           %11.6f\n\n", gv.misc_stats.cleanup_et);

    fprintf(gv.errfp, "a_read_count:         %11d\n", g_a_target.ud_stats.read_count);
    fprintf(gv.errfp, "a_bytes_read:         %11d\n", g_a_target.ud_stats.bytes_read);
    fprintf(gv.errfp, "a_write_count:        %11d\n", g_a_target.ud_stats.write_count);
    fprintf(gv.errfp, "a_bytes_written:      %11d\n", g_a_target.ud_stats.bytes_written);
    fprintf(gv.errfp, "\n");
    fprintf(gv.errfp, "b_read_count:         %11d\n", g_b_target.ud_stats.read_count);
    fprintf(gv.errfp, "b_bytes_read:         %11d\n", g_b_target.ud_stats.bytes_read);
    fprintf(gv.errfp, "b_write_count:        %11d\n", g_b_target.ud_stats.write_count);
    fprintf(gv.errfp, "b_bytes_written:      %11d\n", g_b_target.ud_stats.bytes_written);

    g_b_target.ud_stats.read_count    = 0;
    g_b_target.ud_stats.bytes_read    = 0;
    g_b_target.ud_stats.write_count   = 0;
    g_b_target.ud_stats.bytes_written = 0;
 }

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "total time:             %11.6f\n", gv.misc_stats.diff_et);

TRACE_OUT(1)
}

/* 
   pgcmp reasource usage
*/
static void show_rusage(void)
{
 struct rusage l_ru;
 double l_ru_utime;
 double l_ru_stime;

TRACE_IN(1)

 getrusage(RUSAGE_SELF, &l_ru);

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "-- Resource Usage\n");

 fprintf(gv.errfp,"\n");

 l_ru_utime = l_ru.ru_utime.tv_sec + (1.0e-6) * (double) l_ru.ru_utime.tv_usec;
 l_ru_stime = l_ru.ru_stime.tv_sec + (1.0e-6) * (double) l_ru.ru_stime.tv_usec;

 fprintf(gv.errfp, "user CPU time used: ................. %9.4f   swaps: .............................. %9ld\n", l_ru_utime, l_ru.ru_nswap);
 fprintf(gv.errfp, "system CPU time used: ............... %9.4f   block input operations: ............. %9ld\n", l_ru_stime, l_ru.ru_inblock);
 fprintf(gv.errfp, "maximum resident set size: .......... %9ld   block output operations: ............ %9ld\n", l_ru.ru_maxrss, l_ru.ru_oublock);
 fprintf(gv.errfp, "integral shared memory size: ........ %9ld   IPC messages sent: .................. %9ld\n", l_ru.ru_ixrss, l_ru.ru_msgsnd);
 fprintf(gv.errfp, "integral unshared data size: ........ %9ld   IPC messages received: .............. %9ld\n", l_ru.ru_idrss, l_ru.ru_msgrcv);
 fprintf(gv.errfp, "integral unshared stack size: ....... %9ld   signals received: ................... %9ld\n", l_ru.ru_isrss, l_ru.ru_nsignals);
 fprintf(gv.errfp, "page reclaims (soft page faults): ... %9ld   voluntary context switches: ......... %9ld\n", l_ru.ru_minflt, l_ru.ru_nvcsw);
 fprintf(gv.errfp, "page faults (hard page faults): ..... %9ld   involuntary context switches: ....... %9ld\n", l_ru.ru_majflt, l_ru.ru_nivcsw);
                                                                
 fprintf(gv.errfp,"\n");

TRACE_OUT(1)
}

/*
   table block hash values
*/
static void td_block_hash_values(void)
{ 
 block_t *l_block;
 char l_hash_dash[TD_HASH_SZ];
 int32_t i;
 char l_ne[3];

TRACE_IN(1)

 memset(l_hash_dash, '-', sizeof(l_hash_dash));
 l_hash_dash[TD_HASH_SZ-1] = '\0';

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "-- Block Hash Values\n");
 fprintf(gv.errfp, "\n");

 fprintf(gv.errfp, "block %-*s %-*s ne\n", TD_HASH_SZ-1, "a_hash", TD_HASH_SZ-1, "b_hash");
                              
 fprintf(gv.errfp, "----- %s %s --\n", l_hash_dash, l_hash_dash);

 l_block = gv.td_segment.block;

 for(i = 0; i < gv.td_segment.block_count; i++)
 {
    if (i % gv.td_segment.cpus == 0 && i > 0)
    {
       fprintf(gv.errfp, "\n");
    }
    
    if (!strcmp(l_block->a_hash, l_block->b_hash))
    {
       strcpy(l_ne, "  ");
    }
    else
    {
       strcpy(l_ne, "!=");
    } 

    fprintf(gv.errfp, "%5d %s %s %s\n",
            i,
            l_block->a_hash,
            l_block->b_hash,
            l_ne);
    l_block++;
 }

TRACE_OUT(1)
}

/*
   segment error details
*/
static void td_error_details(void)
{ 
 block_t *l_block;
 char l_key_dash[PQ_NAME_SZ];
 int32_t l_a_sub_total = 0;
 int32_t l_b_sub_total = 0;
 int32_t l_a_total = 0;
 int32_t l_b_total = 0;
 int32_t l_ok_sub_total = 0;
 int32_t l_not_ok_sub_total = 0;
 int32_t l_a_not_found_sub_total = 0;
 int32_t l_b_not_found_sub_total = 0;
 int32_t l_ok_total = 0;
 int32_t l_not_ok_total = 0;
 int32_t l_a_not_found_total = 0;
 int32_t l_b_not_found_total = 0;
 int32_t i;

TRACE_IN(1)

 memset(l_key_dash, '-', sizeof(l_key_dash));
 l_key_dash[gv.td_max_key_len] = '\0';

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "-- Error Detail\n");
 fprintf(gv.errfp, "\n");
 
 fprintf(gv.errfp, "block    a_rows    b_rows     total        ok    not_ok      a_nf      b_nf %-*.*s %-*.*s\n", 
         gv.td_max_key_len, gv.td_max_key_len,
         "start_key", 
          gv.td_max_key_len, gv.td_max_key_len,
         "end_key");
 fprintf(gv.errfp, "----- --------- --------- --------- --------- --------- --------- --------- %-s %-s\n",
          l_key_dash, l_key_dash);

 l_block = gv.td_segment.block;

 for(i = 0; i < gv.td_segment.block_count; i++)
 {
    if (i % gv.td_segment.cpus == 0 && i > 0)
    {
       fprintf(gv.errfp, "      --------- --------- --------- --------- --------- --------- ---------\n");
       fprintf(gv.errfp, "      %9d %9d %9d %9d %9d %9d %9d\n", 
               l_a_sub_total, l_b_sub_total, l_a_sub_total + l_b_sub_total,
               l_ok_sub_total, l_not_ok_sub_total, l_a_not_found_sub_total, l_b_not_found_sub_total);
       l_a_sub_total           = 0;
       l_b_sub_total           = 0;
       l_ok_sub_total          = 0;
       l_not_ok_sub_total      = 0;
       l_a_not_found_sub_total = 0;
       l_b_not_found_sub_total = 0;
       fprintf(gv.errfp, "\n");
    }

    fprintf(gv.errfp, "%5d %9d %9d %9d %9d %9d %9d %9d %-*.*s %-*.*s\n",
            i,
            l_block->a_block_rows,
            l_block->b_block_rows,
            l_block->a_block_rows + l_block->b_block_rows,
            l_block->ok,
            l_block->not_ok,
            l_block->a_not_found,
            l_block->b_not_found,
            gv.td_max_key_len, 
            gv.td_max_key_len,
            l_block->start_key,
            gv.td_max_key_len, 
            gv.td_max_key_len,
            l_block->end_key);

    l_a_sub_total += l_block->a_block_rows;
    l_b_sub_total += l_block->b_block_rows;

    l_a_total += l_block->a_block_rows;
    l_b_total += l_block->b_block_rows;
 
    l_ok_sub_total          += l_block->ok;
    l_not_ok_sub_total      += l_block->not_ok;
    l_a_not_found_sub_total += l_block->a_not_found;
    l_b_not_found_sub_total += l_block->b_not_found;

    l_ok_total              += l_block->ok;
    l_not_ok_total          += l_block->not_ok;
    l_a_not_found_total     += l_block->a_not_found;
    l_b_not_found_total     += l_block->b_not_found;
 
    l_block++;
 }

 fprintf(gv.errfp, "      --------- --------- --------- --------- --------- --------- ---------\n");
 fprintf(gv.errfp, "      %9d %9d %9d %9d %9d %9d %9d\n", 
         l_a_sub_total, l_b_sub_total, l_a_sub_total + l_b_sub_total, 
         l_ok_sub_total, l_not_ok_sub_total, l_a_not_found_sub_total, l_b_not_found_sub_total);

 fprintf(gv.errfp, "\n      %9d %9d %9d %9d %9d %9d %9d\n\n      %9d total\n", 
         l_a_total, l_b_total, l_a_total + l_b_total,
         l_ok_total, l_not_ok_total, l_a_not_found_total, l_b_not_found_total,
         l_not_ok_total + l_a_not_found_total + l_b_not_found_total);

TRACE_OUT(1)
}

/*
   show connection and object info
*/
static void target_info(void)
{
TRACE_IN(1)

 fprintf(gv.errfp, "line %d in file %s\n\n", gv.b_lineno, gv.cfgfile_name);

 fprintf(gv.errfp, "target a:\n"); 
 fprintf(gv.errfp, "host:            %s\n", g_a_target.host);
 fprintf(gv.errfp, "port:            %s\n", g_a_target.port);
 fprintf(gv.errfp, "dbname:          %s\n", g_a_target.dbname);

 if (g_a_target.object != NULL)
 {
    fprintf(gv.errfp, "object:          %s\n", g_a_target.object);
 }

 fprintf(gv.errfp, "\n");
 fprintf(gv.errfp, "target b:\n");
 fprintf(gv.errfp, "host:            %s\n", g_b_target.host);
 fprintf(gv.errfp, "port:            %s\n", g_b_target.port);
 fprintf(gv.errfp, "dbname:          %s\n", g_b_target.dbname);

 if (g_b_target.object != NULL)
 {
    fprintf(gv.errfp, "object:          %s\n", g_b_target.object);
 }

 fprintf(gv.errfp, "\n");

TRACE_OUT(1)
}
