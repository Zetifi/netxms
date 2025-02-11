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
package org.netxms.nxmc.modules.datacollection.propertypages;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.AgentCacheMode;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.DataType;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.Node;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.client.snmp.SnmpVersion;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.DataCollectionObjectEditor;
import org.netxms.nxmc.modules.datacollection.dialogs.IParameterSelectionDialog;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectAgentParamDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectInternalParamDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectParameterScriptDialog;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectSnmpParamDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.SelectWebServiceDlg;
import org.netxms.nxmc.modules.datacollection.dialogs.WinPerfCounterSelectionDialog;
import org.netxms.nxmc.modules.datacollection.widgets.helpers.DataCollectionDisplayInfo;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "General" property page for DCI
 */
public class GeneralItem extends AbstractDCIPropertyPage
{
   private static final I18n i18n = LocalizationHelper.getI18n(GeneralItem.class);
	private static final String[] snmpRawTypes = 
	{ 
		i18n.tr("None"), 
		i18n.tr("32-bit signed integer"), 
		i18n.tr("32-bit unsigned integer"),
		i18n.tr("64-bit signed integer"), 
		i18n.tr("64-bit unsigned integer"),
		i18n.tr("Floating point number"), 
		i18n.tr("IP address"),
		i18n.tr("MAC address")
	};
	
	private DataCollectionItem dci;
	private Text description;
	private LabeledText parameter;
	private Button selectButton;
	private Combo origin;
	private Combo dataType;
	private Button checkInterpretRawSnmpValue;
	private Combo snmpRawType;
	private Button checkUseCustomSnmpPort;
	private Spinner customSnmpPort;
   private Button checkUseCustomSnmpVersion;
   private Combo customSnmpVersion;
	private ObjectSelector sourceNode;
	private Combo agentCacheMode;
	private Combo schedulingMode;
	private Combo retentionMode;
	private LabeledText pollingInterval;
	private LabeledText retentionTime;
	private LabeledSpinner sampleCount;
	
