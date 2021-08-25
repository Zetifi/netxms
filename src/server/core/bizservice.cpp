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
   m_autobindDCIScript = nullptr;
   m_pCompiledAutobindDCIScript = nullptr;
   m_autounbindDCIFlag = false;
   m_autobindObjectScript = nullptr;
   m_pCompiledAutobindObjectScript = nullptr;
   m_autounbindObjectFlag = false;
}

/**
 * Service default constructor
 */
BaseBusinessService::BaseBusinessService(const TCHAR* name) : m_checks(10, 10, Ownership::True), super(name, 0)
{
   m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
   m_autobindDCIScript = nullptr;
   m_pCompiledAutobindDCIScript = nullptr;
   m_autounbindDCIFlag = false;
   m_autobindObjectScript = nullptr;
   m_pCompiledAutobindObjectScript = nullptr;
   m_autounbindObjectFlag = false;
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
         NotifyClientsOnSlmCheckDelete(*this, checkId);
         //unlockProperties();
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
}

BaseBusinessService* BaseBusinessService::createBusinessService(const TCHAR* name, int objectClass, NXCPMessage *request)
{

   BaseBusinessService* service = nullptr;
   if(objectClass == OBJECT_BUSINESS_SERVICE_PROTOTYPE)
   {
      uint32_t instanceDiscoveryMethod = 0;
      if (request->isFieldExist(VID_INSTD_METHOD))
      {
         instanceDiscoveryMethod = request->getFieldAsUInt32(VID_INSTD_METHOD);
      }
      service = new BusinessServicePrototype(name, instanceDiscoveryMethod);
   }
   else
   {

      service = new BusinessService(name);
   }

   /*DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   if (hdb != nullptr)
   {

      if (!service->saveToDatabase(hdb))
      {
         delete_and_null(service);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      delete_and_null(service);
   }*/
   return service;
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
   NotifyClientsOnSlmCheckUpdate(*this, check);
}

bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;
   if (!loadChecksFromDatabase(hdb))
      return false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT object_bind_filter,object_unbind_flag,dci_bind_filter,dci_unbind_flag char ")
													_T("FROM auto_bind_target WHERE object_id=?"));

	if (hStmt == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from auto_bind_target"));
		return false;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == nullptr)
	{
		DBFreeStatement(hStmt);
		return false;
	}

   MemFree(m_autobindObjectScript);
   m_autobindObjectScript = DBGetField(hResult, 0, 0, nullptr, 0);
   m_autounbindObjectFlag = DBGetFieldLong(hResult, 0, 1) ? true : false;
   MemFree(m_autobindDCIScript);
   m_autobindDCIScript = DBGetField(hResult, 0, 2, nullptr, 0);
   m_autounbindDCIFlag = DBGetFieldLong(hResult, 0, 3) ? true : false;

   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   return true;
}

bool BaseBusinessService::saveToDatabase(DB_HANDLE hdb)
{
   if (!super::saveToDatabase(hdb))
      return  false;

   DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("business_services"), _T("service_id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE business_services SET is_prototype=?,prototype_id=?,instance=?,instance_method=?,instance_data=?,instance_filter=? WHERE service_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO business_services (is_prototype,prototype_id,instance,instance_method,instance_data,instance_filter,service_id) VALUES (?,?,?,?,?,?,?)"));
	}

   bool success = false;
	if (hStmt != nullptr)
	{
		//lockProperties();
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, (false ? _T("1") :_T("0")), DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, 0);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, 0);
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, _T(""), DB_BIND_STATIC);
		DBBind(hStmt, 6, DB_SQLTYPE_TEXT, _T(""), DB_BIND_STATIC);
		DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
		DBFreeStatement(hStmt);
		//unlockProperties();
	}

   if (success)
   {
      if (IsDatabaseRecordExist(hdb, _T("auto_bind_target"), _T("object_id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE auto_bind_target SET object_bind_filter=?,object_bind_flag='1',object_unbind_flag=?,dci_bind_filter=?,dci_bind_flag='1',dci_unbind_flag=? WHERE object_id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO auto_bind_target (object_bind_filter,object_bind_flag,object_unbind_flag,dci_bind_filter,dci_bind_flag,dci_unbind_flag,object_id) VALUES (?,'1',?,?,'1',?,?)"));
      }

      if (hStmt != nullptr)
      {
         //lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_TEXT, m_autobindObjectScript, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, (false ? _T("1") :_T("0")), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_TEXT, m_autobindDCIScript, DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, (false ? _T("1") :_T("0")), DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
         //unlockProperties();
      }
      else
      {
         success = false;
      }
   }

   return success;
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
BusinessService::BusinessService(uint32_t id, uint32_t prototypeId, const TCHAR *instance) : BaseBusinessService(id), m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
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
 * Constructor for new service object
 */
