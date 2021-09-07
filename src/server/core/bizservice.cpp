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
BaseBusinessService::BaseBusinessService() : m_checks(10, 10, Ownership::True), super(), AutoBindTarget(this)
{
   m_id = 0;
   m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
   m_prototypeId = 0;
   m_instance = nullptr;
   m_instanceDiscoveryMethod = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

/**
 * Service default constructor
 */
BaseBusinessService::BaseBusinessService(const TCHAR* name) : m_checks(10, 10, Ownership::True), super(name, 0), AutoBindTarget(this)
{
   m_busy = false;
   m_pollingDisabled = false;
	m_lastPollTime = time_t(0);
   m_prototypeId = 0;
   m_instance = nullptr;
   m_instanceDiscoveryMethod = 0;
   m_instanceDiscoveryData = nullptr;
   m_instanceDiscoveryFilter = nullptr;
   m_objectStatusThreshhold = 0;
   m_dciStatusThreshhold = 0;
}

BaseBusinessService::~BaseBusinessService()
{
   MemFree(m_instance);
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
   return service;
}

BaseBusinessService* BaseBusinessService::createBusinessService(DB_HANDLE hdb, uint32_t id)
{
   BaseBusinessService* service = nullptr;
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT is_prototype ")
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

   bool isPrototype = (bool)DBGetFieldULong(hResult, 0, 0);

   if(isPrototype)
   {
      service = new BusinessServicePrototype();
   }
   else
   {
      service = new BusinessService();
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

uint32_t BaseBusinessService::modifyFromMessageInternal(NXCPMessage *request)
{
   AutoBindTarget::modifyFromMessage(request);
   if (request->isFieldExist(VID_INSTANCE))
   {
      MemFree(m_instance);
      m_instance = request->getFieldAsString(VID_INSTANCE);
   }
   if (request->isFieldExist(VID_INSTD_METHOD))
   {
      m_instanceDiscoveryMethod = request->getFieldAsUInt32(VID_INSTD_METHOD);
   }
   if (request->isFieldExist(VID_INSTD_DATA))
   {
      MemFree(m_instanceDiscoveryData);
      m_instanceDiscoveryData = request->getFieldAsString(VID_INSTD_DATA);
   }
   if (request->isFieldExist(VID_INSTD_FILTER))
   {
      MemFree(m_instanceDiscoveryFilter);
      m_instanceDiscoveryFilter = request->getFieldAsString(VID_INSTD_FILTER);
   }
   return super::modifyFromMessageInternal(request);
}

void BaseBusinessService::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   AutoBindTarget::fillMessage(msg);
   msg->setField(VID_INSTANCE, m_instance);
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_DATA, m_instanceDiscoveryData);
   msg->setField(VID_INSTD_FILTER, m_instanceDiscoveryFilter);
   msg->setField(VID_OBJECT_STATUS_THRESHOLD, m_objectStatusThreshhold);
   msg->setField(VID_DCI_STATUS_THRESHOLD, m_dciStatusThreshhold);
   return super::fillMessageInternal(msg, userId);
}

bool BaseBusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;
   if (!loadChecksFromDatabase(hdb))
      return false;
   if (!AutoBindTarget::loadFromDatabase(hdb, id))
      return false;

   return true;
}

