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
** File: fdb.cpp
**
**/

#include "nxcore.h"

/**
 * Constructor
 */
ForwardingDatabase::ForwardingDatabase(uint32_t nodeId, bool portReferenceByIfIndex)
{
   m_nodeId = nodeId;
	m_fdb = nullptr;
	m_fdbSize = 0;
	m_fdbAllocated = 0;
	m_portMap = nullptr;
	m_pmSize = 0;
	m_pmAllocated = 0;
	m_timestamp = time(nullptr);
	m_portReferenceByIfIndex = portReferenceByIfIndex;
	m_currentVlanId = 0;
}

/**
 * Destructor
 */
ForwardingDatabase::~ForwardingDatabase()
{
	MemFree(m_fdb);
	MemFree(m_portMap);
}

/**
 * Add port mapping entry
 */
void ForwardingDatabase::addPortMapping(PORT_MAPPING_ENTRY *entry)
{
	if (m_pmSize == m_pmAllocated)
	{
		m_pmAllocated += 32;
		m_portMap = (PORT_MAPPING_ENTRY *)realloc(m_portMap, sizeof(PORT_MAPPING_ENTRY) * m_pmAllocated);
	}
	memcpy(&m_portMap[m_pmSize], entry, sizeof(PORT_MAPPING_ENTRY));
	m_pmSize++;
}

/**
 * Get interface index for given port number
 */
uint32_t ForwardingDatabase::ifIndexFromPort(uint32_t port)
{
	for(int i = 0; i < m_pmSize; i++)
		if (m_portMap[i].port == port)
			return m_portMap[i].ifIndex;

	// Try to lookup node interfaces because correct bridge port number may be set by driver
	shared_ptr<NetObj> node = FindObjectById(m_nodeId, OBJECT_NODE);
	if (node != nullptr)
	{
	   shared_ptr<Interface> iface = static_cast<Node*>(node.get())->findBridgePort(port);
	   if (iface != nullptr)
	      return iface->getIfIndex();
	}
	return 0;
}

/**
 * Add entry
 */
void ForwardingDatabase::addEntry(FDB_ENTRY *entry)
{
	// Check for duplicate
	for(int i = 0; i < m_fdbSize; i++)
		if (!memcmp(m_fdb[i].macAddr, entry->macAddr, MAC_ADDR_LENGTH))
		{
			memcpy(&m_fdb[i], entry, sizeof(FDB_ENTRY));
			m_fdb[i].ifIndex = m_portReferenceByIfIndex ? entry->port : ifIndexFromPort(entry->port);
			return;
		}

	if (m_fdbSize == m_fdbAllocated)
	{
		m_fdbAllocated += 32;
		m_fdb = MemReallocArray(m_fdb, m_fdbAllocated);
	}
	memcpy(&m_fdb[m_fdbSize], entry, sizeof(FDB_ENTRY));
	m_fdb[m_fdbSize].ifIndex = m_portReferenceByIfIndex ? entry->port : ifIndexFromPort(entry->port);
	m_fdbSize++;
}

/**
 * FDB entry comparator
 */
static int EntryComparator(const void *p1, const void *p2)
{
	return memcmp(static_cast<const FDB_ENTRY*>(p1)->macAddr, static_cast<const FDB_ENTRY*>(p2)->macAddr, MAC_ADDR_LENGTH);
}

/**
 * Find MAC address
 * Returns interface index or 0 if MAC address not found
 */
uint32_t ForwardingDatabase::findMacAddress(const BYTE *macAddr, bool *isStatic)
{
	FDB_ENTRY key;
	memcpy(key.macAddr, macAddr, MAC_ADDR_LENGTH);
	FDB_ENTRY *entry = static_cast<FDB_ENTRY*>(bsearch(&key, m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator));
   if ((entry != nullptr) && (isStatic != nullptr))
      *isStatic = (entry->type == 5);
	return (entry != nullptr) ? entry->ifIndex : 0;
}

/**
 * Find all MAC addresses that contains given pattern
 */
void ForwardingDatabase::findMacAddressByPattern(const BYTE* macPattern, int macPatternSize, HashSet<MacAddress>* hs)
{
   for (int i = 0; i < m_fdbSize; i++)
      if (memmem(m_fdb[i].macAddr, MAC_ADDR_LENGTH, macPattern, macPatternSize))
         hs->put(MacAddress(m_fdb[i].macAddr, MAC_ADDR_LENGTH));
}

/**
 * Check if port has only one MAC in FDB
 * If macAddr parameter is not nullptr, MAC address found on port
 * copied into provided buffer
 */
