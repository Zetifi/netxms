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
** File: slmcheck.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("slm.check")

/**
 * SLM check default constructor
 */
SlmCheck::SlmCheck(uint32_t serviceId)
{
	m_id = 0;
	m_type = 0;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_reason[0] = 0;
	m_relatedObject = 0;
	m_relatedDCI = 0;
	m_currentTicket = 0;
	m_serviceId = serviceId;
	_tcscpy(m_name, _T("Default check name")); //FIXME: check names in DB?
}

/**
 * Service class destructor
 */
SlmCheck::~SlmCheck()
{
	MemFree(m_script);
	delete m_pCompiledScript;
}

void SlmCheck::modifyFromMessage(NXCPMessage *request)
{
	// If new check
   if (m_id == 0)
      m_id = CreateUniqueId(IDG_SLM_CHECK);

	if (request->isFieldExist(VID_SLMCHECK_TYPE))
   {
      m_type = request->getFieldAsUInt32(VID_SLMCHECK_TYPE);
   }
	if (request->isFieldExist(VID_SLMCHECK_RELATED_OBJECT))
   {
      m_relatedObject = request->getFieldAsUInt32(VID_SLMCHECK_RELATED_OBJECT);
   }
	if (request->isFieldExist(VID_SLMCHECK_RELATED_DCI))
   {
      m_relatedDCI = request->getFieldAsUInt32(VID_SLMCHECK_RELATED_DCI);
   }
	if (request->isFieldExist(VID_SLMCHECK_CURRENT_TICKET))
   {
      m_currentTicket = request->getFieldAsUInt32(VID_SLMCHECK_CURRENT_TICKET);
   }
	if (request->isFieldExist(VID_SCRIPT))
   {
		MemFree(m_script);
      m_script = request->getFieldAsString(VID_SCRIPT);
		compileScript();
   }
	if (request->isFieldExist(VID_DESCRIPTION))
   {
      request->getFieldAsString(VID_DESCRIPTION, m_name, 1023);
   }

	saveToDatabase();
}

void SlmCheck::loadFromSelect(DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	m_serviceId = DBGetFieldULong(hResult, row, 1);
	m_type = DBGetFieldULong(hResult, row, 2);
   DBGetField(hResult, row, 3, m_name, 1023);
	m_relatedObject = DBGetFieldULong(hResult, row, 4);
	m_relatedDCI = DBGetFieldULong(hResult, row, 5);
   m_statusThreshold = DBGetFieldULong(hResult, row, 6);
	MemFree(m_script);
	m_script = DBGetField(hResult, row, 7, nullptr, 0);
	m_currentTicket = DBGetFieldULong(hResult, row, 8);
	compileScript();
}

/**
 * Compile script if there is one
 */
void SlmCheck::compileScript()
{
	if (m_type != SCRIPT || m_script == NULL)
	   return;

   const int errorMsgLen = 512;
   TCHAR errorMsg[errorMsgLen];

	if (m_pCompiledScript != nullptr)
		delete m_pCompiledScript;
   m_pCompiledScript = NXSLCompileAndCreateVM(m_script, errorMsg, errorMsgLen, new NXSL_ServerEnv);
   if (m_pCompiledScript != NULL)
   {
      m_pCompiledScript->addConstant("OK", m_pCompiledScript->createValue((LONG)0));
      m_pCompiledScript->addConstant("FAIL", m_pCompiledScript->createValue((LONG)1));
   }
   else
   {
      nxlog_write(NXLOG_WARNING, _T("Failed to compile script for service check object %s [%u] (%s)"), m_name, m_id, errorMsg);
   }
}


void SlmCheck::fillMessage(NXCPMessage *msg, uint64_t baseId)
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_type);
   msg->setField(baseId + 2, getReason());
   msg->setField(baseId + 3, m_relatedDCI);
   msg->setField(baseId + 4, m_relatedObject);
   msg->setField(baseId + 5, m_statusThreshold);
   msg->setField(baseId + 6, m_name);
   msg->setField(baseId + 7, m_script);
}

const TCHAR* SlmCheck::getReason()
{
	if (m_reason[0] == 0 && m_currentTicket != 0)
	{
   	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (hdb)
		{
			DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT reason ")
															_T("FROM slm_tickets WHERE ticket_id=?"));
			if (hStmt == NULL)
			{
				nxlog_debug_tag(DEBUG_TAG, 4, _T("Cannot prepare select from slm_tickets"));
			}
			else
			{
				DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
				DB_RESULT hResult = DBSelectPrepared(hStmt);
				if (hResult != NULL)
				{
					DBGetField(hResult, 0, 1, m_reason, 255);
					DBFreeResult(hResult);
				}
				DBFreeStatement(hStmt);
			}
			DBConnectionPoolReleaseConnection(hdb);
		}
	}
	return m_reason;
}

