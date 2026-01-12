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
// Filename    : ccl/extras/webfs/downloaddraghandler.h
// Description : Download Drag Handler
//
//************************************************************************************************

#ifndef _ccl_downloaddraghandler_h
#define _ccl_downloaddraghandler_h

#include "ccl/app/controls/draghandler.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// DownloadDragHandler
//************************************************************************************************
// TODO: replace by InstallDragHandler for local and remote files!

class DownloadDragHandler: public DragHandler
{
public:
	DownloadDragHandler (IView* view, const FileType& fileType);

	static DownloadDragHandler* create (const DragEvent& event, IView* view, const FileType& fileType);

	// DragHandler
	tbool CCL_API drop (const DragEvent& event) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;

protected:
	FileType fileType;
};

} // namespace Web
} // namespace CCL

#endif // _ccl_downloaddraghandler_h
