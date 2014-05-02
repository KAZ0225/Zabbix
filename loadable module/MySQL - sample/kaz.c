/*
** Zabbix
** Copyright (C) 2001-2014 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "sysinc.h"
#include "module.h"
#include "log.h"
#include <mysql/mysql.h>

/* define */
#define	STR_MAX		256
#define	CONFIG_FILE	"/etc/zabbix/loadable.conf"
#define	LM_TYPE_STR	0
#define	LM_TYPE_INT	1
#define	LM_TYPE_FLOAT	2
#define	LM_TYPE_NONE	99

/* the variable keeps timeout setting for item processing */
static int	item_timeout = 0;

int	zbx_module_get_mysql(AGENT_REQUEST *request, AGENT_RESULT *result);
int	zbx_module_init_configure();
int	zbx_module_get_configure();
int	zbx_module_init_db();
int	zbx_module_exec_sql();
int	zbx_module_close_db();
int	zbx_module_set_configure(char*);
int	zbx_module_trim(char*);

static ZBX_METRIC keys[] =
/*	KEY		FLAG		FUNCTION		TEST PARAMETERS */
{
	{"kaz.mysql",	CF_HAVEPARAMS,	zbx_module_get_mysql,	"Connections"},
	{NULL,		0x00,		NULL,			NULL}
};

/* time */
time_t	last_stat_time;
int	execution_interval;

/* mysql */
typedef struct {
	MYSQL		*conn;
	char		*server;
	char		*account;
	char		*password;
	char		*database;
	unsigned int	port;
} ZBX_MYSQL_INFO;

ZBX_MYSQL_INFO	zbx_mi;

/* configure */
struct lm_cfg_line {
	const char	*parameter;
	const char	*def;
};

struct lm_cfg_line	lm_cfg[] =	{
/*	parameter	def		*/
	{"Server",	"127.0.0.1"},
	{"Account",	"root"},
	{"Password",	"root"},
	{"Database",	"zabbix"},
	{"Port",	"3306"},
	{"Interval",	"300"},
	{NULL,		NULL}
};

/* value */
struct lm_get_values {
	const char	*keyword;
	int		type;
	char		*value;
};

