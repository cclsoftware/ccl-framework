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
// Filename    : ccl/platform/linux/shared/xdgportalrequest.h
// Description : XDG Portal Request
//
//************************************************************************************************

#ifndef _ccl_linux_xdgportalrequest_h
#define _ccl_linux_xdgportalrequest_h

#include "ccl/platform/linux/interfaces/idbussupport.h"

#include "ccl/base/object.h"

#include "org-freedesktop-portal-request-client.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// IXdgPortalResponseHandler
//************************************************************************************************

interface IXdgPortalResponseHandler
{
public:
	virtual void onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results) = 0;
};

//************************************************************************************************
// XdgPortalRequest
//************************************************************************************************

class XdgPortalRequest: public Object,
						public DBusProxy<org::freedesktop::portal::Request_proxy>
{
public:
	XdgPortalRequest (IDBusSupport& dbusSupport, IXdgPortalResponseHandler& handler, const sdbus::ObjectPath& objectPath)
	: DBusProxy (dbusSupport, kDestination, objectPath),
	  handler (handler),
	  responseCount (0)
	{}

	static constexpr CStringPtr kDestination = "org.freedesktop.portal.Desktop";
	static constexpr CStringPtr kObjectPath = "/org/freedesktop/portal/desktop";

	PROPERTY_VARIABLE (int, responseCount, ResponseCount)

	enum Response
	{
		kSuccess = 0,
		kCanceled = 1,
		kUnknown = 2
	};

	void close ()
	{
		try
		{
			Close ();
		}
		CATCH_DBUS_ERROR
	}

	bool receivedResponse () const
	{
		return responseCount > 0;
	}

	// Request_proxy
	void onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results) override
	{
		handler.onResponse (response, results);
		responseCount++;
	}

private:
	IXdgPortalResponseHandler& handler;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_xdgportalrequest_h
