#include "pgcmp.h"

#define  PSQL_EOF      "-- psql query complete"
#define  PGDUMP_EOF    "-- PostgreSQL database dump complete"
#define  PGDUMPALL_EOF "-- PostgreSQL database cluster dump complete"

/*
   query psql 
*/
void *psql_req(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 FILE *l_fp; 
 char l_buf[PIPE_BUF_SZ+1] = "";
 char *l_cptr;
 int32_t l_read_count    = 0;
 int32_t l_bytes_in      = 0;
 int32_t l_write_count   = 0;
 int32_t l_bytes_written = 0;
 int32_t l_len;

TRACE_IN(0)
 
 l_target->rc = -1;

 if ((l_fp = fopen(l_target->ud_filename, "w")) == NULL)
 {
    bail(EX_CANTCREAT, "fopen '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 if (setvbuf(l_fp, NULL, _IONBF, 0) != 0)
 {
    bail(EX_OSERR, "setvbuf '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 l_len = sprintf(l_buf, "%s\n", l_target->worker[0].sql);

 l_bytes_written = write(l_target->ud_rsp_pipe[WRITE_PIPE], l_buf, l_len);
 l_write_count++;

 l_len = sprintf(l_buf, "\\echo %s\n", PSQL_EOF);  // end connection
 l_bytes_written += write(l_target->ud_rsp_pipe[WRITE_PIPE], l_buf, l_len);
 l_write_count++;

 if ((l_len = (int32_t) read(l_target->ud_req_pipe[READ_PIPE], l_target->ud_pipe_buf, PIPE_BUF_SZ)) < 0)
 {
    bail(EX_IOERR, "ud_req_pipe[READ_PIPE] failed  %s", strerror(errno));
 }

 while(1)
 {
    l_read_count++;
    l_bytes_in += l_len;
    l_target->ud_pipe_buf[l_len] = '\0';

    if ((l_cptr = strstr(l_target->ud_pipe_buf, PSQL_EOF)) != NULL)  //  recv end of result set
    {
       *l_cptr =  '\0';
       fprintf(l_fp, "%s\n", l_target->ud_pipe_buf);
       l_target->rc = EX_OK;
       break;
    }

    fprintf(l_fp, "%s", l_target->ud_pipe_buf);
    if ((l_len = (int32_t) read(l_target->ud_req_pipe[READ_PIPE], l_target->ud_pipe_buf, PIPE_BUF_SZ)) < 0)
    {
       bail(EX_IOERR, "ud_req_pipe[READ_PIPE] failed  %s", strerror(errno));
    }
 }

 l_bytes_written += write(l_target->ud_rsp_pipe[WRITE_PIPE], "\\q\n", 3);
 l_write_count++;

 if ((l_len = (int32_t) read(l_target->ud_err_pipe[READ_PIPE], l_buf, PIPE_BUF_SZ)) < 0)
 {  
    bail(EX_IOERR, "ud_err_pipe[READ_PIPE] failed  %s", strerror(errno));
 }
 
 if (l_len > 0)
 {
    if (strstr(l_buf, "No such file or directory") != NULL)
    {
       l_target->rc = EX_OSFILE;
    }
    else if (strstr(l_buf, "Permission denied") != NULL) 
    {
       l_target->rc = EX_NOPERM;
    }
    else
    {
       l_target->rc = EX_SOFTWARE;
    }
    notice("psql failed %s", l_buf);
 }
 
 fclose(l_fp);

 l_target->ud_stats.read_count     = l_read_count;
 l_target->ud_stats.bytes_read     = l_bytes_in;
 l_target->ud_stats.write_count    = l_write_count;
 l_target->ud_stats.bytes_written  = l_bytes_written;

TRACE_OUT(0)

 pthread_exit(NULL);
}

/*
    query pg_dump schema definition or privs only 
*/
void *pg_dump_req(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 FILE *l_tmpfp;
 FILE *l_fp = NULL;
 char l_buf[PIPE_BUF_SZ+1] = "";
 int32_t l_bytes_in = 0;
 int32_t l_read_count = 0;
 int32_t l_len;

TRACE_IN(0)

 l_target->rc = -1;
 if ((l_tmpfp = fopen(l_target->ud_filename, "w")) == NULL)
 {
    bail(EX_CANTCREAT, "fopen '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 if (setvbuf(l_tmpfp, NULL, _IONBF, 0) != 0)
 {
    bail(EX_OSERR, "setvbuf '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 // set to line buffering
 if ((l_fp = fdopen(l_target->ud_req_pipe[READ_PIPE], "r")) == NULL)
 {
    bail(EX_OSERR, "target %c fdopen 'ud_req_pipe[READ_PIPE]' failed %s", l_target->id, strerror(errno));
 }
 
 fgets(l_target->ud_pipe_buf, PIPE_BUF_SZ-2, l_fp);
 l_len = strlen(l_target->ud_pipe_buf);
 while(l_len > 0)
 {
    l_len = strlen(l_target->ud_pipe_buf);
    l_read_count++;
    l_bytes_in += l_len;
 
    // syntax error, bad names
    if (strstr(l_target->ud_pipe_buf, "pg_dump: error:") != NULL)
    {
      notice("%s", l_target->ud_pipe_buf);
      l_target->rc = EX_SOFTWARE;
      break;
    }
 
    if (strstr(l_target->ud_pipe_buf, PGDUMP_EOF) != NULL)
    {
       fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
       l_target->rc = EX_OK;
       break;
    }
 
    if ((g_flags & PRIVS_ONLY_FLG) == 0)
    {
       fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
    }
    else
    {
       if (strstr(l_target->ud_pipe_buf, "GRANT") || strstr(l_target->ud_pipe_buf, "OWNER"))
       {
          fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
       }
    }
 
    fgets(l_target->ud_pipe_buf, PIPE_BUF_SZ-2, l_fp);
 }

 if ((l_len = (int32_t) read(l_target->ud_err_pipe[READ_PIPE], l_buf, PIPE_BUF_SZ)) < 0)
 {
    bail(EX_IOERR, "ud_err_pipe[READ_PIPE] failed  %s", strerror(errno));
 }

 if (l_len > 0)
 {
    notice("pg_dump privs failed %s", l_buf);
    l_target->rc = EX_SOFTWARE;
 }

 fclose(l_tmpfp);
 fclose(l_fp);

 l_target->ud_stats.read_count     = l_read_count;
 l_target->ud_stats.bytes_read     = l_bytes_in;

TRACE_OUT(0)

 pthread_exit(NULL);
}

/*
   query pg_dumpall global variables or privs only
*/
void *pg_dumpall_req(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 FILE *l_tmpfp;
 FILE *l_fp;
 char l_buf[PIPE_BUF_SZ+1] = "";
 int32_t l_read_count = 0;
 int32_t l_bytes_in = 0;
 int32_t l_len;

TRACE_IN(0)

 l_target->rc = -1;
 if ((l_tmpfp = fopen(l_target->ud_filename, "w")) == NULL)
 {
    bail(EX_CANTCREAT, "fopen '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 if (setvbuf(l_tmpfp, NULL, _IONBF, 0) != 0)
 {
    bail(EX_OSERR, "setvbuf '%s' failed %s", l_target->ud_filename, strerror(errno));
 }

 // set to line input
 if ((l_fp = fdopen(l_target->ud_req_pipe[READ_PIPE], "r")) == NULL)
 {
    bail(EX_OSERR, "target %c fdopen 'ud_req_pipe[READ_PIPE]' failed %s",
         l_target->id, strerror(errno));
 }
 
 fgets(l_target->ud_pipe_buf, PIPE_BUF_SZ-2, l_fp);
 l_len = strlen(l_target->ud_pipe_buf);
 while(l_len > 0)
 {
    l_len = strlen(l_target->ud_pipe_buf);
    l_read_count++;
    l_bytes_in += l_len;
 
    if (strstr(l_target->ud_pipe_buf, PGDUMPALL_EOF) != NULL)
    {
       fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
       l_target->rc = EX_OK;
       break;
    }
 
    if ((g_flags & PRIVS_ONLY_FLG) == 0)
    {
       fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
    }
    else
    {
      if (strstr(l_target->ud_pipe_buf, "GRANT") || strstr(l_target->ud_pipe_buf, "OWNER"))
      {
         fwrite(l_target->ud_pipe_buf, 1, l_len, l_tmpfp);
      }
    }
 
    fgets(l_target->ud_pipe_buf, PIPE_BUF_SZ-2, l_fp);
 }

 if ((l_len = (int32_t) read(l_target->ud_err_pipe[READ_PIPE], l_buf, PIPE_BUF_SZ)) < 0)
 {
    bail(EX_IOERR, "ud_err_pipe[READ_PIPE] failed  %s", strerror(errno));
 }

 if (l_len > 0)
 {
    notice("pg_dumpall failed %s", l_buf);
    l_target->rc = EX_SOFTWARE;
 }

 fclose(l_tmpfp);
 fclose(l_fp);

 l_target->ud_stats.read_count     = l_read_count;
 l_target->ud_stats.bytes_read     = l_bytes_in;

TRACE_OUT(0)

 pthread_exit(NULL);
}