bool BaseBusinessService::saveToDatabase(DB_HANDLE hdb)
{
   if (!super::saveToDatabase(hdb))
      return  false;
   DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("business_services"), _T("service_id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE business_services SET is_prototype=?,prototype_id=?,instance=?,instance_method=?,instance_data=?,instance_filter=?,object_status_threshold=?,dci_status_threshold=? WHERE service_id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO business_services (is_prototype,prototype_id,instance,instance_method,instance_data,instance_filter,object_status_threshold,dci_status_threshold,service_id) VALUES (?,?,?,?,?,?,?,?,?)"));
	}
   bool success = false;
	if (hStmt != nullptr)
	{
		//lockProperties();
		DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, getObjectClass() == OBJECT_BUSINESS_SERVICE_PROTOTYPE ? _T("1") :_T("0"), DB_BIND_STATIC);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_prototypeId);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_instance, DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_instanceDiscoveryMethod);
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_instanceDiscoveryData, DB_BIND_STATIC);
		DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_instanceDiscoveryFilter, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_objectStatusThreshhold);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_dciStatusThreshhold);
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
		DBFreeStatement(hStmt);
		//unlockProperties();
	}
   if (success)
   {
      success = AutoBindTarget::saveToDatabase(hdb);
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
BusinessService::BusinessService() : m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
{
   m_hPollerMutex = MutexCreate();
	/*
	m_lastPollStatus = STATUS_UNKNOWN;*/
	//_tcslcpy(m_name, name, MAX_OBJECT_NAME);
   /*m_pCompiledAutobindDCIScript = nullptr;
   m_pCompiledAutobindObjectScript = nullptr;*/
}

/**
 * Constructor for new service object
 */
BusinessService::BusinessService(const TCHAR *name) : BaseBusinessService(name), m_statusPollState(_T("status")), m_configurationPollState(_T("configuration"))
{
   m_hPollerMutex = MutexCreate();
	/*
	m_lastPollStatus = STATUS_UNKNOWN;*/
	//_tcslcpy(m_name, name, MAX_OBJECT_NAME);
   /*m_pCompiledAutobindDCIScript = nullptr;
   m_pCompiledAutobindObjectScript = nullptr;*/
}

/**
 * Destructor
 */
BusinessService::~BusinessService()
{
}

uint32_t BusinessService::modifyFromMessageInternal(NXCPMessage *request)
{
   super::modifyFromMessageInternal(request);


   /*compileObjectBindingScript();
   compileDCIBindingScript();*/
   return super::modifyFromMessageInternal(request);
}

bool BusinessService::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;

   return true;
}


/**
 * Entry point for status poll worker thread
 */
void BusinessService::statusPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   statusPoll(poller, session, rqId);
   delete poller;
}

void BusinessService::statusPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   if (IsShutdownInProgress())
   {
      sendPollerMsg(_T("Server shutdown in progress, poll canceled \r\n"));
      m_busy = false;
      return;
   }

	nxlog_debug_tag(DEBUG_TAG, 5, _T("Started polling of business service %s [%d]"), m_name, (int)m_id);
   sendPollerMsg(_T("Started status poll of business service %s [%d] \r\n"), m_name, (int)m_id);
	m_lastPollTime = time(nullptr);

	// Loop through the kids and execute their either scripts or thresholds
   readLockChildList();
   calculateCompoundStatus();
   unlockChildList();

	for (auto check : m_checks)
	{
      SlmTicketData data = {};
      uint32_t oldStatus = check->getStatus();
      uint32_t newStatus = check->execute(&data);

      if(data.ticket_id != 0)
      {
         unique_ptr<SharedObjectArray<NetObj>> parents = getParents();
         for( auto parent : *parents )
         {
            if(parent->getObjectClass() == OBJECT_BUSINESS_SERVICE)
            {
               static_pointer_cast<BusinessService>(parent)->addChildTicket(&data);
            }
         }
      }
      if (oldStatus != newStatus)
      {
         sendPollerMsg(_T("SLM check \"%s\" status changed, set to: %s\r\n"), check->getName(), newStatus == STATUS_CRITICAL ? _T("Critical") : newStatus == STATUS_NORMAL ? _T("Normal") : _T("Unknown"));
      	NotifyClientsOnSlmCheckUpdate(*this, check);
      }
      if (newStatus > m_status)
      {
         sendPollerMsg(_T("Business service status changed, set to: %s\r\n"), newStatus == STATUS_CRITICAL ? _T("Critical") : newStatus == STATUS_NORMAL ? _T("Normal") : _T("Unknown"));
         m_status = newStatus;
      }
	}

	//m_lastPollStatus = m_status;
   sendPollerMsg(_T("Finished status polling of business service %s [%d] \r\n"), m_name, (int)m_id);
	nxlog_debug_tag(DEBUG_TAG, 5, _T("Finished status polling of business service %s [%d]"), m_name, (int)m_id);
	m_busy = false;
}

