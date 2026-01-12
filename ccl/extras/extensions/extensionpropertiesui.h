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
// Filename    : ccl/extras/extensions/extensionpropertiesui.h
// Description : Extension Properties User Interface
//
//************************************************************************************************

#ifndef _ccl_extensionpropertiesui_h
#define _ccl_extensionpropertiesui_h

#include "ccl/app/component.h"

namespace CCL {
namespace Install {

class ExtensionDescription;

//************************************************************************************************
// ExtensionPropertiesUI
//************************************************************************************************

class ExtensionPropertiesUI: public Component
{
public:
	DECLARE_CLASS (ExtensionPropertiesUI, Component)

	ExtensionPropertiesUI (const ExtensionDescription* description = nullptr, StringRef status = nullptr);

	void setDescription (const ExtensionDescription& description);
	void setStatus (StringRef text);

	bool askInstall ();

protected:
	IImageProvider* iconProvider;
};

} // namespace Install
} // namespace CCL

#endif // _ccl_extensionpropertiesui_h
