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
// Filename    : ccl/app/options/filetypeselector.h
// Description : File Type Selector
//
//************************************************************************************************

#ifndef _ccl_filetypeselector_h
#define _ccl_filetypeselector_h

#include "ccl/app/component.h"

#include "ccl/public/gui/framework/ipalette.h"

namespace CCL {

class FileType;
class FileTypeFilter;
class FileTypeItem;
class FileTypeItemList;
class StringDictionary;

//************************************************************************************************
// FileTypeSelector
//************************************************************************************************

class FileTypeSelector: public Component
{
public:
	DECLARE_CLASS (FileTypeSelector, Component)

	FileTypeSelector (StringRef name = nullptr);
	~FileTypeSelector ();

	PROPERTY_SHARED_AUTO (IImagePalette, iconPalette, IconPalette)
	PROPERTY_STRING (editCategory, EditCategory)

	void addFileTypes (const FileTypeFilter& fileTypes, StringRef category = nullptr);
	void addFileType (const FileType& fileType, StringRef category = nullptr);
	void getFileTypes (FileTypeFilter& fileTypes, StringRef category = nullptr) const;
	void removeFileTypes ();

	/// get icon assignment (key = extension, value = palette index)
	bool getIconAssignment (StringDictionary& dict, StringRef category = nullptr) const;

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	friend class FileTypeItemList;

	FileTypeItemList* itemList;

	FileTypeItem* createItem (const FileType& fileType, StringRef category) const;
	void runAddDialog ();
	void selectionChanged ();
	void removeSelected ();
	void remove (Container& candidates);
};

} // namespace CCL

#endif // _ccl_filetypeselector_h
