/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.propertypages;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.Platform;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ILabelProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowData;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.ui.eclipse.datacollection.Activator;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.AccessListLabelProvider;
import org.netxms.ui.eclipse.datacollection.propertypages.helpers.AbstractDCIPropertyPage;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ElementLabelComparator;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;

/**
 * "Access Control" property page
 */
public class AccessControl extends AbstractDCIPropertyPage
{
   private DataCollectionObject dco;
	private Set<AbstractUserObject> acl = new HashSet<AbstractUserObject>();
	private TableViewer viewer;
	private Button buttonAdd;
	private Button buttonRemove;
	private static final String info[] = {"Inherited from object access rights"};
	private NXCSession session;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
      Composite dialogArea = (Composite)super.createContents(parent);
      dco = editor.getObject();      

      // Initiate loading of user manager plugin if it was not loaded before
      Platform.getAdapterManager().loadAdapter(new User(""), "org.eclipse.ui.model.IWorkbenchAdapter"); //$NON-NLS-1$ //$NON-NLS-2$
		
		// Build internal copy of access list
		session = (NXCSession)ConsoleSharedData.getSession();
		for(Long uid : dco.getAccessList())
		{
			AbstractUserObject o = session.findUserDBObjectById(uid, null);
			if (o != null)
				acl.add(o);
		}
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.INNER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);
		
		Label label = new Label(dialogArea, SWT.NONE);
		label.setText("Restrict access to the following users");
		
		viewer = new TableViewer(dialogArea, SWT.BORDER | SWT.FULL_SELECTION | SWT.MULTI);
		viewer.setContentProvider(new ArrayContentProvider());
		viewer.setLabelProvider(new AccessListLabelProvider());
		viewer.setComparator(new ElementLabelComparator((ILabelProvider)viewer.getLabelProvider()));
		viewer.getTable().setSortDirection(SWT.UP);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.heightHint = 300;
		viewer.getTable().setLayoutData(gd);
		setViewerInput();
		
      Composite buttons = new Composite(dialogArea, SWT.NONE);
      RowLayout buttonLayout = new RowLayout();
      buttonLayout.type = SWT.HORIZONTAL;
      buttonLayout.pack = false;
      buttonLayout.marginWidth = 0;
      buttons.setLayout(buttonLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.verticalIndent = WidgetHelper.OUTER_SPACING - WidgetHelper.INNER_SPACING;
      buttons.setLayoutData(gd);

      buttonAdd = new Button(buttons, SWT.PUSH);
      buttonAdd.setText("Add");
      buttonAdd.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				addUser();
			}
      });
      RowData rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonAdd.setLayoutData(rd);
		
      buttonRemove = new Button(buttons, SWT.PUSH);
      buttonRemove.setText("Remove");
      buttonRemove.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				removeUsers();
			}
      });
      rd = new RowData();
      rd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      buttonRemove.setLayoutData(rd);
      
      getUsersAndRefresh();
		
		return dialogArea;
	}
	
	/**
	 * Get user info and refresh view
	 */
	private void getUsersAndRefresh()
   {
      ConsoleJob job = new ConsoleJob("Synchronize node components", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            if (session.syncMissingUsers(new HashSet<Long>(dco.getAccessList())))
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {       
                     for(Long uid : dco.getAccessList())
                     {
                        AbstractUserObject o = session.findUserDBObjectById(uid, null);
                        if (o != null)
                           acl.add(o);
                     }
                     
                     setViewerInput();
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot synchronize node components";
         }
      };
      job.setUser(false);
      job.start();  
   }

   /**
	 * Set viewer input
	 */
	private void setViewerInput()
	{
      if (acl.isEmpty())
         viewer.setInput(info);
      else
         viewer.setInput(acl.toArray());
	}

	/**
	 * Add users to ACL
	 */
	public void addUser()
	{
		SelectUserDialog dlg = new SelectUserDialog(getShell(), AbstractUserObject.class);
		if (dlg.open() == Window.OK)
		{
			for(AbstractUserObject o : dlg.getSelection())
				acl.add(o);
			setViewerInput();
		}
	}

	/**
	 * Remove users from ACL
	 */
	public void removeUsers()
	{
		IStructuredSelection selection = (IStructuredSelection)viewer.getSelection();
		for(Object o : selection.toList())
			acl.remove(o);
      setViewerInput();
	}

	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		List<Long> list = new ArrayList<Long>(acl.size());
		for(AbstractUserObject o : acl)
			list.add(o.getId());
		dco.setAccessList(list);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

   /* (non-Javadoc)
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      acl.clear();
      setViewerInput();
   }
}
