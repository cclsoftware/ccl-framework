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
// Filename    : ccl/app/controls/listviewitem.cpp
// Description : List View Item
//
//************************************************************************************************

#include "ccl/app/controls/listviewitem.h"
#include "ccl/app/controls/itemviewmodel.h"

#include "ccl/base/storage/attributes.h"
#include "ccl/base/objectconverter.h"

#include "ccl/public/gui/framework/iclipboard.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/guiservices.h"

using namespace CCL;

//************************************************************************************************
// ListViewItemToTextFilter
/** Converts a list of ListViewItems to unicode text, using their titles. */
//************************************************************************************************

class ListViewItemToTextFilter: public ConvertFilter,
							    public StaticSingleton<ListViewItemToTextFilter>
{
public:
	// ConvertFilter
	tbool CCL_API canConvert (IUnknown* object, UIDRef cid) const override
	{
		if(cid == ClipboardFormat::UnicodeText)
		{
			UnknownPtr<IUnknownList> list (object);
			if(list)
				ForEachUnknown (*list, unk)
					if(ListViewItem* item = unknown_cast<ListViewItem> (unk))
						return !item->getTitle ().isEmpty ();
				EndFor
		}
		return false;
	}

	IUnknown* CCL_API convert (IUnknown* object, UIDRef cid) const override
	{
		if(cid == ClipboardFormat::UnicodeText)
		{
			UnknownPtr<IUnknownList> list (object);
			if(list)
			{
				IMemoryStream* stream = NEW MemoryStream;
				AutoPtr<ITextStreamer> streamer (System::CreateTextStreamer (*stream, {Text::kUTF16, Text::kSystemLineFormat}));

				bool isFirst = true;
				ForEachUnknown (*list, unk)
					if(ListViewItem* item = unknown_cast<ListViewItem> (unk))
						if(!item->getTitle ().isEmpty ())
						{
							if(isFirst)
								isFirst = false;
							else
								streamer->writeNewline ();

							streamer->writeString (item->getTitle ());
						}
				EndFor
				return stream;
			}
		}
		return nullptr;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (ListViewItem, kFirstRun)
{
	System::GetClipboard ().registerFilter (&ListViewItemToTextFilter::instance ());
	return true;
}

CCL_KERNEL_TERM_LEVEL (ListViewItem, kFirstRun)
{
	System::GetClipboard ().unregisterFilter (&ListViewItemToTextFilter::instance ());
}

//************************************************************************************************
// ListViewItem
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ListViewItem, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem::ListViewItem (StringRef title)
: title (title),
  enabled (true),
  checked (false),
  details (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ListViewItem::~ListViewItem ()
{
	safe_release (details);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewItem::setIcon (IImage* _icon)
{
	icon = _icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ListViewItem::getIcon ()
{
	return icon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Attributes& ListViewItem::getDetails ()
{
	if(details == nullptr)
		details = NEW Attributes;
	return *details;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewItem::getDetail (Variant& value, StringID id) const
{
	if(details)
		return details->getAttribute (value, id) != 0;
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewItem::drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment)
{
	Variant value;
	if(!getDetail (value, id))
		return false;

	String string;
	IImage* image = nullptr;
	bool checkBox = false;
	
	if(value.isObject ())
	{
		image = UnknownPtr<IImage> (value.asUnknown ());
		if(image == nullptr)
			if(Object* object = unknown_cast<Object> (value.asUnknown ()))
				object->toString (string);
	}
	else
	{
		if(value.isNumeric () && value.isBoolFormat ())
			checkBox = true;
		else
			value.toString (string);
	}

	if(image)
		ItemModelPainter ().drawIcon (info, image, isEnabled ());
	else if(checkBox)
		ItemModelPainter ().drawCheckBox (info, value.asBool (), isEnabled (), alignment);
	else if(!string.isEmpty ())
		ItemModelPainter ().drawTitle (info, string, isEnabled (), 0, alignment);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ListViewItem::getCustomBackground () const
{
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewItem::measureContent (Rect& size, StringID id, const IItemModel::StyleInfo& info)
{
	Variant value;
	if(getDetail (value, id))
	{
		if(!value.isObject ())
		{
			String string;
			value.toString (string);
			Font::measureString (size, string, info.font);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewItem::getTooltip (String& tooltip, StringID id)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* ListViewItem::createDragObject ()
{
	retain ();
	return this->asUnknown ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewItem::toString (String& string, int flags) const
{
	string = title;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewItem::compare (const Object& obj) const
{
	const ListViewItem* other = ccl_cast<ListViewItem> (&obj);
	return other ? compareTitle (*other) : SuperClass::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewItem::compareTitle (const ListViewItem& otherItem) const
{
	return title.compareWithOptions (otherItem.getTitle (), Text::kIgnoreCase|Text::kCompareNumerically);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewItem::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "details")
	{
		var.takeShared (const_cast<ListViewItem*> (this)->getDetails ().asUnknown ());
		return true;
	}
	else if(propertyId == "title")
	{
		var = getTitle ();
		return true;
	}
	else if(propertyId == "icon")
	{
		var.takeShared (const_cast<ListViewItem*> (this)->getIcon ());
		return true;
	}
	else
		return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ListViewItem::setProperty (MemberID propertyId, const Variant& var)
{
	if(propertyId == "title")
	{
		setTitle (var.asString ());
		return true;
	}
	else if(propertyId == "icon")
	{
		UnknownPtr<IImage> image (var.asUnknown ());
		setIcon (image);
		return true;
	}
	else
		return SuperClass::setProperty (propertyId, var);
}
