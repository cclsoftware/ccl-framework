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
// Filename    : ccl/extras/webfs/webfiledraghandler.h
// Description : Web File Drag Handler
//
//************************************************************************************************

#ifndef _ccl_webfiledraghandler_h
#define _ccl_webfiledraghandler_h

#include "ccl/app/browser/filedraghandler.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebFileDragHandler
//************************************************************************************************

class WebFileDragHandler: public Browsable::DragHandlerBase
{
public:
	WebFileDragHandler (IView* view, Browser* browser);
	
	// DragHandlerBase
	bool setTargetNode (BrowserNode* node) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;

private:
	PathList homeFolders;
	PathList childFolders;
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_webfiledraghandler_h
