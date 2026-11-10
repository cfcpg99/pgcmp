#include "pgcmp.h"

/*
   handles psql queries
*/
void *psql_rsp(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 pid_t l_pid;

TRACE_IN(0)

 if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
 {
    bail(EX_OSERR, "signal SIGPIPE failed %s", strerror(errno));
 }

 if (pipe(l_target->ud_rsp_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_rsp_pipe pipe failed %s", strerror(errno));
 }
 
 if (pipe(l_target->ud_req_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_req_pipe pipe failed %s", strerror(errno));
 }

 if (pipe(l_target->ud_err_pipe) == -1)
 {
    bail(EX_OSERR, "ud_err_pipe pipe failed %s", strerror(errno));
 }

 if ((l_pid = fork()) == -1)
 {
    bail(EX_OSERR, "fork failed %s", strerror(errno));
 }

 // child
 if (l_pid == 0)
 {
    if (dup2(l_target->ud_rsp_pipe[READ_PIPE], STDIN_FILENO) == -1) 
    {
       bail(EX_OSERR, "dup2 server READ_PIPE failed %s", strerror(errno));
    }

    if (dup2(l_target->ud_req_pipe[WRITE_PIPE], STDOUT_FILENO) == -1)
    {  
       bail(EX_OSERR, "dup2 ud_req_pipe[WRITE_PIPE] failed %s", strerror(errno));
    }

    if (dup2(l_target->ud_err_pipe[WRITE_PIPE], STDERR_FILENO) == -1)
    {
       bail(EX_OSERR, "dup2 ud_err_pipe[WRITE_PIPE] failed %s", strerror(errno));
    }

    close(l_target->ud_rsp_pipe[WRITE_PIPE]);
    close(l_target->ud_req_pipe[READ_PIPE]);
    close(l_target->ud_err_pipe[READ_PIPE]);

    if (execlp("psql", "psql", "-q", "-X",  l_target->conninfo, NULL) != 0)
    {
       bail(EX_OSERR, "execlp failed %s", strerror(errno));
    }
 }
 
 // parent
 close(l_target->ud_rsp_pipe[READ_PIPE]);
 close(l_target->ud_req_pipe[WRITE_PIPE]);
 close(l_target->ud_err_pipe[WRITE_PIPE]);

TRACE_OUT(0)

 pthread_exit(NULL);
}

/*
   handle pg_dump queries
*/
void *pg_dump_rsp(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 pid_t l_pid;
 char *l_restrict="--restrict-key=NA";

TRACE_IN(0)

 if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
 {
    bail(EX_OSERR, "signal SIGPIPE failed %s", strerror(errno));
 }

 if (pipe(l_target->ud_rsp_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_rsp_pipe pipe failed %s", strerror(errno));
 }
 
 if (pipe(l_target->ud_req_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_req_pipe pipe failed %s", strerror(errno));
 }

 if (pipe(l_target->ud_err_pipe) == -1)
 {
    bail(EX_OSERR, "ud_err_pipe pipe failed %s", strerror(errno));
 }

 if ((l_pid = fork()) == -1)
 {
    bail(EX_OSERR, "fork failed %s", strerror(errno));
 }

 // child
 if (l_pid == 0)
 {
    if (l_target->ud_rsp_pipe[READ_PIPE] != STDIN_FILENO)
    { 
       if (dup2(l_target->ud_rsp_pipe[READ_PIPE], STDIN_FILENO) == -1) 
       {
          bail(EX_OSERR, "dup2 ud_rsp_pipe[READ_PIPE]  failed %s", strerror(errno));
       }
    }

    if (dup2(l_target->ud_req_pipe[WRITE_PIPE], STDOUT_FILENO) == -1)
    {  
       bail(EX_OSERR, "dup2 ud_req_pipe[WRITE_PIPE]  failed %s", strerror(errno));
    }
       
    if (dup2(l_target->ud_err_pipe[WRITE_PIPE], STDERR_FILENO) == -1)
    {
       bail(EX_OSERR, "dup2 ud_err_pipe[WRITE_PIPE] failed %s", strerror(errno));
    }

    close(l_target->ud_rsp_pipe[WRITE_PIPE]);
    close(l_target->ud_req_pipe[READ_PIPE]);

    if (execlp("pg_dump", "pg_dump", "-h", l_target->host, "-d", l_target->dbname, l_restrict, "-s", "-n", l_target->object, NULL) != 0)
    {
       bail(EX_OSERR, "execvp failed %s", strerror(errno));
    }
 } 

 // parent
 close(l_target->ud_rsp_pipe[READ_PIPE]);
 close(l_target->ud_req_pipe[WRITE_PIPE]);
 close(l_target->ud_err_pipe[WRITE_PIPE]);

 l_target->rc = EX_OK;

TRACE_OUT(0)
 
 pthread_exit(NULL);
}

/*
   handle pg_dumpall queries
*/
void *pg_dumpall_rsp(void *p_arg)
{
 target_t *l_target = (target_t *) p_arg;
 pid_t l_pid;
 char l_host[PQ_NAME_SZ*2];
 char l_database[PQ_NAME_SZ*2];
 char *l_restrict="--restrict-key=NA";

TRACE_IN(0)

 if (pipe(l_target->ud_rsp_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_rsp_pipe pipe failed %s", strerror(errno));
 }
 
 if (pipe(l_target->ud_req_pipe) == -1) 
 {
    bail(EX_OSERR, "ud_req_pipe pipe failed %s", strerror(errno));
 }

 if (signal(SIGPIPE, sig_pipe) == SIG_ERR)
 {
    bail(EX_OSERR, "signal SIGPIPE failed %s", strerror(errno));
 }

 if (pipe(l_target->ud_err_pipe) == -1)
 {
    bail(EX_OSERR, "ud_err_pipe pipe failed %s", strerror(errno));
 }

 if ((l_pid = fork()) == -1)
 {
    bail(EX_OSERR, "fork failed %s", strerror(errno));
 }

 // child
 if (l_pid == 0)
 {
    if (l_target->ud_rsp_pipe[READ_PIPE] != STDIN_FILENO)
    { 
       if (dup2(l_target->ud_rsp_pipe[READ_PIPE], STDIN_FILENO) == -1) 
       {
          bail(EX_OSERR, "dup2 ud_rsp_pipe[READ_PIPE]  failed %s", strerror(errno));
       }
    }

    if (dup2(l_target->ud_req_pipe[WRITE_PIPE], STDOUT_FILENO) == -1)
    {  
       bail(EX_OSERR, "dup2 ud_req_pipe[WRITE_PIPE] failed %s", strerror(errno));
    }

    if (dup2(l_target->ud_err_pipe[WRITE_PIPE], STDERR_FILENO) == -1)
    {
       bail(EX_OSERR, "dup2 ud_err_pipe[WRITE_PIPE] failed %s", strerror(errno));
    }

    close(l_target->ud_rsp_pipe[WRITE_PIPE]);
    close(l_target->ud_req_pipe[READ_PIPE]);

    sprintf(l_host, "--host=%s", l_target->host);
    sprintf(l_database, "--database=%s", l_target->dbname);

    if (execlp("pg_dumpall", "pg_dumpall", l_host, l_database, "-g", l_restrict, NULL) != 0)
    {
       bail(EX_OSERR, "execvp failed %s", strerror(errno));
    }
 } 

 // parent
 close(l_target->ud_rsp_pipe[READ_PIPE]);
 close(l_target->ud_req_pipe[WRITE_PIPE]);
 close(l_target->ud_err_pipe[WRITE_PIPE]);

 l_target->rc = EX_OK;

TRACE_OUT(0)
 
 pthread_exit(NULL);
}

/* 
  execute threads on targets a and b
*/
void pg_get_data(void *(*p_routine)(void *))
{
 int32_t l_rc = -1;

TRACE_IN(1)

 if (g_flags & SET_A_TARGET_FLG)
 {
    if ((l_rc = pthread_create(&g_a_target.worker[0].pthread,
                               NULL,
                               p_routine,
                               &g_a_target)) != 0)
    {
       bail(EX_OSERR, "pthread_create thread failed %s", strerror(l_rc));
    }
 }

 if ((l_rc = pthread_create(&g_b_target.worker[0].pthread,
                            NULL,
                            p_routine,
                            &g_b_target)) != 0)
 {
     bail(EX_OSERR, "pthread_create thread failed %s", strerror(l_rc));
 }

 if (g_flags & SET_A_TARGET_FLG)
 {
    if ((l_rc = pthread_join(g_a_target.worker[0].pthread, NULL)) != 0)
    {
       bail(EX_OSERR, "pthread_join a failed %s", strerror(l_rc));
    }
 }

 if ((l_rc = pthread_join(g_b_target.worker[0].pthread, NULL)) != 0)
 {
    bail(EX_OSERR, "pthread_join b failed %s", strerror(l_rc));
 }
TRACE_OUT(1)
}
