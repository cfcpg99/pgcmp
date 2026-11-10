### pgcmp - Find inconsistencies in PostgresSQL configurations, schemas, and table data
```text

USAGE  pgcmp [OPTIONS]... command [FILE]...

```
#### Description:
```text
Compare PostgreSQL database targets 'a' and 'b' and report conflicts.

With no FILE, or when FILE is -, read standard input.

```
#### Commands:
- adhoc_sql
- global_vars
- pg_hba_rules
- pg_ls_dir
- pg_read_file
- pg_settings
- schema_def
- source_code
- table_data

#### Installation:
```text
requires: pg_config

git clone 'https://github.com/cfcpg99/pgcmp.git'

cd pgcmp
make
sudo make install
installs pgcmp in /usr/local/bin

```
#### Options:
```text
-b sql_buf_sz       # sql buffer size default: 2048
-d {rusage|timing|trace|verbose}
-h                  # show help
-V                  # show version
table options:
-c num_cpus         # cpus per target default: 2
-p                  # create an update target 'b' to target 'a' patch
-s num_segs         # segments reduce resource usage default: 1
-e max_errors       # default: unlimited

Tables require primary keys.

```
#### Utility Output:
```text
pgcmp is a frontend to psql, pg_dump, pg_dumpall utilities, and an OS text diff program.
Export the PGCMP_CMP_PROG environment variable to something like diff, sdiff or git diff.

```
#### Table Output:
```text
Lines starting with a '#' contain information about the connections, objects, and config file.
The remaining lines are conflicts.

Data errors have both 'a' and 'b' table rows. Not found errors contain the existing tables row.

Sample Table Conflict Report:
# a 1 acme1_5432_postgres_test.inventory != b 2 acme2_5432_postgres_test.inventory
# file: store_inv.cfg
# column names: 'ts,store,dept,category,class,in_stock'
# column types: 'timestamp,int4,int4,int4,int4,int4'
a "1970-01-01 00:00:00",1,1,1,1,9 b "1970-01-01 00:00:00",1,1,1,1,10
a "1970-01-01 00:00:00",1,20,34,32,9 b "1970-01-01 00:00:00",1,20,34,32,10
a "1970-01-02 00:00:00",2,8,4,32,10
b "1970-01-01 00:00:00",1,10,49,32,10
b "1970-01-16 00:00:00",1,1,1,1,9

```
#### Environment:
```text
PGCMP_CMP_PROG  # OS text diff program       default: "diff --ignore-all-space"
PGCMP_TMPDIR    # temporary files directory  default: /tmp
PATH            # include the path to the PostgreSQL bin directory.

```
#### Configuration Files:
```text
Lines contain a connection string and optional pipe delimited argument.
The first line is the 'a' target and is the standard. The remaining
lines are 'b' targets.

Target 'b' will inherit target 'a's object when it's blank.
conninfo is a PQconnectdb() connection string.
The '#' sign is a comment character.

command         object
-------------   ----------------
global_vars
pg_hba_rules
pg_ls_dir     | directory name
pg_read_file  | file name
pg_settings
schema_def    | schema name
source_code   | schema.proc_signature
adhoc_sql     | sql statement;
table_data    | schema.table

Example Config FIle Contents:
pg_settings Command Config File:
connection                           comment
------------------------------------ -----------------
host=acme1 port=5432 dbname=postgres # 'a' target
host=acme2 port=5432 dbname=postgres # 'b' target
host=acme3 port=5432 dbname=postgres # 'b' target
host=acme4 port=5432 dbname=postgres # 'b' target

schema_def Command Config File:
connection                             object comment
------------------------------------   ------ ----------
host=acme1 port=5432 dbname=postgres | public # 'a' target a object
host=acme2 port=5432 dbname=postgres | public # 'b' target b object
host=acme3 port=5432 dbname=postgres          # 'b' target a object
host=acme4 port=5432 dbname=postgres | public # 'b' target b object

```
#### Exit Codes:
```text
EX_OK             0 targets are equal
EX_FAIL           1 targets are unequal

EX_USAGE         64 command line usage error
EX_NOHOST        68 host name unknown
EX_SOFTWARE      70 internal software error
EX_OSERR         71 system error (e.g., can't fork)
EX_OSFILE        72 critical OS file missing
EX_CANTCREAT     73 can't create (user) output file
EX_IOERR         74 input/output error
EX_NOPERM        77 permission denied
EX_CONFIG        78 configuration error
BAD_TABLE_DEF    79 data type or primary key issue

```
License
This project is licensed under the terms specified in the LICENSE file.
 
MIT License
Copyright (c) 2026 pgcmp
 
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Version](https://img.shields.io/badge/version-0.90-green.svg)