bool ForwardingDatabase::isSingleMacOnPort(uint32_t ifIndex, BYTE *macAddr)
{
	int count = 0;
	for(int i = 0; i < m_fdbSize; i++)
		if (m_fdb[i].ifIndex == ifIndex)
		{
			count++;
			if (count > 1)
				return false;
			if (macAddr != nullptr)
				memcpy(macAddr, m_fdb[i].macAddr, MAC_ADDR_LENGTH);
		}

	return count == 1;
}

/**
 * Get number of MAC addresses on given port
 */
int ForwardingDatabase::getMacCountOnPort(uint32_t ifIndex)
{
	int count = 0;
	for(int i = 0; i < m_fdbSize; i++)
		if (m_fdb[i].ifIndex == ifIndex)
		{
			count++;
		}

	return count;
}

/**
 * Print to console
 */
void ForwardingDatabase::print(CONSOLE_CTX ctx, Node *owner)
{
   TCHAR macAddrStr[64];

   ConsolePrintf(ctx, _T("\x1b[1mMAC address\x1b[0m       | \x1b[1mIfIndex\x1b[0m | \x1b[1mInterface\x1b[0m            | \x1b[1mPort\x1b[0m | \x1b[1mType\x1b[0m    | \x1b[1mNode\x1b[0m  | \x1b[1mNode name\x1b[0m\n"));
   ConsolePrintf(ctx, _T("------------------+---------+----------------------+------+-------+-----------------------------\n"));
	for(int i = 0; i < m_fdbSize; i++)
   {
      shared_ptr<NetObj> node = FindObjectById(m_fdb[i].nodeObject, OBJECT_NODE);
      shared_ptr<Interface> iface = owner->findInterfaceByIndex(m_fdb[i].ifIndex);
      ConsolePrintf(ctx, _T("%s | %7d | %-20s | %4d | %-7s | %5d | %s\n"), MACToStr(m_fdb[i].macAddr, macAddrStr),
         m_fdb[i].ifIndex, (iface != nullptr) ? iface->getName() : _T("\x1b[31;1mUNKNOWN\x1b[0m"),
         m_fdb[i].port, (m_fdb[i].type == 3) ? _T("dynamic") : ((m_fdb[i].type == 5) ? _T("static") : _T("unknown")),
         m_fdb[i].nodeObject, (node != nullptr) ? node->getName() : _T("\x1b[31;1mUNKNOWN\x1b[0m"));
   }
   ConsolePrintf(ctx, _T("\n%d entries\n\n"), m_fdbSize);
}

/**
 * Get interface name from index
 */
String ForwardingDatabase::interfaceIndexToName(const shared_ptr<NetObj>& node, uint32_t index)
{
   StringBuffer name;
   shared_ptr<Interface> iface = (node != nullptr) ? static_cast<Node*>(node.get())->findInterfaceByIndex(index) : shared_ptr<Interface>();
   if (iface != nullptr)
   {
     if (iface->getParentInterfaceId() != 0)
     {
        shared_ptr<Interface> parentIface = static_pointer_cast<Interface>(FindObjectById(iface->getParentInterfaceId(), OBJECT_INTERFACE));
        if ((parentIface != nullptr) &&
            ((parentIface->getIfType() == IFTYPE_ETHERNET_CSMACD) || (parentIface->getIfType() == IFTYPE_IEEE8023ADLAG)))
        {
           name = parentIface->getName();
        }
        else
        {
           name = iface->getName();
        }
     }
     else
     {
        name = iface->getName();
     }
   }
   else
   {
      name.appendFormattedString(_T("[%u]"), index);
   }
   return name;
}

/**
 * Fill NXCP message with FDB data
 */
void ForwardingDatabase::fillMessage(NXCPMessage *msg)
{
   shared_ptr<NetObj> node = FindObjectById(m_nodeId, OBJECT_NODE);
   msg->setField(VID_NUM_ELEMENTS, (UINT32)m_fdbSize);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < m_fdbSize; i++)
   {
      msg->setField(fieldId++, m_fdb[i].macAddr, MAC_ADDR_LENGTH);
      msg->setField(fieldId++, m_fdb[i].ifIndex);
      msg->setField(fieldId++, m_fdb[i].port);
      msg->setField(fieldId++, m_fdb[i].nodeObject);
      msg->setField(fieldId++, m_fdb[i].vlanId);
      msg->setField(fieldId++, m_fdb[i].type);
      msg->setField(fieldId++, interfaceIndexToName(node, m_fdb[i].ifIndex));
      fieldId += 3;
   }
}

/**
 * Get Table filled with switch forwarding database information
 */
