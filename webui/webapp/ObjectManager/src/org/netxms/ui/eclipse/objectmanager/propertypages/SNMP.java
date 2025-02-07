/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.dialogs.PropertyPage;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.objectmanager.Messages;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.PasswordInputField;

/**
 * "SNMP" property page for node
 */
public class SNMP extends PropertyPage
{
   private AbstractNode node;
   private Combo snmpVersion;
   private LabeledText snmpPort;
   private Combo snmpAuth;
   private Combo snmpPriv;
   private ObjectSelector snmpProxy;
   private LabeledText snmpAuthName;
   private PasswordInputField snmpAuthPassword;
   private PasswordInputField snmpPrivPassword;
   private LabeledText snmpCodepage;
   private Button snmpSettingsLocked;

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      node = (AbstractNode)getElement().getAdapter(AbstractNode.class);

      Composite dialogArea = new Composite(parent, SWT.NONE);
      FormLayout dialogLayout = new FormLayout();
      dialogLayout.marginWidth = 0;
      dialogLayout.marginHeight = 0;
      dialogLayout.spacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(dialogLayout);

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      snmpVersion = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Version, fd);
      snmpVersion.add("1"); //$NON-NLS-1$
      snmpVersion.add("2c"); //$NON-NLS-1$
      snmpVersion.add("3"); //$NON-NLS-1$
      snmpVersion.select(snmpVersionToIndex(node.getSnmpVersion()));
      snmpVersion.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onSnmpVersionChange();
         }
      });
      
      snmpPort = new LabeledText(dialogArea, SWT.NONE);
      snmpPort.setLabel(Messages.get().Communication_UDPPort);
      snmpPort.setText(Integer.toString(node.getSnmpPort()));

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
      snmpAuth = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Authentication, fd);
      snmpAuth.add(Messages.get().Communication_AuthNone);
      snmpAuth.add(Messages.get().Communication_AuthMD5);
      snmpAuth.add(Messages.get().Communication_AuthSHA1);
      snmpAuth.add("SHA224");
      snmpAuth.add("SHA256");
      snmpAuth.add("SHA384");
      snmpAuth.add("SHA512");
      snmpAuth.select(node.getSnmpAuthMethod());
      snmpAuth.setEnabled(node.getSnmpVersion() == SnmpVersion.V3);

      fd = new FormData();
      fd.left = new FormAttachment(snmpAuth.getParent(), 0, SWT.RIGHT);
      fd.top = new FormAttachment(snmpVersion.getParent(), 0, SWT.BOTTOM);
      snmpPriv = WidgetHelper.createLabeledCombo(dialogArea, SWT.BORDER | SWT.READ_ONLY, Messages.get().Communication_Encryption, fd);
      snmpPriv.add(Messages.get().Communication_EncNone);
      snmpPriv.add(Messages.get().Communication_EncDES);
      snmpPriv.add(Messages.get().Communication_EncAES);
      snmpPriv.select(node.getSnmpPrivMethod());
      snmpPriv.setEnabled(node.getSnmpVersion() == SnmpVersion.V3);
      
      snmpProxy = new ObjectSelector(dialogArea, SWT.NONE, true);
      snmpProxy.setLabel(Messages.get().Communication_Proxy);
      snmpProxy.setObjectId(node.getSnmpProxyId());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.BOTTOM);
      fd.right = new FormAttachment(snmpPriv.getParent(), 0, SWT.RIGHT);
      snmpProxy.setLayoutData(fd);
      
      snmpAuthName = new LabeledText(dialogArea, SWT.NONE);
      snmpAuthName.setLabel(node.getSnmpVersion() == SnmpVersion.V3 ? Messages.get().Communication_UserName
            : Messages.get().Communication_Community);
      snmpAuthName.setText(node.getSnmpAuthName());
      fd = new FormData();
      fd.left = new FormAttachment(snmpProxy, 0, SWT.RIGHT);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      snmpAuthName.setLayoutData(fd);
      
      snmpAuthPassword = new PasswordInputField(dialogArea, SWT.NONE);
      snmpAuthPassword.setLabel(Messages.get().Communication_AuthPassword);
      snmpAuthPassword.setText(node.getSnmpAuthPassword());
      fd = new FormData();
      fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(snmpAuth.getParent(), 0, SWT.TOP);
      fd.right = new FormAttachment(100, 0);
      snmpAuthPassword.setLayoutData(fd);
      snmpAuthPassword.setInputControlsEnabled(node.getSnmpVersion() == SnmpVersion.V3);
      
      snmpPrivPassword = new PasswordInputField(dialogArea, SWT.NONE);
      snmpPrivPassword.setLabel(Messages.get().Communication_EncPassword);
      snmpPrivPassword.setText(node.getSnmpPrivPassword());
      fd = new FormData();
      fd.left = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(snmpProxy, 0, SWT.TOP);
      fd.right = new FormAttachment(100, 0);
      snmpPrivPassword.setLayoutData(fd);
      snmpPrivPassword.setInputControlsEnabled(node.getSnmpVersion() == SnmpVersion.V3);

      fd = new FormData();
      fd.left = new FormAttachment(snmpVersion.getParent(), 0, SWT.RIGHT);
      fd.right = new FormAttachment(snmpAuthName, 0, SWT.LEFT);
      fd.top = new FormAttachment(0, 0);
      snmpPort.setLayoutData(fd);

      snmpCodepage = new LabeledText(dialogArea, SWT.NONE);
      snmpCodepage.setLabel("Codepage");
      snmpCodepage.setText(node.getSNMPCodepage());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(snmpProxy, 0, SWT.BOTTOM);
      snmpCodepage.setLayoutData(fd);
      
      snmpSettingsLocked = new Button(dialogArea, SWT.CHECK);
      snmpSettingsLocked.setText("&Prevent automatic SNMP configuration changes");
      snmpSettingsLocked.setSelection(node.isSnmpSettingsLocked());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(snmpCodepage, 0, SWT.BOTTOM);
      snmpSettingsLocked.setLayoutData(fd);

      return dialogArea;
   }

   /**
    * Convert SNMP version to index in combo box
    * 
    * @param version SNMP version
    * @return index in combo box
    */
   private int snmpVersionToIndex(SnmpVersion version)
   {
      switch(version)
      {
         case V1:
            return 0;
         case V2C:
            return 1;
         case V3:
            return 2;
         default:
            return 0;
      }
   }
   
   /**
    * Convert selection index in SNMP version combo box to SNMP version
    */
   private SnmpVersion snmpIndexToVersion(int index)
   {
      final SnmpVersion[] versions = { SnmpVersion.V1, SnmpVersion.V2C, SnmpVersion.V3 };
      return versions[index];
   }
   
   /**
    * Handler for SNMP version change
    */
   private void onSnmpVersionChange()
   {
      boolean isV3 = (snmpVersion.getSelectionIndex() == 2);
      snmpAuthName.setLabel(isV3 ? Messages.get().Communication_UserName : Messages.get().Communication_Community);
      snmpAuth.setEnabled(isV3);
      snmpPriv.setEnabled(isV3);
      snmpAuthPassword.setInputControlsEnabled(isV3);
      snmpPrivPassword.setInputControlsEnabled(isV3);
   }

   /**
    * Apply changes
    * 
    * @param isApply true if update operation caused by "Apply" button
    */
   protected boolean applyChanges(final boolean isApply)
   {
      final NXCObjectModificationData md = new NXCObjectModificationData(node.getObjectId());

      if (isApply)
         setValid(false);

      md.setSnmpVersion(snmpIndexToVersion(snmpVersion.getSelectionIndex()));
      try
      {
         md.setSnmpPort(Integer.parseInt(snmpPort.getText(), 10));
      }
      catch(NumberFormatException e)
      {
         MessageDialog.openWarning(getShell(), Messages.get().Communication_Warning, Messages.get().Communication_WarningInvalidSNMPPort);
         if (isApply)
            setValid(true);
         return false;
      }
      md.setSnmpProxy(snmpProxy.getObjectId());
      md.setSnmpAuthentication(snmpAuthName.getText(), snmpAuth.getSelectionIndex(), snmpAuthPassword.getText(), snmpPriv.getSelectionIndex(), snmpPrivPassword.getText());
      md.setSNMPCodepage(snmpCodepage.getText());

      int flags = node.getFlags();
      if (snmpSettingsLocked.getSelection())
         flags |= AbstractNode.NF_SNMP_SETTINGS_LOCKED;
      else
         flags &= ~AbstractNode.NF_SNMP_SETTINGS_LOCKED;
      md.setObjectFlags(flags, AbstractNode.NF_SNMP_SETTINGS_LOCKED);

      final NXCSession session = ConsoleSharedData.getSession();
      new ConsoleJob(String.format("Updating SNMP settings for node %s", node.getObjectName()), null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            session.modifyObject(md);
         }

         @Override
         protected String getErrorMessage()
         {
            return String.format("Cannot update SNMP settings for node %s", node.getObjectName());
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
                     SNMP.this.setValid(true);
                  }
               });
            }
         }
      }.start();
      return true;
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      return applyChanges(false);
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
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
   @Override
   protected void performDefaults()
   {
      super.performDefaults();
      
      snmpVersion.select(0);
      snmpAuth.select(0);
      snmpPriv.select(0);
      snmpAuthName.setText("public"); //$NON-NLS-1$
      snmpAuthPassword.setText(""); //$NON-NLS-1$
      snmpPrivPassword.setText(""); //$NON-NLS-1$
      snmpProxy.setObjectId(0);
      onSnmpVersionChange();
      snmpCodepage.setText(""); //$NON-NLS-1$
   }
}
