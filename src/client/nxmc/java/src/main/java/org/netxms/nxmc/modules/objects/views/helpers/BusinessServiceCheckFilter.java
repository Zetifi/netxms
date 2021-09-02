/**
 * NetXMS - open source network management system
 * Copyright (C) 2016-2021 RadenSolutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.NXCSession;
import org.netxms.client.businessservices.ServiceCheck;
import org.netxms.client.topology.FdbEntry;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.ViewerFilterInternal;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.BusinessServiceChecksView;
import org.xnap.commons.i18n.I18n;

/**
 * Filter for switch forwarding database  
 */
public class BusinessServiceCheckFilter extends ViewerFilter implements ViewerFilterInternal
{
   private static final I18n i18n = LocalizationHelper.getI18n(BusinessServiceCheckLabelProvider.class);
   private static final String[] TYPES = {i18n.tr("Node"), i18n.tr("Script"), i18n.tr("DCI")};

   private NXCSession session = Registry.getSession();
   private String filterString = null;
   
   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;

      ServiceCheck check = (ServiceCheck)element;      
      String status = check.isViolated() ? i18n.tr("Violated") : i18n.tr("Normal");
      
      if (Long.toString(check.getId()).contains(filterString) || check.getDescription().contains(filterString) ||
            TYPES[check.getCheckType()].contains(filterString) || status.contains(filterString) ||
            check.getFailureReason().contains(filterString))
         return true;
      return false;
   }

   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filterString;
   }

   /**
    * @param filterString the filterString to set
    */
   public void setFilterString(String filterString)
   {
      this.filterString = filterString.toLowerCase();
   }   
}
