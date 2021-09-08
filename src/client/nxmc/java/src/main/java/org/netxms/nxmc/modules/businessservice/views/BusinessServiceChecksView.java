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
package org.netxms.nxmc.modules.businessservice.views;

import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.businessservices.ServiceCheck;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessService;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.businessservice.dialogs.EditBSCheckDlg;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceCheckFilter;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceCheckLabelProvider;
import org.netxms.nxmc.modules.businessservice.views.helpers.BusinessServiceComparator;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
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
   private SessionListener sessionListener;
   private SortableTableViewer viewer;
   private BusinessServiceCheckLabelProvider labelProvider;
   private Action actionEdit;
   private Action actionCreate;
   private Action actionDelete;
   private Map<Long, ServiceCheck> checksList;
   
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
      labelProvider = new BusinessServiceCheckLabelProvider();
      BusinessServiceCheckFilter filter = new BusinessServiceCheckFilter(labelProvider);
      setViewerAndFilter(viewer, filter);
      viewer.setComparator(new BusinessServiceComparator(labelProvider));
      viewer.setLabelProvider(labelProvider);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.addSelectionChangedListener(new ISelectionChangedListener() {         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            actionEdit.setEnabled(selection.size() == 1);
            actionDelete.setEnabled(selection.size() > 0);
         }
      });
      
      WidgetHelper.restoreColumnSettings(viewer.getTable(), ID);
      viewer.getTable().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveColumnSettings(viewer.getTable(), ID);
         }
      });
      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            actionEdit.run();            
         }
      });
      
      createActions();
      createPopupMenu();

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case SessionNotification.BUSINESS_SERVICE_CHECK_MODIFY:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        checksList.put(n.getSubCode(), (ServiceCheck)n.getObject());
                        viewer.setInput(checksList.values());
                     }
                  });
                  break;
               case SessionNotification.BUSINESS_SERVICE_CHECK_DELETE:
                  viewer.getControl().getDisplay().asyncExec(new Runnable() {
                     @Override
                     public void run()
                     {
                        checksList.remove(n.getSubCode());
                        viewer.setInput(checksList.values());
                     }
                  });
                  break;
            }
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Get node components"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            checksList = session.getBusinessServiceChecks(getObject().getObjectId());
            
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  viewer.setInput(checksList.values());
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get component information for node {0}", getObject().getObjectName());
         }
      }.start();
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
      manager.add(actionEdit);
      manager.add(actionCreate);
      manager.add(actionDelete);
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionCreate = new Action(i18n.tr("&Create new"), SharedIcons.ADD_OBJECT) 
      {
         @Override
         public void run()
         {
            createCheck();
         }
      };
      actionEdit = new Action(i18n.tr("&Edit..."), SharedIcons.EDIT) 
      {
         @Override
         public void run()
         {
            editCheck();
         }
      };
      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) 
      {
         @Override
         public void run()
         {
            deleteCheck();
         }
      };
   }

   protected void deleteCheck()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.isEmpty())
         return;

      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirm Delete"),
            i18n.tr("Do you really want to delete selected check?")))
         return;

      final Object[] objects = selection.toArray();
      new Job(i18n.tr("Delete actions"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(int i = 0; i < objects.length; i++)
            {
               session.deleteBusinessServiceCheck(getObject().getObjectId(), ((ServiceCheck)objects[i]).getId());
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete business service check");
         }
      }.start();      
   }

   protected void editCheck()
   {
      IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
      if (selection.size() != 1)
         return;

      final ServiceCheck check = new ServiceCheck((ServiceCheck)selection.getFirstElement());
      final EditBSCheckDlg dlg = new EditBSCheckDlg(getWindow().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Update action configuration"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot update action configuration");
            }
         }.start();
      }
   }

   private void createCheck()
   {
      final ServiceCheck check = new ServiceCheck();
      final EditBSCheckDlg dlg = new EditBSCheckDlg(getWindow().getShell(), check, false);
      if (dlg.open() == Window.OK)
      {
         new Job(i18n.tr("Update action configuration"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.modifyBusinessServiceCheck(getObject().getObjectId(), check);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot update action configuration");
            }
         }.start();
      }
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

      Job job = new Job(i18n.tr("Get node components"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            checksList = session.getBusinessServiceChecks(object.getObjectId());
            
            runInUIThread(new Runnable() {               
               @Override
               public void run()
               {
                  viewer.setInput(checksList.values());
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

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      //session.removeListener(sessionListener);
      super.dispose();
   }
}
