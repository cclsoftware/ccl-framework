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
// Filename    : ccl/platform/shared/interfaces/platformalert.h
// Description : CCL GUI Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformalert_h
#define _ccl_platformalert_h

#include "ccl/platform/shared/interfaces/platformdialog.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformAlertObserver
//************************************************************************************************

struct IPlatformAlertObserver: IPlatformDialogObserver
{};

//************************************************************************************************
// IPlatformAlert
//************************************************************************************************

struct IPlatformAlert: Core::IPropertyHandler,
					   IPlatformDialog
{
    enum AlertType
    {
        kUndefined = -1,
        kInfo,
        kWarning,
        kError
    };
    
	virtual tbool open (IPlatformAlertObserver& observer, CStringPtr title, CStringPtr text, int alertType = kUndefined, CStringPtr firstButton = nullptr, CStringPtr secondButton = nullptr, CStringPtr thirdButton = nullptr) = 0;
    virtual tbool close () = 0;
    
	static const Core::InterfaceID kIID = FOUR_CHAR_ID ('A','l','r','t');
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformalert_h