BusinessService::BusinessService(const TCHAR *name) : BaseBusinessService(name), m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
{
	/*
	m_lastPollStatus = STATUS_UNKNOWN;*/
	//_tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_prototypeId = 0;
   m_instance[0] = 0;
}

/**
 * Destructor
 */
BusinessService::~BusinessService()
{
}

uint32_t BusinessService::modifyFromMessageInternal(NXCPMessage *request)
{
   if (request->isFieldExist(VID_AUTOBIND_FLAG))
   {
      MemFree(m_autobindObjectScript);
      m_autobindObjectScript = request->getFieldAsString(VID_AUTOBIND_FLAG);
		compileObjectBindingScript();
   }
   if (request->isFieldExist(VID_AUTOUNBIND_FLAG))
   {
      MemFree(m_autobindObjectScript);
      m_autobindObjectScript = request->getFieldAsString(VID_AUTOUNBIND_FLAG);
		compileObjectBindingScript();
   }
   if (request->isFieldExist(VID_AUTOBIND_FILTER))
   {
      MemFree(m_autobindObjectScript);
      m_autobindObjectScript = request->getFieldAsString(VID_AUTOBIND_FILTER);
		compileObjectBindingScript();
   }
   return AbstractContainer::modifyFromMessageInternal(request);
}

void BusinessService::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   msg->setField(VID_AUTOBIND_FLAG, m_autobindObjectScript != nullptr);
   msg->setField(VID_AUTOUNBIND_FLAG, m_autounbindObjectFlag);
   msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_autobindObjectScript));
   /*msg->setField(VID_AUTOBIND_FLAG, m_autobindDCIScript != nullptr);
   msg->setField(VID_AUTOUNBIND_FLAG, m_autounbindDCIFlag);
   msg->setField(VID_AUTOBIND_FILTER, CHECK_NULL_EX(m_autobindDCIScript));*/
   return AbstractContainer::fillMessageInternal(msg, userId);
}

/**
 * Compile script if there is one
 */
void BusinessService::compileObjectBindingScript()
{
	if (m_autobindObjectScript == nullptr)
	   return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

	if (m_pCompiledAutobindObjectScript != nullptr)
		delete m_pCompiledAutobindObjectScript;
   m_pCompiledAutobindObjectScript = NXSLCompileAndCreateVM(m_autobindObjectScript, errorMsg, errorMsgLen, new NXSL_ServerEnv);
   if (m_pCompiledAutobindObjectScript != nullptr)
   {
      m_pCompiledAutobindObjectScript->addConstant("OK", m_pCompiledAutobindObjectScript->createValue((LONG)0));
      m_pCompiledAutobindObjectScript->addConstant("FAIL", m_pCompiledAutobindObjectScript->createValue((LONG)1));
   }
   else
   {
      nxlog_write(NXLOG_WARNING, _T("Failed to compile script for service check object %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
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

	for (auto check : m_checks)
	{
      uint32_t oldStatus = check->getStatus();
      uint32_t newStatus = check->execute();
      if (oldStatus != newStatus)
      	NotifyClientsOnSlmCheckUpdate(*this, check);
      if (newStatus > m_status)
         m_status = newStatus;
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
   m_instanceDiscoveryMethod = instanceDiscoveryMethod;
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
 * Service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype(const TCHAR *name, uint32_t instanceDiscoveryMethod) : BaseBusinessService(name), m_discoveryPollState(_T("discovery"))
{
   m_instanceDiscoveryMethod = instanceDiscoveryMethod;
   m_instanceDiscoveryData[0] = 0;
}

/**
 * Destructor
 */
BusinessServicePrototype::~BusinessServicePrototype()
{
}

uint32_t BusinessServicePrototype::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   return AbstractContainer::modifyFromMessageInternal(pRequest);
}

void BusinessServicePrototype::fillMessageInternal(NXCPMessage *pMsg, uint32_t userId)
{
   AbstractContainer::fillMessageInternal(pMsg, userId);
}

void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();

   /*StringList* instances = getInstances();
   ObjectArray<BaseBusinessService>* services = getServices();
   if (instances != nullptr && services != nullptr)
   {
      ObjectArray<BaseBusinessService> services = getServices();
      for (auto it = services.begin(); it.hasNext(); it++)
      {
         int index = instances->indexOf(it.next()->getInstance());
         if (index >= 0)
         {
            instances->remove(index);
            it.remove();
         }
         
      }
   }

   for (auto service : *services)
   {
      deleteBusinessService(service);
   }

   for (auto inst : *instances)
   {
      createBusinessService(inst);
   }*/

   delete poller;
}
