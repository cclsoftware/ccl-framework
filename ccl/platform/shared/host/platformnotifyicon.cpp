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
// Filename    : ccl/platform/shared/host/platformnotifyicon.cpp
// Description : Platform Notification Icon
//
//************************************************************************************************

#include "ccl/gui/system/notifyicon.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/public/cclversion.h"

#include "ccl/platform/shared/interfaces/platformnotifyicon.h"

#include "ccl/platform/shared/host/iplatformintegrationloader.h"

using namespace CCL;

//************************************************************************************************
// PlatformNotifyIcon
//************************************************************************************************

class PlatformNotifyIcon: public NotifyIcon
{
public:
	DECLARE_CLASS (PlatformNotifyIcon, NotifyIcon)
	
	PlatformNotifyIcon ();
	~PlatformNotifyIcon ();

protected:
	PlatformIntegration::PlatformImplementationPtr<PlatformIntegration::IPlatformNotifyIcon> platformIcon;	
	
	// NotifyIcon
	void updateVisible (bool state) override;
	void updateTitle () override;
	void updateImage () override;
	void showInfo (const Alert::Event& e) override;
};

//************************************************************************************************
// PlatformNotifyIcon
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (PlatformNotifyIcon, NotifyIcon, "NotifyIcon")
DEFINE_CLASS_UID (PlatformNotifyIcon, 0x6d51b752, 0xb1c9, 0x44c2, 0xb5, 0xb4, 0x88, 0x6c, 0x61, 0x10, 0xc, 0xe4)

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformNotifyIcon::PlatformNotifyIcon ()
: platformIcon (CCLGUI_PACKAGE_ID)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

PlatformNotifyIcon::~PlatformNotifyIcon ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformNotifyIcon::updateVisible (bool state)
{
	if(state && platformIcon == nullptr)
		platformIcon.load ();
	if(platformIcon)
		platformIcon->setVisible (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformNotifyIcon::updateTitle ()
{
	if(platformIcon)
		platformIcon->setTitle (MutableCString (title, Text::kUTF8));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformNotifyIcon::updateImage ()
{
	if(platformIcon == nullptr)
		return;
	
	if(image)
	{
		Point sizeInPoint (32, 32);
		AutoPtr<Bitmap> bitmap = NEW Bitmap (sizeInPoint.x, sizeInPoint.y, Bitmap::kRGBAlpha, 1);
		
		BitmapGraphicsDevice device (bitmap);
		if(device.isNullDevice ()) // give caller a chance for additional error handling
			return;
		ImageResolutionSelector::draw (device, image, Rect (sizeInPoint));
		
		BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
		if(locker.result == kResultOk)
			platformIcon->setIcon (locker.data.scan0, locker.data.width, locker.data.height, locker.data.rowBytes);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PlatformNotifyIcon::showInfo (const Alert::Event& e)
{
	if(platformIcon == nullptr)
		return;
	
	int alertType = -1;
	switch(e.type)
	{
	case Alert::kInformation :
		alertType = PlatformIntegration::IPlatformNotifyIcon::kInformation;
		break;
	case Alert::kWarning :
		alertType = PlatformIntegration::IPlatformNotifyIcon::kWarning;
		break;
	case Alert::kError :
		alertType = PlatformIntegration::IPlatformNotifyIcon::kError;
		break;
	}
	
	platformIcon->showMessage (alertType, MutableCString (e.message, Text::kUTF8));
}