shared_ptr<Table> ForwardingDatabase::getAsTable()
{
   shared_ptr<Table> result = make_shared<Table>();
   result->addColumn(_T("MAC_ADDRESS"), DCI_DT_STRING, _T("Mac address"), true);
   result->addColumn(_T("PORT"), DCI_DT_UINT, _T("Port"));
   result->addColumn(_T("IF_INDEX"), DCI_DT_UINT, _T("Interface index"));
   result->addColumn(_T("IF_NAME"), DCI_DT_STRING, _T("Interface name"));
   result->addColumn(_T("VLAN"), DCI_DT_UINT, _T("VLAN"));
   result->addColumn(_T("NODE_ID"), DCI_DT_UINT, _T("Node id"));
   result->addColumn(_T("NODE_NAME"), DCI_DT_STRING, _T("Node name"));
   result->addColumn(_T("ZONE_UIN"), DCI_DT_INT, _T("Zone UIN"));
   result->addColumn(_T("ZONE_NAME"), DCI_DT_STRING, _T("Zone name"));
   result->addColumn(_T("TYPE"), DCI_DT_STRING, _T("Type"));

   shared_ptr<NetObj> sourceNode = FindObjectById(m_nodeId, OBJECT_NODE);
   for(int i = 0; i < m_fdbSize; i++)
   {
      result->addRow();

      TCHAR mac[64];
      result->set(0, MACToStr(m_fdb[i].macAddr, mac));
      result->set(1, m_fdb[i].port);
      result->set(2, m_fdb[i].ifIndex);
      result->set(3, interfaceIndexToName(sourceNode, m_fdb[i].ifIndex));
      result->set(4, m_fdb[i].vlanId);
      result->set(5, m_fdb[i].nodeObject);
      shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(m_fdb[i].nodeObject, OBJECT_NODE));
      if (node != nullptr)
      {
         result->set(6, node->getName());
         result->set(7, node->getZoneUIN());
         shared_ptr<Zone> zone = FindZoneByUIN(node->getZoneUIN());
         if (zone != nullptr)
         {
            result->set(8, zone->getName());
         }
         else
         {
            result->set(8, _T(""));
         }
      }
      else
      {
         result->set(6, _T(""));
         result->set(7, _T(""));
         result->set(8, _T(""));
      }
      result->set(9, (m_fdb[i].type) == 3 ? _T("Dynamic") : ((m_fdb[i].type == 5) ? _T("Static") : _T("Unknown")));
   }

   return result;
}

/**
 * Sort FDB
 */
void ForwardingDatabase::sort()
{
	qsort(m_fdb, m_fdbSize, sizeof(FDB_ENTRY), EntryComparator);
}

/**
 * FDB walker's callback
 */
static uint32_t FDBHandler(SNMP_Variable *pVar, SNMP_Transport *pTransport, ForwardingDatabase *fdb)
{
   SNMP_ObjectId oid(pVar->getName());

	// Get port number and status
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), pTransport->getSnmpVersion());

	oid.changeElement(10, 2);	// .1.3.6.1.2.1.17.4.3.1.2 - port number
	request.bindVariable(new SNMP_Variable(oid));

	oid.changeElement(10, 3);	// .1.3.6.1.2.1.17.4.3.1.3 - status
	request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = pTransport->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
	if (rcc == SNMP_ERR_SUCCESS)
   {
      SNMP_Variable *varPort = response->getVariable(0);
      SNMP_Variable *varStatus = response->getVariable(1);
      if (varPort != nullptr && varStatus != nullptr)
      {
         int port = varPort->getValueAsInt();
         int status = varStatus->getValueAsInt();
         if ((port > 0) && ((status == 3) || (status == 5)))  // status: 3 == learned, 5 == static
         {
            FDB_ENTRY entry;
            memset(&entry, 0, sizeof(FDB_ENTRY));
            entry.port = static_cast<uint32_t>(port);
            pVar->getRawValue(entry.macAddr, MAC_ADDR_LENGTH);
            shared_ptr<Node> node = FindNodeByMAC(entry.macAddr);
            entry.nodeObject = (node != nullptr) ? node->getId() : 0;
            entry.vlanId = fdb->getCurrentVlanId();
            entry.type = static_cast<uint16_t>(status);
            fdb->addEntry(&entry);
         }
      }
      delete response;
	}

	return rcc;
}

/**
 * dot1qTpFdbEntry walker's callback
 */
