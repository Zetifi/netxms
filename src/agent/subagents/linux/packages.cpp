/*
** NetXMS subagent for GNU/Linux
** Copyright (C) 2013-2021 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**/

#include "linux_subagent.h"
#include <sys/utsname.h>

/**
 * Handler for System.InstalledProducts table
 */
LONG H_InstalledProducts(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   const char *command;
   if (access("/bin/rpm", X_OK) == 0)
   {
		command = "/bin/rpm -qa --queryformat '@@@ #%{NAME}:%{ARCH}|%{VERSION}%|RELEASE?{-%{RELEASE}}:{}||%{VENDOR}|%{INSTALLTIME}|%{URL}|%{SUMMARY}\\n'";
   }
   else if (access("/usr/bin/dpkg-query", X_OK) == 0)
   {
		command = "/usr/bin/dpkg-query -W -f '@@@${Status}#${package}:${Architecture}|${version}|||${homepage}|${description}\\n' | grep '@@@install.*installed.*#'";
	}
	else
	{
		return SYSINFO_RC_UNSUPPORTED;
	}

   struct utsname un;
   const char *arch;
   if (uname(&un) != -1)
   {
      if (!strcmp(un.machine, "i686") || !strcmp(un.machine, "i586") || !strcmp(un.machine, "i486") || !strcmp(un.machine, "i386"))
      {
         arch = ":i686:i586:i486:i386";
      }
      else if (!strcmp(un.machine, "amd64") || !strcmp(un.machine, "x86_64"))
      {
         arch = ":amd64:x86_64";
      }
      else
      {
         memmove(&un.machine[1], un.machine, strlen(un.machine) + 1);
         un.machine[0] = ':';
         arch = un.machine;
      }
   }
   else
   {
      arch = ":i686:i586:i486:i386";
   }

	FILE *pipe = popen(command, "r");
	if (pipe == nullptr)
		return SYSINFO_RC_ERROR;

	value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
	value->addColumn(_T("VERSION"), DCI_DT_STRING, _T("Version"), true);
	value->addColumn(_T("VENDOR"), DCI_DT_STRING, _T("Vendor"));
	value->addColumn(_T("DATE"), DCI_DT_STRING, _T("Install Date"));
	value->addColumn(_T("URL"), DCI_DT_STRING, _T("URL"));
	value->addColumn(_T("DESCRIPTION"), DCI_DT_STRING, _T("Description"));

	while(true)
	{
		char line[1024];
		if (fgets(line, 1024, pipe) == nullptr)
			break;

		if (memcmp(line, "@@@", 3))
			continue;

		value->addRow();
		char *curr = strchr(&line[3], '#');
		if (curr != nullptr)
			curr++;
		else
			curr = &line[3];
		for(int i = 0; i < 6; i++)
		{
			char *ptr = strchr(curr, '|');
			if (ptr != nullptr)
			{
				*ptr = 0;
			}
			else
			{
				ptr = strchr(curr, '\n');
				if (ptr != nullptr)
				{
					*ptr = 0;
					ptr = nullptr;
				}
			}

			// Remove architecture from package name if it is the same as
			// OS architecture or package is architecture-independent
			if (i == 0)
			{
			   char *pa = strrchr(curr, ':');
			   if (pa != nullptr)
			   {
			      if (!strcmp(pa, ":all") || !strcmp(pa, ":noarch") || !strcmp(pa, ":(none)") || (strstr(arch, pa) != nullptr))
			         *pa = 0;
			   }
			}

#ifdef UNICODE
			value->setPreallocated(i, WideStringFromMBString(curr));
#else
			value->set(i, curr);
#endif
			if (ptr == nullptr)
				break;
			curr = ptr + 1;
		}
	}
	pclose(pipe);

	return SYSINFO_RC_SUCCESS;
}
