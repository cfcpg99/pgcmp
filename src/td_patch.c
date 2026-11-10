#include <fcntl.h>
#include <sys/stat.h>

#include "td_patch.h"

#define KEY_DATA 0
#define KEY_ONLY 1
static void keydata2row(char *p_values, int32_t p_parse);
static void insert_patch(const char *p_keydata);
static void update_patch(const char *p_keydata1,
                         const char *p_keydata2);
static void delete_patch(target_t *p_target, const char *p_keydata);

void td_connect_patch(target_t *p_target)
{
TRACE_IN(1)

  p_target->pq.conn_info = p_target->conninfo;
  p_target->pq.errfp = gv.errfp;

  if (pq_connect(&p_target->pq) != 0)
  {
     bail(EX_SOFTWARE, "patch connection %c failed\n%s\nconninfo: '%s'",
          p_target->id, p_target->pq.error_msg, p_target->pq.conn_info);
  }

TRACE_OUT(1)
}

void td_open_patch(void)
{
  char l_buf[SMALL_BUF_SZ];
  char *l_cptr = strrchr(gv.cfgfile_name, '/');

TRACE_IN(1)

  if (l_cptr != NULL)
  {
     l_cptr++;
  }
  else
  {
     l_cptr = gv.cfgfile_name;
  }

  sprintf(l_buf, "%s/%s_fix", gv.tmpdir, l_cptr);

  if ((gv.patchfp = fopen(l_buf, "w")) == NULL)
  {
     bail(EX_CANTCREAT, "%s fopen '%s' failed %s", gv.tmpdir, l_buf, strerror(errno));
  }

  if (setvbuf(gv.patchfp, NULL, _IONBF, 0) != 0)
  {
     bail(EX_OSERR, "setvbuf '%s' failed %s", l_buf, strerror(errno));
  }

  if (fchmod(fileno(gv.patchfp), 0755))
  {
     bail(EX_OSERR, "chmod '%s' failed %s", l_buf, strerror(errno));
  }

  fprintf(gv.patchfp, "psql -X -v ON_ERROR_STOP=1 \"%s\" <<EOF\n", g_b_target.conninfo);
  fprintf(gv.patchfp, "begin;\n\n");

TRACE_OUT(1)
}

void td_disconnect_patch(target_t *p_target)
{
TRACE_IN(1)

 if (p_target->pq.conn != NULL)
 {
    pq_finish(&p_target->pq);
    p_target->pq.conn = NULL;
 }

 if (p_target->pq.error_msg != NULL)
 {
    free(p_target->pq.error_msg);
    p_target->pq.error_msg = NULL;
 }

 if (p_target->pq.notice_msg != NULL)
 {
    free(p_target->pq.notice_msg);
    p_target->pq.notice_msg = NULL;
 }

TRACE_OUT(1)
}

