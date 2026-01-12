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
// Filename    : ccl/extras/extensions/extensionpropertiesui.cpp
// Description : Extension Properties User Interface
//
//************************************************************************************************

#include "ccl/extras/extensions/extensionpropertiesui.h"
#include "ccl/extras/extensions/extensiondescription.h"

#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"

using namespace CCL;
using namespace Install;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum ExtensionPropertiesUITags
	{
		kTitle = 100,
		kDescription,
		kVendor,
		kVersion,
		kCopyright,
		kWebsite,
		kIcon,
		kStatus
	};
}

//************************************************************************************************
// ExtensionPropertiesUI
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ExtensionPropertiesUI, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

ExtensionPropertiesUI::ExtensionPropertiesUI (const ExtensionDescription* description, StringRef status)
: Component (String ("ExtensionPropertiesUI")),
  iconProvider (nullptr)
{
	paramList.addString ("title", Tag::kTitle);
	paramList.addString ("description", Tag::kDescription);
	paramList.addString ("vendor", Tag::kVendor);
	paramList.addString ("version", Tag::kVersion);
	paramList.addString ("copyright", Tag::kCopyright);
	paramList.addString ("website", Tag::kWebsite);
	iconProvider = paramList.addImage  ("icon", Tag::kIcon);
	paramList.addString ("status", Tag::kStatus);

	if(description)
		setDescription (*description);
	if(!status.isEmpty ())
		setStatus (status);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPropertiesUI::setDescription (const ExtensionDescription& description)
{
	paramList.byTag (Tag::kTitle)->fromString (description.getTitle ());
	paramList.byTag (Tag::kDescription)->fromString (description.getDescription ());
	paramList.byTag (Tag::kVendor)->fromString (description.getVendor ());
	paramList.byTag (Tag::kVersion)->fromString (description.getVersion ());
	paramList.byTag (Tag::kCopyright)->fromString (description.getCopyright ());
	paramList.byTag (Tag::kWebsite)->fromString (description.getWebsite ());
	iconProvider->setImage (description.getIcon ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ExtensionPropertiesUI::setStatus (StringRef text)
{
	paramList.byTag (Tag::kStatus)->fromString (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ExtensionPropertiesUI::askInstall ()
{
	int result = DialogResult::kCancel;

	IView* view = getTheme ()->createView ("CCL/ExtensionAskInstallBox", asUnknown ());
	ASSERT (view != nullptr)
	if(view)
		result = DialogBox ()->runDialog (view);

	return result == DialogResult::kOkay;
}
