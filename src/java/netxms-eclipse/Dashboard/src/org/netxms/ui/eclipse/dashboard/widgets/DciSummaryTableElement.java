/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.ui.IViewPart;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DciSummaryTableConfig;
import org.netxms.ui.eclipse.datacollection.widgets.SummaryTableWidget;

/**
 * DCI summary table element for dashboard
 */
public class DciSummaryTableElement extends ElementWidget
{
	private DciSummaryTableConfig config;
	private SummaryTableWidget viewer;
	
	/**
	 * @param parent
	 * @param data
	 */
	public DciSummaryTableElement(DashboardControl parent, DashboardElement element, IViewPart viewPart)
	{
		super(parent, element, viewPart);

		try
		{
			config = DciSummaryTableConfig.createFromXml(element.getData());
		}
		catch(Exception e)
		{
			e.printStackTrace();
			config = new DciSummaryTableConfig();
		}
		
      processCommonSettings(config);

      viewer = new SummaryTableWidget(getContentArea(), SWT.NONE, viewPart, config.getTableId(), config.getBaseObjectId());
      viewer.setShowNumLine(config.getNumRowShown());
		viewer.setSortColumns(config.getSortingColumnList());
      viewer.getViewer().getControl().addFocusListener(new FocusListener() {
         @Override
         public void focusLost(FocusEvent e)
         {
         }
         
         @Override
         public void focusGained(FocusEvent e)
         {
            setSelectionProviderDelegate(viewer.getObjectSelectionProvider());
         }
      });
		viewer.refresh();
      viewer.setAutoRefresh(config.getRefreshInterval());
	}
}
