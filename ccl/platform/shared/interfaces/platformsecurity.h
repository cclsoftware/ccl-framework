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
// Filename    : ccl/platform/shared/interfaces/platformsecurity.h
// Description : CCL Security Platform Integration
//
//************************************************************************************************

#ifndef _ccl_platformsecurity_h
#define _ccl_platformsecurity_h

#include "ccl/public/base/platform.h"

#include "core/public/coreplugin.h"
#include "core/public/corestream.h"

namespace CCL {
namespace PlatformIntegration {

//************************************************************************************************
// IPlatformCredentialStore
//************************************************************************************************

struct IPlatformCredentialStore: Core::IPropertyHandler
{
	virtual tbool unlock (tbool silent = false) = 0;
	virtual tbool setCredentials (CStringPtr targetName, CStringPtr userName, void* data, uint32 dataLength) = 0;
	virtual tbool getCredentials (Core::IO::IByteStream& username, Core::IO::IByteStream& data, CStringPtr targetName) = 0;
	virtual tbool removeCredentials (CStringPtr targetName) = 0;
	
	static const Core::InterfaceID kIID = FOUR_CHAR_ID ('C','r','S','t');
};

} // namespace PlatformIntegration
} // namespace CCL

#endif // _ccl_platformsecurity_h
