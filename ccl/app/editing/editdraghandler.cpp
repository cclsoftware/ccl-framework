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
// Filename    : ccl/app/editing/editdraghandler.cpp
// Description : Edit Drag Handler
//
//************************************************************************************************

#include "ccl/app/editing/editdraghandler.h"
#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editview.h"

#include "ccl/app/utilities/multisprite.h"

#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/plugservices.h"

using namespace CCL;

//************************************************************************************************
// EditDragHandler
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EditDragHandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditDragHandler::EditDragHandler (EditView& editView)
: DragHandler (editView),
  editView (editView),
  tooltipUsed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditView& EditDragHandler::getEditView () const
{
	return editView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel& EditDragHandler::getModel () const
{
	return getEditView ().getModel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditDragHandler::isOnSourceView (const DragEvent& event) const
{
	return isEqualUnknown (event.session.getSource (), getEditView ().asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditDragHandler::setEditTooltip (StringRef tooltip)
{
	getEditView ().setEditTooltip (tooltip);
	tooltipUsed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditDragHandler::hideEditTooltip ()
{
	getEditView ().hideEditTooltip ();
	tooltipUsed = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditDragHandler::dragLeave (const DragEvent& event)
{
	if(tooltipUsed)
		getEditView ().hideEditTooltip ();

	return SuperClass::dragLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditDragHandler::drop (const DragEvent& event)
{
	if(tooltipUsed)
		getEditView ().hideEditTooltip ();

	return SuperClass::drop (event);
}

//************************************************************************************************
// EditItemIndicator::ItemDrawable
//************************************************************************************************

class EditItemIndicator::ItemDrawable: public SolidDrawable
{
public:
	ItemDrawable (const SolidBrush& brush)
	: SolidDrawable (brush)
	{}

	void updateItems (const ObjectList& items, EditModel& model, EditView& view);
	void getBoundingBox (Rect& boundingBox);

	// SolidDrawable
	void CCL_API draw (const DrawArgs& args);

private:
	class DrawItem
	{
	public:
		DrawItem (Object* item = nullptr)
		: item (item)
		{}

		PROPERTY_POINTER (Object, item, Item)
		PROPERTY_OBJECT (Rect, rect, Rect)
	};
	LinkedList<DrawItem> drawItems;
};

//************************************************************************************************
// EditItemIndicator::ItemDrawable
//************************************************************************************************

void EditItemIndicator::ItemDrawable::updateItems (const ObjectList& items, EditModel& model, EditView& view)
{
	// remove outdated items
	drawItems.removeIf ([&] (DrawItem& i) { return !items.contains (i.getItem ()); });

	Rect client;
	view.getVisibleClient (client);

	// add new items and determine sizes
	for(auto* item : items)
	{
		if(!drawItems.findIf ([&] (DrawItem& i) { return i.getItem () == item; }).getItem ())
		{
			Rect rect;
			if(model.getItemSize (rect, view, item) && rect.bound (client))
			{
				DrawItem drawItem (item);
				drawItem.setRect (rect);
				drawItems.append (drawItem);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditItemIndicator::ItemDrawable::getBoundingBox (Rect& boundingBox)
{
	boundingBox.setReallyEmpty ();
	for(auto& item : drawItems)
		boundingBox.join (item.getRect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditItemIndicator::ItemDrawable::draw (const DrawArgs& args)
{
	for(auto& item : drawItems)
	{
		Rect rect (item.getRect ());
		if(rect.bound (args.updateRgn.bounds))
			args.graphics.fillRect (rect, getBrush ());
	}
}

//************************************************************************************************
// EditItemIndicator
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (EditItemIndicator, EditDragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditItemIndicator::EditItemIndicator (EditView& editView)
: EditDragHandler (editView),
  styleName (CCLSTR ("EditItemDragOverlay")),
  dragResultVoid (IDragSession::kDropNone),
  dragResultOnItem (IDragSession::kDropCopyReal),
  keepSpriteBelowChild (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditItemIndicator::findItem (const DragEvent& event) const
{
	Object* item = getModel ().findItem (getEditView (), event.where);
	if(item && !verifyItem (item))
	{
		item->release ();
		item = nullptr;
	}
	return item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditItemIndicator::hasMatchingTargetItem (const DragEvent& event) const
{
	AutoPtr<Object> item (findItem (event)); // calls verifyItem
	return item != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditItemIndicator::verifyItem (Object* item) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditItemIndicator::collectHighlightItems (Container& items, Object* mouseItem, const DragEvent& event)
{
	mouseItem->retain ();
	items.add (mouseItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditItemIndicator::moveSprite (const DragEvent& event)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditItemIndicator::dragEnter (const DragEvent& event)
{
	if(!sprite)
	{
		ASSERT (!sprite)

		ITheme& theme = editView.getTheme ();
		Color backColor (theme.getStyle (styleName).getColor (StyleID::kBackColor, theme.getThemeColor (ThemeElements::kAlphaSelectionColor)));

		AutoPtr<ItemDrawable> drawable (NEW ItemDrawable (SolidBrush (backColor)));
		drawable->takeOpacity ();

		ISprite* sprite = ccl_new<ISprite> (ClassID::FloatingSprite);
		sprite->construct (view, Rect (), drawable, ISprite::kKeepOnTop);
		setSprite (sprite);
		// note: this hides source feedback
	}
	return EditDragHandler::dragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditItemIndicator::dragOver (const DragEvent& event)
{
	AutoPtr<Object> mouseItem (findItem (event));

	auto* drawable = sprite ? static_cast<ItemDrawable*> (sprite->getDrawable ()) : nullptr;
	if(drawable)
	{
		ObjectList items;
		items.objectCleanup (true);

		if(mouseItem)
			collectHighlightItems (items, mouseItem, event);

		drawable->updateItems (items, getModel (), getEditView ());

		if(items.isEmpty ())
			sprite->hide ();
		else
		{
			tbool wasVisible = sprite->isVisible ();

			Rect boundingBox;
			drawable->getBoundingBox (boundingBox);
			sprite->move (boundingBox);
			sprite->show ();

			if(keepSpriteBelowChild && !wasVisible)
				if(DragHandler* child = unknown_cast<DragHandler> (getChildDragHandler ()))
				{
					ISprite* childSprite = child->getSprite ();
					if(childSprite && childSprite->isVisible ())
					{
						// elevate child sprite to top
						childSprite->hide ();
						childSprite->show ();
					}
				}
		}
	}
	event.session.setResult (mouseItem ? dragResultOnItem : dragResultVoid);

	return EditDragHandler::dragOver (event);
}