/**
 * Save service check to database
 */
bool SlmCheck::saveToDatabase()
{
   bool success = false;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("slm_checks"), _T("id"), m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE slm_checks SET service_id=?,type=?,description=?,related_object=?,related_dci=?,status_threshold=?,content=?,current_ticket=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO slm_checks (service_id,type,description,related_object,related_dci,status_threshold,content,current_ticket,id) VALUES (?,?,?,?,?,?,?,?,?)"));
	}

	if (hStmt != nullptr)
	{
		//lockProperties();
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_serviceId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (uint32_t)m_type);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_relatedObject);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, m_relatedDCI);
		DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_statusThreshold);
		DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
		DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
		DBFreeStatement(hStmt);
		//unlockProperties();
	}
	else
	{
		success = false;
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Delete object from database
 */
bool SlmCheck::deleteFromDatabase()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM slm_checks WHERE id=?"));
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Create NXCP message with object's data
 */
/*void SlmCheck::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
	super::fillMessageInternal(pMsg, userId);
	pMsg->setField(VID_SLMCHECK_TYPE, UINT32(m_type));
	pMsg->setField(VID_SCRIPT, CHECK_NULL_EX(m_script));
	pMsg->setField(VID_REASON, m_reason);
	pMsg->setField(VID_TEMPLATE_ID, m_templateId);
	pMsg->setField(VID_IS_TEMPLATE, (WORD)(m_isTemplate ? 1 : 0));
	if (m_threshold != NULL)
		m_threshold->fillMessage(pMsg, VID_THRESHOLD_BASE);
}*/

/**
 * Modify object from message
 */
/*UINT32 SlmCheck::modifyFromMessageInternal(NXCPMessage *pRequest)
{
	if (pRequest->isFieldExist(VID_SLMCHECK_TYPE))
		m_type = CheckType(pRequest->getFieldAsUInt32(VID_SLMCHECK_TYPE));

	if (pRequest->isFieldExist(VID_SCRIPT))
	{
		TCHAR *script = pRequest->getFieldAsString(VID_SCRIPT);
		setScript(script);
		MemFree(script);
	}

	if (pRequest->isFieldExist(VID_THRESHOLD_BASE))
	{
		if (m_threshold == NULL)
			m_threshold = new Threshold;
		m_threshold->updateFromMessage(pRequest, VID_THRESHOLD_BASE);
	}

	return super::modifyFromMessageInternal(pRequest);
}*/

/**
 * Callback for post-modify hook
 */
/*static void UpdateFromTemplateCallback(NetObj *object, void *data)
{
	SlmCheck *check = (SlmCheck *)object;
	SlmCheck *tmpl = (SlmCheck *)data;

	if (check->getTemplateId() == tmpl->getId())
		check->updateFromTemplate(tmpl);
}*/

/**
 * Post-modify hook
 */
/*void SlmCheck::postModify()
{
	if (m_isTemplate)
		g_idxServiceCheckById.forEach(UpdateFromTemplateCallback, this);
}*/

/**
 * Execute check
 */
uint32_t SlmCheck::execute()
{
	uint32_t oldStatus = m_status;
	switch (m_type)
	{
		case 0: //Object
			{
				shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
				if (obj != nullptr)
				{
					m_status = obj->getStatus();
				}
			}
			break;
		case 1: //Script
			if (m_pCompiledScript != NULL)
			{
				NXSL_VariableSystem *pGlobals = NULL;

				m_pCompiledScript->setGlobalVariable("$reason", m_pCompiledScript->createValue(m_reason));
				m_pCompiledScript->setGlobalVariable("$node", getNodeObjectForNXSL(m_pCompiledScript));
				if (m_pCompiledScript->run(0, NULL, &pGlobals))
				{
					NXSL_Value *pValue = m_pCompiledScript->getResult();
					m_status = (pValue->getValueAsInt32() == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
					if (m_status == STATUS_CRITICAL)
					{
						NXSL_Variable *reason = pGlobals->find("$reason");
						if (reason != nullptr)
						{
							_tcslcpy(m_reason, CHECK_NULL_EX(reason->getValue()->getValueAsCString()), 256);
						}
						else
						{
							_tcslcpy(m_reason, _T("Check script returns error"), 256);
						}
					}
					nxlog_write_tag(6, DEBUG_TAG, _T("SlmCheck::execute script: %s [%ld] return value %d"), m_name, (long)m_id, pValue->getValueAsInt32());
					nxlog_write_tag(6, DEBUG_TAG, _T("SlmCheck::script: %s"), m_script);
				}
				else
				{
					TCHAR buffer[1024];

					_sntprintf(buffer, 1024, _T("ServiceCheck::%s::%d"), m_name, m_id);
					PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_pCompiledScript->getErrorText(), m_id);
			      nxlog_write_tag(2, DEBUG_TAG, _T("Failed to execute script for service check object %s [%u] (%s)"), m_name, m_id, m_pCompiledScript->getErrorText());
					m_status = STATUS_UNKNOWN;
				}
				delete pGlobals;
			}
			else
			{
				m_status = STATUS_UNKNOWN;
			}
			break;
		case 2: //DCI
			{
				shared_ptr<NetObj> obj = FindObjectById(m_relatedObject);
				if (obj != nullptr && obj->isDataCollectionTarget())
				{
					shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(obj);
					m_status = target->getDciThreshold(m_relatedDCI);
					//nxlog_write_tag(6, DEBUG_TAG, _T("SlmCheck::execute DCI: %s [%ld] DCI value %d"), m_name, (long)m_id, pValue->getValueAsInt32());
				}
			}
			break;
		default:
			nxlog_write_tag(4, DEBUG_TAG, _T("SlmCheck::execute() called for undefined check type, check %s/%ld"), m_name, (long)m_id);
			m_status = STATUS_UNKNOWN;
			break;
	}

	if (m_status != oldStatus)
	{
		if (m_status == STATUS_CRITICAL)
			insertTicket();
		else
			closeTicket();
	}
	return m_status;
}

/**
 * Insert ticket for this check into slm_tickets
 */
bool SlmCheck::insertTicket()
{
	nxlog_write_tag(4, DEBUG_TAG, _T("SlmCheck::insertTicket() called for %s [%d], reason='%s'"), m_name, (int)m_id, m_reason);

	if (m_status == STATUS_NORMAL)
		return false;

	m_currentTicket = CreateUniqueId(IDG_SLM_TICKET);

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO slm_tickets (ticket_id,check_id,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_serviceId);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_TRANSIENT);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}

	hStmt = DBPrepare(hdb, _T("UPDATE slm_checks SET current_ticket=? WHERE id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicket);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}

	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Close current ticket
 */
void SlmCheck::closeTicket()
{
	/*DbgPrintf(4, _T("SlmCheck::closeTicket() called for %s [%d], ticketId=%d"), m_name, (int)m_id, (int)m_currentTicketId);
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE slm_tickets SET close_timestamp=? WHERE ticket_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);
	m_currentTicketId = 0;*/
}

/**
 * Get ID of owning SLM object (business service or node link)
 */
/*UINT32 SlmCheck::getOwnerId()
{
	UINT32 ownerId = 0;

	readLockParentList();
	for(int i = 0; i < getParentList().size(); i++)
	{
      NetObj *object = getParentList().get(i);
		if ((object->getObjectClass() == OBJECT_BUSINESSSERVICE) ||
		    (object->getObjectClass() == OBJECT_NODELINK))
		{
			ownerId = object->getId();
			break;
		}
	}
	unlockParentList();
	return ownerId;
}*/

/**
 * Get related node object for use in NXSL script
 * Will return NXSL_Value of type NULL if there are no associated node
 */
NXSL_Value *SlmCheck::getNodeObjectForNXSL(NXSL_VM *vm)
{
	NXSL_Value *value = nullptr;
	shared_ptr<NetObj> node = FindObjectById(m_relatedObject);
	if ((node != nullptr) && (node->getObjectClass() == OBJECT_NODE)) //FIXME: Maybe any netobj?
	{
		value = node->createNXSLObject(vm);
	}

	return (value != nullptr) ? value : vm->createValue();
}

/**
 * Object deletion handler
 */
/*void SlmCheck::onObjectDelete(UINT32 objectId)
{
	// Delete itself if object curemtly being deleted is
	// a template used to create this check
	if (objectId == m_templateId)
	{
		DbgPrintf(4, _T("SlmCheck %s [%d] delete itself because of template deletion"), m_name, (int)m_id);
		deleteObject();
	}
   super::onObjectDelete(objectId);
}*/
