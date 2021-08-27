/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.client.objects;

import java.util.Set;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.objects.interfaces.AutoBindDCIObject;
import org.netxms.client.objects.interfaces.AutoBindObject;
import org.netxms.client.objects.interfaces.PollingTarget;

/**
 * Business service representation
 */
public class BusinessService extends ServiceContainer implements AutoBindObject, AutoBindDCIObject, PollingTarget
{
   private boolean objectAutoBind;
   private boolean objectAutoUnbind;
   private String objectAutoBindFilter;
   private boolean dciAutoBind;
   private boolean dciAutoUnbind;
   private String dciAutoBindFilter;
   private int dciStatusThreshold;
   private int nodeStatusThreshold;
   
   /**
    * @param msg
    */
   public BusinessService(NXCPMessage msg, NXCSession session)
   {
      super(msg, session);
      objectAutoBindFilter = msg.getFieldAsString(NXCPCodes.VID_AUTOBIND_FILTER);
      objectAutoBind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOBIND_FLAG);
      objectAutoUnbind = msg.getFieldAsBoolean(NXCPCodes.VID_AUTOUNBIND_FLAG);
      dciAutoBindFilter = msg.getFieldAsString(NXCPCodes.VID_DCI_AUTOBIND_FILTER);
      dciAutoBind = msg.getFieldAsBoolean(NXCPCodes.VID_DCI_AUTOBIND_FLAG);
      dciAutoUnbind = msg.getFieldAsBoolean(NXCPCodes.VID_DCI_AUTOUNBIND_FLAG);
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAllowedOnMap()
    */
   @Override
   public boolean isAllowedOnMap()
   {
      return true;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#isAlarmsVisible()
    */
   @Override
   public boolean isAlarmsVisible()
   {
      return true;
   }

   /**
    * @return true if automatic bind is enabled
    */
   public boolean isAutoBindEnabled()
   {
      return objectAutoBind;
   }

   /**
    * @return true if automatic unbind is enabled
    */
   public boolean isAutoUnbindEnabled()
   {
      return objectAutoUnbind;
   }

   /**
    * @return Filter script for automatic bind
    */
   public String getAutoBindFilter()
   {
      return objectAutoBindFilter;
   }

   @Override
   public boolean isDciAutoBindEnabled()
   {
      return dciAutoBind;
   }

   @Override
   public boolean isDciAutoUnbindEnabled()
   {
      // TODO Auto-generated method stub
      return dciAutoUnbind;
   }

   @Override
   public String getDciAutoBindFilter()
   {
      // TODO Auto-generated method stub
      return dciAutoBindFilter;
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.GenericObject#getObjectClassName()
    */
   @Override
   public String getObjectClassName()
   {
      return "Container";
   }

   /* (non-Javadoc)
    * @see org.netxms.client.objects.AbstractObject#getStrings()
    */
   @Override
   public Set<String> getStrings()
   {
      Set<String> strings = super.getStrings();
      addString(strings, objectAutoBindFilter);
      return strings;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getIfXTablePolicy()
    */
   @Override
   public int getIfXTablePolicy()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getAgentCacheMode()
    */
   @Override
   public AgentCacheMode getAgentCacheMode()
   {
      return null;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getFlags()
    */
   @Override
   public int getFlags()
   {
      return flags;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#getPollerNodeId()
    */
   @Override
   public long getPollerNodeId()
   {
      return 0;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveAgent()
    */
   @Override
   public boolean canHaveAgent()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHaveInterfaces()
    */
   @Override
   public boolean canHaveInterfaces()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canHavePollerNode()
    */
   @Override
   public boolean canHavePollerNode()
   {
      return false;
   }

   /**
    * @see org.netxms.client.objects.interfaces.PollingTarget#canUseEtherNetIP()
    */
   @Override
   public boolean canUseEtherNetIP()
   {
      return false;
   }

   /**
    * @return the dciStatusThreshold
    */
   public int getDciStatusThreshold()
   {
      return dciStatusThreshold;
   }

   /**
    * @return the nodeStatusThreshold
    */
   public int getNodeStatusThreshold()
   {
      return nodeStatusThreshold;
   }
}
