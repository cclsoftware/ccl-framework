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
// Filename    : ccl/public/gui/framework/skinxmldefs.h
// Description : Skin XML Definitions
//
//************************************************************************************************

#ifndef _ccl_skinxmldefs_h
#define _ccl_skinxmldefs_h

// Include generated tag and attribute definitions
#include "ccl/meta/generated/cpp/skinxmldefs-generated.h"

//************************************************************************************************
// Skin XML Enumerations
//************************************************************************************************

#define MAKE_SKIN_ENUMERATION(TagName, AttrName) TagName "." AttrName

//************************************************************************************************
// Skin XML Tag Groups
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// Documentation Groups
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DOC_GROUP_GENERAL		"General"
#define DOC_GROUP_RESOURCES		"Resources"
#define DOC_GROUP_STYLES		"Styles"
#define DOC_GROUP_SHAPES		"Shapes"
#define DOC_GROUP_VIEWS			"Views"
#define DOC_GROUP_CONTROLS		"Controls"
#define DOC_GROUP_LAYOUT		"Layout"
#define DOC_GROUP_ANIMATION		"Animation"
#define DOC_GROUP_3D			"3D"
#define DOC_GROUP_WORKSPACE		"Workspace"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Schema Groups
//////////////////////////////////////////////////////////////////////////////////////////////////

#define MAKE_SCHEMA_GROUP(TagName) TagName ".Children"

#define SCHEMA_GROUP_VIEWSSTATEMENTS		"ViewsAndStatements"
#define SCHEMA_GROUP_FRAMECHILDREN			MAKE_SCHEMA_GROUP (TAG_FRAME)
#define SCHEMA_GROUP_IMAGECHILDREN			MAKE_SCHEMA_GROUP (TAG_IMAGE)
#define SCHEMA_GROUP_PERSPECTIVECHILDREN	MAKE_SCHEMA_GROUP (TAG_PERSPECTIVE)
#define SCHEMA_GROUP_RESOURCES				MAKE_SCHEMA_GROUP (TAG_RESOURCES)
#define SCHEMA_GROUP_SHAPES					MAKE_SCHEMA_GROUP (TAG_SHAPES)
#define SCHEMA_GROUP_TOPLEVEL				MAKE_SCHEMA_GROUP (TAG_SKIN)
#define SCHEMA_GROUP_STYLECHILDREN			MAKE_SCHEMA_GROUP (TAG_STYLE)
#define SCHEMA_GROUP_THEMEELEMENTCHILDREN	MAKE_SCHEMA_GROUP (TAG_THEMESTYLE)
#define SCHEMA_GROUP_TRIGGERCHILDREN		MAKE_SCHEMA_GROUP (TAG_TRIGGER)
#define SCHEMA_GROUP_SWITCHCHILDREN			MAKE_SCHEMA_GROUP (TAG_SWITCH)
#define SCHEMA_GROUP_MODEL3DCHILDREN		MAKE_SCHEMA_GROUP (TAG_MODEL3D)
#define SCHEMA_GROUP_SCENE3DCHILDREN		MAKE_SCHEMA_GROUP (TAG_SCENE3D)
#define SCHEMA_GROUP_TEXTUREMATERIAL3DCHILDREN MAKE_SCHEMA_GROUP (TAG_TEXTUREMATERIAL3D)

#endif // _ccl_skinxmldefs_h
