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
package org.netxms.nxmc.modules.businessservice.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.netxms.client.businessservices.SlmTicket;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.modules.businessservice.views.BusinessServiceAvailabilityView;

/**
 * Comparator for 802.1x port state table
 */
public class SlmTicketComparator extends ViewerComparator
{
   /**
    * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
	@Override
	public int compare(Viewer viewer, Object e1, Object e2)
	{
	   SlmTicket t1 = (SlmTicket)e1;
	   SlmTicket t2 = (SlmTicket)e2;
		
		int result = 0;
		switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
		{
         case BusinessServiceAvailabilityView.COLUMN_ID:   
            result = Long.signum(t1.getId() - t2.getId());  
            break;
         case BusinessServiceAvailabilityView.COLUMN_SERVICE_ID:      
            result = Long.signum(t1.getServiceId() - t2.getServiceId());  
            break; 
         case BusinessServiceAvailabilityView.COLUMN_CHECK_ID:     
            result = Long.signum(t1.getCheckId() - t2.getCheckId());  
            break;  
         case BusinessServiceAvailabilityView.COLUMN_CHECK_DESCRIPTION:     
            result = t1.getCheckDescription().compareTo(t2.getCheckDescription());  
            break;    
         case BusinessServiceAvailabilityView.COLUMN_CREATION_TIME:   
            result = t1.getCreationTime().compareTo(t2.getCreationTime());  
            break;             
         case BusinessServiceAvailabilityView.COLUMN_TERMINATION_TIME:  
            result = t1.getCloseTime().compareTo(t2.getCloseTime());  
            break;             
         case BusinessServiceAvailabilityView.COLUMN_REASON:   
            result = t1.getReason().compareTo(t2.getReason());  
            break;     
		}
		return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
	}
}
