//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/linux/gui/dbusapplication.cpp
// Description : Application DBus Service
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/gui/dbusapplication.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"

#include "ccl/main/cclargs.h"

#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cclstdstring.h"
#include "ccl/public/gui/iapplication.h"

#include <fcntl.h>

namespace CCL {
namespace Linux {

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// DBusApplication
//************************************************************************************************

DBusApplication::DBusApplication (IDBusSupport& dbusSupport, const std::string& applicationId)
: DBusAdapter (dbusSupport, kObjectPath),
  applicationId (applicationId)
{
	try
	{
		getObject ().getConnection ().requestName (applicationId);
	}
	CATCH_DBUS_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DBusApplication::ProcessCommandLine (const std::vector<std::string>& arguments)
{
	IApplication* application = GUI.getApplication ();
	if(application == nullptr)
		return;

	Vector<String> args;
	for(const std::string arg : arguments)
		args.add (fromStdString (arg));

	application->processCommandLine (ArgumentList (args.count (), args));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DBusApplication::Activate (const std::string& tokenString)
{
	CCL_PRINTF ("received activation token %s\n", tokenString.c_str ())
	
	Window* window = Desktop.getLastWindow ();
	if(window == nullptr)
		return;

	LinuxWindow* linuxWindow = LinuxWindow::cast (window);
	linuxWindow->activate (tokenString.c_str ());
}

//************************************************************************************************
// DBusApplicationClient
//************************************************************************************************

DBusApplicationClient::DBusApplicationClient (IDBusSupport& dbusSupport, const std::string& applicationId)
: DBusProxy (dbusSupport, applicationId, DBusApplication::kObjectPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DBusApplicationClient::sendCommandLine (ArgsRef args)
{
	if(args.count () < 2)
		return false;

	std::vector<std::string> arguments;
	for(int i = 0; i < args.count (); i++)
		arguments.push_back (toStdString (args.at (i)));
	
	try
	{
		ProcessCommandLine (arguments);
	}
	CATCH_DBUS_ERROR

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DBusApplicationClient::activate (CStringPtr tokenString)
{
	try
	{
		Activate (tokenString);
	}
	CATCH_DBUS_ERROR

	return true;
}