void BusinessService::addChildTicket(SlmTicketData *data)
{
   unique_ptr<SharedObjectArray<NetObj>> parents = getParents();
   for( auto parent : *parents )
   {
      if(parent->getObjectClass() == OBJECT_BUSINESS_SERVICE)
      {
         static_pointer_cast<BusinessService>(parent)->addChildTicket(data);
      }
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO slm_tickets (ticket_id,original_ticket_id,original_service_id,check_id,check_description,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_SLM_TICKET));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, data->ticket_id);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, data->service_id);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, data->check_id);
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, data->description, DB_BIND_TRANSIENT);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (uint32_t)(data->create_timestamp));
		DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, data->reason, DB_BIND_TRANSIENT);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
}

void BusinessService::configurationPollWorkerEntry(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   poller->startObjectTransaction();
   configurationPoll(poller, session, rqId);
   poller->endObjectTransaction();
   delete poller;
}

void BusinessService::configurationPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   lockProperties();
   if (m_isDeleteInitiated || IsShutdownInProgress())
   {
      m_configurationPollState.complete(0);
      sendPollerMsg(_T("Server shutdown in progress, poll canceled \r\n"));
      unlockProperties();
      return;
   }
   unlockProperties();

   poller->setStatus(_T("wait for lock"));
   pollerLock(configuration);

   if (IsShutdownInProgress())
   {
      pollerUnlock();
      return;
   }

   objectCheckAutoBinding();
   dciCheckAutoBinding();

   //m_pollRequestor = pSession;
   //m_pollRequestId = dwRqId;

   sendPollerMsg(_T("Configuration poll finished\r\n"));
   nxlog_debug_tag(DEBUG_TAG, 6, _T("BusinessServiceConfPoll(%s): finished"), m_name);

   pollerUnlock();
}

void BusinessService::objectCheckAutoBinding()
{
   if (isAutoBindEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): Auto binding object SLM checks"), m_name);
      sendPollerMsg(_T("Business service(%s): Auto binding object SLM checks \r\n"), m_name);
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      int bindedCount = 0;
      int unbindedCount = 0;
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         if (object != nullptr)
         {
            AutoBindDecision des = isApplicable(object);
            if (des != AutoBindDecision_Ignore)
            {
               uint32_t foundCheckId = 0;
               for (auto check : m_checks)
               {
                  if (check->getType() == SlmCheck::OBJECT && check->getRelatedObject() == object->getId())
                  {
                     foundCheckId = check->getId();
                     break;
                  }
               }
               if (foundCheckId != 0 && des == AutoBindDecision_Unbind && isAutoUnbindEnabled())
               {
                  nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): object check %ld unbinded"), foundCheckId);
                  deleteCheck(foundCheckId);
                  unbindedCount++;
               }
               if (foundCheckId == 0 && des == AutoBindDecision_Bind)
               {
                  SlmCheck* check = new SlmCheck(m_id);
                  m_checks.add(check);
                  check->setRelatedObject(object->getId());
                  TCHAR checkName[MAX_OBJECT_NAME];
                  _sntprintf(checkName, MAX_OBJECT_NAME, _T("%s[%ld] check"), object->getName(), object->getId());
                  check->setName(checkName);
                  check->generateId();
                  check->setThreshold(m_objectStatusThreshhold != 0 ? m_objectStatusThreshhold : ConfigReadInt(_T("BusinessServices.Check.Threshold.Objects"), 1));
                  check->saveToDatabase();
                  nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): object check %s[%ld] binded"), checkName, foundCheckId);
                  NotifyClientsOnSlmCheckUpdate(*this, check);
                  bindedCount++;
               }
            }
         }
      }
      sendPollerMsg(_T("Binded new object SLM checks: %ld, unbinded old object SLM checks: %ld \r\n"), bindedCount, unbindedCount);
   }
   else
   {
      sendPollerMsg(_T("Autobind for objects disabled \r\n"));
   }
}

