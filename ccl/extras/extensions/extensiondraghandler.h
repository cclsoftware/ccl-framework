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
// Filename    : ccl/extras/extensions/extensiondraghandler.cpp
// Description : Extension Drag Handler
//
//************************************************************************************************

#ifndef _ccl_extensiondraghandler_h
#define _ccl_extensiondraghandler_h

#include "ccl/app/controls/draghandler.h"

namespace CCL {
namespace Install {

//************************************************************************************************
// ExtensionDragHandler
//************************************************************************************************

class ExtensionDragHandler: public DragHandler
{
public:
	ExtensionDragHandler (IView* view);

	// DragHandler
	tbool CCL_API drop (const DragEvent& event) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;

protected:
	virtual bool matches (const FileType& fileType) const;
	virtual void install (UrlRef path);
};

} // namespace Install
} // namespace CCL

#endif // _ccl_extensiondraghandler_h
