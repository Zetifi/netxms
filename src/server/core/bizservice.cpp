/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Raden Solutions
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

#define DEBUG_TAG _T("business.service")

/**
 * Service default constructor
 */
BusinessService::BusinessService() : m_checks(10, 10, Ownership::True), super()
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

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT is_prototype,prototype_id,instance,instance_method,instance_data,instance_filter ")
													_T("FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from business_services"));
		return false;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return false;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot load business service object %ld - record missing"), (long)m_id);
		return false;
	}

   m_id = id;
   isPrototype = (bool)DBGetFieldULong(hResult, 0, 0);
   prototypeId = DBGetFieldULong(hResult, 0, 1);
	DBGetField(hResult, 0, 2, instance, 1024);
   uint32_t instanceMethod = DBGetFieldULong(hResult, 0, 3);
	DBGetField(hResult, 0, 4, instanceData, 1024);

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return true;
}

/**
 * Create object from database data
 */
bool BusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
	nxlog_debug_tag(DEBUG_TAG, 4, _T("Loading service checks for business service %ld"), (long)m_id);

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,type,description,related_object,related_dci,status_threshold,content,current_ticket ")
													_T("FROM slm_checks WHERE service_id=?"));

	if (hStmt == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from slm_checks"));
		return false;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == nullptr)
	{
		DBFreeStatement(hStmt);
		return false;
	}

   int rows = DBGetNumRows(hResult);
   for (int i = 0; i < rows; i++)
   {
      SlmCheck* check = new SlmCheck();
      check->loadFromSelect(hResult, i);
      m_checks.add(check);
   }

	return true;
}


void BusinessService::deleteCheck(uint32_t checkId)
{
   for (auto it = m_checks.begin(); it.hasNext(); it++)
   {
      if (it.value()->getId() == checkId)
      {
         it.remove();
         break;
      }
   }
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
 * Check if service is ready for poll
 */
bool BusinessService::isReadyForPolling()
{
   //lockProperties();
	//bool ready = (time(NULL) - m_lastPollTime > g_slmPollingInterval) && !m_busy && !m_pollingDisabled;
   //unlockProperties();
   return true;
}

/**
 * Lock service for polling
 */
void BusinessService::lockForPolling()
{
   /*lockProperties();
	m_busy = true;
   unlockProperties();*/
}

/**
 * A callback for poller threads
 */
void BusinessService::poll(PollerInfo *poller)
{
   poller->startExecution();
   poll(NULL, 0, poller);
   delete poller;
}

/**
 * Status poll
 */
void BusinessService::poll(ClientSession *pSession, UINT32 dwRqId, PollerInfo *poller)
{
   /*if (IsShutdownInProgress())
   {
      m_busy = false;
      return;
   }*/

	DbgPrintf(5, _T("Started polling of business service %s [%d]"), m_name, (int)m_id);
	//m_lastPollTime = time(NULL);

	// Loop through the kids and execute their either scripts or thresholds
   //readLockChildList();

   // Set the status based on what the kids' been up to
   calculateCompoundStatus();

	for (int i = 0; i < m_checks.size(); i++)
	{
      uint32_t checkStatus = m_checks.get(i)->execute();
      if (checkStatus > m_status)
         m_status = checkStatus;
	}

	//m_lastPollStatus = m_status;
	DbgPrintf(5, _T("Finished polling of business service %s [%d]"), m_name, (int)m_id);
	//m_busy = false;
}



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

void GetCheckList(uint32_t serviceId, NXCPMessage *response)
{
   shared_ptr<BusinessService> service = static_pointer_cast<BusinessService>(FindObjectById(serviceId, OBJECT_BUSINESS_SERVICE));
   if (service == nullptr)
   {
      return;
   }

   int counter = 0;
   for ( auto check : *service->getChecks())
   {
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10), check->getId());
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10) + 1, check->getType());
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10) + 2, check->getReason());
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10) + 3, check->getRelatedDCI());
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10) + 4, check->getRelatedObject());
      response->setField(VID_SLM_CHECKS_LIST_BASE + (counter * 10) + 5, check->getCurrentTicket());
      counter++;
   }
   response->setField(VID_SLMCHECKS_COUNT, counter);
   
}


uint32_t ModifyCheck(NXCPMessage *request)
{
   uint32_t serviceId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<BusinessService> service = static_pointer_cast<BusinessService>(FindObjectById(serviceId, OBJECT_BUSINESS_SERVICE));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }

   uint32_t checkId = request->getFieldAsUInt32(VID_SLMCHECK_ID);
   SlmCheck* check = nullptr;
   for ( auto c : *service->getChecks())
   {
      if(c->getId() == checkId)
      {
         check = c;
         break;
      }
   }

   if(check == nullptr)
   {
      check = new SlmCheck();
      service->addCheck(check);
   }
   check->modifyFromMessage(request);

   return RCC_SUCCESS;
}


uint32_t DeleteCheck(uint32_t serviceId, uint32_t checkId)
{
   shared_ptr<BusinessService> service = static_pointer_cast<BusinessService>(FindObjectById(serviceId, OBJECT_BUSINESS_SERVICE));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }

   service->deleteCheck(checkId);

   return RCC_SUCCESS;
}