void BusinessService::dciCheckAutoBinding()
{
   if (isAutoBindDCIEnabled())
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): Auto binding DCI SLM checks"), m_name);
      sendPollerMsg(_T("Business service(%s): Auto binding DCI SLM checks \r\n"), m_name);
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      int bindedCount = 0;
      int unbindedCount = 0;
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         if (object != nullptr && object->isDataCollectionTarget())
         {
            shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(object);
            unique_ptr<IntegerArray<uint32_t>> dciIds = target->getDCIIds();
            for (int y = 0; y < dciIds->size(); y++)
            {
               shared_ptr<DCObject> dci = target->getDCObjectById(dciIds->get(y), 0);
               if (dci != nullptr)
               {
                  AutoBindDecision des = isApplicable(object, dci);
                  if (des != AutoBindDecision_Ignore)
                  {
                     uint32_t foundCheckId = 0;
                     for (auto check : m_checks)
                     {
                        if (check->getType() == SlmCheck::DCI && check->getRelatedObject() == object->getId() && check->getRelatedDCI() == dci->getId())
                        {
                           foundCheckId = check->getId();
                           break;
                        }
                     }
                     if (foundCheckId != 0 && des == AutoBindDecision_Unbind && isAutoUnbindEnabled())
                     {
                        nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): DCI check %ld unbinded"), foundCheckId);
                        deleteCheck(foundCheckId);
                        unbindedCount++;
                     }
                     if (foundCheckId == 0 && des == AutoBindDecision_Bind)
                     {
                        SlmCheck* check = new SlmCheck(m_id);
                        m_checks.add(check);
                        check->setType(SlmCheck::DCI);
                        check->setRelatedObject(object->getId());
                        check->setRelatedDCI(dci->getId());
                        TCHAR checkName[MAX_OBJECT_NAME];
                        _sntprintf(checkName, MAX_OBJECT_NAME, _T("%s in %s[%ld] DCI check"), dci->getName().cstr(), object->getName(), object->getId());
                        check->setName(checkName);
                        check->generateId();
                        check->setThreshold(m_dciStatusThreshhold != 0 ? m_dciStatusThreshhold : ConfigReadInt(_T("BusinessServices.Check.Threshold.DataCollection"), 1));
                        check->saveToDatabase();
                        nxlog_debug_tag(DEBUG_TAG, 2, _T("Business service(%s): DCI check %s[%ld] binded"), checkName, foundCheckId);
                        NotifyClientsOnSlmCheckUpdate(*this, check);
                        bindedCount++;
                     }
                  }
               }
            }

         }
      }
      sendPollerMsg(_T("Binded new DCI SLM checks: %ld, unbinded old DCI SLM checks: %ld \r\n"), bindedCount, unbindedCount);
   }
   else
   {
      sendPollerMsg(_T("Autobind for DCI disabled \r\n"));
   }
}

/**
 * Lock node for status poll
 */
bool BusinessService::lockForStatusPoll()
{
   bool success = false;

   lockProperties();      
   if (static_cast<uint32_t>(time(nullptr) - m_statusPollState.getLastCompleted()) > g_statusPollingInterval)
   {
      success = m_statusPollState.schedule();
   }
   unlockProperties();
   return success;
}

/**
 * Lock object for configuration poll
 */
bool BusinessService::lockForConfigurationPoll()
{
   bool success = false;

   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_configurationPollState.getLastCompleted()) > g_configurationPollingInterval)
   {
      success = m_configurationPollState.schedule();
   }
   unlockProperties();
   return success;
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

double GetServiceUptime(uint32_t serviceId, time_t from, time_t to)
{
   double res = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT from_timestamp,to_timestamp FROM slm_service_history ")
                                          _T("WHERE service_id=? AND "
                                             "((from_timestamp BETWEEN ? AND ? OR to_timestamp BETWEEN ? and ?) OR (from_timestamp<? AND ( to_timestamp=0 OR to_timestamp>? )))"));
      if (hStmt == NULL)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from slm_service_history"));
      }
		else
		{
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, to);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, to);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, to);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            time_t totalUptime = to - from;
            int count = DBGetNumRows(hResult);
            for (int i = 0; i < count; i++)
            {
               time_t from_timestamp = DBGetFieldUInt64(hResult, i, 1);
               time_t to_timestamp = DBGetFieldUInt64(hResult, i, 2);
               time_t downtime = (to_timestamp > to ? to : to_timestamp) - (from_timestamp < from ? from : from_timestamp);
               totalUptime -= downtime;
            }
            double res = (double)totalUptime / (double)((to - from) / 100);
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   return res;
}

