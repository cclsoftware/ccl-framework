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
// Filename    : ccl/platform/linux/gui/dbusapplication.h
// Description : Application DBus Service
//
//************************************************************************************************

#ifndef _ccl_dbusapplication_h
#define _ccl_dbusapplication_h

#include "ccl/platform/linux/gui/dbus.h"

#include "dev-ccl-application-server.h"
#include "dev-ccl-application-client.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// DBusApplication
//************************************************************************************************

class DBusApplication: public DBusAdapter<dev::ccl::Application_adaptor>
{
public:
	DBusApplication (IDBusSupport& dbusSupport, const std::string& applicationId);

	static constexpr CStringPtr kObjectPath = "/dev/ccl/application";
	
	// Application_adaptor
	void ProcessCommandLine (const std::vector<std::string>& args) override;
	void Activate (const std::string& tokenString) override;
	std::string ApplicationID () override { return applicationId; }

private:
	std::string applicationId;
};

//************************************************************************************************
// DBusApplicationClient
//************************************************************************************************

class DBusApplicationClient: public DBusProxy<dev::ccl::Application_proxy>
{
public:
	DBusApplicationClient (IDBusSupport& dbusSupport, const std::string& applicationId);

	bool sendCommandLine (ArgsRef arguments);
	bool activate (CStringPtr tokenString);
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_dbusapplication_h
