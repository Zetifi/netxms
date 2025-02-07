/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: libnxdb.h
**
**/

#ifndef _libnxsrv_h_
#define _libnxsrv_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nms_threads.h>
#include <nxdbapi.h>

#define DEBUG_TAG_CONNECTION  _T("db.conn")
#define DEBUG_TAG_DRIVER      _T("db.drv")
#define DEBUG_TAG_QUERY       _T("db.query")

/**
 * Max number of loaded database drivers
 */
#define MAX_DB_DRIVERS			16

/**
 * Database driver structure
 */
struct db_driver_t
{
	const char *m_name;
	DBDriverCallTable m_callTable;
   void (*m_fpEventHandler)(uint32_t, const WCHAR *, const WCHAR *, bool, void *);
	int m_refCount;
	int m_reconnect;
   int m_defaultPrefetchLimit;
	Mutex *m_mutexReconnect;
	HMODULE m_handle;
	void *m_context;
};

/**
 * Prepared statement
 */
struct db_statement_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_STATEMENT m_statement;
	TCHAR *m_query;
};

/**
 * Database connection structure
 */
struct db_handle_t
{
   DBDRV_CONNECTION m_connection;
	DB_DRIVER m_driver;
	bool m_reconnectEnabled;
   Mutex *m_mutexTransLock;      // Transaction lock
   int m_transactionLevel;
   char *m_server;
   char *m_login;
   char *m_password;
   char *m_dbName;
   char *m_schema;
   ObjectArray<db_statement_t> *m_preparedStatements;
   Mutex *m_preparedStatementsLock;
};

/**
 * SELECT query result
 */
struct db_result_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_RESULT m_data;
};

/**
 * Unbuffered SELECT query result
 */
struct db_unbuffered_result_t
{
	DB_DRIVER m_driver;
	DB_HANDLE m_connection;
	DBDRV_UNBUFFERED_RESULT m_data;
};

/**
 * Global variables
 */
extern uint32_t g_sqlQueryExecTimeThreshold;

#endif   /* _libnxsrv_h_ */
