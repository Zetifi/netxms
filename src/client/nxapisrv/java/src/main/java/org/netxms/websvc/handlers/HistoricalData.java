/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.websvc.handlers;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.TableRow;

/**
 * Objects request handler
 */
public class HistoricalData extends AbstractObjectHandler {

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object get(String id, Map<String, String> query) throws Exception {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      long dciId = 0;
      try {
         dciId = Long.parseLong(id);
      } catch (NumberFormatException e) {
         dciId = session.dciNameToId(object.getObjectId(), id);
      }

      if ((object == null) || (dciId == 0) || !(object instanceof DataCollectionTarget))
         throw new NXCException(RCC.INVALID_OBJECT_ID);
      String timeFrom = query.get("from");
      String timeTo = query.get("to");
      String timeInterval = query.get("timeInterval");
      String itemCount = query.get("itemCount");

      DataCollectionConfiguration dataCollectionConfiguration = session.openDataCollectionConfiguration(getObjectId());
      DataCollectionObject dataCollectionObject = dataCollectionConfiguration.findItem(dciId);

      if (dataCollectionObject instanceof DataCollectionItem) {
         DciData data = null;

         if (timeFrom != null || timeTo != null) {
            data = session.getCollectedData(object.getObjectId(), dciId,
                  new Date(parseLong(timeFrom, 0) * 1000),
                  new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000),
                  parseInt(itemCount, 0), HistoricalDataType.PROCESSED);
         } else if (timeInterval != null) {
            Date now = new Date();
            long from = now.getTime() - parseLong(timeInterval, 0) * 1000;
            data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), new Date(),
                  parseInt(itemCount, 0), HistoricalDataType.PROCESSED);
         } else if (itemCount != null) {
            data = session.getCollectedData(object.getObjectId(), dciId, null, null, parseInt(itemCount, 0),
                  HistoricalDataType.PROCESSED);
         } else {
            Date now = new Date();
            long from = now.getTime() - 3600000; // one hour
            data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), now,
                  parseInt(itemCount, 0), HistoricalDataType.PROCESSED);
         }

         return new ResponseContainer("values", data);
      } else if (dataCollectionObject instanceof DataCollectionTable) {
         HashMap<Object, Object> response = new HashMap<Object, Object>();
         ArrayList<HashMap<Object, Object>> tableData = new ArrayList<HashMap<Object, Object>>();

         Date dateTimeFrom = null;
         Date dateTimeTo = null;
         if (timeFrom != null || timeTo != null) {
            dateTimeFrom = new Date(parseLong(timeFrom, 0) * 1000);
            dateTimeTo = new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000);
         } else if (timeInterval != null) {
            dateTimeTo = new Date();
            dateTimeFrom = new Date(dateTimeTo.getTime() - parseLong(timeInterval, 0) * 1000);
         }

         Table table = session.getTableLastValues(getObjectId(), dciId);
         response.put("nodeId", getObjectId());
         response.put("dciId", dciId);
         response.put("title", table.getTitle());
         response.put("source", table.getSource());
         response.put("columns", table.getColumns());

         ArrayList<String> tableDciInstances = getTableDciInstances(table);

         for (String instance : tableDciInstances) {
            HashMap<Object, Object> rowResult = new HashMap<Object, Object>();
            for (TableColumnDefinition tableColumnDefinition : table.getColumns()) {
               DciData tableCellData = null;
               String tableColumnName = tableColumnDefinition.getName();

               tableCellData = session.getCollectedTableData(object.getObjectId(), dciId, instance,
                     tableColumnName,
                     dateTimeFrom, dateTimeTo, parseInt(itemCount, 0));

               rowResult.put(tableColumnName, tableCellData.getValues());
            }
            tableData.add(rowResult);
         }
         response.put("data", tableData);
         return new ResponseContainer("values", response);
      }
      return new ResponseContainer("values", null);
   }

   /**
    * Get the instance column values for a given table
    * supports only dci table with a single instance column
    */
   private ArrayList<String> getTableDciInstances(Table table) throws Exception {
      ArrayList<String> instances = new ArrayList<String>();
      String instanceColumnName = null;
      for (TableColumnDefinition tableColumnDefinition : table.getColumns()) {
         if (tableColumnDefinition.isInstanceColumn()) {
            instanceColumnName = tableColumnDefinition.getName();
            break;
         }
         throw new Exception("Incompatible operation");
      }
      for (TableRow row : table.getAllRows()) {
         instances.add(row.getValue(table.getColumnIndex(instanceColumnName)));
      }
      return instances;
   }

   // This code is unreachable as it doesn't have an endpoint
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   // @Override
   // protected Object getCollection(Map<String, String> query) throws Exception {
   // NXCSession session = getSession();

   // String dciQuery = query.get("dciList");
   // String[] requestPairs = dciQuery.split(";");
   // if (requestPairs == null)
   // throw new NXCException(RCC.INVALID_DCI_ID);

   // HashMap<Long, DciData> dciData = new HashMap<Long, DciData>();

   // for (int i = 0; i < requestPairs.length; i++) {
   // String[] dciPairs = requestPairs[i].split(",");
   // if (dciPairs == null)
   // throw new NXCException(RCC.INVALID_DCI_ID);

   // String dciId = dciPairs[0];
   // String nodeId = dciPairs[1];
   // String timeFrom = dciPairs[2];
   // String timeTo = dciPairs[3];
   // String timeInterval = dciPairs[4];
   // String timeUnit = dciPairs[5];

   // if (dciId == null || nodeId == null
   // || !(session.findObjectById(parseLong(nodeId, 0)) instanceof
   // DataCollectionTarget))
   // throw new NXCException(RCC.INVALID_OBJECT_ID);

   // DciData collectedData = null;

   // if (!timeFrom.equals("0") || !timeTo.equals("0")) {
   // collectedData = session.getCollectedData(parseLong(nodeId, 0),
   // parseLong(dciId, 0),
   // new Date(parseLong(timeFrom, 0) * 1000),
   // new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000),
   // 0, HistoricalDataType.PROCESSED);
   // } else if (!timeInterval.equals("0")) {
   // Date now = new Date();
   // long from;
   // if (parseInt(timeUnit, 0) == TimeUnit.HOUR.getValue())
   // from = now.getTime() - parseLong(timeInterval, 0) * 3600000;
   // else if (parseInt(timeUnit, 0) == TimeUnit.DAY.getValue())
   // from = now.getTime() - parseLong(timeInterval, 0) * 3600000 * 24;
   // else
   // from = now.getTime() - parseLong(timeInterval, 0) * 60000;
   // collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId,
   // 0), new Date(from),
   // new Date(), 0, HistoricalDataType.PROCESSED);
   // } else {
   // Date now = new Date();
   // long from = now.getTime() - 3600000; // one hour
   // collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId,
   // 0), new Date(from), now,
   // 0, HistoricalDataType.PROCESSED);
   // }

   // dciData.put((long) parseInt(dciId, 0), collectedData);
   // }

   // return new ResponseContainer("values", dciData);
   // }
}
