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
// Filename    : ccl/platform/shared/interfaces/platformnotifyicon.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformnotifyicon_h
#define _ccl_platformnotifyicon_h

#include "ccl/public/base/platform.h"

#include "core/public/coreplugin.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformNotifyIcon
//************************************************************************************************

struct IPlatformNotifyIcon: Core::IPropertyHandler
{
    enum AlertType
    {
        kInformation,
        kWarning,
        kError
    };
    
    virtual void setVisible (tbool state) = 0;
    virtual void setTitle (CStringPtr title) = 0;
    virtual void setIcon (void* bits, int width, int height, uint32 rowBytes) = 0;
    virtual void showMessage (int alertType, CStringPtr message) = 0;
    
	static const Core::InterfaceID kIID = FOUR_CHAR_ID ('N','t','f','y');
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformnotifyicon_h
