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
// Filename    : ccl/extras/webfs/webfsbrowserextension.h
// Description : WebFS Browser Extension
//
//************************************************************************************************

#ifndef _ccl_webfsbrowserextension_h
#define _ccl_webfsbrowserextension_h

#include "ccl/base/object.h"

#include "ccl/public/app/ibrowser.h"
#include "ccl/public/gui/icommandhandler.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// BrowserExtension
//************************************************************************************************

class BrowserExtension: public Object,
						public IBrowserExtension
{
public:
	DECLARE_CLASS (BrowserExtension, Object)

	BrowserExtension ();

	bool onUpload (CmdArgs args, VariantRef data);

	// IBrowserExtension
	CCL::tresult CCL_API extendBrowserNodeMenu (IBrowserNode* node, IContextMenu& menu, IUnknownList* selectedNodes) override;

	CLASS_INTERFACE (IBrowserExtension, Object)
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webfsbrowserextension_h