void td_patch(int32_t p_target_id,
              const char *p_keydata1,
              const char *p_keydata2)
{
 int32_t l_len;

 TRACE_IN(1);

 l_len = strlen(p_keydata1)                       +
                strlen(g_a_target.object)         +
                strlen(gv.table_desc.pk_col_names) +
                strlen(gv.table_desc.col_names);
 l_len += SMALL_BUF_SZ;;

 if (p_keydata1 != NULL && p_keydata2 != NULL)
 {
    if ((g_a_target.buf = calloc(l_len, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc g_a_target.buf %d failed %s", l_len, strerror(errno));
    }

    if ((g_b_target.buf = calloc(l_len, sizeof(char))) == NULL)
    {
       bail(EX_OSERR, "calloc g_b_target.buf %d failed %s", l_len, strerror(errno));
    }

    update_patch(p_keydata1, p_keydata2);

    free(g_a_target.buf);
    free(g_b_target.buf);
 }
 else if (p_target_id == A_TARGET)
 {
    if ((g_b_target.buf = calloc(l_len, sizeof(char))) == NULL)
    { 
        bail(EX_OSERR, "calloc g_b_target.buf %d failed %s", l_len, strerror(errno));
    } 

    insert_patch(p_keydata1);

    free(g_b_target.buf);
 }
 else
 {
    if (p_target_id == A_TARGET)
    {
       if ((g_a_target.buf = calloc(l_len, sizeof(char))) == NULL)
       {
          bail(EX_OSERR, "calloc g_a_target.buf %d failed %s", l_len, strerror(errno));
       }

       delete_patch(&g_a_target, p_keydata1);
       free(g_a_target.buf);
    }
    else
    {
      if ((g_b_target.buf = calloc(l_len, sizeof(char))) == NULL)
      {
         bail(EX_OSERR, "calloc g_b_target.buf %d failed %s", l_len, strerror(errno));
      }

       delete_patch(&g_b_target, p_keydata1);

       free(g_b_target.buf);
    }
 }

TRACE_OUT(1);
}

static void keydata2row(char *p_values, int32_t p_parse)
{
  char *l_cptr = strchr(p_values, GROUP_SEP);
  char *l_cptr2 = p_values;
  char *l_group_sep;

 TRACE_IN(1);

 if ((l_cptr = strchr(p_values, GROUP_SEP)) == NULL)
 {
    bail(EX_SOFTWARE, "missing statement GROUP_SEP");
 }
 
 *l_cptr = ' '; // replace values GROUP_SEP

 if (p_parse == KEY_DATA)
 {
    if ((l_group_sep = strchr(l_cptr, GROUP_SEP)) == NULL)
    {
       bail(EX_SOFTWARE, "missing keydata GROUP_SEP");
    }
 }
 else
 {
     l_group_sep = strchr(l_cptr, ')');
 }
 
 l_cptr = p_values;

 if ((l_cptr = strstr(p_values, "(\"")) != NULL)
 {
    l_cptr++;
    *l_cptr = '\'';
 }

 l_cptr = p_values;

 while((l_cptr2 = strstr(l_cptr, ",\"")) != NULL)
 {
    if (p_parse == KEY_ONLY && l_cptr2 > l_group_sep)
    {
       break;
    }

    l_cptr2++;
    *l_cptr2 = '\'';
    l_cptr = l_cptr2;
 }

 l_cptr = p_values;

 while((l_cptr2 = strstr(l_cptr, "\",")) != NULL)
 {
    if (p_parse == KEY_ONLY && l_cptr2 >= l_group_sep)
    {
       break;
    }

    *l_cptr2 = '\'';
    l_cptr2++;
    l_cptr = l_cptr2;
 }

 if (p_parse == KEY_ONLY)
 {
    l_group_sep++;
    l_group_sep = '\0';
 }
 else
 {
    // join key and data at GROUP_SEP
    l_group_sep--;
    *l_group_sep = ' ';  // left paren
    l_group_sep++;
    *l_group_sep = ',';  // group sep
    l_group_sep++;
    *l_group_sep = ' ';  // right paren
    // newline
    p_values[strlen(p_values) -1] = '\0';
 }


TRACE_OUT(1);
}

static void insert_patch(const char *p_keydata)
{
 char *l_insert =
"insert\n"
"into %s\n"
"(%s%c%s)\n"
"values\n"
"%c%s\n";
 char l_comma = ',';
 char *l_cptr;
 char *l_cptr2;
 int32_t l_len;

 TRACE_IN(1);

 if (gv.table_desc.num_cols == 0)
 {
    l_comma = ' '; 
 }

 sprintf(g_b_target.buf, l_insert, 
         g_b_target.object,
         gv.table_desc.pk_col_names, l_comma, gv.table_desc.col_names,
         GROUP_SEP, p_keydata);

 // remove delimiters from primary key and data
 keydata2row(g_b_target.buf, KEY_DATA);

 if (gv.table_desc.num_cols > 0)
 {
    if ((l_cptr = strstr(g_b_target.buf, " ,,")) != NULL)
    {
       l_len = l_cptr - g_b_target.buf;
       fprintf(gv.patchfp, "%.*s", l_len, g_b_target.buf);
       fprintf(gv.patchfp, "null");
       l_cptr++;
    }
    else
    {
       l_cptr = g_b_target.buf;
    }

    // replace ,, with ,null,
    while((l_cptr2 = strstr(l_cptr, ",,")) != NULL)
    {
       l_len = l_cptr2 - l_cptr;
       l_len++;
       fprintf(gv.patchfp, "%.*s", l_len, l_cptr);
       fprintf(gv.patchfp, "null");
       l_cptr = l_cptr2+1;
    }

    fprintf(gv.patchfp, "%s;\n\n", l_cptr);
 }
 else 
 {
    fprintf(gv.patchfp,"\n%s;", g_b_target.buf);
 }

 TRACE_OUT(1);
}

static void update_patch(const char *p_keydata1,
                         const char *p_keydata2)
{
 char *l_query =
"select\n"
"%s,%s\n"
"from   %s\n"
"where  row(%s) = %c row%.*s";

 char *l_update =
 "update\n"
 "%s\n"
 "set ";
 char *l_cptr;
 int32_t l_len;
 int32_t i;
 int32_t j;
 int32_t l_num_tuples;
 int32_t l_num_fields;
 int32_t l_set_count = 0;

 TRACE_IN(1);

 // remove delimiters from primary key
 l_cptr = strchr(p_keydata1, GROUP_SEP);
 l_len = l_cptr - p_keydata1;

 sprintf(g_a_target.buf, l_query,
         gv.table_desc.pk_col_names, gv.table_desc.col_names,
         g_a_target.object,
         gv.table_desc.pk_col_names,
         GROUP_SEP,
         l_len, p_keydata1);


 keydata2row(g_a_target.buf, KEY_ONLY);

 sprintf(g_b_target.buf, l_query,
         gv.table_desc.pk_col_names, gv.table_desc.col_names,
         g_b_target.object,
         gv.table_desc.pk_col_names,
         GROUP_SEP,
         l_len, p_keydata2);

 keydata2row(g_b_target.buf, KEY_ONLY);

 if ((g_a_target.rset = pq_exec(&g_a_target.pq, g_a_target.buf)) == NULL)
 {
    bail(EX_SOFTWARE, "patch_query_ pq_exec failed %s\n'%s'",
         g_a_target.pq.error_msg,
         g_a_target.buf);
 }

 if ((g_b_target.rset = pq_exec(&g_b_target.pq, g_b_target.buf)) == NULL)
 {
    bail(EX_SOFTWARE, "patch_query_ pq_exec failed %s\n'%s'",
         g_b_target.pq.error_msg,
         g_b_target.buf);
 }

 l_num_tuples = PQntuples(g_a_target.rset);
 l_num_fields = PQnfields(g_a_target.rset);

 for(i = 0; i < l_num_tuples; i++)
 {
    for(j = 0; j < l_num_fields; j++)
    {
       if (strcmp(PQgetvalue(g_a_target.rset, i, j), PQgetvalue(g_b_target.rset, i, j)))
       {
          if (l_set_count == 0)
          {
             fprintf(gv.patchfp, l_update, g_b_target.object);
          }
          else
          {
             fprintf(gv.patchfp, ",\n"); 
          }

          fprintf(gv.patchfp, "%s = %s", 
                  PQfname(g_a_target.rset, j),
                  PQgetisnull(g_a_target.rset, i, j) ? 
                              "null" : PQgetvalue(g_a_target.rset, i, j));
          l_set_count++;
       }
    }
 }

 fprintf(gv.patchfp, "\n");
 
 sprintf(g_b_target.buf, "where row(%s) = %c row%.*s;",
         gv.table_desc.pk_col_names, GROUP_SEP,
         l_len, p_keydata2);

 keydata2row(g_b_target.buf, KEY_ONLY);

 fprintf(gv.patchfp, "%s\n", g_b_target.buf);
 if (l_set_count != 0)
 {
    fprintf(gv.patchfp, "\n");
 } 

 PQclear(g_a_target.rset);
 PQclear(g_b_target.rset);

 TRACE_OUT(1);
}

static void delete_patch(target_t *p_target, const char *p_keydata)
{
 char *l_query =
"delete\n"
"from   %s\n"
"where  row(%s) = %c row%.*s";
 char *l_cptr;
 int32_t l_len;

 TRACE_IN(1);

 // remove delimiters from primary key
 l_cptr = strchr(p_keydata, GROUP_SEP);
 l_len = l_cptr - p_keydata;

 sprintf(p_target->buf, l_query,
         p_target->object,
         gv.table_desc.pk_col_names,
         GROUP_SEP,
         l_len, p_keydata);

 keydata2row(p_target->buf, KEY_ONLY);

 fprintf(gv.patchfp, "%s;\n\n", p_target->buf);

 TRACE_OUT(1);
}
