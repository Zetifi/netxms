/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
** File: netmap.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
Dashboard::Dashboard() : super()
{
	m_elements = new ObjectArray<DashboardElement>(0, 16, Ownership::True);
	m_numColumns = 1;
	m_options = 0;
	m_status = STATUS_NORMAL;
}

/**
 * Constructor for creating new dashboard object
 */
Dashboard::Dashboard(const TCHAR *name) : super(name, 0)
{
   m_elements = new ObjectArray<DashboardElement>(0, 16, Ownership::True);
	m_numColumns = 1;
	m_options = 0;
	m_status = STATUS_NORMAL;
}

/**
 * Destructor
 */
Dashboard::~Dashboard()
{
	delete m_elements;
}

/**
 * Redefined status calculation
 */
void Dashboard::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Create object from database
 */
bool Dashboard::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
	if (!super::loadFromDatabase(hdb, dwId))
		return false;

	m_status = STATUS_NORMAL;

	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT num_columns,options FROM dashboards WHERE id=%d"), (int)dwId);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult == NULL)
		return false;
	if (DBGetNumRows(hResult) > 0)
	{
		m_numColumns = (int)DBGetFieldLong(hResult, 0, 0);
		m_options = DBGetFieldULong(hResult, 0, 1);
	}
	DBFreeResult(hResult);

	_sntprintf(query, 256, _T("SELECT element_type,element_data,layout_data FROM dashboard_elements ")
								  _T("WHERE dashboard_id=%d ORDER BY element_id"), (int)dwId);
	hResult = DBSelect(hdb, query);
	if (hResult == NULL)
		return false;

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		DashboardElement *e = new DashboardElement;
		e->m_type = (int)DBGetFieldLong(hResult, i, 0);
		e->m_data = DBGetField(hResult, i, 1, NULL, 0);
		e->m_layout = DBGetField(hResult, i, 2, NULL, 0);
		m_elements->add(e);
	}

	DBFreeResult(hResult);
	return true;
}

/**
 * Save object to database
 */
bool Dashboard::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("dashboards"), _T("id"), m_id))
         hStmt = DBPrepare(hdb, _T("UPDATE dashboards SET num_columns=?,options=? WHERE id=?"));
      else
         hStmt = DBPrepare(hdb, _T("INSERT INTO dashboards (num_columns,options,id) VALUES (?,?,?)"));
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_numColumns);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_options);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
      {
         success = executeQueryOnObject(hdb, _T("DELETE FROM dashboard_elements WHERE dashboard_id=?"));
         lockProperties();
         if (success && !m_elements->isEmpty())
         {
            hStmt = DBPrepare(hdb, _T("INSERT INTO dashboard_elements (dashboard_id,element_id,element_type,element_data,layout_data) VALUES (?,?,?,?,?)"));
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
               for(int i = 0; success && (i < m_elements->size()); i++)
               {
                  DashboardElement *element = m_elements->get(i);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, i);
                  DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, element->m_type);
                  DBBind(hStmt, 4, DB_SQLTYPE_TEXT, element->m_data, DB_BIND_STATIC);
                  DBBind(hStmt, 5, DB_SQLTYPE_TEXT, element->m_layout, DB_BIND_STATIC);
                  success = DBExecute(hStmt);
               }
               DBFreeStatement(hStmt);
            }
            else
            {
               success = false;
            }
         }
         unlockProperties();
      }
   }
   return success;
}

/**
 * Delete object from database
 */
bool Dashboard::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dashboards WHERE id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM dashboard_elements WHERE dashboard_id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void Dashboard::fillMessageInternal(NXCPMessage *msg, UINT32 userId)
{
   super::fillMessageInternal(msg, userId);

	msg->setField(VID_NUM_COLUMNS, (WORD)m_numColumns);
	msg->setField(VID_NUM_ELEMENTS, (UINT32)m_elements->size());

	UINT32 varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_elements->size(); i++)
	{
		DashboardElement *element = m_elements->get(i);
		msg->setField(varId++, (WORD)element->m_type);
		msg->setField(varId++, CHECK_NULL_EX(element->m_data));
		msg->setField(varId++, CHECK_NULL_EX(element->m_layout));
		varId += 7;
	}
}

/**
 * Modify object from NXCP message
 */
uint32_t Dashboard::modifyFromMessageInternal(const NXCPMessage& msg)
{
	if (msg.isFieldExist(VID_NUM_COLUMNS))
		m_numColumns = (int)msg.getFieldAsUInt16(VID_NUM_COLUMNS);

	if (msg.isFieldExist(VID_FLAGS))
	   m_options = (int)msg.getFieldAsUInt32(VID_FLAGS);

	if (msg.isFieldExist(VID_NUM_ELEMENTS))
	{
		m_elements->clear();

		int count = (int)msg.getFieldAsUInt32(VID_NUM_ELEMENTS);
		UINT32 varId = VID_ELEMENT_LIST_BASE;
		for(int i = 0; i < count; i++)
		{
			DashboardElement *e = new DashboardElement;
			e->m_type = (int)msg.getFieldAsUInt16(varId++);
			e->m_data = msg.getFieldAsString(varId++);
			e->m_layout = msg.getFieldAsString(varId++);
			varId += 7;
			m_elements->add(e);
		}
	}

	return super::modifyFromMessageInternal(msg);
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool Dashboard::showThresholdSummary() const
{
	return false;
}

/**
 * Serialize object to JSON
 */
json_t *Dashboard::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "numColumns", json_integer(m_numColumns));
   json_object_set_new(root, "options", json_integer(m_options));
   json_object_set_new(root, "elements", json_object_array(m_elements));
   unlockProperties();

   return root;
}

/**
 * Redefined status calculation for dashboard group
 */
void DashboardGroup::calculateCompoundStatus(BOOL bForcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool DashboardGroup::showThresholdSummary() const
{
   return false;
}
