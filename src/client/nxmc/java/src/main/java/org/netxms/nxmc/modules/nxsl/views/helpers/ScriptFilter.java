/**
 * NetXMS - open source network management system
 * Copyright (C) 2021 Raden Solutions
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
package org.netxms.nxmc.modules.nxsl.views.helpers;

import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.netxms.client.Script;
import org.netxms.nxmc.base.views.AbstractViewerFilter;

/**
 * Script filtering class
 */
public final class ScriptFilter extends ViewerFilter implements AbstractViewerFilter
{
   private String filterString;

   /**
    * @see org.eclipse.jface.viewers.ViewerFilter#select(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
    */
   @Override
   public boolean select(Viewer viewer, Object parentElement, Object element)
   {
      if ((filterString == null) || (filterString.isEmpty()))
         return true;
      
      final Script script = (Script)element;      
      return script.getName().toLowerCase().contains(filterString);
   }

   /**
    * Set filter script 
    * 
    * @param text filter sctring
    */
   public void setFilterString(String text)
   {
      filterString = text.toLowerCase();
   }

   /**
    * @return the filterString
    */
   public String getFilterString()
   {
      return filterString;
   }
}