void GetServiceTickets(uint32_t serviceId, time_t from, time_t to, NXCPMessage* msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (hdb)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT ticket_id,original_ticket_id,original_service_id,check_id,create_timestamp,close_timestamp,reason FROM slm_tickets ")
                                          _T("WHERE service_id=? AND "
                                             "((create_timestamp BETWEEN ? AND ? OR close_timestamp BETWEEN ? and ?) OR (create_timestamp<? AND ( close_timestamp=0 OR close_timestamp>? )))"));
      if (hStmt == NULL)
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from slm_tickets"));
      }
		else
		{
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, serviceId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, to);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, to);
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, from);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, to);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for (int i = 0; i < count; i++)
            {
               uint32_t ticket_id = DBGetFieldUInt64(hResult, i, 1);
               uint32_t original_ticket_id = DBGetFieldUInt64(hResult, i, 2);
               uint32_t original_service_id = DBGetFieldUInt64(hResult, i, 3);
               uint32_t check_id = DBGetFieldUInt64(hResult, i, 4);
               time_t create_timestamp = DBGetFieldUInt64(hResult, i, 5);
               time_t close_timestamp = DBGetFieldUInt64(hResult, i, 6);
               TCHAR reason[256];
               DBGetField(hResult, i, 7, reason, 256);

               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ),     original_ticket_id != 0 ? original_ticket_id : ticket_id);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 1, original_ticket_id != 0 ? original_service_id : serviceId);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 2, check_id);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 3, create_timestamp);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 4, close_timestamp);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 5, reason);
               msg->setField(VID_SLM_TICKETS_LIST_BASE + ( i * 10 ) + 6, _T("Placeholder for check description"));
            }
            msg->setField(VID_SLM_TICKETS_COUNT, count);
            DBFreeResult(hResult);
         }
         DBFreeStatement(hStmt);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
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
BusinessServicePrototype::BusinessServicePrototype() : m_discoveryPollState(_T("discovery"))
{
   m_instanceDiscoveryMethod = 0;
   m_instanceDiscoveryData = nullptr;
   m_pCompiledInstanceDiscoveryScript = nullptr;
}

/**
 * Service prototype constructor
 */
BusinessServicePrototype::BusinessServicePrototype(const TCHAR *name, uint32_t instanceDiscoveryMethod) : BaseBusinessService(name), m_discoveryPollState(_T("discovery"))
{
   m_instanceDiscoveryMethod = instanceDiscoveryMethod;
   m_instanceDiscoveryData = nullptr;
   m_pCompiledInstanceDiscoveryScript = nullptr;
}

/**
 * Destructor
 */
BusinessServicePrototype::~BusinessServicePrototype()
{
}

bool BusinessServicePrototype::loadFromDatabase(DB_HANDLE hdb, UINT32 id)
{
   if (!super::loadFromDatabase(hdb, id))
      return false;

   if (m_pCompiledInstanceDiscoveryScript != nullptr)
   {
      delete_and_null(m_pCompiledInstanceDiscoveryScript);
   }
   compileInstanceDiscoveryScript();
   return true;
}

uint32_t BusinessServicePrototype::modifyFromMessageInternal(NXCPMessage *request)
{
   if (request->isFieldExist(VID_INSTD_METHOD))
   {
      m_instanceDiscoveryMethod = request->getFieldAsUInt32(VID_INSTD_METHOD);
   }
   if (request->isFieldExist(VID_INSTD_DATA))
   {
      MemFree(m_instanceDiscoveryData);
      m_instanceDiscoveryData = request->getFieldAsString(VID_INSTD_DATA);
		compileInstanceDiscoveryScript();
   }
   return super::modifyFromMessageInternal(request);
}

/**
 * Compile object script if there is one
 */
void BusinessServicePrototype::compileInstanceDiscoveryScript()
{
	if (m_instanceDiscoveryData == nullptr)
	   return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

	if (m_pCompiledInstanceDiscoveryScript != nullptr)
		delete m_pCompiledInstanceDiscoveryScript;
   m_pCompiledInstanceDiscoveryScript = NXSLCompile(m_instanceDiscoveryData, errorMsg, errorMsgLen, nullptr);
   if (m_pCompiledInstanceDiscoveryScript != nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Failed to compile script for service instance discovery %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
}

void BusinessServicePrototype::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   msg->setField(VID_INSTD_METHOD, m_instanceDiscoveryMethod);
   msg->setField(VID_INSTD_FILTER, m_instanceDiscoveryData);
   super::fillMessageInternal(msg, userId);
}