	/**
	 * Constructor
	 * 
	 * @param editor
	 */
   public GeneralItem(DataCollectionObjectEditor editor)
   {
      super(i18n.tr("General"), editor);
   }
	
   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createContents(Composite parent)
	{		
	   Composite dialogArea = (Composite)super.createContents(parent);
		dci = editor.getObjectAsItem();
		
		GridLayout layout = new GridLayout();
		layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      /** description area **/
      Group groupDescription = new Group(dialogArea, SWT.NONE);
      groupDescription.setText(i18n.tr("Description"));
      FillLayout descriptionLayout = new FillLayout();
      descriptionLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      descriptionLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      groupDescription.setLayout(descriptionLayout);
      description = new Text(groupDescription, SWT.BORDER);
      description.setTextLimit(255);
      description.setText(dci.getDescription());
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupDescription.setLayoutData(gd);
      
      /** data area **/
      Group groupData = new Group(dialogArea, SWT.NONE);
      groupData.setText(i18n.tr("Data"));
      FormLayout dataLayout = new FormLayout();
      dataLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      dataLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      groupData.setLayout(dataLayout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupData.setLayoutData(gd);
      
      parameter = new LabeledText(groupData, SWT.NONE);
      parameter.setLabel(i18n.tr("Parameter"));
      parameter.getTextControl().setTextLimit(255);
      parameter.setText(dci.getName());
      
      selectButton = new Button(groupData, SWT.PUSH);
      selectButton.setText(i18n.tr("&Select..."));
      selectButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				selectParameter();
			}
      });

      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(selectButton, -WidgetHelper.INNER_SPACING, SWT.LEFT);
      parameter.setLayoutData(fd);

      fd = new FormData();
      fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(parameter, 0, SWT.BOTTOM);
      fd.width = WidgetHelper.BUTTON_WIDTH_HINT;
      selectButton.setLayoutData(fd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(50, -WidgetHelper.OUTER_SPACING / 2);
      origin = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, i18n.tr("Origin"), fd);
      origin.add(i18n.tr("Internal"));
      origin.add(i18n.tr("NetXMS Agent"));
      origin.add(i18n.tr("SNMP"));
      origin.add(i18n.tr("Web Service"));
      origin.add(i18n.tr("Push"));
      origin.add(i18n.tr("Windows Performance Counters"));
      origin.add(i18n.tr("SM-CLP"));
      origin.add(i18n.tr("Script"));
      origin.add(i18n.tr("SSH"));
      origin.add(i18n.tr("MQTT"));
      origin.add(i18n.tr("Network Device Driver"));
      origin.select(dci.getOrigin().getValue());
      origin.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				onOriginChange();
			}
      });
      
      fd = new FormData();
      fd.left = new FormAttachment(50, WidgetHelper.OUTER_SPACING / 2);
      fd.top = new FormAttachment(parameter, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      dataType = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, i18n.tr("Data Type"), fd);
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER32));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.INT64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.UINT64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.COUNTER64));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.FLOAT));
      dataType.add(DataCollectionDisplayInfo.getDataTypeName(DataType.STRING));
      dataType.select(getDataTypePosition(dci.getDataType()));

      checkInterpretRawSnmpValue = new Button(groupData, SWT.CHECK);
      checkInterpretRawSnmpValue.setText(i18n.tr("Interpret SNMP octet string raw value as"));
      checkInterpretRawSnmpValue.setSelection(dci.isSnmpRawValueInOctetString());
      checkInterpretRawSnmpValue.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      snmpRawType.setEnabled(checkInterpretRawSnmpValue.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(origin.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      checkInterpretRawSnmpValue.setLayoutData(fd);
      checkInterpretRawSnmpValue.setEnabled(dci.getOrigin() == DataOrigin.SNMP);

      snmpRawType = new Combo(groupData, SWT.BORDER | SWT.READ_ONLY);
      for(int i = 0; i < snmpRawTypes.length; i++)
      	snmpRawType.add(snmpRawTypes[i]);
      snmpRawType.select(dci.getSnmpRawValueType());
      snmpRawType.setEnabled((dci.getOrigin() == DataOrigin.SNMP) && dci.isSnmpRawValueInOctetString());
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(checkInterpretRawSnmpValue, 0, SWT.RIGHT);
      snmpRawType.setLayoutData(fd);
      
      checkUseCustomSnmpPort = new Button(groupData, SWT.CHECK);
      checkUseCustomSnmpPort.setText(i18n.tr("Use custom SNMP port:"));
      checkUseCustomSnmpPort.setSelection(dci.getSnmpPort() != 0);
      checkUseCustomSnmpPort.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
		      customSnmpPort.setEnabled(checkUseCustomSnmpPort.getSelection());
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
      fd = new FormData();
      fd.left = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(dataType.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      checkUseCustomSnmpPort.setLayoutData(fd);
      checkUseCustomSnmpPort.setEnabled(dci.getOrigin() == DataOrigin.SNMP);

      customSnmpPort = new Spinner(groupData, SWT.BORDER);
      customSnmpPort.setMinimum(1);
      customSnmpPort.setMaximum(65535);
      if ((dci.getOrigin() == DataOrigin.SNMP) && (dci.getSnmpPort() != 0))
      {
      	customSnmpPort.setEnabled(true);
      	customSnmpPort.setSelection(dci.getSnmpPort());
      }
      else
      {
      	customSnmpPort.setEnabled(false);
      }
      fd = new FormData();
      fd.left = new FormAttachment(checkInterpretRawSnmpValue, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(checkUseCustomSnmpPort, 0, SWT.RIGHT);
      customSnmpPort.setLayoutData(fd);

      checkUseCustomSnmpVersion = new Button(groupData, SWT.CHECK);
      checkUseCustomSnmpVersion.setText(i18n.tr("Use custom SNMP version:"));
      checkUseCustomSnmpVersion.setSelection(dci.getSnmpVersion() != SnmpVersion.DEFAULT);
      checkUseCustomSnmpVersion.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            customSnmpVersion.setEnabled(checkUseCustomSnmpVersion.getSelection());
         }

         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }
      });
      fd = new FormData();
      fd.left = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(dataType.getParent(), WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      checkUseCustomSnmpVersion.setLayoutData(fd);
      checkUseCustomSnmpVersion.setEnabled(dci.getOrigin() == DataOrigin.SNMP);

      customSnmpVersion = new Combo(groupData, SWT.BORDER | SWT.READ_ONLY);
      customSnmpVersion.add("1");
      customSnmpVersion.add("2c");
      customSnmpVersion.add("3");
      customSnmpVersion.select(indexFromSnmpVersion(dci.getSnmpVersion()));
      customSnmpVersion.setEnabled(dci.getSnmpVersion() != SnmpVersion.DEFAULT);
      fd = new FormData();
      fd.left = new FormAttachment(checkUseCustomSnmpPort, WidgetHelper.OUTER_SPACING, SWT.RIGHT);
      fd.top = new FormAttachment(checkUseCustomSnmpVersion, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      customSnmpVersion.setLayoutData(fd);

      sampleCount = new LabeledSpinner(groupData, SWT.NONE);
      sampleCount.setLabel(i18n.tr("Sample count for average value calculation (0 to disable)"));
      sampleCount.setRange(0, 65535);
      sampleCount.setSelection(dci.getSampleCount());
      sampleCount.setEnabled(dci.getOrigin() == DataOrigin.WINPERF);
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(snmpRawType, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      sampleCount.setLayoutData(fd);
      
      sourceNode = new ObjectSelector(groupData, SWT.NONE, true);
      sourceNode.setLabel(i18n.tr("Source node"));
      sourceNode.setObjectClass(Node.class);
      sourceNode.setObjectId(dci.getSourceNode());
      sourceNode.setEnabled(dci.getOrigin() != DataOrigin.PUSH);
      
      fd = new FormData();
      fd.top = new FormAttachment(sampleCount, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(100, 0);
      agentCacheMode = WidgetHelper.createLabeledCombo(groupData, SWT.READ_ONLY, i18n.tr("Agent cache mode"), fd);
      agentCacheMode.add(i18n.tr("Default"));
      agentCacheMode.add(i18n.tr("On"));
      agentCacheMode.add(i18n.tr("Off"));
      agentCacheMode.select(dci.getCacheMode().getValue());
      agentCacheMode.setEnabled((dci.getOrigin() == DataOrigin.AGENT) || (dci.getOrigin() == DataOrigin.SNMP));

      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.top = new FormAttachment(sampleCount, WidgetHelper.OUTER_SPACING, SWT.BOTTOM);
      fd.right = new FormAttachment(agentCacheMode.getParent(), -WidgetHelper.OUTER_SPACING, SWT.LEFT);
      sourceNode.setLayoutData(fd);
      
      /** polling area **/
      Group groupPolling = new Group(dialogArea, SWT.NONE);
      groupPolling.setText(i18n.tr("Polling"));
      FormLayout pollingLayout = new FormLayout();
      pollingLayout.marginHeight = WidgetHelper.OUTER_SPACING;
      pollingLayout.marginWidth = WidgetHelper.OUTER_SPACING;
      groupPolling.setLayout(pollingLayout);
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      groupPolling.setLayoutData(gd);
      
      fd = new FormData();
      fd.left = new FormAttachment(0, 0);
      fd.right = new FormAttachment(50, -WidgetHelper.OUTER_SPACING / 2);
      fd.top = new FormAttachment(0, 0);
      schedulingMode = WidgetHelper.createLabeledCombo(groupPolling, SWT.READ_ONLY, i18n.tr("Polling mode"), fd);
      schedulingMode.add(i18n.tr("Fixed intervals (default)"));
      schedulingMode.add(i18n.tr("Fixed intervals (custom)"));
      schedulingMode.add(i18n.tr("Custom schedule"));
      schedulingMode.select(dci.getPollingScheduleType());
      schedulingMode.setEnabled(dci.getOrigin() != DataOrigin.PUSH);
      schedulingMode.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				pollingInterval.setEnabled(schedulingMode.getSelectionIndex() == 1);
			}
      });
      
      pollingInterval = new LabeledText(groupPolling, SWT.NONE);
      pollingInterval.setLabel(i18n.tr("Polling interval (seconds)"));
      pollingInterval.setText(dci.getPollingInterval());
      pollingInterval.setEnabled((dci.getPollingScheduleType() == DataCollectionObject.POLLING_SCHEDULE_CUSTOM) && (dci.getOrigin() != DataOrigin.PUSH));
      fd = new FormData();
      fd.left = new FormAttachment(50, WidgetHelper.OUTER_SPACING / 2);
      fd.right = new FormAttachment(100, 0);
      fd.top = new FormAttachment(0, 0);
      pollingInterval.setLayoutData(fd);
      
      /** storage **/
      Group groupStorage = new Group(dialogArea, SWT.NONE);
      groupStorage.setText(i18n.tr("Storage"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.horizontalSpan = 2;
      groupStorage.setLayoutData(gd);
      GridLayout storageLayout = new GridLayout();
      storageLayout.numColumns = 2;
      storageLayout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      groupStorage.setLayout(storageLayout);
      
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      retentionMode = WidgetHelper.createLabeledCombo(groupStorage, SWT.READ_ONLY, i18n.tr("Retention mode"), gd);
      retentionMode.add(i18n.tr("Use default retention time"));
      retentionMode.add(i18n.tr("Use custom retention time"));
      retentionMode.add(i18n.tr("Do not save collected data to database"));
      retentionMode.select(dci.getRetentionType());
      retentionMode.addSelectionListener(new SelectionListener() {
         @Override
         public void widgetDefaultSelected(SelectionEvent e)
         {
            widgetSelected(e);
         }

         @Override
         public void widgetSelected(SelectionEvent e)
         {
            int mode = retentionMode.getSelectionIndex();
            retentionTime.setEnabled(mode == 1);
         }
      });
      
      retentionTime = new LabeledText(groupStorage, SWT.NONE);
      retentionTime.setLabel(i18n.tr("Retention time (days)"));
      retentionTime.setText(dci.getRetentionTime());
      retentionTime.setEnabled(dci.getRetentionType() == DataCollectionObject.RETENTION_CUSTOM);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      retentionTime.setLayoutData(gd);
      
      onOriginChange();
      return dialogArea;
	}

   /**
    * Calculate selection index from SNMP version
    * 
    * @param version
    * @return
    */
   private static int indexFromSnmpVersion(SnmpVersion version)
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
            return -1;
	   }
	}

   /**
    * Get SNMP version value from selection index
    * 
    * @param index
    * @return
    */
   private static SnmpVersion indexToSnmpVersion(int index)
   {
      switch(index)
      {
         case 0:
            return SnmpVersion.V1;
         case 1:
            return SnmpVersion.V2C;
         case 2:
            return SnmpVersion.V3;
         default:
            return SnmpVersion.DEFAULT;
      }
   }

	/**
	 * Handler for changing item origin
	 */
	private void onOriginChange()
	{
      DataOrigin dataOrigin = DataOrigin.getByValue(origin.getSelectionIndex());
		sourceNode.setEnabled(dataOrigin != DataOrigin.PUSH);
		schedulingMode.setEnabled((dataOrigin != DataOrigin.PUSH) && (dataOrigin != DataOrigin.MQTT));
		pollingInterval.setEnabled((dataOrigin != DataOrigin.PUSH) && (dataOrigin != DataOrigin.MQTT) && (schedulingMode.getSelectionIndex() == 1));
		checkInterpretRawSnmpValue.setEnabled(dataOrigin == DataOrigin.SNMP);
		snmpRawType.setEnabled((dataOrigin == DataOrigin.SNMP) && checkInterpretRawSnmpValue.getSelection());
		checkUseCustomSnmpPort.setEnabled(dataOrigin == DataOrigin.SNMP);
		customSnmpPort.setEnabled((dataOrigin == DataOrigin.SNMP) && checkUseCustomSnmpPort.getSelection());
      checkUseCustomSnmpVersion.setEnabled(dataOrigin == DataOrigin.SNMP);
      customSnmpVersion.setEnabled((dataOrigin == DataOrigin.SNMP) && checkUseCustomSnmpVersion.getSelection());
		sampleCount.setEnabled(dataOrigin == DataOrigin.WINPERF);
		agentCacheMode.setEnabled((dataOrigin == DataOrigin.AGENT) || (dataOrigin == DataOrigin.SNMP));
		selectButton.setEnabled(
		      (dataOrigin == DataOrigin.AGENT) || 
		      (dataOrigin == DataOrigin.SNMP) || 
		      (dataOrigin == DataOrigin.INTERNAL) || 
		      (dataOrigin == DataOrigin.WINPERF) || 
		      (dataOrigin == DataOrigin.WEB_SERVICE) || 
            (dataOrigin == DataOrigin.DEVICE_DRIVER) || 
		      (dataOrigin == DataOrigin.SCRIPT));
	}
	
	/**
	 * Select parameter
	 */
	private void selectParameter()
	{
		Dialog dlg;
      DataOrigin dataOrigin = DataOrigin.getByValue(origin.getSelectionIndex());
      switch(dataOrigin)
		{
         case INTERNAL:
			   if (sourceNode.getObjectId() != 0)
			      dlg = new SelectInternalParamDlg(getShell(), sourceNode.getObjectId(), false);
			   else
			      dlg = new SelectInternalParamDlg(getShell(), dci.getNodeId(), false);
				break;
         case AGENT:
         case DEVICE_DRIVER:
			   if (sourceNode.getObjectId() != 0)
               dlg = new SelectAgentParamDlg(getShell(), sourceNode.getObjectId(), dataOrigin, false);
			   else
               dlg = new SelectAgentParamDlg(getShell(), dci.getNodeId(), dataOrigin, false);
				break;
         case SNMP:
				SnmpObjectId oid;
				try
				{
					oid = SnmpObjectId.parseSnmpObjectId(parameter.getText());
				}
				catch(SnmpObjectIdFormatException e)
				{
					oid = null;
				}
				if (sourceNode.getObjectId() != 0)
				   dlg = new SelectSnmpParamDlg(getShell(), oid, sourceNode.getObjectId());
				else
				   dlg = new SelectSnmpParamDlg(getShell(), oid, dci.getNodeId());
				break;
         case WINPERF:
			   if (sourceNode.getObjectId() != 0)
			      dlg = new WinPerfCounterSelectionDialog(getShell(), sourceNode.getObjectId());
			   else
			      dlg = new WinPerfCounterSelectionDialog(getShell(), dci.getNodeId());
				break;
         case SCRIPT:
            dlg = new SelectParameterScriptDialog(getShell());
            break;
         case WEB_SERVICE:
            dlg = new SelectWebServiceDlg(getShell(), false);
            break;
			default:
				dlg = null;
				break;
		}
		
		if ((dlg != null) && (dlg.open() == Window.OK))
		{
			IParameterSelectionDialog pd = (IParameterSelectionDialog)dlg;
			description.setText(pd.getParameterDescription());
			parameter.setText(pd.getParameterName());
			dataType.select(getDataTypePosition(pd.getParameterDataType()));
         editor.fireOnSelectItemListeners(DataOrigin.getByValue(origin.getSelectionIndex()), pd.getParameterName(),
               pd.getParameterDescription(), pd.getParameterDataType());
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
   @Override
	protected boolean applyChanges(final boolean isApply)
	{
		dci.setDescription(description.getText().trim());
		dci.setName(parameter.getText().trim());
      dci.setOrigin(DataOrigin.getByValue(origin.getSelectionIndex()));
		dci.setDataType(getDataTypeByPosition(dataType.getSelectionIndex()));
		dci.setSampleCount(sampleCount.getSelection());
		dci.setSourceNode(sourceNode.getObjectId());
		dci.setCacheMode(AgentCacheMode.getByValue(agentCacheMode.getSelectionIndex()));
		dci.setPollingScheduleType(schedulingMode.getSelectionIndex());
		dci.setPollingInterval((schedulingMode.getSelectionIndex() == 1) ? pollingInterval.getText() : null);
		dci.setRetentionType(retentionMode.getSelectionIndex());
		dci.setRetentionTime((retentionMode.getSelectionIndex() == 1) ? retentionTime.getText() : null);
		dci.setSnmpRawValueInOctetString(checkInterpretRawSnmpValue.getSelection());
		dci.setSnmpRawValueType(snmpRawType.getSelectionIndex());
		if (checkUseCustomSnmpPort.getSelection())
		{
			dci.setSnmpPort(customSnmpPort.getSelection());
		}
		else
		{
			dci.setSnmpPort(0);
		}
      if (checkUseCustomSnmpVersion.getSelection())
      {
         dci.setSnmpVersion(indexToSnmpVersion(customSnmpVersion.getSelectionIndex()));
      }
      else
      {
         dci.setSnmpVersion(SnmpVersion.DEFAULT);
      }
		
		editor.modify();
		return true;
	}

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
    */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		
		NXCSession session = Registry.getSession();
		
		schedulingMode.select(0);
		pollingInterval.setText(Integer.toString(session.getDefaultDciPollingInterval()));
		retentionMode.select(0);
		retentionTime.setText(Integer.toString(session.getDefaultDciRetentionTime()));
		checkInterpretRawSnmpValue.setSelection(false);
		checkUseCustomSnmpPort.setSelection(false);
		customSnmpPort.setSelection(161);
	}

	/**
	 * Get selector position for given data type
	 * 
	 * @param type data type
	 * @return corresponding selector's position
	 */
	private static int getDataTypePosition(DataType type)
	{
	   switch(type)
	   {
	      case COUNTER32:
	         return 2;
	      case COUNTER64:
	         return 5;
	      case FLOAT:
	         return 6;
	      case INT32:
	         return 0;
	      case INT64:
	         return 3;
	      case STRING:
	         return 7;
	      case UINT32:
	         return 1;
	      case UINT64:
	         return 4;
         default:
            return 0;  // fallback to int32
	   }
	}
	
   /**
    * Data type positions in selector
    */
   private static final DataType[] TYPES = { 
      DataType.INT32, DataType.UINT32, DataType.COUNTER32, DataType.INT64,
      DataType.UINT64, DataType.COUNTER64, DataType.FLOAT, DataType.STRING
      };
   
	/**
	 * Get data type by selector position
	 *  
	 * @param position selector position
	 * @return corresponding data type
	 */
	private static DataType getDataTypeByPosition(int position)
	{
      return TYPES[position];
	}
}
