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
// Filename    : ccl/app/browser/browserextender.h
// Description : Browser Extender
//
//************************************************************************************************

#ifndef _ccl_browserextender_h
#define _ccl_browserextender_h

#include "ccl/app/component.h"

#include "ccl/public/app/ibrowser.h"

#include "ccl/public/collections/linkedlist.h"

namespace CCL {

class BrowserNode;

//************************************************************************************************
// BrowserExtender
//************************************************************************************************

class BrowserExtender: public Component
{
public:
	DECLARE_CLASS (BrowserExtender, Component)

	BrowserExtender ();
	~BrowserExtender ();

	void addExtension (IBrowserExtension* extension);
	void addExtensionPlugIns (StringRef category);

	bool extendBrowserNodeMenu (BrowserNode* node, IContextMenu& menu, Container* selectedNodes);

	// Component
	tresult CCL_API terminate () override;

protected:
	LinkedList<IBrowserExtension*> extensions;

	void destroyAll ();
};

} // namespace CCL

#endif // _ccl_browserextender_h
