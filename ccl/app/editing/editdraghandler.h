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
// Filename    : ccl/app/editing/editdraghandler.h
// Description : Edit Drag Handler
//
//************************************************************************************************

#ifndef _ccl_editdraghandler_h
#define _ccl_editdraghandler_h

#include "ccl/app/controls/draghandler.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class EditView;
class EditModel;
class MultiSprite;

//************************************************************************************************
// EditDragHandler
//************************************************************************************************

class EditDragHandler: public DragHandler
{
public:
	DECLARE_CLASS_ABSTRACT (EditDragHandler, DragHandler)

	EditDragHandler (EditView& editView);

	EditView& getEditView () const;
	EditModel& getModel () const;
	bool isOnSourceView (const DragEvent& event) const;

	void setEditTooltip (StringRef tooltip);
	void hideEditTooltip ();

	// DragHandler
	tbool CCL_API dragLeave (const DragEvent& event) override;
	tbool CCL_API drop (const DragEvent& event) override;

protected:
	EditView& editView;
	bool tooltipUsed;
};

//************************************************************************************************
// EditItemIndicator
/** Displays a sprite over the edit item under the mouse. To be used as child draghandler. */
//************************************************************************************************

class EditItemIndicator: public EditDragHandler
{
public:
	DECLARE_CLASS_ABSTRACT (EditItemIndicator, EditDragHandler)

	EditItemIndicator (EditView& editView);

	PROPERTY_MUTABLE_CSTRING (styleName, StyleName)
	PROPERTY_VARIABLE (int, dragResultVoid, dragResultVoid)
	PROPERTY_VARIABLE (int, dragResultOnItem, DragResultOnitem)
	PROPERTY_VARIABLE (int, keepSpriteBelowChild, keepSpriteBelowChild)	///< try to keep a sprite of a childDraghandler above ours (enabled by default)

	Object* findItem (const DragEvent& event) const; ///< caller owns item
	bool hasMatchingTargetItem (const DragEvent& event) const;

protected:
	class ItemDrawable;

	virtual bool verifyItem (Object* item) const;
	virtual void collectHighlightItems (Container& items, Object* mouseItem, const DragEvent& event);	///< container owns items

	// EditDragHandler
	void moveSprite (const DragEvent& event) override;
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
};

} // namespace CCL

#endif // _ccl_editdraghandler_h
