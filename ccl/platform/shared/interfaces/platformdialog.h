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
// Filename    : ccl/platform/shared/interfaces/platformdialog.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformdialog_h
#define _ccl_platformdialog_h

#include "ccl/public/base/platform.h"

#include "core/public/coreplugin.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformDialog
//************************************************************************************************

struct IPlatformDialog
{
	virtual void setParent (void* nativeWindowHandle) = 0;
};

//************************************************************************************************
// IPlatformDialogObserver
//************************************************************************************************

struct IPlatformDialogObserver
{
    enum Result
    {
        kFirstButton,
        kSecondButton,
        kThirdButton,
        kCanceled
    };
	
	virtual void opened (void* nativeWindowHandle) = 0;
	virtual void closed (int result = -1) = 0;
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformalert_h