unique_ptr<StringList> BusinessServicePrototype::getInstances()
{
   StringList* instances = new StringList();
   NXSL_VM *filter = nullptr;

   if (m_pCompiledInstanceDiscoveryScript != nullptr)
   {
      filter = CreateServerScriptVM(m_pCompiledInstanceDiscoveryScript, nullptr);
   }

   if (filter == nullptr)
   {
      /*TCHAR buffer[1024];
      _sntprintf(buffer, 1024, _T("%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), m_this->getId());
      PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, _T("Script load error"), m_this->getId());
      nxlog_write(NXLOG_WARNING, _T("Failed to load autobind script for object %s [%u]"), m_this->getName(), m_this->getId());*/
   }

   if (filter != nullptr)
   {
      if (filter->run())
      {
         NXSL_Value *value = filter->getResult();
         if (value->isArray())
         {
            value->getValueAsArray()->toStringList(instances);
         }
      }
      else
      {
         /*internalLock();
         TCHAR buffer[1024];
         _sntprintf(buffer, 1024, _T("%s::%s::%d"), m_this->getObjectClassName(), m_this->getName(), m_this->getId());
         PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, filter->getErrorText(), m_this->getId());
         nxlog_write(NXLOG_WARNING, _T("Failed to execute autobind script for object %s [%u] (%s)"), m_this->getName(), m_this->getId(), filter->getErrorText());
         internalUnlock();*/
      }
      delete filter;
   }
   return unique_ptr<StringList>(instances);
}

unique_ptr<SharedObjectArray<BusinessService>> BusinessServicePrototype::getServices()
{
   unique_ptr<SharedObjectArray<BusinessService>> services = make_unique<SharedObjectArray<BusinessService>>();
   unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
   for (int i = 0; i < objects->size(); i++)
   {
      shared_ptr<NetObj> object = objects->getShared(i);
      if (object != nullptr && object->getObjectClass() == OBJECT_BUSINESS_SERVICE && static_pointer_cast<BusinessService>(object)->getPrototypeId() == m_id)
      {
         services->add(static_pointer_cast<BusinessService>(object));
      }
   }
   return services;
}

void BusinessServicePrototype::instanceDiscoveryPoll(PollerInfo *poller, ClientSession *session, UINT32 rqId)
{
   poller->startExecution();
   unique_ptr<StringList> instances = getInstances();
   unique_ptr<SharedObjectArray<BusinessService>> services = getServices();
   if (instances != nullptr && services != nullptr)
   {
      for (auto it = services->begin(); it.hasNext(); )
      {
         int index = instances->indexOf(it.next()->getInstance());
         if(index >= 0)
         {
            it.remove();
            instances->remove(index);
         }
      }
   }

   for (auto it = services->begin(); it.hasNext(); )
   {
      ObjectTransactionStart();
      it.next()->deleteObject();
      ObjectTransactionEnd();
      it.remove();
   }

   for (int i = 0; i < instances->size(); i++ )
   {
      TCHAR serviceName[MAX_OBJECT_NAME];
      _sntprintf(serviceName, MAX_OBJECT_NAME, _T("%s [%s]"), m_name, instances->get(i));
      auto service = make_shared<BusinessService>(serviceName);
      if (service != nullptr)
      {
         service->setInstance(instances->get(i));
         service->setPrototypeId(m_id);
         //service->setChecks(m_checks);
         NetObjInsert(service, true, false);  // Insert into indexes
      }
      else     // Object load failed
      {
         nxlog_write(NXLOG_ERROR, _T("Failed to create business service object"));
      }
   }

   delete poller;
}

/**
 * Lock object for configuration poll
 */
bool BusinessServicePrototype::lockForDiscoveryPoll()
{
   bool success = false;

   lockProperties();
   if (static_cast<uint32_t>(time(nullptr) - m_discoveryPollState.getLastCompleted()) > g_discoveryPollingInterval)
   {
      success = m_discoveryPollState.schedule();
   }
   unlockProperties();
   return success;
}
