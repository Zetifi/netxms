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

/* ************************************
 *
 * Base Business Service class
 *
 * *************************************
*/

/**
 * Service default constructor
 */
BaseBusinessService::BaseBusinessService(uint32_t id) : m_checks(10, 10, Ownership::True)
{
   m_id = id;
   m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
}

/**
 * Load SLM checks from database
 */
bool BaseBusinessService::loadChecksFromDatabase(DB_HANDLE hdb)
{
	nxlog_debug_tag(DEBUG_TAG, 4, _T("Loading service checks for business service %ld"), (long)m_id);

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id,service_id,type,description,related_object,related_dci,status_threshold,content,current_ticket ")
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

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);
	return true;
}

void BaseBusinessService::deleteCheck(uint32_t checkId)
{
   for (auto it = m_checks.begin(); it.hasNext();)
   {
      if (it.next()->getId() == checkId)
      {
         it.remove();
         deleteCheckFromDatabase(checkId);
         break;
      }
   }
}

void BaseBusinessService::deleteCheckFromDatabase(uint32_t checkId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if(hdb != nullptr)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM slm_checks WHERE id=?"));
      if (hStmt != nullptr)
      {
         //lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, checkId);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
         //unlockProperties();
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
}

BaseBusinessService* BaseBusinessService::createBusinessService(DB_HANDLE hdb, uint32_t id)
{
   BaseBusinessService* service = nullptr;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT prototype_id,instance,instance_method,instance_data,instance_filter ")
													_T("FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from business_services"));
		return service;
	}

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return service;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot load business service object %ld - record missing"), (long)id);
		return service;
	}

   TCHAR instance[1024];
   TCHAR instanceDiscoveryData[1024];
   uint32_t prototypeId = DBGetFieldULong(hResult, 0, 0);
	DBGetField(hResult, 0, 1, instance, 1024);
   uint32_t instanceDiscoveryMethod = DBGetFieldULong(hResult, 0, 2);
	DBGetField(hResult, 0, 3, instanceDiscoveryData, 1024);

   if(instanceDiscoveryMethod != 0)
   {
      service = new BusinessServicePrototype(id, instanceDiscoveryMethod, instanceDiscoveryData);
   }
   else
   {
      service = new BusinessService(id, prototypeId, instance);
   }

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

   if (!service->loadFromDatabase(hdb, id))
   {
      delete_and_null(service);
   }

   return service;
}

void BaseBusinessService::modifyCheckFromMessage(NXCPMessage *request)
{
   uint32_t checkId = request->getFieldAsUInt32(VID_SLMCHECK_ID);
   SlmCheck* check = nullptr;
   if (checkId != 0)
   {
      for (auto c : m_checks)
      {
         if(c->getId() == checkId)
         {
            check = c;
            break;
         }
      }
   }
   if(check == nullptr)
   {
      check = new SlmCheck(m_id);
      m_checks.add(check);
   }
   check->modifyFromMessage(request);
}

bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return  false;
   return loadChecksFromDatabase(hdb);
}


/* ************************************
 *
 * Business Service class
 *
 * *************************************
*/

/**
 * Constructor for new service object
 */
BusinessService::BusinessService(uint32_t id, uint32_t prototypeId, const TCHAR *instance) : BaseBusinessService(id), m_statusPollState(_T("status")), m_configurationPollState(_T("status"))
{
	/*
	m_lastPollStatus = STATUS_UNKNOWN;*/
	//_tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_prototypeId = prototypeId;
   if (instance != nullptr)
   {
      _tcsncpy(m_instance, instance, 1023);
   }
   else
   {
      m_instance[0] = 0;
   }
}

/**
 * Destructor
 */
BusinessService::~BusinessService()
{
}



/**
 * Check if service is ready for poll
 */
bool BusinessService::isReadyForPolling()
{
   lockProperties();
	bool ready = (time(NULL) - m_lastPollTime > g_slmPollingInterval) && !m_busy && !m_pollingDisabled;
   unlockProperties();
   return ready;
}

/**
 * Lock service for polling
 */
void BusinessService::lockForPolling()
{
   lockProperties();
	m_busy = true;
   unlockProperties();
}

void BusinessService::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   if (IsShutdownInProgress())
   {
      m_busy = false;
      return;
   }

	DbgPrintf(5, _T("Started polling of business service %s [%d]"), m_name, (int)m_id);
	m_lastPollTime = time(NULL);

	// Loop through the kids and execute their either scripts or thresholds
   readLockChildList();
   calculateCompoundStatus();
   unlockChildList();

	for (int i = 0; i < m_checks.size(); i++)
	{
      uint32_t checkStatus = m_checks.get(i)->execute();
      if (checkStatus > m_status)
         m_status = checkStatus;
	}

	//m_lastPollStatus = m_status;
	DbgPrintf(5, _T("Finished status polling of business service %s [%d]"), m_name, (int)m_id);
	m_busy = false;
   delete poller;
}

void BusinessService::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->startExecution();

   poller->setStatus(_T("wait for lock"));
   //pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      //pollerUnlock();
      return;
   }

   //m_pollRequestor = pSession;
   //m_pollRequestId = dwRqId;

   nxlog_debug_tag(DEBUG_TAG, 5, _T("Business service(%s): Auto binding SLM checks"), m_name);
   if (true)
      updateSLMChecks();

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG, 6, _T("BusinessServiceConfPoll(%s): finished"), m_name);

   //pollerUnlock();
   delete poller;
}

void BusinessService::updateSLMChecks()
{

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


/* ************************************
 *
 * Functions
 *
 * *************************************
*/

void GetCheckList(uint32_t serviceId, NXCPMessage *response)
{
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return;
   }

   int counter = 0;
   for (auto check : *service->getChecks())
   {
      check->fillMessage(response, VID_SLM_CHECKS_LIST_BASE + (counter * 10));
      counter++;
   }
   response->setField(VID_SLMCHECKS_COUNT, counter);
   
}

uint32_t ModifyCheck(NXCPMessage *request)
{
   uint32_t serviceId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }
   service->modifyCheckFromMessage(request);
   return RCC_SUCCESS;
}


uint32_t DeleteCheck(uint32_t serviceId, uint32_t checkId)
{
   shared_ptr<BaseBusinessService> service = static_pointer_cast<BaseBusinessService>(FindObjectById(serviceId));
   if (service == nullptr)
   {
      return RCC_INVALID_OBJECT_ID;
   }

   service->deleteCheck(checkId);

   return RCC_SUCCESS;
}


/* ************************************
 *
 * Business Service Prototype
 *
 * *************************************
*/

/**
 * Service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype(uint32_t id, uint32_t instanceDiscoveryMethod, const TCHAR *instanceDiscoveryData) : BaseBusinessService(id), m_discoveryPollState(_T("discovery"))
{
   if (instanceDiscoveryData != nullptr)
   {
      _tcsncpy(m_instanceDiscoveryData, instanceDiscoveryData, 1023);
   }
   else
   {
      m_instanceDiscoveryData[0] = 0;
   }
}

/**
 * Destructor
 */
BusinessServicePrototype::~BusinessServicePrototype()
{
}

void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();


   delete poller;
}
