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
// Filename    : ccl/platform/shared/interfaces/platformintegration.h
// Description : Platform Integration Definitions
//
//************************************************************************************************

#ifndef _platformintegration_h
#define _platformintegration_h

#include "core/public/coreplugin.h"

//************************************************************************************************
// Class Definitions
//************************************************************************************************

#define CLASS_TYPE_PLATFORMINTEGRATION "PlatformIntegration"

#define DEFINE_PLATFORMINTEGRATION_CLASS(VarName, displayName, classID, createInstance) \
	DEFINE_CORE_CLASSINFO (VarName, 0, CLASS_TYPE_PLATFORMINTEGRATION, displayName, classID, "", createInstance)

#define DEFINE_PLATFORMINTEGRATION_CLASS2(VarName, displayName, classID, classAttr, createInstance) \
	DEFINE_CORE_CLASSINFO (VarName, 0, CLASS_TYPE_PLATFORMINTEGRATION, displayName, classID, classAttr, createInstance)

#define PLATFORMINTEGRATION_ENVIRONMENT "platformIntegrationEnvironment"
#define PLATFORMINTEGRATION_DEPENDENCIES "platformIntegrationDepends"
	
#define DEFINE_PLATFORMINTEGRATION_ATTRIBUTES(environment, depends) \
	PLATFORMINTEGRATION_ENVIRONMENT "=" environment "\n" \
	PLATFORMINTEGRATION_DEPENDENCIES "=" depends

#endif // _platformintegration_h
