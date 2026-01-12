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
// Filename    : ccl/app/browser/filexportdraghandler.h
// Description : Drag handler for exporting objects as files by dragging them onto a DirectoryNode
//
//************************************************************************************************

#ifndef _ccl_filexportdraghandler_h
#define _ccl_filexportdraghandler_h

#include "ccl/app/browser/filedraghandler.h"

namespace CCL {
	
interface IExportFilter;
interface IFilePromise;

namespace Browsable {

//************************************************************************************************
// Browsable::FileExportDraghandler
//************************************************************************************************

class FileExportDraghandler: public FileDraghandlerBase
{
public:
	DECLARE_CLASS_ABSTRACT (FileExportDraghandler, FileDraghandlerBase)

	FileExportDraghandler (IView* view, Browser* browser);

	// FileDraghandlerBase
	bool checkDataTarget (IDataTarget* dataTarget, IDragSession* session) override;
	IUnknown* prepareDataItem (IUnknown& item, IUnknown* context) override;
	void finishPrepare () override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;

private:
	class Entry;
	class Alternative;

	bool anyAsync;
	int alternativeIndex;
	uint32 lastModifiers;
	ObjectArray entries;
	ObjectArray alternatives;

	Alternative* getAlternative (IExportFilter* filter);
	Alternative* getSelectedAlternative ();
	void selectAlternative (int index);
	String makeAlternativeTitle (StringRef title, bool selected);

	bool makeEntries (IUnknown* item, IUnknown* context);
	void addToSprite (IFilePromise& promise);
};

} // namespace Browsable
} // namespace CCL

#endif // _ccl_filexportdraghandler_h
