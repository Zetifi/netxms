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
package org.netxms.nxmc.modules.objects.views;

import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.businessservices.ServiceCheck;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.helpers.BusinessServiceCheckFilter;
import org.netxms.nxmc.modules.objects.views.helpers.BusinessServiceCheckLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Business service "Checks" view
 */
public class BusinessServiceChecksView extends ObjectView
{
   private static final I18n i18n = LocalizationHelper.getI18n(BusinessServiceChecksView.class);
   private static final String ID = "BusinessServiceChecks";

   public static final int COLUMN_ID = 0;
   public static final int COLUMN_DESCRIPTION = 1;
   public static final int COLUMN_TYPE = 2;
   public static final int COLUMN_STATUS = 3;
   public static final int COLUMN_FAIL_REASON = 4;

   private NXCSession session;
   private SortableTableViewer viewer;
   private BusinessServiceCheckLabelProvider labelProvider;
   private Action actionEdit;
   private Action actionCreate;
   private Action actionDelete;
   private List<ServiceCheck> checksList;
   
   /**
    * @param name
    * @param image
    */
   public BusinessServiceChecksView()
   {
      super(i18n.tr("BusinessServiceChecks"), ResourceManager.getImageDescriptor("icons/object-views/service_check.gif"), "Checks", true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      session = Registry.getSession();
      
      // Setup table columns
      final String[] names = { 
            i18n.tr("ID"), 
            i18n.tr("Description"),
            i18n.tr("Type"),
            i18n.tr("Status"),
            i18n.tr("Reason")
         };
      final int[] widths = { 70, 200, 100, 70, 300 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SortableTreeViewer.DEFAULT_STYLE);
      BusinessServiceCheckFilter filter = new BusinessServiceCheckFilter();
      setViewerAndFilter(viewer, filter);
      labelProvider = new BusinessServiceCheckLabelProvider();
      viewer.setComparator(new ViewerComparator());
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      WidgetHelper.restoreColumnSettings(viewer.getTable(), ID);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), ID);
         }
      });
      createActions();
      createPopupMenu();
   }

   /**
    * Create pop-up menu
    */
   private void createPopupMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager manager)
         {
            fillContextMenu(manager);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(viewer.getControl());
      viewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(IMenuManager manager)
   {
      //TODO:
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      //TODO:
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
   @Override
   protected void onObjectChange(AbstractObject object)
   {
      viewer.setInput(new Object[0]);
      if (object == null)
         return;

      final NXCSession session = Registry.getSession();
      Job job = new Job(i18n.tr("Get node components"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            checksList = session.getBusinessServiceChecks(object.getObjectId());
            
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  viewer.setInput(checksList);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get component information for node {0}", object.getObjectName());
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if ((context != null) && (context instanceof BusinessService))
      {
         return true;
      }
      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 60;
   }
}