static uint32_t Dot1qTpFdbHandler(SNMP_Variable *pVar, SNMP_Transport *pTransport, ForwardingDatabase *fdb)
{
	int port = pVar->getValueAsInt();
	if (port == 0)
		return SNMP_ERR_SUCCESS;

	// Get port number and status
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), pTransport->getSnmpVersion());

   SNMP_ObjectId oid(pVar->getName());
   oid.changeElement(12, 3);	// .1.3.6.1.2.1.17.7.1.2.2.1.3 - status
	request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = pTransport->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
	if (rcc == SNMP_ERR_SUCCESS)
   {
		int status = response->getVariable(0)->getValueAsInt();
		if ((status == 3) || (status == 5)) // status: 3 == learned, 5 == static
		{
			FDB_ENTRY entry;

			memset(&entry, 0, sizeof(FDB_ENTRY));
			entry.port = static_cast<uint32_t>(port);
			size_t oidLen = oid.length();
			for(size_t i = oidLen - MAC_ADDR_LENGTH, j = 0; i < oidLen; i++)
				entry.macAddr[j++] = oid.getElement(i);
			shared_ptr<Node> node = FindNodeByMAC(entry.macAddr);
			entry.nodeObject = (node != nullptr) ? node->getId() : 0;
         entry.vlanId = static_cast<uint16_t>(oid.getElement(oidLen - MAC_ADDR_LENGTH - 1));
         entry.type = static_cast<uint16_t>(status);
         fdb->addEntry(&entry);
		}
      delete response;
	}

	return rcc;
}

/**
 * dot1dBasePortTable walker's callback
 */
static uint32_t Dot1dPortTableHandler(SNMP_Variable *pVar, SNMP_Transport *pTransport, ForwardingDatabase *fdb)
{
   const SNMP_ObjectId& oid = pVar->getName();
	PORT_MAPPING_ENTRY pm;
	pm.port = oid.value()[oid.length() - 1];
	pm.ifIndex = pVar->getValueAsUInt();
	fdb->addPortMapping(&pm);
	return SNMP_ERR_SUCCESS;
}

#define FDB_CHECK_FAILURE(s) if ((s) != SNMP_ERR_SUCCESS) { return shared_ptr<ForwardingDatabase>(); }

/**
 * Get switch forwarding database from node
 */
shared_ptr<ForwardingDatabase> GetSwitchForwardingDatabase(Node *node)
{
	if (!node->isBridge())
		return shared_ptr<ForwardingDatabase>();

	bool portReferenceByIfIndex = node->isFdbUsingIfIndex();
	shared_ptr<ForwardingDatabase> fdb = make_shared<ForwardingDatabase>(node->getId(), portReferenceByIfIndex);

	if (!portReferenceByIfIndex)
	{
      FDB_CHECK_FAILURE(node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.1.4.1.2"), Dot1dPortTableHandler, fdb.get(), nullptr, true));
      if (node->isPerVlanFdbSupported())
      {
         shared_ptr<VlanList> vlans = node->getVlans();
         if (vlans != nullptr)
         {
            for(int i = 0; i < vlans->size(); i++)
            {
               char context[16];
               sprintf(context, "%s%d", (node->getSNMPVersion() < SNMP_VERSION_3) ? "" : "vlan-", vlans->get(i)->getVlanId());
               if (node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.1.4.1.2"), Dot1dPortTableHandler, fdb.get(), context, true) != SNMP_ERR_SUCCESS)
               {
                  // Some Cisco switches may not return data for certain system VLANs
                  nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("FDB: cannot read port table in context %hs"), context);
               }
            }
         }
      }
	}

   FDB_CHECK_FAILURE(node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.7.1.2.2.1.2"), Dot1qTpFdbHandler, fdb.get(), nullptr, true));
   int size = fdb->getSize();
   nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("FDB: %d entries read from dot1qTpFdbTable"), size);

   fdb->setCurrentVlanId(1);
   FDB_CHECK_FAILURE(node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb.get(), nullptr, true));
   nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("FDB: %d entries read from dot1dTpFdbTable"), fdb->getSize() - size);
   size = fdb->getSize();

	if (node->isPerVlanFdbSupported())
	{
		shared_ptr<VlanList> vlans = node->getVlans();
		if (vlans != nullptr)
		{
			for(int i = 0; i < vlans->size(); i++)
			{
            char context[16];
            sprintf(context, "%s%d", (node->getSNMPVersion() < SNMP_VERSION_3) ? "" : "vlan-", vlans->get(i)->getVlanId());
            fdb->setCurrentVlanId((UINT16)vlans->get(i)->getVlanId());
				if (node->callSnmpEnumerate(_T(".1.3.6.1.2.1.17.4.3.1.1"), FDBHandler, fdb.get(), context) != SNMP_ERR_SUCCESS)
				{
               // Some Cisco switches may not return data for certain system VLANs
				   nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("FDB: cannot read  FDB in context %hs"), context);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("FDB: %d entries read from dot1dTpFdbTable in context %hs"), fdb->getSize() - size, context);
            }
				size = fdb->getSize();
			}
		}
	}

	fdb->sort();
	return fdb;
}
