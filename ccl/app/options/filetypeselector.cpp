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
// Filename    : ccl/app/options/filetypeselector.cpp
// Description : File Type Selector
//
//************************************************************************************************

#include "ccl/app/options/filetypeselector.h"

#include "ccl/app/utilities/fileicons.h"
#include "ccl/app/controls/listviewmodel.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/collections/stringdictionary.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/systemservices.h"

namespace CCL {

//************************************************************************************************
// FileTypeItem
//************************************************************************************************

class FileTypeItem: public ListViewItem
{
public:
	FileTypeItem (): editable (false) {}

	PROPERTY_OBJECT (FileType, fileType, FileType)
	PROPERTY_STRING (category, Category)
	PROPERTY_BOOL (editable, Editable)

	// ListViewItem
	int compare (const Object& obj) const override;
};

//************************************************************************************************
// FileTypeItemList
//************************************************************************************************

class FileTypeItemList: public ListViewModel
{
public:
	FileTypeItemList (FileTypeSelector& component);

	enum Columns
	{
		kIcon,
		kExtension,
		kDescription,
		kCategory
	};

	FileTypeItem* find (const FileType& fileType) const;
	void collect (FileTypeFilter& fileTypes, StringRef category) const;
	void collect (Container& fileTypes, StringRef category) const;

