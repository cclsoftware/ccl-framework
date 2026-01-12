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
// Filename    : ccl/platform/linux/cclgui-xdgportal/xdgportal.cpp
// Description : CCL GUI Integration using XDG Portal
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/interfaces/platformfileselector.h"
#include "ccl/platform/shared/interfaces/platformnotifyicon.h"
#include "ccl/platform/shared/interfaces/platformintegration.h"
#include "ccl/platform/linux/platformintegration/guiintegration.h"
#include "ccl/platform/linux/platformintegration/dbusintegration.h"
#include "ccl/platform/linux/shared/xdgportalrequest.h"
#include "ccl/platform/linux/linuxplatform.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/singleton.h"

#include "ccl/public/text/cclstdstring.h"

#include "version.h"

#include "org-freedesktop-portal-filechooser-client.h"
#include "org-freedesktop-portal-notification-client.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// XdgPortalUI
//************************************************************************************************

class XdgPortalUI: public PlatformGUIBase,
				   public StaticSingleton<XdgPortalUI>
{
public:
	// PlatformGUIBase
	void onIdle () override {}
};

//************************************************************************************************
// XdgPortalFileSelector
//************************************************************************************************

class XdgPortalFileSelector: public IPlatformFileSelector,
							 public Linux::IXdgPortalResponseHandler,
							 public DBusProxy<org::freedesktop::portal::FileChooser_proxy>
{	
public:
	XdgPortalFileSelector (IDBusSupport& dbusSupport);
	
	// IPlatformFileSelector
	void addFilter (CStringPtr description, CStringPtr filter) override;
	tbool open (IPlatformFileSelectorObserver& observer, int mode, int fileMode, CStringPtr title, CStringPtr defaultSuffix = "", CStringPtr initialDirectory = "", CStringPtr initialFileName = "") override;
	tbool close () override;
	void setParent (void* nativeWindowHandle) override;
	void setProperty (const Core::Property& value) override;
	void getProperty (Core::Property& value) override;
	void release () override;

	// IXdgPortalResponseHandler
	void onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results) override;
	
private:
	IPlatformFileSelectorObserver* observer;
	std::string parentWindowId;
	AutoPtr<Linux::XdgPortalRequest> request;
	std::vector<sdbus::Struct<std::string, std::vector<sdbus::Struct<uint32_t, std::string>>>> filters;
};

//************************************************************************************************
// XdgPortalNotification
//************************************************************************************************

class XdgPortalNotification: public IPlatformNotifyIcon,
							 public DBusProxy<org::freedesktop::portal::Notification_proxy>
{
public:
	XdgPortalNotification (IDBusSupport& dbusSupport);
	
	// IPlatformNotifyIcon
	void setVisible (tbool state) override;
	void setTitle (CStringPtr title) override;
	void setIcon (void* bits, int width, int height, uint32 rowBytes) override;
	void showMessage (int alertType, CStringPtr message) override;
	void setProperty (const Core::Property& value) override;
	void getProperty (Core::Property& value) override;
	void release () override;

	// Notification_proxy
	void onActionInvoked (const std::string& id, const std::string& action, const std::vector<sdbus::Variant>& parameter) override {}

protected:
	std::map<std::string, sdbus::Variant> notificationInfo;
};

} // namespace PlatformIntegration
} // namespace CCL

using namespace CCL;
using namespace Linux;
using namespace PlatformIntegration;
using namespace Core;

//************************************************************************************************
// XdgPortalFileSelector
//************************************************************************************************