struct lm_get_values	lm_gv[] =	{
/*	keyword					type		value	*/
	{"Aborted_clients",			LM_TYPE_INT,	NULL},
	{"Aborted_connects",			LM_TYPE_INT,	NULL},
	{"Binlog_cache_disk_use",		LM_TYPE_INT,	NULL},
	{"Binlog_cache_use",			LM_TYPE_INT,	NULL},
	{"Bytes_received",			LM_TYPE_INT,	NULL},
	{"Bytes_sent",				LM_TYPE_INT,	NULL},
	{"Com_admin_commands",			LM_TYPE_INT,	NULL},
	{"Com_assign_to_keycache",		LM_TYPE_INT,	NULL},
	{"Com_alter_db",			LM_TYPE_INT,	NULL},
	{"Com_alter_db_upgrade",		LM_TYPE_INT,	NULL},
	{"Com_alter_event",			LM_TYPE_INT,	NULL},
	{"Com_alter_function",			LM_TYPE_INT,	NULL},
	{"Com_alter_procedure",			LM_TYPE_INT,	NULL},
	{"Com_alter_server",			LM_TYPE_INT,	NULL},
	{"Com_alter_table",			LM_TYPE_INT,	NULL},
	{"Com_alter_tablespace",		LM_TYPE_INT,	NULL},
	{"Com_analyze",				LM_TYPE_INT,	NULL},
	{"Com_backup_table",			LM_TYPE_INT,	NULL},
	{"Com_begin",				LM_TYPE_INT,	NULL},
	{"Com_binlog",				LM_TYPE_INT,	NULL},
	{"Com_call_procedure",			LM_TYPE_INT,	NULL},
	{"Com_change_db",			LM_TYPE_INT,	NULL},
	{"Com_change_master",			LM_TYPE_INT,	NULL},
	{"Com_check",				LM_TYPE_INT,	NULL},
	{"Com_checksum",			LM_TYPE_INT,	NULL},
	{"Com_commit",				LM_TYPE_INT,	NULL},
	{"Com_create_db",			LM_TYPE_INT,	NULL},
	{"Com_create_event",			LM_TYPE_INT,	NULL},
	{"Com_create_function",			LM_TYPE_INT,	NULL},
	{"Com_create_index",			LM_TYPE_INT,	NULL},
	{"Com_create_procedure",		LM_TYPE_INT,	NULL},
	{"Com_create_server",			LM_TYPE_INT,	NULL},
	{"Com_create_table",			LM_TYPE_INT,	NULL},
	{"Com_create_trigger",			LM_TYPE_INT,	NULL},
	{"Com_create_udf",			LM_TYPE_INT,	NULL},
	{"Com_create_user",			LM_TYPE_INT,	NULL},
	{"Com_create_view",			LM_TYPE_INT,	NULL},
	{"Com_dealloc_sql",			LM_TYPE_INT,	NULL},
	{"Com_delete",				LM_TYPE_INT,	NULL},
	{"Com_delete_multi",			LM_TYPE_INT,	NULL},
	{"Com_do",				LM_TYPE_INT,	NULL},
	{"Com_drop_db",				LM_TYPE_INT,	NULL},
	{"Com_drop_event",			LM_TYPE_INT,	NULL},
	{"Com_drop_function",			LM_TYPE_INT,	NULL},
	{"Com_drop_index",			LM_TYPE_INT,	NULL},
	{"Com_drop_procedure",			LM_TYPE_INT,	NULL},
	{"Com_drop_server",			LM_TYPE_INT,	NULL},
	{"Com_drop_table",			LM_TYPE_INT,	NULL},
	{"Com_drop_trigger",			LM_TYPE_INT,	NULL},
	{"Com_drop_user",			LM_TYPE_INT,	NULL},
	{"Com_drop_view",			LM_TYPE_INT,	NULL},
	{"Com_empty_query",			LM_TYPE_INT,	NULL},
	{"Com_execute_sql",			LM_TYPE_INT,	NULL},
	{"Com_flush",				LM_TYPE_INT,	NULL},
	{"Com_grant",				LM_TYPE_INT,	NULL},
	{"Com_ha_close",			LM_TYPE_INT,	NULL},
	{"Com_ha_open",				LM_TYPE_INT,    NULL},
	{"Com_ha_read",				LM_TYPE_INT,    NULL},
	{"Com_help",				LM_TYPE_INT,    NULL},
	{"Com_insert",				LM_TYPE_INT,    NULL},
	{"Com_insert_select",			LM_TYPE_INT,    NULL},
	{"Com_install_plugin",			LM_TYPE_INT,    NULL},
	{"Com_kill",				LM_TYPE_INT,    NULL},
	{"Com_load",				LM_TYPE_INT,    NULL},
	{"Com_load_master_data",		LM_TYPE_INT,    NULL},
	{"Com_load_master_table",		LM_TYPE_INT,    NULL},
	{"Com_lock_tables",			LM_TYPE_INT,    NULL},
	{"Com_optimize",			LM_TYPE_INT,    NULL},
	{"Com_preload_keys",			LM_TYPE_INT,    NULL},
	{"Com_prepare_sql",			LM_TYPE_INT,    NULL},
	{"Com_purge",				LM_TYPE_INT,    NULL},
	{"Com_purge_before_date",		LM_TYPE_INT,    NULL},
	{"Com_release_savepoint",		LM_TYPE_INT,    NULL},
	{"Com_rename_table",			LM_TYPE_INT,    NULL},
	{"Com_rename_user",			LM_TYPE_INT,    NULL},
	{"Com_repair",				LM_TYPE_INT,    NULL},
	{"Com_replace",				LM_TYPE_INT,    NULL},
	{"Com_replace_select",			LM_TYPE_INT,    NULL},
	{"Com_reset",				LM_TYPE_INT,    NULL},
	{"Com_restore_table",			LM_TYPE_INT,    NULL},
	{"Com_revoke",				LM_TYPE_INT,    NULL},
	{"Com_revoke_all",			LM_TYPE_INT,    NULL},
	{"Com_rollback",			LM_TYPE_INT,    NULL},
	{"Com_rollback_to_savepoint",		LM_TYPE_INT,    NULL},
	{"Com_savepoint",			LM_TYPE_INT,    NULL},
	{"Com_select",				LM_TYPE_INT,    NULL},
	{"Com_set_option",			LM_TYPE_INT,    NULL},
	{"Com_show_authors",			LM_TYPE_INT,    NULL},
	{"Com_show_binlog_events",		LM_TYPE_INT,    NULL},
	{"Com_show_binlogs",			LM_TYPE_INT,    NULL},
	{"Com_show_charsets",			LM_TYPE_INT,    NULL},
	{"Com_show_collations",			LM_TYPE_INT,    NULL},
	{"Com_show_column_types",		LM_TYPE_INT,    NULL},
	{"Com_show_contributors",		LM_TYPE_INT,    NULL},
	{"Com_show_create_db",			LM_TYPE_INT,    NULL},
	{"Com_show_create_event",		LM_TYPE_INT,    NULL},
	{"Com_show_create_func",		LM_TYPE_INT,    NULL},
	{"Com_show_create_proc",		LM_TYPE_INT,    NULL},
	{"Com_show_create_table",		LM_TYPE_INT,    NULL},
	{"Com_show_create_trigger",		LM_TYPE_INT,    NULL},
	{"Com_show_databases",			LM_TYPE_INT,    NULL},
	{"Com_show_engine_logs",		LM_TYPE_INT,    NULL},
	{"Com_show_engine_mutex",		LM_TYPE_INT,    NULL},
	{"Com_show_engine_status",		LM_TYPE_INT,    NULL},
	{"Com_show_events",			LM_TYPE_INT,    NULL},
	{"Com_show_errors",			LM_TYPE_INT,    NULL},
	{"Com_show_fields",			LM_TYPE_INT,    NULL},
	{"Com_show_function_status",		LM_TYPE_INT,    NULL},
	{"Com_show_grants",			LM_TYPE_INT,    NULL},
	{"Com_show_keys",			LM_TYPE_INT,    NULL},
	{"Com_show_master_status",		LM_TYPE_INT,    NULL},
	{"Com_show_new_master",			LM_TYPE_INT,    NULL},
	{"Com_show_open_tables",		LM_TYPE_INT,    NULL},
	{"Com_show_plugins",			LM_TYPE_INT,    NULL},
	{"Com_show_privileges",			LM_TYPE_INT,    NULL},
	{"Com_show_procedure_status",		LM_TYPE_INT,    NULL},
	{"Com_show_processlist",		LM_TYPE_INT,    NULL},
	{"Com_show_profile",			LM_TYPE_INT,    NULL},
	{"Com_show_profiles",			LM_TYPE_INT,    NULL},
	{"Com_show_slave_hosts",		LM_TYPE_INT,    NULL},
	{"Com_show_slave_status",		LM_TYPE_INT,    NULL},
	{"Com_show_status",			LM_TYPE_INT,    NULL},
	{"Com_show_storage_engines",		LM_TYPE_INT,    NULL},
	{"Com_show_table_status",		LM_TYPE_INT,    NULL},
	{"Com_show_tables",			LM_TYPE_INT,    NULL},
	{"Com_show_triggers",			LM_TYPE_INT,    NULL},
	{"Com_show_variables",			LM_TYPE_INT,    NULL},
	{"Com_show_warnings",			LM_TYPE_INT,    NULL},
	{"Com_slave_start",			LM_TYPE_INT,    NULL},
	{"Com_slave_stop",			LM_TYPE_INT,    NULL},
	{"Com_stmt_close",			LM_TYPE_INT,    NULL},
	{"Com_stmt_execute",			LM_TYPE_INT,    NULL},
	{"Com_stmt_fetch",			LM_TYPE_INT,    NULL},
	{"Com_stmt_prepare",			LM_TYPE_INT,    NULL},
	{"Com_stmt_reprepare",			LM_TYPE_INT,    NULL},
	{"Com_stmt_reset",			LM_TYPE_INT,    NULL},
	{"Com_stmt_send_long_data",		LM_TYPE_INT,    NULL},
	{"Com_truncate",			LM_TYPE_INT,    NULL},
	{"Com_uninstall_plugin",		LM_TYPE_INT,    NULL},
	{"Com_unlock_tables",			LM_TYPE_INT,    NULL},
	{"Com_update",				LM_TYPE_INT,    NULL},
	{"Com_update_multi",			LM_TYPE_INT,    NULL},
	{"Com_xa_commit",			LM_TYPE_INT,    NULL},
	{"Com_xa_end",				LM_TYPE_INT,    NULL},
	{"Com_xa_prepare",			LM_TYPE_INT,    NULL},
	{"Com_xa_recover",			LM_TYPE_INT,    NULL},
	{"Com_xa_rollback",			LM_TYPE_INT,    NULL},
	{"Com_xa_start",			LM_TYPE_INT,    NULL},
	{"Compression",				LM_TYPE_STR,	NULL},
	{"Connections",				LM_TYPE_INT,    NULL},
	{"Created_tmp_disk_tables",		LM_TYPE_INT,    NULL},
	{"Created_tmp_files",			LM_TYPE_INT,    NULL},
	{"Created_tmp_tables",			LM_TYPE_INT,    NULL},
	{"Delayed_errors",			LM_TYPE_INT,    NULL},
	{"Delayed_insert_threads",		LM_TYPE_INT,    NULL},
	{"Delayed_writes",			LM_TYPE_INT,    NULL},
	{"Flush_commands",			LM_TYPE_INT,    NULL},
	{"Handler_commit",			LM_TYPE_INT,    NULL},
	{"Handler_delete",			LM_TYPE_INT,    NULL},
	{"Handler_discover",			LM_TYPE_INT,    NULL},
	{"Handler_prepare",			LM_TYPE_INT,    NULL},
	{"Handler_read_first",			LM_TYPE_INT,    NULL},
	{"Handler_read_key",			LM_TYPE_INT,    NULL},
	{"Handler_read_next",			LM_TYPE_INT,    NULL},
	{"Handler_read_prev",			LM_TYPE_INT,    NULL},
	{"Handler_read_rnd",			LM_TYPE_INT,    NULL},
	{"Handler_read_rnd_next",		LM_TYPE_INT,    NULL},
	{"Handler_rollback",			LM_TYPE_INT,    NULL},
	{"Handler_savepoint",			LM_TYPE_INT,    NULL},
	{"Handler_savepoint_rollback",		LM_TYPE_INT,    NULL},
	{"Handler_update",			LM_TYPE_INT,    NULL},
	{"Handler_write",			LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_data",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_dirty",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_flushed",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_free",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_misc",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_pages_total",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_read_ahead_rnd",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_read_ahead_seq",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_read_requests",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_reads",		LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_wait_free",	LM_TYPE_INT,    NULL},
	{"Innodb_buffer_pool_write_requests",	LM_TYPE_INT,    NULL},
	{"Innodb_data_fsyncs",			LM_TYPE_INT,    NULL},
	{"Innodb_data_pending_fsyncs",		LM_TYPE_INT,    NULL},
	{"Innodb_data_pending_reads",		LM_TYPE_INT,    NULL},
	{"Innodb_data_pending_writes",		LM_TYPE_INT,    NULL},
	{"Innodb_data_read",			LM_TYPE_INT,    NULL},
	{"Innodb_data_reads",			LM_TYPE_INT,    NULL},
	{"Innodb_data_writes",			LM_TYPE_INT,    NULL},
	{"Innodb_data_written",			LM_TYPE_INT,    NULL},
	{"Innodb_dblwr_pages_written",		LM_TYPE_INT,    NULL},
	{"Innodb_dblwr_writes",			LM_TYPE_INT,    NULL},
	{"Innodb_log_waits",			LM_TYPE_INT,    NULL},
	{"Innodb_log_write_requests",		LM_TYPE_INT,    NULL},
	{"Innodb_log_writes",			LM_TYPE_INT,    NULL},
	{"Innodb_os_log_fsyncs",		LM_TYPE_INT,    NULL},
	{"Innodb_os_log_pending_fsyncs",	LM_TYPE_INT,    NULL},
	{"Innodb_os_log_pending_writes",	LM_TYPE_INT,    NULL},
	{"Innodb_os_log_written",		LM_TYPE_INT,    NULL},
	{"Innodb_page_size",			LM_TYPE_INT,    NULL},
	{"Innodb_pages_created",		LM_TYPE_INT,    NULL},
	{"Innodb_pages_read",			LM_TYPE_INT,    NULL},
	{"Innodb_pages_written",		LM_TYPE_INT,    NULL},
	{"Innodb_row_lock_current_waits",	LM_TYPE_INT,    NULL},
	{"Innodb_row_lock_time",		LM_TYPE_INT,    NULL},
	{"Innodb_row_lock_time_avg",		LM_TYPE_INT,    NULL},
	{"Innodb_row_lock_time_max",		LM_TYPE_INT,    NULL},
	{"Innodb_row_lock_waits",		LM_TYPE_INT,    NULL},
	{"Innodb_rows_deleted",			LM_TYPE_INT,    NULL},
	{"Innodb_rows_inserted",		LM_TYPE_INT,    NULL},
	{"Innodb_rows_read",			LM_TYPE_INT,    NULL},
	{"Innodb_rows_updated",			LM_TYPE_INT,    NULL},
	{"Key_blocks_not_flushed",		LM_TYPE_INT,    NULL},
	{"Key_blocks_unused",			LM_TYPE_INT,    NULL},
	{"Key_blocks_used",			LM_TYPE_INT,    NULL},
	{"Key_read_requests",			LM_TYPE_INT,    NULL},
	{"Key_reads",				LM_TYPE_INT,    NULL},
	{"Key_write_requests",			LM_TYPE_INT,    NULL},
	{"Key_writes",				LM_TYPE_INT,    NULL},
	{"Last_query_cost",			LM_TYPE_FLOAT,	NULL},
	{"Max_used_connections",		LM_TYPE_INT,    NULL},
	{"Not_flushed_delayed_rows",		LM_TYPE_INT,    NULL},
	{"Open_files",				LM_TYPE_INT,    NULL},
	{"Open_streams",			LM_TYPE_INT,    NULL},
	{"Open_table_definitions",		LM_TYPE_INT,    NULL},
	{"Open_tables",				LM_TYPE_INT,    NULL},
	{"Opened_files",			LM_TYPE_INT,    NULL},
	{"Opened_table_definitions",		LM_TYPE_INT,    NULL},
	{"Opened_tables",			LM_TYPE_INT,    NULL},
	{"Prepared_stmt_count",			LM_TYPE_INT,    NULL},
	{"Qcache_free_blocks",			LM_TYPE_INT,    NULL},
	{"Qcache_free_memory",			LM_TYPE_INT,    NULL},
	{"Qcache_hits",				LM_TYPE_INT,    NULL},
	{"Qcache_inserts",			LM_TYPE_INT,    NULL},
	{"Qcache_lowmem_prunes",		LM_TYPE_INT,    NULL},
	{"Qcache_not_cached",			LM_TYPE_INT,    NULL},
	{"Qcache_queries_in_cache",		LM_TYPE_INT,    NULL},
	{"Qcache_total_blocks",			LM_TYPE_INT,    NULL},
	{"Queries",				LM_TYPE_INT,    NULL},
	{"Questions",				LM_TYPE_INT,    NULL},
	{"Rpl_status",				LM_TYPE_STR,	NULL},
	{"Select_full_join",			LM_TYPE_INT,    NULL},
	{"Select_full_range_join",		LM_TYPE_INT,    NULL},
	{"Select_range",			LM_TYPE_INT,    NULL},
	{"Select_range_check",			LM_TYPE_INT,    NULL},
	{"Select_scan",				LM_TYPE_INT,    NULL},
	{"Slave_open_temp_tables",		LM_TYPE_INT,    NULL},
	{"Slave_retried_transactions",		LM_TYPE_INT,    NULL},
	{"Slave_running",			LM_TYPE_STR,	NULL},
	{"Slow_launch_threads",			LM_TYPE_INT,    NULL},
	{"Slow_queries",			LM_TYPE_INT,    NULL},
	{"Sort_merge_passes",			LM_TYPE_INT,    NULL},
	{"Sort_range",				LM_TYPE_INT,    NULL},
	{"Sort_rows",				LM_TYPE_INT,    NULL},
	{"Sort_scan",				LM_TYPE_INT,    NULL},
	{"Ssl_accept_renegotiates",		LM_TYPE_INT,    NULL},
	{"Ssl_accepts",				LM_TYPE_INT,    NULL},
	{"Ssl_callback_cache_hits",		LM_TYPE_INT,    NULL},
	{"Ssl_cipher",				LM_TYPE_INT,    NULL},
	{"Ssl_cipher_list",			LM_TYPE_INT,    NULL},
	{"Ssl_client_connects",			LM_TYPE_INT,    NULL},
	{"Ssl_connect_renegotiates",		LM_TYPE_INT,    NULL},
	{"Ssl_ctx_verify_depth",		LM_TYPE_INT,    NULL},
	{"Ssl_ctx_verify_mode",			LM_TYPE_INT,    NULL},
	{"Ssl_default_timeout",			LM_TYPE_INT,    NULL},
	{"Ssl_finished_accepts",		LM_TYPE_INT,    NULL},
	{"Ssl_finished_connects",		LM_TYPE_INT,    NULL},
	{"Ssl_session_cache_hits",		LM_TYPE_INT,    NULL},
	{"Ssl_session_cache_misses",		LM_TYPE_INT,    NULL},
	{"Ssl_session_cache_mode",		LM_TYPE_STR,	NULL},
	{"Ssl_session_cache_overflows",		LM_TYPE_INT,    NULL},
	{"Ssl_session_cache_size",		LM_TYPE_INT,    NULL},
	{"Ssl_session_cache_timeouts",		LM_TYPE_INT,    NULL},
	{"Ssl_sessions_reused",			LM_TYPE_INT,    NULL},
	{"Ssl_used_session_cache_entries",	LM_TYPE_INT,    NULL},
	{"Ssl_verify_depth",			LM_TYPE_INT,    NULL},
	{"Ssl_verify_mode",			LM_TYPE_INT,    NULL},
	{"Ssl_version",				LM_TYPE_INT,    NULL},
	{"Table_locks_immediate",		LM_TYPE_INT,    NULL},
	{"Table_locks_waited",			LM_TYPE_INT,    NULL},
	{"Tc_log_max_pages_used",		LM_TYPE_INT,    NULL},
	{"Tc_log_page_size",			LM_TYPE_INT,    NULL},
	{"Tc_log_page_waits",			LM_TYPE_INT,    NULL},
	{"Threads_cached",			LM_TYPE_INT,    NULL},
	{"Threads_connected",			LM_TYPE_INT,    NULL},
	{"Threads_created",			LM_TYPE_INT,    NULL},
	{"Threads_running",			LM_TYPE_INT,    NULL},
	{"Uptime",				LM_TYPE_INT,    NULL},
	{"Uptime_since_flush_status",		LM_TYPE_INT,    NULL},
	{NULL,					LM_TYPE_NONE,	NULL}
};

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_api_version                                           *
 *                                                                            *
 * Purpose: returns version number of the module interface                    *
 *                                                                            *
 * Return value: ZBX_MODULE_API_VERSION_ONE - the only version supported by   *
 *               Zabbix currently                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_api_version()
{
	return ZBX_MODULE_API_VERSION_ONE;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_timeout                                          *
 *                                                                            *
 * Purpose: set timeout value for processing of items                         *
 *                                                                            *
 * Parameters: timeout - timeout in seconds, 0 - no timeout set               *
 *                                                                            *
 ******************************************************************************/
void	zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_list                                             *
 *                                                                            *
 * Purpose: returns list of item keys supported by the module                 *
 *                                                                            *
 * Return value: list of item keys                                            *
 *                                                                            *
 ******************************************************************************/
ZBX_METRIC	*zbx_module_item_list()
{
	return keys;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init                                                  *
 *                                                                            *
 * Purpose: the function is called on agent startup                           *
 *          It should be used to call any initialization routines             *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
 *                                                                            *
 ******************************************************************************/

int	zbx_module_init()
{
	int ret = ZBX_MODULE_FAIL;
	/* initialization */
	zbx_module_init_configure();
	ret = zbx_module_get_configure();
/*
printf("server   :%s\n", zbx_mi.server);
printf("account  :%s\n", zbx_mi.account);
printf("password :%s\n", zbx_mi.password);
printf("database :%s\n", zbx_mi.database);
printf("port     :%d\n", zbx_mi.port);
*/
	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_uninit                                                *
 *                                                                            *
 * Purpose: the function is called on agent shutdown                          *
 *          It should be used to cleanup used resources if there are any      *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_uninit()
{
	return ZBX_MODULE_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_get_mysql                                             *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters: request - structure that contains item key and parameters      *
 *              request->key - item key without parameters                    *
 *              request->nparam - number of parameters                        *
 *              request->timeout - processing should not take longer than     *
 *                                 this number of seconds                     *
 *              request->params[N-1] - pointers to item key parameters        *
 *                                                                            *
 *             result - structure that will contain result                    *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_get_mysql(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	int	i, ret = SYSINFO_RET_FAIL;
	char	*c_keyword, *error;
	long	n;
	float	f;

	// parameter check
	if (request->nparam != 1) {
		goto end;
	}

	// keyword
	c_keyword = get_rparam(request, 0);

	// interval check
	if (time(NULL) - last_stat_time >= execution_interval){
		if(zbx_module_init_db() == SYSINFO_RET_FAIL) goto end;
		zbx_module_exec_sql();
		last_stat_time = time(NULL);
	}

	// search memory
	for (i = 0; NULL != lm_gv[i].keyword; i++) {
		if (!strcmp(lm_gv[i].keyword, c_keyword)) {
			if (lm_gv[i].value == NULL) {
				zabbix_log(LOG_LEVEL_WARNING, "%s : no - value", c_keyword);
				break;
			}
			switch (lm_gv[i].type) {
			case LM_TYPE_INT:
				n = strtol(lm_gv[i].value, &error, 0);
				if (*error == '\0') {
					SET_UI64_RESULT(result, n);
					ret = SYSINFO_RET_OK;
				}
				break;
			case LM_TYPE_FLOAT:
				f = strtof(lm_gv[i].value, &error);
				if (*error == '\0') {
					SET_DBL_RESULT(result, f);
					ret = SYSINFO_RET_OK;
				}
				break;
			default: /* LM_TYPE_STR */
				SET_STR_RESULT(result, strdup(lm_gv[i].value));
				ret = SYSINFO_RET_OK;
				break;
			}
			break;
			ret = SYSINFO_RET_OK;
		}
	}

end:
	return ret;
}
/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init_db                                               *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_init_db()
{
	int     ret = SYSINFO_RET_FAIL;

	if (zbx_mi.conn == NULL){
		// connect
		zbx_mi.conn = mysql_init(NULL);
		if (mysql_real_connect(zbx_mi.conn, zbx_mi.server, zbx_mi.account, zbx_mi.password, zbx_mi.database, zbx_mi.port, NULL, 0)) {
			ret = SYSINFO_RET_OK;
		}else{
			zabbix_log(LOG_LEVEL_ERR, "mysql_real_connect error : %s", mysql_error(zbx_mi.conn));
			zbx_module_close_db();
		}
	}else{
		zabbix_log(LOG_LEVEL_WARNING, "zbx_module_init_db : A persistent connection.");
		ret = SYSINFO_RET_OK;
	}
	return ret;
}
/******************************************************************************
 *                                                                            *
 * Function: zbx_module_exec_sql                                              *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_exec_sql()
{
	int		i, ret = SYSINFO_RET_FAIL;
	unsigned int	j;

	MYSQL_RES	*res;
	MYSQL_ROW	row;
	my_ulonglong	row_count;

	// execute query
	if (mysql_query(zbx_mi.conn, "show status;")) {
		zabbix_log(LOG_LEVEL_ERR, "mysql_query error : %s", mysql_error(zbx_mi.conn));
		goto end;
	}
	if ((res = mysql_store_result(zbx_mi.conn)) == NULL) goto end;
	
	// parse
	row_count = mysql_num_rows(res);

	for (j = 0; j < row_count; j++) {
		row = mysql_fetch_row(res);
		for (i = 0; NULL != lm_gv[i].keyword; i++) {
			if (!strcmp(lm_gv[i].keyword, row[0])) {
				if (lm_gv[i].value != NULL) free(lm_gv[i].value);
				lm_gv[i].value = strdup(row[1]);
			}
		}
	}
	ret = SYSINFO_RET_OK;

end:
	if (res != NULL) mysql_free_result(res);
	if (ret == SYSINFO_RET_FAIL) zbx_module_close_db();
	return ret;

}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_close_db                                              *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Return value: SYSINFO_RET_FAIL - function failed, item will be marked      *
 *                                 as not supported by zabbix                 *
 *               SYSINFO_RET_OK - success                                     *
 *                                                                            *
 * Comment:                                                                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_close_db()
{
	int ret = SYSINFO_RET_FAIL;
	if (zbx_mi.conn) {
		/* mysql disconnect */
		mysql_close(zbx_mi.conn);
		zbx_mi.conn = NULL;
	}

	ret = SYSINFO_RET_OK;

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init_configure                                        *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int zbx_module_init_configure()
{
	char	*str;

	zbx_mi.conn	= NULL;

	str = (char *)malloc(sizeof(lm_cfg[0].def));
	strcpy(str, lm_cfg[0].def);
	zbx_mi.server	= str;

	str = (char *)malloc(sizeof(lm_cfg[1].def));
	strcpy(str, lm_cfg[1].def);
	zbx_mi.account	= str;

	str = (char *)malloc(sizeof(lm_cfg[2].def));
	strcpy(str, lm_cfg[2].def);
	zbx_mi.password	= str;

	str = (char *)malloc(sizeof(lm_cfg[3].def));
	strcpy(str, lm_cfg[3].def);
	zbx_mi.database	= str;

	zbx_mi.port	= (unsigned int)atoi(lm_cfg[4].def);

	execution_interval = atoi(lm_cfg[5].def);

	last_stat_time = 0;

	return ZBX_MODULE_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_get_configure                                         *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
 int zbx_module_get_configure()
{
	char c_line[STR_MAX];
	FILE *fl;

	if ((fl = fopen(CONFIG_FILE, "r")) == NULL) return ZBX_MODULE_FAIL;
	while (fgets(c_line, STR_MAX, fl) != NULL) zbx_module_set_configure(c_line);
	fclose(fl);

	return ZBX_MODULE_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_set_configure                                         *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int zbx_module_set_configure(char *c_line)
{
	int	i, ret;
	char	c_param[STR_MAX], *str;
	size_t	i_len;

	memset(c_param, 0, STR_MAX);
	for (i = 0; NULL != lm_cfg[i].parameter; i++) {
		i_len = strlen(lm_cfg[i].parameter);
		if (!strncmp(c_line, lm_cfg[i].parameter, i_len)) {
			while (c_line[i_len++] != '=') {}
			strcpy(c_param, &c_line[i_len]);
			ret = zbx_module_trim(c_param);

			if (ret == ZBX_MODULE_OK && strlen(c_param) > 0) {
				switch (i) {
				case 0:	/* Server */
					str = (char *)malloc(sizeof(c_param));
					strcpy(str, c_param);
					zbx_mi.server = str;
					break;
				case 1:	/* Account */
					str = (char *)malloc(sizeof(c_param));
					strcpy(str, c_param);
					zbx_mi.account = str;
					break;
				case 2:	/* Password */
					str = (char *)malloc(sizeof(c_param));
					strcpy(str, c_param);
					zbx_mi.password = str;
					break;
				case 3:	/* Database */
					str = (char *)malloc(sizeof(c_param));
					strcpy(str, c_param);
					zbx_mi.database = str;
					break;
				case 4:	/* Port */
					zbx_mi.port = (unsigned int)atoi(c_param);
					break;
				case 5:	/* Interval */
					execution_interval = atoi(c_param);
					break;
				default:
					break;
				}
			}
			break;
		}
	}
	return ZBX_MODULE_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_trim                                                  *
 *                                                                            *
 * Purpose:                                                                   *
 *                                                                            *
 * Parameters:                                                                *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int zbx_module_trim(char *s)
{
	int i, j;

	if (s == NULL) return ZBX_MODULE_FAIL;

	// delete last LF
	i = (int)strlen(s) - 1;
	while (i >= 0 && s[i] == '\n' ) i--;
	s[i+1] = '\0';

	// delete last space
	i = (int)strlen(s) - 1;
	while (i >= 0 && s[i] == ' ' ) i--;
	s[i+1] = '\0';

	// delete top space or tab
	i = 0;
	while (i < (int)strlen(s) && (s[i] == ' ' || s[i] == '\t')) i++;

	// copy
	j = 0;
	while (s[i] != '\0') s[j++] = s[i++];
	s[j]='\0';

	return ZBX_MODULE_OK;
}
