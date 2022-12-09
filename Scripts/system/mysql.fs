
autoforget mysql

DLLVocabulary mysql libmysql.dll
also mysql definitions

DLLStdCall

// look at http://zetcode.com/tutorials/mysqlcapitutorial/   and    http://dev.mysql.com/doc/refman/5.0/en/c-api.html

// MYSQL* mysql_init( MYSQL* )

dll_1 mysql_init

// MYSQL *mysql_real_connect(MYSQL *mysql, const char *host, const char *user, const char *passwd,
//   const char *db, unsigned int port, const char *unix_socket, unsigned long client_flag)
// mysql_real_connect(con, "localhost", "user12", "34klq*", "testdb", 0, NULL, 0)
dll_8 mysql_real_connect

// int mysql_query(MYSQL *mysql, const char *stmt_str)
dll_2 mysql_query

// void mysql_close(MYSQL *mysql)
DLLVoid dll_1 mysql_close

// MYSQL_RES *mysql_store_result(MYSQL *mysql)
dll_1 mysql_store_result

// MYSQL_ROW mysql_fetch_row(MYSQL_RES *result)
dll_1 mysql_fetch_row

// int mysql_real_query(MYSQL *mysql, const char *stmt_str, unsigned long length)
dll_3 mysql_real_query

// MYSQL_RES *mysql_use_result(MYSQL *mysql)
dll_1 mysql_use_result

// unsigned long *mysql_fetch_lengths(MYSQL_RES *result)
dll_1 mysql_fetch_lengths

// my_ulonglong mysql_num_rows(MYSQL_RES *result)
DLLLong dll_1 mysql_num_rows

// unsigned int mysql_num_fields(MYSQL_RES *result)
dll_1 mysql_num_fields

// my_ulonglong mysql_insert_id(MYSQL *mysql)
DLLLong dll_1 mysql_insert_id

// void mysql_free_result(MYSQL_RES *result)
DLLVoid dll_1 mysql_free_result

loaddone
