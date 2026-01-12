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
// Filename    : ccl/public/cclversion.h
// Description : Version Information
//
//************************************************************************************************

#ifndef _ccl_version_h
#define _ccl_version_h

#include "ccl/public/base/platform.h"

#include "core/public/coreversion.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Version
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_VERSION_MAJOR		CORE_VERSION_MAJOR
#define CCL_VERSION_MINOR		CORE_VERSION_MINOR
#define CCL_VERSION_REVISION	CORE_VERSION_REVISION
#define CCL_VERSION_BUILD		CORE_VERSION_BUILD

#define CCL_VERSION_STRING		CORE_VERSION_STRING
#define CCL_BUILD_TIMESTAMP		CORE_BUILD_TIMESTAMP

/**
	CCL ABI (Application Binary Interface) version, increment when binary compatibility is lost. 
	Inbetween changes need to be put at end of vtable and marked with @@FROZEN-CCL-ABI-rXX@@
*/
#define CCL_ABI_VERSION			26

//////////////////////////////////////////////////////////////////////////////////////////////////
// Legal Information
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_SHORT_NAME			"CCL"
#define CCL_PRODUCT_NAME		"Crystal Class Library"
#define CCL_PRODUCT_WEBSITE		"https://ccl.dev"

#define CCL_AUTHOR_NAME			CORE_AUTHOR_NAME
#define CCL_AUTHOR_COPYRIGHT	CORE_AUTHOR_COPYRIGHT
#define CCL_COPYRIGHT_YEAR		CORE_COPYRIGHT_YEAR

#define CCL_PACKAGE_DOMAIN		"dev.ccl"
#define CCL_MIME_TYPE			"application/x.ccl"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Shared Definitions and File Descriptions
//////////////////////////////////////////////////////////////////////////////////////////////////

#define CCL_SPY_NAME					"CCL Spy"
#define CCL_SPY_COMMAND_CATEGORY		"Spy"
#define CCL_SPY_COMMAND_NAME			"Show " CCL_SPY_NAME

#define CCL_SKIN_TYPELIB_NAME			"Skin Elements"
#define CCL_STYLES_TYPELIB_NAME			"Visual Styles"
#define CORE_SKIN_TYPELIB_NAME			"Core Skin Elements"

#define CCL_SETTINGS_NAME				"CCL Cross-platform Framework"

#define CCLTEXT_PACKAGE_ID				CCL_PACKAGE_DOMAIN ".ccltext"
#define CCLTEXT_FILE_DESCRIPTION		"Cross-platform Text Framework"

#define CCLSYSTEM_PACKAGE_ID			CCL_PACKAGE_DOMAIN ".cclsystem"
#define CCLSYSTEM_FILE_DESCRIPTION		"Cross-platform System Framework"

#define CCLGUI_PACKAGE_ID				CCL_PACKAGE_DOMAIN ".cclgui"
#define CCLGUI_FILE_DESCRIPTION			"Cross-platform GUI Framework"

#define CCLNET_PACKAGE_ID				CCL_PACKAGE_DOMAIN ".cclnet"
#define CCLNET_FILE_DESCRIPTION			"Cross-platform Network Framework"

#define CCLSECURITY_PACKAGE_ID			CCL_PACKAGE_DOMAIN ".cclsecurity"
#define CCLSECURITY_FILE_DESCRIPTION	"Cross-platform Security Framework"

#endif // _ccl_version_h
