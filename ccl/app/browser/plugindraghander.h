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
// Filename    : ccl/app/browser/plugindraghander.h
// Description : Drag handler for moving plugins to another sort folder
//
//************************************************************************************************

#ifndef _ccl_plugindraghander_h
#define _ccl_plugindraghander_h

#include "ccl/app/browser/filedraghandler.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {
namespace Browsable {

class PlugInCategoryNode;

//************************************************************************************************
// Browsable::PluginDraghandler
//************************************************************************************************

class PluginDraghandler: public DragHandlerBase
{
public:
	DECLARE_CLASS_ABSTRACT (PluginDraghandler, DragHandlerBase)

	PluginDraghandler (IView* view = nullptr, Browser* browser = nullptr);

	// DragHandlerBase
	bool setTargetNode (BrowserNode* node) override;

protected:
	String targetSortPath;

	PlugInCategoryNode* findCategoryNode (StringRef category);

	// DragHandlerBase
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
};

} // namespace Browsable
} // namespace CCL

#endif // _ccl_plugindraghander_h
