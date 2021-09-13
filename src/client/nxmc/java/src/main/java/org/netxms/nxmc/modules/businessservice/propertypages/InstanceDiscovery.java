/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Raden Solutions
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
package org.netxms.nxmc.modules.businessservice.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.BusinessServicePrototype;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage;
import org.netxms.nxmc.tools.WidgetFactory;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Instance Discovery" property page for DCO
 */
public class InstanceDiscovery extends ObjectPropertyPage
{
   private static final I18n i18n = LocalizationHelper.getI18n(InstanceDiscovery.class);   
	private Combo discoveryMethod;
	private LabeledText discoveryData;
	private ScriptEditor filterScript;
   
   /**
    * Constructor
    * 
    * @param editor
    */
   public InstanceDiscovery(AbstractObject prototype)
   {
      super(i18n.tr("Instance Discovery"), prototype);
   }
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{		
      Composite dialogArea = new Composite(parent, SWT.NONE);
      BusinessServicePrototype prototype = (BusinessServicePrototype)object;
      
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		dialogArea.setLayout(layout);

      discoveryMethod = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, i18n.tr("Instance discovery method"),
                                                         WidgetHelper.DEFAULT_LAYOUT_DATA);
      discoveryMethod.add(i18n.tr("Script"));
      discoveryMethod.add(i18n.tr("Agent List"));
      discoveryMethod.add(i18n.tr("Agent Table"));
      discoveryMethod.select(prototype.getInstanceDiscoveryMethod());
      discoveryMethod.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				int method = discoveryMethod.getSelectionIndex();
		      discoveryData.setLabel(getDataLabel(method));
			}

			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

      discoveryData = new LabeledText(dialogArea, SWT.NONE);
      discoveryData.setLabel(getDataLabel(prototype.getInstanceDiscoveryMethod()));
      discoveryData.setText(prototype.getInstanceDiscoveryData());
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      discoveryData.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 0;
      gd.heightHint = 0;
      final WidgetFactory factory = new WidgetFactory() {
			@Override
			public Control createControl(Composite dialogArea, int style)
			{
				return new ScriptEditor(dialogArea, style, SWT.H_SCROLL | SWT.V_SCROLL, false, 
				      "Variables:\r\n\t$1\t\t\tInstance to test;\r\n\t$2\t\t\tInstance data (secondary value for Script method);\r\n\t$businessService\t\tthis business service prototype object;\r\n\r\nReturn value:\r\n\ttrue/false to accept or reject instance without additional changes or\r\n\tarray of two or three elements to modify instance:\r\n\t\t1st element - true/false to indicate acceptance;\r\n\t\t2nd element - new instance name;\r\n\t\t3rd element - new instance display name.");
			}
      };
      filterScript = (ScriptEditor)WidgetHelper.createLabeledControl(dialogArea, SWT.BORDER,
            factory, i18n.tr("Instance discovery filter script"), gd);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      filterScript.setLayoutData(gd);
      filterScript.setText(prototype.getInstanceDiscoveryFilter());

		return dialogArea;
	}

	/**
	 * Get label for data field
	 * 
	 * @param method
	 * @return
	 */
	private static String getDataLabel(int method)
	{
		switch(method)
		{
			case BusinessServicePrototype.IDM_SCRIPT:
			   return "Script name";
		}
		return ""; //$NON-NLS-1$
	}

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#applyChanges(boolean)
    */
   @Override
   protected boolean applyChanges(final boolean isApply)
	{       
      if (isApply)
         setValid(false);
      NXCSession session = Registry.getSession();     
      final NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
      md.setInstanceDiscoveryMethod(discoveryMethod.getSelectionIndex());
      md.setInstanceDiscoveryData(discoveryData.getText());
      md.setInstanceDiscoveryFilter(filterScript.getText());

      new Job(String.format(i18n.tr("Modify instance discovery for busines service prototype %s"), object.getObjectName()), null, null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected void jobFinalize()
         {
            if (isApply)
            {
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     InstanceDiscovery.this.setValid(true);
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot modify instance discovery");
         }
      }.start();
      
      return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		discoveryMethod.select(BusinessServicePrototype.IDM_SCRIPT);
		discoveryData.setLabel(getDataLabel(BusinessServicePrototype.IDM_SCRIPT));
		discoveryData.setText(""); //$NON-NLS-1$
		filterScript.setText(""); //$NON-NLS-1$
	}

   @Override
   public String getId()
   {
      return "InstanceDiscovery";
   }

   @Override
   public boolean isVisible()
   {
      return object instanceof BusinessServicePrototype;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.propertypages.ObjectPropertyPage#getPriority()
    */
   @Override
   public int getPriority()
   {
      return 30;
   }
}