XdgPortalFileSelector::XdgPortalFileSelector (IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, XdgPortalRequest::kDestination, XdgPortalRequest::kObjectPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::addFilter (CStringPtr description, CStringPtr filterString)
{
	std::vector<sdbus::Struct<uint32_t, std::string>> filterContent;
	ForEachCStringToken (filterString, ";", pattern)
		filterContent.push_back ({ 0, pattern });
	EndFor
	if(!filterContent.empty ())
		filters.push_back ({ description, filterContent });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool XdgPortalFileSelector::open (IPlatformFileSelectorObserver& newObserver, int mode, int fileMode, CStringPtr title, CStringPtr defaultSuffix, CStringPtr initialDirectory, CStringPtr initialFileName)
{
	observer = &newObserver;
	
	std::map<std::string, sdbus::Variant> options;
	options["filters"] = filters;
	options["modal"] = true;
	
	if(!CString (defaultSuffix).isEmpty ())
	{
		for(const auto& filter : filters)
		{
			for(const auto& glob : std::get<1> (filter))
			{
				if(CString (std::get<1> (glob).c_str ()).endsWith (defaultSuffix))
				{
					options["current_filter"] = filter;
					break;
				}
			}
		}
	}

	if(!CString (initialDirectory).isEmpty ())
	{
		options["current_folder"] = initialDirectory;
		Url selectedUrl;
		selectedUrl.fromDisplayString (initialDirectory);
		if(!CString (initialFileName).isEmpty ())
			selectedUrl.descend (initialFileName, IUrl::kFile);
		options["uris"] = { MutableCString (UrlFullString (selectedUrl), Text::kUTF8).str () };
	}

	switch(fileMode)
	{
	case kFile :
		options["directory"] = false;
		options["multiple"] = false;
		break;
	case kMultipleFiles :
		options["directory"] = false;
		options["multiple"] = true;
		break;
	case kDirectory :
		options["directory"] = true;
		options["multiple"] = false;
		break;
	}

	sdbus::ObjectPath result;
	switch(mode)
	{
	case IPlatformFileSelector::kOpen :
		try
		{
			result = OpenFile (parentWindowId, title, options);
		}
		CATCH_DBUS_ERROR
		break;
	case IPlatformFileSelector::kSave :
		try
		{
			result = SaveFile (parentWindowId, title, options);
		}
		CATCH_DBUS_ERROR
		break;
	}

	if(result.empty ())
		return false;

	request = NEW XdgPortalRequest (dbusSupport, *this, result);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool XdgPortalFileSelector::close ()
{
	if(!request.isValid ())
		return false;
	request->close ();
	request.release ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results)
{
	if(observer)
	{
		if(response == XdgPortalRequest::kSuccess && results.find ("uris") != results.end ())
		{
			const sdbus::Variant& data = results.at ("uris");
			std::vector<std::string> uris = data.get<std::vector<std::string>> ();
			for(const std::string& uri : uris)
			{
				Url resultUrl;
				UrlUtils::fromEncodedString (resultUrl, fromStdString (uri));
				observer->addResult (MutableCString (UrlDisplayString (resultUrl), Text::kUTF8));
			}
		}
		observer->closed ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::setParent (void* nativeWindowHandle)
{
	NativeWindowHandle* handle = static_cast<NativeWindowHandle*> (nativeWindowHandle);
	if(!CString (handle->exportedHandle).isEmpty ())
	{
		parentWindowId = "wayland:";
		parentWindowId.append (handle->exportedHandle);
	}
	else if(!CString (handle->exportedHandleV1).isEmpty ())
	{
		parentWindowId = "wayland:";
		parentWindowId.append (handle->exportedHandleV1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::setProperty (const Core::Property& value)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::getProperty (Core::Property& value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalFileSelector::release ()
{
	delete this;
}

//************************************************************************************************
// XdgPortalNotification
//************************************************************************************************

XdgPortalNotification::XdgPortalNotification (IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, XdgPortalRequest::kDestination, XdgPortalRequest::kObjectPath)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::setVisible (tbool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::setTitle (CStringPtr title)
{
	notificationInfo["title"] = title;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::setIcon (void* bits, int width, int height, uint32 rowBytes)
{
	// TODO
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::showMessage (int alertType, CStringPtr message)
{
	switch(alertType)
	{
	case kInformation :
		notificationInfo["priority"] = "normal";
		break;
	case kWarning :
		notificationInfo["priority"] = "high";
		break;
	case kError :
		notificationInfo["priority"] = "urgent";
		break;
	}
	notificationInfo["body"] = message;

	MutableCString id;
	UID uid;
	uid.generate ();
	uid.toCString (id);

	try
	{
		AddNotification (id.str (), notificationInfo);
	}
	CATCH_DBUS_ERROR
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::setProperty (const Property& value)
{}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::getProperty (Property& value)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPortalNotification::release ()
{
	delete this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

typedef PlatformGUIFactory<XdgPortalUI> XdgPortalUIFactory;
typedef GUIClassFactory<XdgPortalUI, DBusClassFactory<XdgPortalFileSelector, IPlatformFileSelector>> XdgPortalFileSelectorFactory;
typedef GUIClassFactory<XdgPortalUI, DBusClassFactory<XdgPortalNotification, IPlatformNotifyIcon>> XdgPortalNotificationFactory;

#define XDG_PORTAL_ENVIRONMENT DBUS_ENVIRONMENT ";XDG_CURRENT_DESKTOP"
DEFINE_PLATFORMINTEGRATION_CLASS2 (XdgPortalUIClass, "XdgPortalUI", "{1f71d5d6-3f82-4247-8b48-f92ad7fac9a5}", DEFINE_PLATFORMINTEGRATION_ATTRIBUTES (XDG_PORTAL_ENVIRONMENT, ""), XdgPortalUIFactory::createInstance)
DEFINE_PLATFORMINTEGRATION_CLASS2 (XdgPortalFileSelectorClass, "XdgPortalFileSelector", "{73f4cbd5-ed32-40fc-b344-f7c77dacbcdf}", DEFINE_PLATFORMINTEGRATION_ATTRIBUTES (XDG_PORTAL_ENVIRONMENT, ""), XdgPortalFileSelectorFactory::createInstance)
DEFINE_PLATFORMINTEGRATION_CLASS2 (XdPortalNotificationClass, "XdgPortalNotification", "{d559deca-9c35-40eb-9ba4-4473104cc090}", DEFINE_PLATFORMINTEGRATION_ATTRIBUTES (XDG_PORTAL_ENVIRONMENT, ""), XdgPortalNotificationFactory::createInstance)

BEGIN_CORE_CLASSINFO_BUNDLE (DEFINE_CORE_VERSIONINFO (PLUG_NAME, PLUG_COMPANY, PLUG_VERSION, PLUG_COPYRIGHT, PLUG_WEBSITE))
	ADD_CORE_CLASSINFO (XdgPortalUIClass),
	ADD_CORE_CLASSINFO (XdgPortalFileSelectorClass),
	ADD_CORE_CLASSINFO (XdPortalNotificationClass)
END_CORE_CLASSINFO_BUNDLE
