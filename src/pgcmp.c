#include "pgcmp.h"
#include "version.h"

target_t g_a_target;
target_t g_b_target;
uint32_t g_flags;
global_var_t gv;

extern const char *version(void);

static void check_command(const char *p_command);
static void setup(void);
static void usage(void);

/*
   parse command line and run process
*/
int32_t main(int argc, char *argv[])
{
 char *l_cptr;
 int32_t i    = 0;
 int32_t l_rc = -1;

 // initialize defaults
 memset(gv.indent, '\0', sizeof(gv.indent));
 gv.td_particles  = DEF_TD_SEGMENTS;
 gv.td_cpus       = DEF_TD_CPUS;
 gv.diff_cmd      = DEF_CMP_PROG;
 gv.cfgfile_name  = "stdin";
 gv.sql_buf_sz    = DEF_SQL_BUF_SZ;
 gv.errfp         = stderr;
 gv.td_max_errors = ULLONG_MAX;
 gv.pid           = getpid();
 gv.progname      = basename(argv[0]);
 gv.outfp         = stdout;
 gv.tmpdir        = "/tmp";

/*
   parse command line and set flags
*/
 while((i = getopt(argc, argv, ":b:c:d:e:hps:v")) != EOF)
 {
    switch(i)
    {
       case 'b':
         gv.sql_buf_sz = atoi(optarg);
       break;
       case 'c':
         gv.td_cpus = atoi(optarg);
       break;
       case 'd':
         if (!strcmp(optarg, "rusage"))
         {
            g_flags |= DBG_RUSAGE_FLG; 
         }
         else if (!strcmp(optarg, "timing"))
         {
            g_flags |= DBG_TIMING_FLG;
         }
         else if (!strcmp(optarg, "trace"))
         { 
            g_flags |= DBG_TRACE_ON_FLG;
            TRACE_IN(1)
         }
         else if (!strcmp(optarg, "verbose"))
         {
            g_flags |= DBG_VERBOSE_FLG;
         }
         else
         {
            fprintf(stderr, "%s: unknown debug option '%s'.\n", gv.progname, optarg);
            usage();
            exit(EX_USAGE);
         }
       break;
       case 'e':
         gv.td_max_errors = strtoul(optarg, &l_cptr, 10);
       break;
       case 'h':
         usage();
         exit(0);
       break;
       case 'p':
         g_flags |= CREATE_PATCH_FLG;
       break;
       case 's':
         gv.td_particles = atoi(optarg);
       break;
       case 'v':
         fprintf(stdout, "%s: version %s\n", gv.progname, version());
         exit(0);
       break;
       case '?':
         fprintf(stderr, "%s: unknown option '%c'.\n", gv.progname, optopt);
         usage();
         exit(EX_USAGE);
       break;
       case ':':
         fprintf(stderr, "%s: option '%c' missing argument.\n", 
                 gv.progname, optopt);
         usage();
         exit(EX_USAGE);
       break;
    }
 }

 // no args show help and exit
 if (argc == 1)
 {
    usage();
    exit(EX_USAGE);
 }

 i = optind;
 
 check_command(argv[i]);

 setup();

 i++;

 while(i < argc)
 {
    gv.a_lineno  = 0; 
    gv.b_lineno  = 0; 
    g_flags  |= SET_A_TARGET_FLG;
    gv.cfgfile_name = argv[i];

    if (gv.cfgfile_name[0] == '-')
    {
       gv.cfgfile_name = "stdin";
    }

    l_rc = process();
    i++;
 }

TRACE_OUT(1)

 return(l_rc);
}

/*
  check for a valid command
*/
static void check_command(const char *p_command)
{
 if (!strcmp(p_command, "global_vars"))
 {
    gv.command = GLOBAL_VARS_CMD;
 }
 else if (!strcmp(p_command, "global_privs"))
 {
    gv.command = GLOBAL_PRIVS_CMD;
 }
 else if (!strcmp(p_command, "schema_def"))
 {
    gv.command = SCHEMA_DEF_CMD;
 }
 else if (!strcmp(p_command, "pg_roles"))
 {
    gv.command = PG_ROLES_CMD;
 }
 else if (!strcmp(p_command, "schema_privs"))
 {
    gv.command = SCHEMA_PRIVS_CMD;
 }
 else if (!strcmp(p_command, "pg_settings"))
 {
    gv.command = PG_SETTINGS_CMD;
 }
 else if (!strcmp(p_command, "pg_hba_rules"))
 {
    gv.command = PG_HBA_RULES_CMD;
 }
 else if (!strcmp(p_command, "source_code"))
 {
    gv.command = SOURCE_CODE_CMD;
 }
 else if (!strcmp(p_command, "pg_ls_dir"))
 {
    gv.command = PG_LS_DIR_CMD;
 }
 else if (!strcmp(p_command, "pg_read_file"))
 {
    gv.command = PG_READ_FILE_CMD;
 }
 else if (!strcmp(p_command, "adhoc_sql"))
 {
    gv.command = ADHOC_SQL_CMD;
 }
 else if (!strcmp(p_command, "table_data"))
 {
    gv.command = TABLE_DATA_CMD;
    if (gv.td_cpus < DEF_CPU_SET_SZ)
    {
       fprintf(stderr, "%s: cpus %d must be >= %d\n",
               gv.progname, gv.td_cpus, DEF_CPU_SET_SZ);
       exit(EX_CONFIG);
    }

    if (gv.td_particles < DEF_CPU_SEGMENTS)
    {
       fprintf(stderr, "%s: segments %d must be >= %d\n",
               gv.progname, gv.td_particles, DEF_CPU_SEGMENTS);
       exit(EX_CONFIG);
    }
 }
 else
 {
    fprintf(stderr, "%s: unknown command '%s'\n", gv.progname, p_command);
    usage();
    exit(EX_USAGE);
 }

 if (gv.sql_buf_sz < DEF_SQL_BUF_SZ)
 {
    fprintf(stderr, "%s: sql_buf_sz %d must be >= %d\n",
             gv.progname, gv.sql_buf_sz, DEF_SQL_BUF_SZ);
    exit(EX_USAGE);
 }
}

