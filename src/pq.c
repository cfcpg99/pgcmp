#include "pq.h"

static void err_msg(pq_t *p_pq, const char *p_message);
static void notice_processor(void *p_arg, const char *p_message);

/*
   create a postgres connection
*/
int32_t pq_connect(pq_t *p_pq)
{
 int32_t l_rc = -1;
 
 p_pq->conn = PQconnectdb(p_pq->conn_info);
 if ((l_rc = PQstatus(p_pq->conn)) != CONNECTION_OK)
 {
    if (p_pq->conn != NULL)
    {
       p_pq->error = PQstatus(p_pq->conn);
       err_msg(p_pq, PQerrorMessage(p_pq->conn));
       PQfinish(p_pq->conn);
       p_pq->conn = NULL;
    }
    l_rc = -1;
 }
 else
 {
    PQsetNoticeProcessor(p_pq->conn, notice_processor, p_pq);
 }

 return (l_rc);
}

/*
   run query
*/
PGresult *pq_exec(pq_t *p_pq, const char *p_sql)
{
 PGresult *l_rset = NULL;
 int32_t l_status = 0;

 p_pq->error  = 0;
 p_pq->notice = 0;

 if (p_pq->error_msg != NULL)
 {
    free(p_pq->error_msg);
    p_pq->error_msg = NULL;
 }

 if (p_pq->notice_msg != NULL)
 {
    free(p_pq->notice_msg);
    p_pq->notice_msg = NULL;
 }

 if (p_pq->errfp == NULL)
 {
    p_pq->errfp = stderr;
 }

 if ((l_rset = PQexec(p_pq->conn, p_sql)) != NULL)
 {
    l_status = PQresultStatus(l_rset);
    switch(l_status)
    {
       case PGRES_TUPLES_OK:
       case PGRES_COMMAND_OK:
       case PGRES_COPY_IN:
       case PGRES_COPY_OUT:
        return(l_rset);
       break;
    }

    p_pq->error = l_status;
    err_msg(p_pq, PQresultErrorMessage(l_rset));

    PQclear(l_rset);
    l_rset = NULL;
 }
 else
 {
    p_pq->error = l_status;
    err_msg(p_pq, PQerrorMessage(p_pq->conn));
 }

 return(l_rset);
}

/*
   close postgres connection
*/
void pq_finish(pq_t *p_pq)
{
 p_pq->error  = 0;
 p_pq->notice = 0;

 if (p_pq->error_msg != NULL)
 {
    free(p_pq->error_msg);
    p_pq->error_msg = NULL;
 }

 if (p_pq->notice_msg != NULL)
 {
    free(p_pq->notice_msg);
    p_pq->notice_msg = NULL;
 }

 if (p_pq->conn != NULL)
 {
    PQfinish(p_pq->conn);
    p_pq->conn = NULL;
 }
}

/*
   set error message
*/
static void err_msg(pq_t *p_pq, const char *p_message)
{
 if (p_pq->error_msg != NULL)
 {
    free(p_pq->error_msg);
 }

 if ((p_pq->error_msg = strdup(p_message)) == NULL)
 {
    fprintf(p_pq->errfp, "strdup error message failed %s %d %s\n", __FILE__, __LINE__, strerror(errno));
    exit(EX_OSERR);
 }

 p_pq->error_msg[strlen(p_pq->error_msg) -1] = '\0';
 p_pq->error = 1;
 p_pq->error_count++;
}

/*
   show notice message
*/
static void notice_processor(void *p_arg, const char *p_message)
{
 pq_t  *l_pq = (pq_t *) p_arg;

 if (l_pq->notice_msg != NULL)
 {
    free(l_pq->notice_msg);
 }

 if ((l_pq->notice_msg = strdup(p_message)) == NULL)
 {
    fprintf(l_pq->errfp, "strdup error message failed %s %d %s\n", __FILE__, __LINE__, strerror(errno));
    exit(EX_OSERR);
 }

 l_pq->notice_msg[strlen(l_pq->notice_msg) -1] = '\0';
 l_pq->notice = 1;
 l_pq->notice_count++;
}
