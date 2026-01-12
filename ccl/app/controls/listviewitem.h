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
// Filename    : ccl/app/controls/listviewitem.h
// Description : List View Item
//
//************************************************************************************************

#ifndef _ccl_listviewitem_h
#define _ccl_listviewitem_h

#include "ccl/base/object.h"

#include "ccl/public/gui/graphics/iimage.h"
#include "ccl/public/gui/framework/iitemmodel.h"

namespace CCL {

class Attributes;

//************************************************************************************************
// ListViewItem
/** Base class for items represented in a ListView control. */
//************************************************************************************************

class ListViewItem: public Object
{
public:
	DECLARE_CLASS (ListViewItem, Object)

	ListViewItem (StringRef title = nullptr);
	~ListViewItem ();

	PROPERTY_STRING (title, Title)

	void setIcon (IImage* icon);
	virtual IImage* getIcon ();

	PROPERTY_SHARED_AUTO (IImage, thumbnail, Thumbnail)

	PROPERTY_BOOL (enabled, Enabled)
	PROPERTY_BOOL (checked, Checked)

	Attributes& getDetails ();

	/** Get data for column with given identifier (image or string). */
	virtual bool getDetail (Variant& value, StringID id) const;

	/** Draw column with given identifier. */
	virtual bool drawDetail (const IItemModel::DrawInfo& info, StringID id, AlignmentRef alignment);

	/** Get an optional background color id. It is looked up in the visual style of the view. */
	virtual StringID getCustomBackground () const;

	/** Measure content of cell with given identifier. */
	virtual bool measureContent (Rect& size, StringID id, const IItemModel::StyleInfo& info);

	/** Get tooltip for column with given identifier. */
	virtual bool getTooltip (String& tooltip, StringID id);

	/** Returns "retained" this or new object owned by caller. */
	virtual IUnknown* createDragObject ();

	// Object
	bool toString (String& string, int flags = 0) const override;
	int compare (const Object& obj) const override;

protected:
	SharedPtr<IImage> icon;
	Attributes* details;

	int compareTitle (const ListViewItem& otherItem) const;

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

} // namespace CCL

#endif // _ccl_listviewitem_h