/*
  set program variables
*/
static void setup(void)
{
 if ((gv.command & TABLE_DATA_CMD) == 0)
 {
    gv.td_cpus      = 2;
    gv.td_particles = 1;
 }

 if ((gv.tmpdir = getenv("PGCMP_TMPDIR")) == NULL)
 {
    if ((gv.tmpdir = getenv("TMPDIR")) == NULL)
    {
       gv.tmpdir = "/tmp";
    }
 }

 if ((gv.diff_cmd = getenv("PGCMP_CMP_PROG")) == NULL)
 {
    gv.diff_cmd = DEF_CMP_PROG;
 }
}

/*
   show help
*/
static void usage(void)
{
 fprintf(stderr, "\n");
 fprintf(stderr, "USAGE  %s [OPTIONS]... command [FILE]...\n", gv.progname);
 fprintf(stderr, "\n");
 fprintf(stderr, "Description:\n");
 fprintf(stderr, "Compare PostgreSQL database targets 'a' and 'b' and report conflicts.\n");
 fprintf(stderr, "\n");
 
 fprintf(stderr, "With no FILE, or when FILE is -, read standard input.\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "Commands:\n");
 fprintf(stderr, "- adhoc_sql\n");
 fprintf(stderr, "- global_vars\n");
 fprintf(stderr, "- pg_hba_rules\n");
 fprintf(stderr, "- pg_ls_dir\n");
 fprintf(stderr, "- pg_read_file\n");
 fprintf(stderr, "- pg_settings\n");
 fprintf(stderr, "- schema_def\n");
 fprintf(stderr, "- source_code\n");
 fprintf(stderr, "- table_data\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "Options:\n");
 fprintf(stderr, "  -b sql_buf_sz       # sql buffer size default: %d\n", DEF_SQL_BUF_SZ);
 fprintf(stderr, "  -d {rusage|timing|trace|verbose}\n");
 fprintf(stderr, "  -h                  # show help\n");
 fprintf(stderr, "  -V                  # show version\n");
 fprintf(stderr, "     table options:\n");
 fprintf(stderr, "  -c num_cpus         # cpus per target default: %d\n", DEF_TD_CPUS);
 fprintf(stderr, "  -p                  # create an update target 'b' to target 'a' patch\n");
 fprintf(stderr, "  -s num_segs         # segments reduce resource usage default: %d\n", DEF_TD_SEGMENTS);
 fprintf(stderr, "  -e max_errors       # default: unlimited\n");
 fprintf(stderr, "  \n");
 fprintf(stderr, "  Both tables require primary keys.  Table 'a' needs to be analyzed.\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "Utility Output:\n");
 fprintf(stderr, "pgcmp is a frontend to psql, pg_dump, pg_dumpall utilities, and an OS text diff program.\n"); 
 fprintf(stderr, "Export the PGCMP_CMP_PROG environment variable to something like diff, sdiff or git diff.\n");
 fprintf(stderr, "\n");
 
 fprintf(stderr, "Table Output:\n");
 fprintf(stderr, "Lines starting with a '#' contain information about the connections, objects, and config file.\n");
 fprintf(stderr, "The remaining lines are conflicts.\n");  
 fprintf(stderr, "\n");
 fprintf(stderr, "Data errors have both 'a' and 'b' table rows. Not found errors contain the existing tables row.\n");
 fprintf(stderr, "\n");
 fprintf(stderr, "Sample Table Conflict Report:\n");
 fprintf(stderr, "# a 1 acme1_5432_postgres_test.inventory != b 2 acme2_5432_postgres_test.inventory\n");
 fprintf(stderr, "# file: store_inv.cfg\n");
 fprintf(stderr, "# column names: 'ts,store,dept,category,class,in_stock'\n");
 fprintf(stderr, "# column types: 'timestamp,int4,int4,int4,int4,int4'\n");
 fprintf(stderr, "a \"1970-01-01 00:00:00\",1,1,1,1,9 b \"1970-01-01 00:00:00\",1,1,1,1,10\n");
 fprintf(stderr, "a \"1970-01-01 00:00:00\",1,20,34,32,9 b \"1970-01-01 00:00:00\",1,20,34,32,10\n");
 fprintf(stderr, "a \"1970-01-02 00:00:00\",2,8,4,32,10\n");
 fprintf(stderr, "b \"1970-01-01 00:00:00\",1,10,49,32,10\n");
 fprintf(stderr, "b \"1970-01-16 00:00:00\",1,1,1,1,9\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "Environment:\n");
 fprintf(stderr, "    PGCMP_CMP_PROG  # OS text diff program       default: \"%s\"\n", gv.diff_cmd);
 fprintf(stderr, "    PGCMP_TMPDIR    # temporary files directory  default: /tmp\n");
 fprintf(stderr, "    PATH            # include the path to the PostgreSQL bin directory.\n");

 fprintf(stderr, "\n");
 fprintf(stderr, "Configuration Files:\n");
 fprintf(stderr, "Lines contain a connection string and optional pipe delimited argument.\n");
 fprintf(stderr, "The first line is the 'a' target and is the standard. The remaining\n");
 fprintf(stderr, "lines are 'b' targets.\n\n");

 fprintf(stderr, "Target 'b' will inherit target 'a's object when it's blank.\n");
 fprintf(stderr, "conninfo is a PQconnectdb() connection string.\n");
 fprintf(stderr, "The '#' sign is a comment character.\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "command         object\n");
 fprintf(stderr, "-------------   ----------------\n");
 fprintf(stderr, "global_vars\n");
 fprintf(stderr, "pg_hba_rules\n");
 fprintf(stderr, "pg_ls_dir     | directory name \n");
 fprintf(stderr, "pg_read_file  | file name\n");
 fprintf(stderr, "pg_settings\n");
 fprintf(stderr, "schema_def    | schema name\n");
 fprintf(stderr, "source_code   | schema.proc_signature\n");
 fprintf(stderr, "adhoc_sql     | sql statement;\n");
 fprintf(stderr, "table_data    | schema.table\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "Example Config FIle Contents:\n");

 fprintf(stderr, "pg_settings Command Config File:\n");
 fprintf(stderr, "connection                           comment\n");
 fprintf(stderr, "------------------------------------ -----------------\n");
 fprintf(stderr, "host=acme1 port=5432 dbname=postgres # 'a' target\n");
 fprintf(stderr, "host=acme2 port=5432 dbname=postgres # 'b' target\n");
 fprintf(stderr, "host=acme3 port=5432 dbname=postgres # 'b' target\n");
 fprintf(stderr, "host=acme4 port=5432 dbname=postgres # 'b' target\n");
 fprintf(stderr, "\n");

 fprintf(stderr, "schema_def Command Config File:\n");
 fprintf(stderr, "connection                             object comment\n");
 fprintf(stderr, "------------------------------------   ------ ----------\n");
 fprintf(stderr, "host=acme1 port=5432 dbname=postgres | public # 'a' target a object\n");
 fprintf(stderr, "host=acme2 port=5432 dbname=postgres | public # 'b' target b object\n");
 fprintf(stderr, "host=acme3 port=5432 dbname=postgres          # 'b' target a object\n");
 fprintf(stderr, "host=acme4 port=5432 dbname=postgres | public # 'b' target b object\n");

 fprintf(stderr, "\n");
 fprintf(stderr, "Exit Codes:\n");
 fprintf(stderr, "    EX_OK             0 targets are equal\n");
 fprintf(stderr, "    EX_FAIL           1 targets are unequal\n");
 fprintf(stderr, "\n");
 fprintf(stderr, "    EX_USAGE         64 command line usage error\n");
 fprintf(stderr, "    EX_NOHOST        68 host name unknown\n");
 fprintf(stderr, "    EX_SOFTWARE      70 internal software error\n");
 fprintf(stderr, "    EX_OSERR         71 system error (e.g., can't fork)\n");
 fprintf(stderr, "    EX_OSFILE        72 critical OS file missing\n");
 fprintf(stderr, "    EX_CANTCREAT     73 can't create (user) output file\n");
 fprintf(stderr, "    EX_IOERR         74 input/output error\n");
 fprintf(stderr, "    EX_NOPERM        77 permission denied\n");
 fprintf(stderr, "    EX_CONFIG        78 configuration error\n");
 fprintf(stderr, "    BAD_TABLE_DEF    79 data type or primary key issue\n");
 fprintf(stderr, "\n");
}
