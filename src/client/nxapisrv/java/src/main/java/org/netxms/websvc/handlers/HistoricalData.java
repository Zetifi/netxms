/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciData;
import org.netxms.client.datacollection.GraphSettings;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;
import org.netxms.client.datacollection.DataCollectionItem;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.ColumnDefinition;
import org.netxms.client.Table;
import org.netxms.client.TableColumnDefinition;
import org.netxms.client.TableRow;

/**
 * Objects request handler
 */
public class HistoricalData extends AbstractObjectHandler {
   private static Logger log = LoggerFactory.getLogger(HistoricalData.class);

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
      String timeInteval = query.get("timeInterval");
      String itemCount = query.get("itemCount");

      DciData data = null;
      DataCollectionConfiguration dc = session.openDataCollectionConfiguration(getObjectId());
      DataCollectionObject dco = dc.findItem(dciId);

      if (dco instanceof DataCollectionItem) {
         log.debug("DataCollectionItem");
         if (timeFrom != null || timeTo != null) {
            data = session.getCollectedData(object.getObjectId(), dciId, new Date(parseLong(timeFrom, 0) * 1000),
                  new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), parseInt(itemCount, 0),
                  HistoricalDataType.PROCESSED);
         } else if (timeInteval != null) {
            Date now = new Date();
            long from = now.getTime() - parseLong(timeInteval, 0) * 1000;
            data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), new Date(),
                  parseInt(itemCount, 0), HistoricalDataType.PROCESSED);
         } else if (itemCount != null) {
            data = session.getCollectedData(object.getObjectId(), dciId, null, null, parseInt(itemCount, 0),
                  HistoricalDataType.PROCESSED);
         } else {
            Date now = new Date();
            long from = now.getTime() - 3600000; // one hour
            data = session.getCollectedData(object.getObjectId(), dciId, new Date(from), now, parseInt(itemCount, 0),
                  HistoricalDataType.PROCESSED);
         }
      } else if (dco instanceof DataCollectionTable) {
         log.debug("DataCollectionTable");
         DataCollectionTable dct = (DataCollectionTable) dco;

         for (ColumnDefinition cd : dct.getColumns()) {
            log.debug("column: " + cd.getName());
         }

         log.debug(dct.getInstanceColumn());

         data = session.getCollectedTableData(object.getObjectId(), dciId, "001EC035FE03", "MAC Address",
               new Date(parseLong(timeFrom, 0) * 1000),
               new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), parseInt(itemCount, 0));
         Table table = session.getTableLastValues(getObjectId(), dciId);
         // TableRow tableRow = table.getRow(0);
         for (TableRow row : table.getAllRows()) {
            // log.debug("row: " + row.getRowId());
            log.debug(row.toString());
         }
         // for (TableColumnDefinition tcd : table.getColumns()) {
         // log.debug("column: " + tcd.getName());
         // }

         log.debug(dct.getInstanceColumn());
      }

      return new ResponseContainer("values", data);
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception {
      NXCSession session = getSession();

      String dciQuery = query.get("dciList");
      String[] requestPairs = dciQuery.split(";");
      if (requestPairs == null)
         throw new NXCException(RCC.INVALID_DCI_ID);

      HashMap<Long, DciData> dciData = new HashMap<Long, DciData>();

      for (int i = 0; i < requestPairs.length; i++) {
         String[] dciPairs = requestPairs[i].split(",");
         if (dciPairs == null)
            throw new NXCException(RCC.INVALID_DCI_ID);

         String dciId = dciPairs[0];
         String nodeId = dciPairs[1];
         String timeFrom = dciPairs[2];
         String timeTo = dciPairs[3];
         String timeInterval = dciPairs[4];
         String timeUnit = dciPairs[5];

         if (dciId == null || nodeId == null
               || !(session.findObjectById(parseLong(nodeId, 0)) instanceof DataCollectionTarget))
            throw new NXCException(RCC.INVALID_OBJECT_ID);

         DciData collectedData = null;

         if (!timeFrom.equals("0") || !timeTo.equals("0")) {
            collectedData = session.getCollectedData(parseLong(nodeId, 0), parseLong(dciId, 0),
                  new Date(parseLong(timeFrom, 0) * 1000),
                  new Date(parseLong(timeTo, System.currentTimeMillis() / 1000) * 1000), 0,
                  HistoricalDataType.PROCESSED);
         } else if (!timeInterval.equals("0")) {
            Date now = new Date();
            long from;
            if (parseInt(timeUnit, 0) == GraphSettings.TIME_UNIT_HOUR)
               from = now.getTime() - parseLong(timeInterval, 0) * 3600000;
            else if (parseInt(timeUnit, 0) == GraphSettings.TIME_UNIT_DAY)
               from = now.getTime() - parseLong(timeInterval, 0) * 3600000 * 24;
            else
               from = now.getTime() - parseLong(timeInterval, 0) * 60000;

            collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId, 0), new Date(from),
                  new Date(), 0, HistoricalDataType.PROCESSED);
         } else {
            Date now = new Date();
            long from = now.getTime() - 3600000; // one hour
            collectedData = session.getCollectedData(parseInt(nodeId, 0), parseInt(dciId, 0), new Date(from), now, 0,
                  HistoricalDataType.PROCESSED);
         }

         dciData.put((long) parseInt(dciId, 0), collectedData);
      }

      return new ResponseContainer("values", dciData);
   }
}
