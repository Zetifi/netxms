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

/**
 * SLM check default constructor
 */
SlmCheck::SlmCheck()
{
	m_id = 0;
	m_type = 0;
	m_script = NULL;
	m_pCompiledScript = NULL;
	m_reason[0] = 0;
	m_relatedObject = 0;
	m_relatedDCI = 0;
	m_currentTicket = 0;
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
	if (request->isFieldExist(VID_SLMCHECK_ID))
   {
      m_id = request->getFieldAsUInt32(VID_SLMCHECK_ID);
   }
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
}

void SlmCheck::loadFromSelect(DB_RESULT hResult, int row)
{
	m_id = DBGetFieldULong(hResult, row, 0);
	m_type = DBGetFieldULong(hResult, row, 1);
   DBGetField(hResult, row, 2, m_name, 1023);
	m_relatedObject = DBGetFieldULong(hResult, row, 3);
	m_relatedDCI = DBGetFieldULong(hResult, row, 4);
   m_statusThreshold = DBGetFieldULong(hResult, row, 5);
	m_currentTicket = DBGetFieldULong(hResult, row, 6);
	MemFree(m_script);
	m_script = DBGetField(hResult, row, 7, nullptr, 0);
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

   m_pCompiledScript = NXSLCompileAndCreateVM(m_script, errorMsg, errorMsgLen, new NXSL_ServerEnv);
   if (m_pCompiledScript != NULL)
   {
      m_pCompiledScript->addConstant("OK", m_pCompiledScript->createValue((LONG)0));
      m_pCompiledScript->addConstant("FAIL", m_pCompiledScript->createValue((LONG)1));
   }
   else
   {
      nxlog_write(NXLOG_WARNING, _T("Failed to compile script for service check object %s [%u] (%s)"), _T("Default Name"), m_id, errorMsg);
   }
}


void SlmCheck::fillMessage(NXCPMessage *msg, uint64_t baseId)
{
   msg->setField(baseId, m_id);
   msg->setField(baseId + 1, m_type);
   msg->setField(baseId + 2, m_reason);
   msg->setField(baseId + 3, m_relatedDCI);
   msg->setField(baseId + 4, m_relatedObject);
   msg->setField(baseId + 5, m_statusThreshold);
   msg->setField(baseId + 6, m_name);
   msg->setField(baseId + 7, m_script);
}

/**
 * Save service check to database
 */
/*bool SlmCheck::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
	if (success && (m_modified & MODIFY_OTHER))
	{
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("slm_checks"), _T("id"), m_id))
      {
         hStmt = DBPrepare(hdb, _T("UPDATE slm_checks SET type=?,content=?,threshold_id=?,reason=?,is_template=?,template_id=?,current_ticket=? WHERE id=?"));
      }
      else
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO slm_checks (type,content,threshold_id,reason,is_template,template_id,current_ticket,id) VALUES (?,?,?,?,?,?,?,?)"));
      }

      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (UINT32)m_type);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, m_script, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_threshold ? m_threshold->getId() : 0);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_STATIC);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)(m_isTemplate ? 1 : 0));
         DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_templateId);
         DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, m_currentTicketId);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
         unlockProperties();
      }
      else
      {
         success = false;
      }
	}
	return success;
}*/

/**
 * Delete object from database
 */
/*bool SlmCheck::deleteFromDatabase(DB_HANDLE hdb)
{
	bool success = super::deleteFromDatabase(hdb);
	if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM slm_checks WHERE id=?"));
	return success;
}*/

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
						setReason((reason != NULL) ? reason->getValue()->getValueAsCString() : _T("Check script returns error"));
					}
					DbgPrintf(6, _T("SlmCheck::execute: %s [%ld] return value %d"), m_name, (long)m_id, pValue->getValueAsInt32());
				}
				else
				{
					TCHAR buffer[1024];

					_sntprintf(buffer, 1024, _T("ServiceCheck::%s::%d"), m_name, m_id);
					PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", buffer, m_pCompiledScript->getErrorText(), m_id);
			      nxlog_write(NXLOG_WARNING, _T("Failed to execute script for service check object %s [%u] (%s)"), m_name, m_id, m_pCompiledScript->getErrorText());
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
					//FIXME: wrong, need to rewrite
					m_status = STATUS_NORMAL;
				}
			}
			break;
		default:
			DbgPrintf(4, _T("SlmCheck::execute() called for undefined check type, check %s/%ld"), m_name, (long)m_id);
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
	DbgPrintf(4, _T("SlmCheck::insertTicket() called for %s [%d], reason='%s'"), _T("SomeName"), (int)m_id, m_reason);

	/*if (m_status == STATUS_NORMAL)
		return false;

	m_currentTicketId = CreateUniqueId(IDG_SLM_TICKET);

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO slm_tickets (ticket_id,check_id,service_id,create_timestamp,close_timestamp,reason) VALUES (?,?,?,?,0,?)"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_currentTicketId);
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, getOwnerId());
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)time(NULL));
		DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, m_reason, DB_BIND_TRANSIENT);
		success = DBExecute(hStmt) ? true : false;
		DBFreeStatement(hStmt);
	}
	DBConnectionPoolReleaseConnection(hdb);*/
	return true /*success*/ ;
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
