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
// Filename    : ccl/platform/linux/platformintegration/dbusintegration.h
// Description : DBus Platform Integration
//
//************************************************************************************************

#ifndef _ccl_dbusintegration_h
#define _ccl_dbusintegration_h

#include "ccl/platform/linux/interfaces/ilinuxsystem.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace PlatformIntegration {

#define DBUS_ENVIRONMENT "DBUS_SESSION_BUS_ADDRESS;SYSTEMD_EXEC_PID"

//************************************************************************************************
// DBusClassFactory
//************************************************************************************************

template <class Class, class Interface>
struct DBusClassFactory
{
	static void* createInstance (Core::InterfaceID iid = Interface::kIID)
	{
		if(iid == Interface::kIID)
		{
			UnknownPtr<ILinuxSystem> linuxSystem (&System::GetSystem ());
			if(!linuxSystem.isValid ())
				return nullptr;
			IDBusSupport* dbusSupport = linuxSystem->getDBusSupport ();
			if(dbusSupport)
				return static_cast<Interface*> (NEW Class (*dbusSupport));
		}
		return nullptr;
	}
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_dbusintegration_h
