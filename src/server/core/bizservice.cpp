/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: bizservice.cpp
**
**/

#include "nxcore.h"

#define QUERY_LENGTH		(512)

/**
 * Service default constructor
 */
BusinessService::BusinessService() : super()
{
	/*m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = STATUS_UNKNOWN;
	_tcscpy(m_name, _T("Default"));*/
}

/**
 * Constructor for new service object
 */
BusinessService::BusinessService(const TCHAR *name) : super()
{
	/*m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = STATUS_UNKNOWN;
	nx_strncpy(m_name, name, MAX_OBJECT_NAME);*/
}

/**
 * Destructor
 */
BusinessService::~BusinessService()
{
}

/**
 * Create object from database data
 */
bool BusinessService::loadFromDatabase(DB_HANDLE hdb, uint32_t id)
{
	if (!super::loadFromDatabase(hdb, id))
		return false;

	// now it doesn't make any sense but hopefully will do in the future
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT is_prototype,prototype_id,instance,instance_method,instance_data,instance_filter ")
													_T("FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from business_services"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load biz service object %ld - record missing"), (long)m_id);
		return FALSE;
	}

   m_id = id;
   isPrototype = (bool)DBGetFieldULong(hResult, 0, 0);
   prototypeId = DBGetFieldULong(hResult, 0, 1);
	DBGetField(hResult, 0, 2, instance, 1024);
   uint32_t instanceMethod = DBGetFieldULong(hResult, 0, 3);
	DBGetField(hResult, 0, 4, instanceData, 1024);

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return TRUE;
}

/**
 * Create object from database data
 */
bool BusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
	/*DbgPrintf(2, _T("Loading service checks..."));

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM slm_checks WHERE "));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldULong(hResult, i, 0);
         auto check = make_shared<SlmCheck>();
         if (check->loadFromDatabase(hdb, id))
         {
            NetObjInsert(check, false, false);  // Insert into indexes
         }
         else     // Object load failed
         {
            check->destroy();
            nxlog_write(NXLOG_ERROR, _T("Failed to load service check object with ID %u from database"), id);
         }
      }
      DBFreeResult(hResult);
   }*/
	return true;
}

/**
 * Save service to database
 */
/*bool BusinessService::saveToDatabase(DB_HANDLE hdb)
{
   if (!IsDatabaseRecordExist(hdb, _T("business_services"), _T("service_id"), m_id))
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("INSERT INTO business_services (service_id) VALUES (%u)"), m_id);
      if (!DBQuery(hdb, query))
         return false;
   }

	return super::saveToDatabase(hdb);
}*/

/**
 * Delete object from database
 */
/*bool BusinessService::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM business_services WHERE service_id=?"));
   }
   return success;
}*/

/**
 * Create NXCP message with object's data
 */
/*void BusinessService::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
}*/

/**
 * Modify object from message
 */
/*UINT32 BusinessService::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return super::modifyFromMessageInternal(pRequest);
}*/

/**
 * Check if service is ready for poll
 */
/*bool BusinessService::isReadyForPolling()
{
   lockProperties();
	bool ready = (time(NULL) - m_lastPollTime > g_slmPollingInterval) && !m_busy && !m_pollingDisabled;
   unlockProperties();
   return ready;
}*/

/**
 * Lock service for polling
 */
/*void BusinessService::lockForPolling()
{
   lockProperties();
	m_busy = true;
   unlockProperties();
}*/

/**
 * A callback for poller threads
 */
/*void BusinessService::poll(PollerInfo *poller)
{
   poller->startExecution();
   poll(NULL, 0, poller);
   delete poller;
}*/

/**
 * Status poll
 */
/*void BusinessService::poll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller)
{
   if (IsShutdownInProgress())
   {
      m_busy = false;
      return;
   }

	DbgPrintf(5, _T("Started polling of business service %s [%d]"), m_name, (int)m_id);
	m_lastPollTime = time(NULL);*/

	// Loop through the kids and execute their either scripts or thresholds
   /*readLockChildList();
	for (int i = 0; i < getChildList().size(); i++)
	{
	   NetObj *object = getChildList().get(i);
		if (object->getObjectClass() == OBJECT_SLMCHECK)
			((SlmCheck *)object)->execute();
		else if (object->getObjectClass() == OBJECT_NODELINK)
			((NodeLink*)object)->execute();
	}
   unlockChildList();*/

	//readLockChildList()

	// Set the status based on what the kids' been up to
	//calculateCompoundStatus();

	/*m_lastPollStatus = m_status;
	DbgPrintf(5, _T("Finished polling of business service %s [%d]"), m_name, (int)m_id);
	m_busy = false;
}*/



/**
 * Prepare business service object for deletion
 */
/*void BusinessService::prepareForDeletion()
{
   // Prevent service from being queued for polling
   lockProperties();
   m_pollingDisabled = true;
   unlockProperties();

   // wait for outstanding poll to complete
   while(true)
   {
      lockProperties();
      if (!m_busy)
      {
         unlockProperties();
         break;
      }
      unlockProperties();
      ThreadSleep(100);
   }
	DbgPrintf(4, _T("BusinessService::PrepareForDeletion(%s [%d]): no outstanding polls left"), m_name, (int)m_id);
   super::prepareForDeletion();
}*/
