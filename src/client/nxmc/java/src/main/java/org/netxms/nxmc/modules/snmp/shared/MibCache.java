/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.snmp.shared;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Date;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.client.snmp.MibObject;
import org.netxms.client.snmp.MibTree;
import org.netxms.client.snmp.SnmpObjectId;
import org.netxms.client.snmp.SnmpObjectIdFormatException;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * SNMP MIB cache
 */
public final class MibCache 
{	
   private static I18n i18n = LocalizationHelper.getI18n(MibCache.class);
   private static Logger logger = LoggerFactory.getLogger(MibCache.class);
	private static MibTree mibTree = null;

	/**
	 * Init MIB cache
	 * 
	 * @param session
	 * @param display
	 */
	public static void init(final NXCSession session, Display display)
	{
	   /*
	   Boolean slowLink = (Boolean)ConsoleSharedData.getProperty("SlowLink"); //$NON-NLS-1$
	   if ((slowLink != null) && slowLink.booleanValue())
	      return;
	   */
	   
		Job job = new Job(i18n.tr("Load MIB file on startup"), null) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				File targetDir = new File(Registry.getStateDir(), "mibFile");
				if (!targetDir.exists())
				   targetDir.mkdirs();
				File mibFile = new File(targetDir, "netxms.mib"); //$NON-NLS-1$
				
				Date serverMibTimestamp = session.getMibFileTimestamp();
				if (!mibFile.exists() || (serverMibTimestamp.getTime() > mibFile.lastModified()))
				{
					File file = session.downloadMibFile();
					logger.info("MIB file downloaded to: " + file.getPath() + " (size " + file.length() + " bytes)"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$

					if (mibFile.exists())
						mibFile.delete();
					
					if (!file.renameTo(mibFile))
					{
						// Rename failed, try to copy file
						InputStream in = null;
						OutputStream out = null;
						try
						{
							in = new FileInputStream(file);
							out = new FileOutputStream(mibFile);
							byte[] buffer = new byte[16384];
					      int len;
					      while((len = in.read(buffer)) > 0)
					      	out.write(buffer, 0, len);
						}
						catch(Exception e)
						{
							throw e;
						}
						finally
						{
							if (in != null)
								in.close();
							if (out != null)
								out.close();
						}
				      
				      file.delete();
					}
				}
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot load MIB file from server");
			}
		};
		job.setUser(false);
		job.start();
	}

	/**
	 * @return the mibTree
	 */
	public static MibTree getMibTree()
	{
	   if (mibTree != null)
	      return mibTree;
      File targetDir = new File(Registry.getStateDir(), "mibFile");
      File mibFile = new File(targetDir, "netxms.mib"); //$NON-NLS-1$
      if (mibFile.exists())
      {
         try
         {
            mibTree = new MibTree(mibFile);
         }
         catch(Exception e)
         {
            logger.error("Cannot load MIB file", e); //$NON-NLS-1$
         }
      }
		return (mibTree != null) ? mibTree : new MibTree();
	}
	
	/**
	 * Find matching object in tree. If exactMatch set to true, method will search for object with
	 * ID equal to given. If exactMatch set to false, and object with given id cannot be found, closest upper level
	 * object will be returned (i.e., if object .1.3.6.1.5 does not exist in the tree, but .1.3.6.1 does, .1.3.6.1 will
	 * be returned in search for .1.3.6.1.5).
	 * 
	 * @param oid object id to find
	 * @param exactMatch set to true if exact match required
	 * @return MIB object or null if matching object not found
	 */
	public static MibObject findObject(String oid, boolean exactMatch)
	{
	   if (mibTree == null)
	      return null;
	   
		SnmpObjectId id;
		try
		{
			id = SnmpObjectId.parseSnmpObjectId(oid);
		}
		catch(SnmpObjectIdFormatException e)
		{
			return null;
		}
		return mibTree.findObject(id, exactMatch);
	}
}
