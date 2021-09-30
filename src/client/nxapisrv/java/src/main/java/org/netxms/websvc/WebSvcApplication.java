/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
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
package org.netxms.websvc;

import java.util.Arrays;
import java.util.HashSet;
import org.netxms.websvc.handlers.AccessIntegrationTools;
import org.netxms.websvc.handlers.Alarms;
import org.netxms.websvc.handlers.BindHandler;
import org.netxms.websvc.handlers.BindToHandler;
import org.netxms.websvc.handlers.DCObjectLastValue;
import org.netxms.websvc.handlers.DataCollectionConfigurationHandler;
import org.netxms.websvc.handlers.DataCollectionObjectHandler;
import org.netxms.websvc.handlers.GrafanaAlarms;
import org.netxms.websvc.handlers.GrafanaDataCollection;
import org.netxms.websvc.handlers.HistoricalData;
import org.netxms.websvc.handlers.InfoHandler;
import org.netxms.websvc.handlers.LastValues;
import org.netxms.websvc.handlers.NotificationHandler;
import org.netxms.websvc.handlers.ObjectToolOutputHandler;
import org.netxms.websvc.handlers.ObjectTools;
import org.netxms.websvc.handlers.Objects;
import org.netxms.websvc.handlers.PredefinedGraphs;
import org.netxms.websvc.handlers.PushDataHandler;
import org.netxms.websvc.handlers.RootHandler;
import org.netxms.websvc.handlers.Sessions;
import org.netxms.websvc.handlers.SummaryTableAdHoc;
import org.netxms.websvc.handlers.UnbindFromHandler;
import org.netxms.websvc.handlers.UnbindHandler;
import org.netxms.websvc.handlers.UserAgentNotifications;
import org.netxms.websvc.handlers.UserPassword;
import org.netxms.websvc.handlers.Users;
import org.netxms.websvc.handlers.Events;
import org.restlet.Application;
import org.restlet.Restlet;
import org.restlet.data.Method;
import org.restlet.routing.Router;
import org.restlet.service.CorsService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * NetXMS web service API application
 */
public class WebSvcApplication extends Application
{
   private Logger log = LoggerFactory.getLogger(WebSvcApplication.class);
   
   /**
    * Default constructor
    */
   public WebSvcApplication()
   {
      CorsService corsService = new CorsService();
      corsService.setAllowedOrigins(new HashSet<String>(Arrays.asList("*")));
      corsService.setAllowedCredentials(true);
      corsService.setAllowedHeaders(new HashSet<String>(Arrays.asList("Content-Type", "Authorization")));
      corsService.setDefaultAllowedMethods(new HashSet<Method>(Arrays.asList(Method.DELETE, Method.GET, Method.OPTIONS, Method.POST, Method.PUT)));
      getServices().add(corsService);
      WebSvcStatusService statusService = new WebSvcStatusService();
      statusService.setOverwriting(true);
      setStatusService(statusService);
   }

   /**
    * @see org.restlet.Application#createInboundRoot()
    */
   @Override
   public synchronized Restlet createInboundRoot()
   {
      log.debug("createInboundRoot() called");
      Router router = new Router(getContext());
      router.attach("/", RootHandler.class);
      router.attach("/alarms", Alarms.class);
      router.attach("/alarms/{id}", Alarms.class);
      router.attach("/authenticate", AccessIntegrationTools.class);
      router.attach("/grafana/alarms", GrafanaAlarms.class);
      router.attach("/grafana/datacollection", GrafanaDataCollection.class);
      router.attach("/info", InfoHandler.class);
      router.attach("/notifications", NotificationHandler.class);
      router.attach("/objects", Objects.class);
      router.attach("/objects/{object-id}", Objects.class);
      router.attach("/objects/{object-id}/bind", BindHandler.class);
      router.attach("/objects/{object-id}/bindTo", BindToHandler.class);
      router.attach("/objects/{object-id}/lastvalues", LastValues.class);
      router.attach("/objects/{object-id}/datacollection", DataCollectionConfigurationHandler.class);
      router.attach("/objects/{object-id}/datacollection/{id}", DataCollectionObjectHandler.class);
      router.attach("/objects/{object-id}/datacollection/{id}/values", HistoricalData.class);
      router.attach("/objects/{object-id}/datacollection/{id}/lastvalue", DCObjectLastValue.class);
      router.attach("/objects/{object-id}/objecttools", ObjectTools.class);
      router.attach("/objects/{object-id}/objecttools/output/{id}", ObjectToolOutputHandler.class);
      router.attach("/objects/{object-id}/unbind", UnbindHandler.class);
      router.attach("/objects/{object-id}/unbindFrom", UnbindFromHandler.class);
      router.attach("/predefinedgraphs", PredefinedGraphs.class);
      router.attach("/pushData", PushDataHandler.class);
      router.attach("/sessions", Sessions.class);
      router.attach("/sessions/{id}", Sessions.class);
      router.attach("/summaryTable/adHoc", SummaryTableAdHoc.class);
      router.attach("/userAgentNotifications", UserAgentNotifications.class);
      router.attach("/users", Users.class);
      router.attach("/users/{id}", Users.class);
      router.attach("/users/{id}/password", UserPassword.class);
      router.attach("/events", Events.class);
      return router;
   }
}