	// ListViewModel
	tbool CCL_API createColumnHeaders (IColumnHeaderList& list) override;
	tbool CCL_API drawCell (ItemIndexRef index, int column, const DrawInfo& info) override;
	tbool CCL_API canRemoveItem (ItemIndexRef index) override;
	tbool CCL_API removeItem (ItemIndexRef index) override;
	using ListViewModel::removeItem;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	FileTypeSelector& component;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (Extension, "Extension")
	XSTRING (Description, "Description")
	XSTRING (Category, "Category")
	XSTRING (Icon, "Icon")
	XSTRING (AddFileType, "Add File Type")
	XSTRING (ExtFile, "%(1) File")
	XSTRING (WarnFileTypeAlreadyExists, "This file type already exists.")
	XSTRING (WarnFileTypeInvalid, "This file type is invalid.")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum FileTypeSelectorTags
	{
		kAddType = 100,
		kRemoveType
	};
}

//************************************************************************************************
// FileTypeItem
//************************************************************************************************

int FileTypeItem::compare (const Object& obj) const
{
	const FileTypeItem& other = (const FileTypeItem&)obj;
	if(category == other.category)
	{
		return fileType.getExtension ().compare (other.fileType.getExtension ());
	}
	else
		return category.compare (other.category);
}

//************************************************************************************************
// FileTypeSelector
//************************************************************************************************

DEFINE_CLASS (FileTypeSelector, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeSelector::FileTypeSelector (StringRef name)
: Component (name.isEmpty () ? CCLSTR ("FileTypes") : name),
  itemList (nullptr)
{
	itemList = NEW FileTypeItemList (*this);

	paramList.addParam (CSTR ("addType"), Tag::kAddType);
	paramList.addParam (CSTR ("removeType"), Tag::kRemoveType)->enable (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeSelector::~FileTypeSelector ()
{
	itemList->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API FileTypeSelector::getObject (StringID name, UIDRef classID)
{
	if(name == "itemList")
		return ccl_as_unknown (itemList);
	else
		return SuperClass::getObject (name, classID);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeItem* FileTypeSelector::createItem (const FileType& fileType, StringRef category) const
{
	FileTypeItem* item = NEW FileTypeItem;
	item->setFileType (fileType);
	item->setTitle (fileType.getExtension ());
	item->setCategory (category);
	item->setEditable (category == getEditCategory ());
	item->setIcon (AutoPtr<IImage> (FileIcons::instance ().createIcon (fileType)));
	item->setEnabled (true);
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::addFileTypes (const FileTypeFilter& fileTypes, StringRef category)
{
	VectorForEach (fileTypes.getContent (), FileType, fileType)
		addFileType (fileType, category);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::addFileType (const FileType& fileType, StringRef category)
{
	FileTypeItem* item = createItem (fileType, category);
	itemList->addSorted (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::getFileTypes (FileTypeFilter& fileTypes, StringRef category) const
{
	itemList->collect (fileTypes, category);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FileTypeSelector::getIconAssignment (StringDictionary& dict, StringRef category) const
{
	dict.removeAll ();

	ASSERT (iconPalette != nullptr)
	if(iconPalette == nullptr)
		return false;

	ObjectList items;
	itemList->collect (items, category);
	ForEach (items, FileTypeItem, item)
		if(item->getIcon ())
		{
			int index = iconPalette->getIndex (item->getIcon ());
			ASSERT (index != -1)
			if(index != -1)
				dict.setEntry (item->getFileType ().getExtension (), String () << index);
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::removeFileTypes ()
{
	itemList->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeSelector::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kAddType :
		runAddDialog ();
		break;

	case Tag::kRemoveType :
		removeSelected ();
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::runAddDialog ()
{
	ParamContainer params;

	IImageProvider* iconParam = nullptr;
	if(iconPalette)
	{
		iconParam = params.addImage (XSTR_REF (Icon).getKey ());
		iconParam->setImage (UnknownPtr<IImage> (iconPalette->getAt (0).asUnknown ()));
		UnknownPtr<IPaletteProvider> (iconParam)->setPalette (getIconPalette ());
	}

	IParameter* extParam = params.addString (XSTR_REF (Extension).getKey ());
	IParameter* descParam = params.addString (XSTR_REF (Description).getKey ());

	if(DialogBox ()->runWithParameters ("AddFileTypeDialog", params, XSTR (AddFileType)) != DialogResult::kOkay)
		return;

	IImage* icon = iconParam ? iconParam->getImage () : nullptr;
	String extension, description;
	extParam->toString (extension);
	descParam->toString (description);
	extension.trimWhitespace ();
	description.trimWhitespace ();

	// verify extension
	String savedExtension = extension;
	extension = LegalFileName (extension);
	extension.replace (CCLSTR ("."), CCLSTR ("_"));
	extension = String (MutableCString (extension)); // ASCII conversion
	if(extension.isEmpty () || extension != savedExtension)
	{
		Alert::warn (XSTR (WarnFileTypeInvalid));
		return;
	}

	if(description.isEmpty ())
		description.appendFormat (XSTR (ExtFile), String (extension).toUppercase ());
	extension.toLowercase ();

	FileType newType;
	newType.setExtension (extension);
	newType.setDescription (description);
	//newType.setMimeType (CCLSTR ("application/octet-stream"));

	if(itemList->find (newType))
	{
		Alert::warn (XSTR (WarnFileTypeAlreadyExists));
		return;
	}

	FileTypeItem* item = createItem (newType, getEditCategory ());
	if(icon != nullptr)
		item->setIcon (icon);
	itemList->addSorted (item);
	itemList->signal (Message (kChanged));
	
	signal (Message (kChanged));

	// select in list view
	if(IItemView* itemView = itemList->getItemView ())
	{
		ItemIndex itemIndex;
		if(itemList->getIndex (itemIndex, item))
			itemView->setFocusItem (itemIndex);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::selectionChanged ()
{
	ObjectList selected;
	itemList->getSelectedItems (selected);

	bool canRemove = false;
	ForEach (selected, FileTypeItem, item)
		if(item->isEditable ())
		{
			canRemove = true;
			break;
		}
	EndFor

	paramList.byTag (Tag::kRemoveType)->enable (canRemove);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::removeSelected ()
{
	ObjectList selected;
	itemList->getSelectedItems (selected);

	ObjectList candidates;
	ForEach (selected, FileTypeItem, item)
		if(item->isEditable ())
			candidates.add (item);
	EndFor
	
	if(!candidates.isEmpty ())
		remove (candidates);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeSelector::remove (Container& candidates)
{
	ForEach (candidates, FileTypeItem, item)
		ASSERT (item->isEditable ())
		itemList->removeItem (item);
		item->release ();
	EndFor

	itemList->signal (Message (kChanged));

	signal (Message (kChanged));
}

//************************************************************************************************
// FileTypeItemList
//************************************************************************************************

FileTypeItemList::FileTypeItemList (FileTypeSelector& component)
: component (component)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileTypeItem* FileTypeItemList::find (const FileType& fileType) const
{
	ForEach (items, FileTypeItem, item)
		if(item->getFileType () == fileType)
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeItemList::collect (FileTypeFilter& fileTypes, StringRef category) const
{
	ForEach (items, FileTypeItem, item)
		if(category.isEmpty () || item->getCategory () == category)
			fileTypes.addFileType (item->getFileType ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FileTypeItemList::collect (Container& fileTypes, StringRef category) const
{
	ForEach (items, FileTypeItem, item)
		if(category.isEmpty () || item->getCategory () == category)
			fileTypes.add (item);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeItemList::createColumnHeaders (IColumnHeaderList& list)
{
	list.addColumn ( 24);						// kIcon
	list.addColumn ( 80, XSTR (Extension));		// kExtension
	list.addColumn (180, XSTR (Description));	// kDescription
	list.addColumn (120, XSTR (Category));		// kCategory
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeItemList::drawCell (ItemIndexRef index, int column, const DrawInfo& info)
{
	FileTypeItem* item = (FileTypeItem*)resolve (index);
	if(!item)
		return false;

	int fontStyle = 0;
	bool enabled = true;
	if(item->isEditable ())
		fontStyle = Font::kBold;

	switch(column)
	{
	case kIcon :
		drawIcon (info, item->getIcon ());
		break;

	case kExtension :
		drawTitle (info, item->getFileType ().getExtension (), enabled, fontStyle);
		break;

	case kDescription :
		drawTitle (info, item->getFileType ().getDescription (), enabled, fontStyle);
		break;

	case kCategory :
		drawTitle (info, item->getCategory (), enabled, fontStyle);
		break;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeItemList::canRemoveItem (ItemIndexRef index)
{
	FileTypeItem* item = (FileTypeItem*)resolve (index);
	return item && item->isEditable ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileTypeItemList::removeItem (ItemIndexRef index)
{
	FileTypeItem* item = (FileTypeItem*)resolve (index);
	if(item && item->isEditable ())
	{
		ObjectList candidates;
		candidates.add (item);
		component.remove (candidates);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FileTypeItemList::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kSelectionChanged)
	{
		component.selectionChanged ();
	}
	else
		SuperClass::notify (subject, msg);
}
