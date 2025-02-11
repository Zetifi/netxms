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
package org.netxms.nxmc.modules.datacollection.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.preference.ColorSelector;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.datacollection.widgets.DciSelector;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Data source editing dialog
 */
public class DataSourceEditDlg extends Dialog
{
   private static final I18n i18n = LocalizationHelper.getI18n(DataSourceEditDlg.class);

	private ChartDciConfig dci;
	private DciSelector dciSelector;
	private LabeledText name;
   private LabeledText displayFormat;
   private LabeledText dciName;
   private LabeledText dciDescription;
	private Button colorAuto;
	private Button colorCustom;
	private ColorSelector colorSelector;
	private Combo displayType;
	private Button checkShowThresholds;
	private Button checkInvertValues;
   private Button checkRawValues;
   private Button checkMultipeMatch;
	private LabeledText instance;
	private LabeledText dataColumn;
	private boolean graphIsTemplate;
	
	/**
	 * @param parentShell
	 * @param dci
	 */
	public DataSourceEditDlg(Shell parentShell, ChartDciConfig dci, boolean graphIsTemplate)
	{
		super(parentShell);
		this.dci = dci;
		this.graphIsTemplate = graphIsTemplate;
	}

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
      newShell.setText(i18n.tr("Edit Data Source"));
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
		layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);

      GridData gd = new GridData();
      if (!graphIsTemplate)
      {
   		dciSelector = new DciSelector(dialogArea, SWT.NONE, false);
         dciSelector.setLabel(i18n.tr("Data collection item"));
   		dciSelector.setDciId(dci.nodeId, dci.dciId);
   		dciSelector.setDcObjectType(dci.type);
   		gd.horizontalAlignment = SWT.FILL;
   		gd.grabExcessHorizontalSpace = true;
   		gd.widthHint = 400;
   		gd.horizontalSpan = 2;
   		dciSelector.setLayoutData(gd);
      }
		
		name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel(i18n.tr("Display name"));
		name.setText(dci.name);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		name.setLayoutData(gd);

      displayFormat = new LabeledText(dialogArea, SWT.NONE);
      displayFormat.setLabel(i18n.tr("Display format"));
      displayFormat.setText(dci.displayFormat);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      displayFormat.setLayoutData(gd);

      if (graphIsTemplate)
		{
		   dciName = new LabeledText(dialogArea, SWT.NONE);
		   dciName.setLabel("DCI Name");
		   dciName.setText(dci.dciName);
	      gd = new GridData();
	      gd.horizontalAlignment = SWT.FILL;
	      gd.grabExcessHorizontalSpace = true;
	      gd.horizontalSpan = 2;
	      dciName.setLayoutData(gd);	      

	      dciDescription = new LabeledText(dialogArea, SWT.NONE);
	      dciDescription.setLabel("DCI Description");
	      dciDescription.setText(dci.dciDescription);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         dciDescription.setLayoutData(gd);
		}

		if (dci.type == ChartDciConfig.TABLE)
		{
			Group tableGroup = new Group(dialogArea, SWT.NONE);
         tableGroup.setText(i18n.tr("Table Cell"));
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			gd.horizontalSpan = 2;
			tableGroup.setLayoutData(gd);
			
			layout = new GridLayout();
			tableGroup.setLayout(layout);
			
			dataColumn = new LabeledText(tableGroup, SWT.NONE);
         dataColumn.setLabel(i18n.tr("Data column"));
			dataColumn.setText(dci.column);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			dataColumn.setLayoutData(gd);
			
			instance = new LabeledText(tableGroup, SWT.NONE);
         instance.setLabel(i18n.tr("Instance"));
			instance.setText(dci.instance);
			gd = new GridData();
			gd.horizontalAlignment = SWT.FILL;
			gd.grabExcessHorizontalSpace = true;
			instance.setLayoutData(gd);
		}

      /*** Display type ***/
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      displayType = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY | SWT.BORDER, i18n.tr("Display type"), gd);
      displayType.add("Default");
      displayType.add("Line");
      displayType.add("Area");
      displayType.select(dci.getLineChartType() + 1);

      /*** Options group ***/
      Group optionsGroup = new Group(dialogArea, SWT.NONE);
      optionsGroup.setText(i18n.tr("Options"));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 2;
      optionsGroup.setLayoutData(gd);
      
      layout = new GridLayout();
      optionsGroup.setLayout(layout);

      checkShowThresholds = new Button(optionsGroup, SWT.CHECK);
      checkShowThresholds.setText(i18n.tr("Show &thresholds"));
      checkShowThresholds.setSelection(dci.showThresholds);
      
      checkInvertValues = new Button(optionsGroup, SWT.CHECK);
      checkInvertValues.setText(i18n.tr("Invert values"));
      checkInvertValues.setSelection(dci.invertValues);
      
      checkRawValues = new Button(optionsGroup, SWT.CHECK);
      checkRawValues.setText(i18n.tr("Raw values"));
      checkRawValues.setSelection(dci.useRawValues);
      
      if (graphIsTemplate)
      {
         checkMultipeMatch = new Button(optionsGroup, SWT.CHECK);
         checkMultipeMatch.setText(i18n.tr("Multiple match"));
         checkMultipeMatch.setSelection(dci.multiMatch);         
      }

      /*** Color group ***/
		Group colorGroup = new Group(dialogArea, SWT.NONE);
      colorGroup.setText(i18n.tr("Color"));
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		colorGroup.setLayoutData(gd);
		
		layout = new GridLayout();
		layout.numColumns = 2;
		colorGroup.setLayout(layout);
		
		colorAuto = new Button(colorGroup, SWT.RADIO);
      colorAuto.setText(i18n.tr("&Automatic color"));
		colorAuto.setSelection(dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		gd = new GridData();
		gd.horizontalSpan = 2;
		colorAuto.setLayoutData(gd);
		colorAuto.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				colorSelector.setEnabled(false);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		colorCustom = new Button(colorGroup, SWT.RADIO);
      colorCustom.setText(i18n.tr("&Custom color:"));
		colorCustom.setSelection(!dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		colorCustom.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				colorSelector.setEnabled(true);
			}
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});
		
		colorSelector = new ColorSelector(colorGroup);
		colorSelector.setColorValue(ColorConverter.rgbFromInt(dci.getColorAsInt()));
		gd = new GridData();
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		colorSelector.getButton().setLayoutData(gd);
		colorSelector.setEnabled(!dci.color.equalsIgnoreCase(ChartDciConfig.UNSET_COLOR));
		
		return dialogArea;
	}

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
	@Override
	protected void okPressed()
	{
      if (graphIsTemplate)
      {
         dci.dciName = dciName.getText();
         dci.dciDescription = dciDescription.getText();
         dci.multiMatch = checkMultipeMatch.getSelection();
      }
      else
      {
   		dci.nodeId = dciSelector.getNodeId();
   		dci.dciId = dciSelector.getDciId();
      }
		dci.name = name.getText();
		dci.displayFormat = displayFormat.getText();
		if (colorAuto.getSelection())
		{
			dci.color = ChartDciConfig.UNSET_COLOR;
		}
		else
		{
			dci.color = "0x" + Integer.toHexString(ColorConverter.rgbToInt(colorSelector.getColorValue())); //$NON-NLS-1$
		}
      dci.lineChartType = displayType.getSelectionIndex() - 1;
		dci.showThresholds = checkShowThresholds.getSelection();
		dci.invertValues = checkInvertValues.getSelection();
      dci.useRawValues = checkRawValues.getSelection();
		if (dci.type == ChartDciConfig.TABLE)
		{
			dci.column = dataColumn.getText().trim();
			dci.instance = instance.getText();
		}
		super.okPressed();
	}
}
