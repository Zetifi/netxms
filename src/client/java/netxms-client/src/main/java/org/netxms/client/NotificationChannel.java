/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.client;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;

/**
 * Represents NetXMS server's action
 *
 */
public class NotificationChannel
{
	private String name;
	private String description;
	private String driverName;
	private String configuration;
	private boolean isActive;
	private NCConfigurationTemplate configurationTemplate;
	private String errorMessage;
	private int lastStatus;
	
	/**
	 * Create notification channel object from NXCP message
	 * 
	 * @param msg NXCP message
	 * @param baseId base field ID
	 */
	protected NotificationChannel(final NXCPMessage msg, long baseId)
	{
      name = msg.getFieldAsString(baseId);
      description = msg.getFieldAsString(baseId + 1);
      driverName = msg.getFieldAsString(baseId + 2);
      configuration = msg.getFieldAsString(baseId + 3);
      isActive = msg.getFieldAsBoolean(baseId + 4);
      configurationTemplate = new NCConfigurationTemplate(msg, baseId + 5);
      errorMessage = msg.getFieldAsString(baseId + 7);
      lastStatus = msg.getFieldAsInt32(baseId + 8);
	}
	
	/**
	 * Create empty notification channel object.
	 */
	public NotificationChannel()
   {
	   name = "";
	   description = "";
	   driverName = null;
	   configuration = "";
	   isActive = false;
	   configurationTemplate = null;
   }

   /**
	 * Fill NXCP message with action's data
	 *
	 * @param msg NXCP message
	 */
	public void fillMessage(final NXCPMessage msg)
	{
	   msg.setField(NXCPCodes.VID_NAME, name);
	   msg.setField(NXCPCodes.VID_DESCRIPTION, description);
	   msg.setField(NXCPCodes.VID_DRIVER_NAME, driverName);
	   msg.setField(NXCPCodes.VID_XML_CONFIG, configuration);
	}

   /**
    * Get channel name.
    *
    * @return channel name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @param name the name to set
    */
   public void setName(String name)
   {
      this.name = name;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }

   /**
    * @param description the description to set
    */
   public void setDescription(String description)
   {
      this.description = description;
   }

   /**
    * @return the driverName
    */
   public String getDriverName()
   {
      return driverName;
   }

   /**
    * @param driverName the driverName to set
    */
   public void setDriverName(String driverName)
   {
      this.driverName = driverName;
   }

   /**
    * @return the configuration
    */
   public String getConfiguration()
   {
      return configuration;
   }

   /**
    * @param configuration the configuration to set
    */
   public void setConfiguration(String configuration)
   {
      this.configuration = configuration;
   }

   /**
    * @return the configurationTemplate
    */
   public NCConfigurationTemplate getConfigurationTemplate()
   {
      return configurationTemplate;
   }

   /**
    * @return the isActive
    */
   public boolean isActive()
   {
      return isActive;
   }

   /**
    * @return the errorMessage
    */
   public String getErrorMessage()
   {
      return errorMessage;
   }

   /**
    * @return the lastStatus
    */
   public int getLastStatus()
   {
      return lastStatus;
   }

   /**
    * @return the lastStatus
    */
   public String getLastStatusAsString()
   {
      return lastStatus == 0 ? "Unknown" : lastStatus == 1 ? "Success" : "Error";
   }
}
