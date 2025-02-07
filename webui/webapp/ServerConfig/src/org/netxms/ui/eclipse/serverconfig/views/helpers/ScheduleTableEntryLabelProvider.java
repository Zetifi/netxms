package org.netxms.ui.eclipse.serverconfig.views.helpers;

import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.widgets.Display;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.ui.eclipse.console.UserRefreshRunnable;
import org.netxms.ui.eclipse.console.resources.RegionalSettings;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.serverconfig.views.ScheduledTaskView;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

public class ScheduleTableEntryLabelProvider extends LabelProvider implements ITableLabelProvider, IColorProvider
{
   private static final Color COLOR_DISABLED = new Color(Display.getDefault(), new RGB(126, 137, 185));
   private static final Color COLOR_SYSTEM = new Color(Display.getDefault(), new RGB(196, 170, 94));
   
   private static final int EXECUTED = 0;
   private static final int PENDING = 1;
   private static final int DISBALED = 2;
   
   
   private NXCSession session;
   private WorkbenchLabelProvider wbLabelProvider;
   private Image statusImages[];
   private SortableTableViewer viewer;
   
   /**
    * Default constructor 
    * @param viewer 
    */
   public ScheduleTableEntryLabelProvider(SortableTableViewer viewer)
   {
      session = (NXCSession)ConsoleSharedData.getSession();
      wbLabelProvider = new WorkbenchLabelProvider();
      this.viewer = viewer;
            
      statusImages = new Image[3];
      statusImages[EXECUTED] = Activator.getImageDescriptor("icons/active.gif").createImage(); //$NON-NLS-1$
      statusImages[PENDING] = Activator.getImageDescriptor("icons/pending.gif").createImage(); //$NON-NLS-1$
      statusImages[DISBALED] = Activator.getImageDescriptor("icons/inactive.gif").createImage(); //$NON-NLS-1$
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      ScheduledTask task = (ScheduledTask)element;
      
      switch(columnIndex)
      {
         case ScheduledTaskView.SCHEDULE_ID:
            if(task.isDisabled())
               return statusImages[DISBALED];
            if((task.getFlags() & ScheduledTask.EXECUTED) != 0 || (task.getFlags() & ScheduledTask.RUNNING) != 0)
               return statusImages[EXECUTED];
            return statusImages[PENDING];
         case ScheduledTaskView.OBJECT:
            if(task.getObjectId() == 0)
               return null;
            AbstractObject object = session.findObjectById(((ScheduledTask)element).getObjectId());
            return (object != null) ? wbLabelProvider.getImage(object) : null;
      }
      return null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      ScheduledTask task = (ScheduledTask)element;
      
      switch(columnIndex)
      {
         case ScheduledTaskView.SCHEDULE_ID:
            return Long.toString(task.getId());
         case ScheduledTaskView.CALLBACK_ID:
            return task.getTaskHandlerId();
         case ScheduledTaskView.OBJECT:
            if (task.getObjectId() == 0)
               return "";
            AbstractObject object = session.findObjectById(task.getObjectId());
            return (object != null) ? object.getObjectName() : ("[" + Long.toString(task.getObjectId()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ScheduledTaskView.PARAMETERS:
            return task.getParameters();
         case ScheduledTaskView.EXECUTION_TIME:
            return task.getSchedule().isEmpty() ? RegionalSettings.getDateTimeFormat().format(task.getExecutionTime()) : task.getSchedule();
         case ScheduledTaskView.LAST_EXECUTION_TIME:
            return (task.getLastExecutionTime().getTime() == 0) ? "" : RegionalSettings.getDateTimeFormat().format(task.getLastExecutionTime());
         case ScheduledTaskView.STATUS:
            return task.getStatus();
         case ScheduledTaskView.MANAGMENT_STATE:
            return task.isDisabled() ? "Disabled" : "Enabled";
         case ScheduledTaskView.OWNER:
            if ((task.getFlags() & ScheduledTask.SYSTEM) != 0)
               return "system";
            AbstractUserObject user = session.findUserDBObjectById(task.getOwner(), new UserRefreshRunnable(viewer, element));
            return (user != null) ? user.getName() : ("[" + Long.toString(task.getOwner()) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
         case ScheduledTaskView.COMMENTS:
            return task.getComments();
         case ScheduledTaskView.TIMER_KEY:
            return task.getKey();
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if (((ScheduledTask)element).isDisabled())
         return COLOR_DISABLED;
      if (((ScheduledTask)element).isSystem())
         return COLOR_SYSTEM;
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
