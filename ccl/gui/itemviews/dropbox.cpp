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
// Filename    : ccl/gui/itemviews/dropbox.cpp
// Description : Drop Box
//
//************************************************************************************************

#include "ccl/gui/itemviews/dropbox.h"

#include "ccl/gui/layout/anchorlayout.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/gui.h"

#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/iviewfactory.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/math/mathprimitives.h"

#include "ccl/base/message.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("DropBox")
	XSTRING (RemoveItem, "Remove %(1)")
END_XSTRINGS

namespace CCL {

//************************************************************************************************
// DummyView
//************************************************************************************************

class DummyView: public CCL::View
{
public:
	DECLARE_CLASS_ABSTRACT (DummyView, View)

	DummyView (IUnknown* controller)
	: controller (controller)
	{}

	// IView
	IUnknown* CCL_API getController () const override { return controller; }
	#if DEBUG
	void attached (View* parent) override { ASSERT (false) }
	#endif

private:
	SharedPtr<IUnknown> controller;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// DummyView
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DummyView, View)

//************************************************************************************************
// DropBoxControl
//************************************************************************************************
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (DropBoxStyle, VisualStyle, "DropBoxStyle")
	ADD_VISUALSTYLE_METRIC ("noSnap")		///< don't snap to views size
	ADD_VISUALSTYLE_METRIC ("spacing")		///< spacing between items
	ADD_VISUALSTYLE_METRIC ("freespace")	///< free space after last item 
END_VISUALSTYLE_CLASS (DropBoxStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_MEMBER_ (DropBoxControl, kResizeDropBox, "resizeDropBox")

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (DropBoxControl, ItemControlBase)
DEFINE_CLASS_UID (DropBoxControl, 0x801E6857, 0x625B, 0x4CC2, 0x96, 0xFE, 0xAC, 0x54, 0x74, 0xCB, 0xF5, 0xDE)

//////////////////////////////////////////////////////////////////////////////////////////////////

DropBoxControl::DropBoxControl (const Rect& size, StyleRef dropBoxStyle, StyleRef scrollViewStyle)
: ItemControlBase (size, NEW DropBox (Rect (), dropBoxStyle), scrollViewStyle),
  shouldAutoResize (dropBoxStyle.isCustomStyle (Styles::kDropBoxLayoutAutoResize)),
  lastColumnCount (0)
{
	if(dropBoxStyle.isCustomStyle (Styles::kDropBoxLayoutWrapViews))
	{
		getItemView ()->setSizeMode (kAttachAll);
		getItemView ()->setSize (Rect (0, 0, size.getSize ()));
	}
	else if(dropBoxStyle.isHorizontal ())
	{
		getItemView ()->setSizeMode (kHFitSize|kAttachTop|kAttachBottom);
		getItemView ()->setSize (Rect (0, 0, dropBoxStyle.isCustomStyle (Styles::kDropBoxLayoutCentered) ? size.getWidth () : 0, size.getHeight ()));
	}
	else
	{
		getItemView ()->setSizeMode (kVFitSize|kAttachLeft|kAttachRight);
		getItemView ()->setSize (Rect (0, 0, size.getWidth (), dropBoxStyle.isCustomStyle (Styles::kDropBoxLayoutCentered) ? size.getHeight () : 0));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DropBoxControl::~DropBoxControl ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxControl::attached (View* parent)
{
	SuperClass::attached (parent);

	if(getItemView ()->getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews))
	{
		if(sizeMode & kVFitSize)
		{
			int sizeMode = getItemView ()->getSizeMode ();
			sizeMode |= kVFitSize;
			getItemView ()->setSizeMode (sizeMode);

			sizeMode = getItemView ()->getFirst ()->getSizeMode ();
			sizeMode |= kVFitSize;
			getItemView ()->getFirst ()->setSizeMode (sizeMode);
		}

		if(sizeMode & kHFitSize)
		{
			int sizeMode = getItemView ()->getSizeMode ();
			sizeMode |= kHFitSize;
			getItemView ()->setSizeMode (sizeMode);

			sizeMode = getItemView ()->getFirst ()->getSizeMode ();
			sizeMode |= kHFitSize;
			getItemView ()->getFirst ()->setSizeMode (sizeMode);
		}

		if(!hasExplicitSizeLimits ())
			static_cast<DropBox*> (getItemView ())->resetClientLimits ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxControl::calcSizeLimits ()
{
	if(getItemView ()->getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews) && getStyle ().isHorizontal () && (getSizeMode () & kVFitSize))
		sizeLimits = getItemView ()->getSizeLimits ();
	else 
		SuperClass::calcSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DropBoxControl::onMouseWheel (const MouseWheelEvent& event)
{
	// mousewheel scrolling only if explicitely desired
	if(getItemView ()->getStyle ().isCustomStyle (Styles::kDropBoxBehaviorScrollWheel))
		return SuperClass::onMouseWheel (event);
	else
		return View::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxControl::onSize (PointRef delta)
{
	SuperClass::onSize (delta);
	
	auto getResizeDelay = [&]()
	{
		static constexpr int kImmediately = -1;
		static constexpr int kAvoidFlickerDelay = 250;
		
		if(Window* window = getWindow ())
			if(window->isResizing ())
				return kAvoidFlickerDelay;
				
		return kImmediately;
	};
	
	if(shouldAutoResize)
		(NEW Message (kResizeDropBox))->post (this, getResizeDelay ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DropBoxControl::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kResizeDropBox)
	{
		ScopedVar<bool> scope (shouldAutoResize, false); // suppress posting the message again in onSize
		resizeDropBox ();
	}
	
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxControl::resizeDropBox ()
{
	Rect size (getSize ());
	updateScrollViewSize (size, countItems (), getItemSpacing ());
	setSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DropBoxControl::countItems () const
{
	DropBox* dropBox = static_cast<DropBox*> (getItemView ());
	int i = 0;
	IView* itemView = nullptr;
	do
	{
		ItemIndex index (i);
		itemView = dropBox->getViewItem (index);
		i++;
	} while (itemView);

	return --i;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point DropBoxControl::getItemSize () const
{
	DropBox* dropBox = static_cast<DropBox*> (getItemView ());
	ItemIndex index (0);

	if(IView* itemView = dropBox->getViewItem (index))
		return itemView->getSize ().getRightBottom ();

	return Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord DropBoxControl::getMinItemWidth (Coord itemWidth) const
{
	DropBox* dropBox = static_cast<DropBox*> (getItemView ());
	ItemIndex index (0);

	if(IView* itemView = dropBox->getViewItem (index))
		if((ViewBox(itemView).getSizeMode () & (IView::kAttachLeft|IView::kAttachRight)) == (IView::kAttachLeft|IView::kAttachRight))
			if(itemWidth > itemView->getSizeLimits ().minWidth)
				itemWidth -= 1;
				
	return itemWidth;
}


//////////////////////////////////////////////////////////////////////////////////////////////////

Coord DropBoxControl::getItemSpacing () const
{
	return getVisualStyle ().getMetric<Coord> ("spacing", 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBoxControl::updateScrollViewSize (Rect& dropBoxSize, int itemCount, Coord itemSpacing) const
{
	DropBox* dropBox = static_cast<DropBox*> (getItemView ());

	if(itemCount > 0 && dropBoxSize.getHeight () > 0)
	{
		Coord neededHeight = 0;
		Coord currentRowWidth = 0;
		Coord currentRowHeight = 0;
		int currentRowItemCount = 0;
		int rowCount = itemCount > 0 ? 1 : 0;
		for(int i = 0; i < itemCount; i++)
		{
			IView* itemView = dropBox->getViewItem (i);
			if(itemView == nullptr)
				continue;

			if(currentRowWidth + currentRowItemCount * itemSpacing + itemView->getSize ().getWidth () < dropBoxSize.getWidth ())
			{
				currentRowWidth += itemView->getSize ().getWidth ();
				currentRowItemCount++;
				currentRowHeight = ccl_max (itemView->getSize ().getHeight (), currentRowHeight);
			}
			else
			{
				neededHeight += currentRowHeight + itemSpacing;
				currentRowWidth = itemView->getSize ().getWidth ();
				currentRowHeight = itemView->getSize ().getHeight ();
				rowCount++;
				currentRowItemCount = 1;
			}
		}
		if(currentRowItemCount > 0)
			neededHeight += currentRowHeight + itemSpacing;

		Coord autoResizeLimit = (getVisualStyle ().getMetric<Coord> ("autoresizelimit", sizeLimits.maxHeight));
		if(neededHeight > autoResizeLimit)
			neededHeight = autoResizeLimit;

		dropBoxSize.setHeight (neededHeight);
	}
}


//************************************************************************************************
// DropBox::DeleteItemDragHandler
//************************************************************************************************

class DropBox::DeleteItemDragHandler: public Unknown,
									  public AbstractDragHandler
{
public:
	DeleteItemDragHandler (DropBox* dropBox, ItemIndexRef index)
	: dropBox (dropBox), index (index)
	{
		const IVisualStyle& vs = dropBox->getTheme ().getStyle ("Standard.Sprite");

		String title;		
		dropBox->getModel ()->getItemTitle (title, index);

		String text;
		text.appendFormat (XSTR (RemoveItem), title);

		Rect rect;
		Font font (vs.getTextFont ());
		font.isBold (true);
		Font::measureString (rect, text, font);
		rect.right += 8;
		rect.bottom += 8;

		float contentScaleFactor = 1.f;
		if(Window* window = dropBox->getWindow ())
			contentScaleFactor = window->getContentScaleFactor ();
		
		AutoPtr<Bitmap> spriteBitmap (NEW Bitmap (rect.getWidth (), rect.getHeight (), Bitmap::kRGB, contentScaleFactor));
		BitmapGraphicsDevice graphics (spriteBitmap);
		graphics.fillRect (rect, vs.getBackBrush ());
		graphics.drawString (rect, text, font, vs.getTextBrush (), Alignment::kCenter);

		AutoPtr<ImageDrawable> drawable = NEW ImageDrawable (spriteBitmap, 0.8f);
		drawable->takeOpacity ();
		sprite = NEW FloatingSprite (dropBox->getWindow (), drawable, rect, ISprite::kKeepOnTop);

		setSpriteOffset (Point (0, 30));
	}

	// AbstractDragHandler
	void moveSprite (const DragEvent& event) override
	{
		bool active = canDelete (event);
		if(active)
			AbstractDragHandler::moveSprite (event);
		else
			hideSprite ();

		unknown_cast<DragSession> (&event.session)->setSourceResult (active ? IDragSession::kDropMove : IDragSession::kDropNone);
	}
		
	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		if(canDelete (event))
		{
			event.session.getItems ().removeAll (); // remove drag data first!
			dropBox->notifyRemove (index);
		}
		return AbstractDragHandler::afterDrop (event);
	}

	bool canDelete (const DragEvent& event) const
	{
		// allow delete outside dropbox when command key is pressed
		if(event.session.wasCanceled () || !event.keys.isSet (KeyState::kCommand))
			return false;

		Point p (event.where);
		return !dropBox->isInsideClient (dropBox->windowToClient (p));
	}

	CLASS_INTERFACE (IDragHandler, Unknown)

protected:
	DropBox* dropBox;
	ItemIndex index;
};

//************************************************************************************************
// DropBox
//************************************************************************************************

BEGIN_STYLEDEF (DropBox::customStyles)
	{"wrap",		Styles::kDropBoxLayoutWrapViews},
	{"dragnowhere",	Styles::kDropBoxBehaviorDragNowhereToRemove},
	{"scrollwheel",	Styles::kDropBoxBehaviorScrollWheel},
	{"alignend",	Styles::kDropBoxLayoutAlignEnd},
	{"colorize",	Styles::kDropBoxAppearanceColorize},
	{"dragswipeh",	Styles::kItemViewBehaviorDragSwipeH},
	{"dragswipev",	Styles::kItemViewBehaviorDragSwipeV},
	{"centered",	Styles::kDropBoxLayoutCentered},
	{"autoresize",	Styles::kDropBoxLayoutAutoResize},
	{"commonbasesize",Styles::kDropBoxLayoutCommonBaseSize},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (DropBox, ItemViewBase)
DEFINE_CLASS_UID (DropBox, 0x25f3965e, 0x3d6c, 0x4f45, 0x8c, 0x9a, 0x7d, 0x88, 0x5c, 0xb4, 0x7e, 0x7a)

//////////////////////////////////////////////////////////////////////////////////////////////////

DropBox::DropBox (const Rect& size, StyleRef style)
: ItemViewBase (size, style),
  clientView (nullptr),
  spacing (1),
  freeSpace (0),
  snap (1)
{
	setName ("DropBox");
	
	// create client
	StyleFlags clientStyle;
	int sizeMode = 0;

	if(style.isHorizontal ())
	{
		clientStyle.common = Styles::kHorizontal;
		sizeMode = kHFitSize|kAttachTop|kAttachBottom;
		if(style.isCustomStyle (Styles::kDropBoxLayoutCentered))
			sizeMode |= kHCenter;
	}
	else
	{
		clientStyle.common = Styles::kVertical;
		sizeMode = kVFitSize|kAttachLeft|kAttachRight;
		if(style.isCustomStyle (Styles::kDropBoxLayoutCentered))
			sizeMode |= kVCenter;
	}

	if(style.isCustomStyle (Styles::kDropBoxLayoutWrapViews))
	{
		clientStyle.custom = Styles::kLayoutWrap;
		sizeMode = style.isHorizontal () ? kAttachAll|kVFitSize : kAttachAll|kHFitSize;
	}

	if(style.isCustomStyle (Styles::kDropBoxLayoutCommonBaseSize))
		clientStyle.custom |= Styles::kLayoutCommonBaseSize;

	clientView = NEW BoxLayoutView (Rect (0, 0, size.getWidth (), size.getHeight ()), clientStyle);
	clientView->setSpacing (spacing);
	clientView->setMargin (0);
	clientView->setSizeMode (sizeMode);
	addView (clientView);

	itemViews.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DropBox::~DropBox ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::setStyle (StyleRef style)
{
	SuperClass::setStyle (style);

	// adjust clientView
	if(clientView)
	{
		bool horizontal = style.isHorizontal ();
		StyleFlags s (clientView->getStyle ());
		s.setCommonStyle (Styles::kHorizontal, horizontal);
		s.setCommonStyle (Styles::kVertical, !horizontal);
		clientView->setStyle (s);

		int sizeMode = horizontal ? (kHFitSize|kAttachTop|kAttachBottom) : (kVFitSize|kAttachLeft|kAttachRight);
		if(style.isCustomStyle (Styles::kDropBoxLayoutAlignEnd))
			sizeMode |= horizontal ? kAttachRight : kAttachBottom;
		clientView->setSizeMode (sizeMode);
		clientView->setSize (Rect (0, 0, size.getWidth (), size.getHeight ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	spacing = 1;
	if(visualStyle)
		spacing = visualStyle->getMetric ("spacing", spacing);

	if(clientView)
		clientView->setSpacing (spacing);

	if(visualStyle)
		freeSpace = visualStyle->getMetric ("freespace", freeSpace);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API DropBox::getController () const
{
	// use model as controller for context menu
	// TODO: use IItemModel::appendItemMenu() instead of IContextMenuHandler???
	return getModel (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::modelChanged (int changeType, ItemIndexRef)
{
	UnknownList items;
	if(model)
		model->getSubItems (items, ItemIndex ());

	ScrollView* scrollView = ScrollView::getScrollView (this);
	if(scrollView && (scrollView->getTarget () != this || !(scrollView->getSizeMode () & kVFitSize)))
		scrollView = nullptr;

	if(items.isEmpty ())
	{
		clientView->removeAll ();
		itemViews.removeAll ();

		// provoke autosizing of scrollview
		if(scrollView)
			scrollView->onChildSized (this, Point ());
		return;
	}

	// suspend resizing layout container during view manipulation
	int clientSizeMode = clientView->getSizeMode ();
	if((clientSizeMode & kFitSize) && !style.isCustomStyle (Styles::kDropBoxLayoutAutoResize))
		clientView->setSizeMode (clientSizeMode & ~kFitSize);

	// check if each item has a corresponding view at the correct index
	int index = 0;		// index in items
	int viewIndex = 0;  // index in itemViews (does not count dummies)
	ForEachUnknown (items, obj)
		View* view = (View*)itemViews.at (index);
		if(view && isEqualUnknown (view->getController (), obj))
		{
			// this is the view for that controller item
			if(!view->canCast (ccl_typeid<DummyView> ()))
				viewIndex++;
		}
		else
		{
			// this is not the view for that item
			View* otherView = getViewByObject (obj);
			if(otherView)
			{
				// found view at other index: move it
				ASSERT (itemViews.index (otherView) > index) // must be at higher index

				itemViews.remove (otherView);
				itemViews.insertAt (index, otherView);

				if(!otherView->canCast (ccl_typeid<DummyView> ()))
				{
					clientView->removeView (otherView);

					Rect size = (otherView->getSize ());
					adjustViewSize (size);
					otherView->setSize (size);

					clientView->insertView (viewIndex, otherView);
					viewIndex++;
				}
			}
			else
			{
				// no view for this item: insert new one
				View* view = createView (obj);
				if(view)
				{
					itemViews.insertAt (index, view);
					view->retain ();

					clientView->insertView (viewIndex, view);
					viewIndex++;
				}
				else
				{
					// no view created, insert dummy view as placeholder
					view = NEW DummyView (obj);
					itemViews.insertAt (index, view);
				}
			}
		}
		index++;
	EndFor

	// remove remaining views
	while(View* view = (View*)itemViews.at (index))
	{
		if(!view->canCast (ccl_typeid<DummyView> ()))
		{
			clientView->removeView (view);
			view->release ();
		}
		itemViews.remove (view);
		view->release ();
		//index++; item has been removed, do not increment index!
	}

	if(clientSizeMode & kFitSize)
	{
		clientView->setSizeMode (clientSizeMode);
		clientView->onViewsChanged ();
		resetSizeLimits ();
	}

	if(style.isCustomStyle (Styles::kDropBoxLayoutAlignEnd))
	{
		// move layout container to end
		Rect clientRect = clientView->getSize ();
		Point offset;
		if(style.isHorizontal ())
			offset.x = size.getWidth () - clientRect.right;
		else
			offset.y = size.getHeight () - clientRect.bottom;
		clientRect.offset (offset);
		clientView->setSize (clientRect);
	}

	recalcSnap ();

	// provoke autosizing of scrollview
	if(scrollView)
		scrollView->onChildSized (this, Point ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::recalcSnap ()
{
	snap = 1;
	
	bool noSnap = getVisualStyle ().getMetric<bool> ("noSnap", false);
	if(noSnap)
		return;
	
	if(itemViews.count () == 1)
	{
		// allow scrolling when only one view
		View* view = (View*)itemViews.at (0);
		Coord viewLength = (style.isHorizontal () ? view->getWidth () : view->getHeight ());
		Coord containerLength = (style.isHorizontal () ? getWidth () : getHeight ());
		Coord scrollRange = viewLength - containerLength;
		if(scrollRange > 2)
		{
			snap = ccl_max (scrollRange / 2, 1);
			ccl_upper_limit (snap, viewLength);
		}
		return;
	}

	// recalc snap: lowest height/width
	bool first = true;
	ForEach (itemViews, View, view)
		Coord viewLength = (style.isHorizontal () ? view->getWidth () : view->getHeight ()) + spacing;
		if(first)
		{
			snap = viewLength;
			first = false;
		}
		else if(viewLength != snap)
		{
			ccl_upper_limit (snap, viewLength);
			break;
		}
	EndFor
	
	ccl_lower_limit (snap, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::calcSizeLimits ()
{
	SuperClass::calcSizeLimits ();

	if(getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews) && getStyle ().isHorizontal () && getSizeMode () & kVFitSize)
	{
		if(sizeLimits.minHeight > 0)
			sizeLimits.setFixedHeight (sizeLimits.minHeight);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::resetClientLimits ()
{
	clientView->resetSizeLimits ();
	onChildLimitsChanged (clientView); // trigger notification upwards
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DropBox::getViewByObject (IUnknown* object) const
{
	ForEach (itemViews, View, v)
		if(isEqualUnknown (v->getController (), object))
			return v;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::adjustViewSize (Rect& size) const
{
	if(getStyle ().isHorizontal ())
	{
		if(!getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews))
			size.setHeight (getSize ().getHeight ());
	}
	else
	{
		if(!getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews))
			size.setWidth (getSize ().getWidth ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* DropBox::createView (IUnknown* object) const
{
	View* result = nullptr;
	UnknownPtr<IViewFactory> viewFactory (model);
	ASSERT (viewFactory != nullptr)
	
	Attributes* dropBoxArguments = nullptr;
	if(auto* dropBoxControl = ccl_cast<DropBoxControl> (ScrollView::getScrollView (this)))
		dropBoxArguments = &dropBoxControl->getDropBoxArguments ();
	
	if(viewFactory)
	{
		// Make a default size rect
		Rect size (0, 0, 10, 10);
		adjustViewSize (size);

		MutableCString itemViewName (name);
		itemViewName.append (kItemSuffix);
		IView* itemView = viewFactory->createView (itemViewName, Variant (object), size);
		if(itemView == nullptr)
			itemView = getTheme ().createView (itemViewName, object, dropBoxArguments);

		if(itemView == nullptr)
			return nullptr;

		result = unknown_cast<View> (itemView);
		ASSERT (result != nullptr && isEqualUnknown (result->getController (), object))

		size = result->getSize ();
		adjustViewSize (size);
		result->setSize (size);
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DropBox::dragItem (ItemIndexRef index, int dragDevice)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorNoDrag))
		return false;

	if(model)
	{
		IUnknown* data = model->createDragSessionData (index);
		if(data)
		{
			AutoPtr<DragSession> session (DragSession::create (this->asUnknown (), dragDevice));
			session->getItems ().add (data, false); // owned by drag session!
            
			if(IImage* dragImage = getDragImageForItem (index))
				session->setDragImage (dragImage, getVisualStyle ().getBackColor ());

			if(style.isCustomStyle (Styles::kDropBoxBehaviorDragNowhereToRemove))
			{
				AutoPtr<IDragHandler> deleteHandler (NEW DeleteItemDragHandler (this, index));
				session->setSourceDragHandler (deleteHandler);
			}

			session->drag ();
            
			if(!session->wasCanceled ())
			{
				if(session->getTargetID () == kTrashBinTargetID)
                {
					session->getItems ().removeAll (); // remove drag data first!
					notifyRemove (index);
				}
			}
			return true;
		}
	}
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DropBox::onGesture (const GestureEvent& event)
{
    if((event.getType () == GestureEvent::kLongPress || event.getType () == GestureEvent::kSwipe)
	   && event.getState () == GestureEvent::kBegin)
    {
        ItemIndex index;
        if(model && findItem (index, event.where))
			if(dragItem (index, DragSession::kTouchInput))
                return true;
    }
    else if(event.getType () == GestureEvent::kSingleTap)
    {
        ItemIndex index;
        if(model && findItem (index, event.where))
		{
			// 1. try open item
            if(openItem (index, -1, event))
                return true;
			
			// 2. try edit item
			Rect itemRect;
			getItemRect(itemRect, index);
			if(editCell (index, -1, itemRect, event))
                return true;
		}
    }
	
    return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DropBox::onMouseDown (const MouseEvent& event)
{
	// select current item
	ItemIndex index;
	if(model && findItem (index, event.where))
		onItemFocused (index);

	if(View::onMouseDown (event))
		return true;

	if(!model || !index.isValid ())
		return false;

	bool leftClick = event.keys.isSet (KeyState::kLButton);

	// try to open current item
	if(leftClick && !style.isCustomStyle (Styles::kItemViewBehaviorNoDoubleClick) && detectDoubleClick (event))
		return openItem (index, -1, event) != 0;

	// try drag current item
	else if(leftClick && !style.isCustomStyle (Styles::kItemViewBehaviorNoDrag) && detectDrag (event))
	{
        if(dragItem (index))
			return true;
	}
	else
	{
		Rect itemRect;
		getItemRect (itemRect, index);
		
		if(editCell (index, -1, itemRect, event))
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DropBox::onContextMenu (const ContextMenuEvent& event)
{
	ItemIndex index;
	
	if(model)
	{
		Rect itemRect;
		if(findItem (index, event.where))
		{
			// call editCell with context menu event (e.g. to allow changing the menu position in the event)
			Rect itemRect;
			getItemRect (itemRect, index);
		
			if(editCell (index, -1, itemRect, event))
				return true; // model->editCell might decide to handle the menu event completely

			return model->appendItemMenu (event.contextMenu, index, getSelection ()) != 0;
		}
		else if(editCell (ItemIndex (-1), -1, itemRect, event))
			return true; // model->editCell might decide to handle the menu event completely
	}
	
	return SuperClass::onContextMenu (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::onSize (const Point& delta)
{
	if(getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews))
	{
		// let layout view fill our client rect
		Rect clientRect;
		getClientRect (clientRect);
		clientView->setSize (clientRect);
		ScopedFlag<kAttachDisabled> disableAttach (sizeMode);
		SuperClass::onSize (delta);
	}
	else
		SuperClass::onSize (delta);

	if(getStyle ().isCustomStyle (Styles::kDropBoxLayoutWrapViews) && !hasExplicitSizeLimits ())
		if(getStyle ().isHorizontal () ? delta.x : delta.y)
			resetClientLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::onChildSized (View* child, const Point& delta)
{
	recalcSnap ();
	updateSize ();

	// sizing is done now; suppress View::checkFitSize, it would resize to 0 when no item is visible
	//SuperClass::onChildSized (child, delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::notifyRemove (ItemIndexRef index)
{
#if 1
	if(model)
		model->removeItem (index);
#else
	Variant v;
	index.toVariant (v);
	Message* m = NEW Message ("removeItem", v);
	m->post (this);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DropBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "removeItem")
	{
		const Variant& v = msg.getArg (0);
		if(model)
			model->removeItem (ItemIndex (v));
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DropBox::findItem (ItemIndex& index, const Point& where) const
{
	int i = 0;
	ForEach (itemViews, View, view)
		// coordinates relative to this view
		Point offset;
		View* v = view;
		while(v && v != this)
		{
			offset.offset (v->getPosition ());
			v = v->getParent ();
		}

		Rect rect;
		view->getVisibleClient (rect);
		if(style.isHorizontal ())
			rect.right += spacing;
		else
			rect.bottom += spacing;
		rect.offset (offset);

		if(rect.pointInside (where))
		{
			index = ItemIndex (i);
			return true;
		}

		i++;
	EndFor

	index = ItemIndex ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DropBox::getItemRect (Rect& rect, ItemIndexRef index, int column) const
{
	View* view = (View*)itemViews.at (index.getIndex ());
	if(view)
	{
		rect = view->getSize ();

		View* p = view->getParent ();
		while(p && p != this)
		{
			rect.offset (p->getPosition ());
			p = p->getParent ();
		}
	}
	else
		rect.setEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API DropBox::getViewItem (ItemIndexRef index)
{
	View* view = (View*)itemViews.at (index.getIndex ());
	return ccl_cast<DummyView> (view) ? nullptr : view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::getSizeInfo (SizeInfo& info)
{
	Rect contentSize;
	if(clientView)
		clientView->calcAutoSize (contentSize);

	Rect clipperSize (parent ? parent->getSize () : getSize ());

	if(style.isHorizontal ())
	{
		if(style.isCustomStyle (Styles::kDropBoxLayoutWrapViews) && parent)
		{
			info.width = parent->getWidth ();
			info.height = contentSize.getHeight ();
		}
		else
		{
			info.width = contentSize.getWidth () + freeSpace;
			info.height = clipperSize.getHeight (); // adjust to available height in clip view
		}
		info.hSnap = snap;
		info.vSnap = 1;
	}
	else
	{
		if(style.isCustomStyle (Styles::kDropBoxLayoutWrapViews) && parent)
		{
			info.width = contentSize.getWidth ();
			info.height = parent->getHeight ();
		}
		else
		{
			info.width = clipperSize.getWidth (); // adjust to available width in clip view
			info.height = contentSize.getHeight () + freeSpace;
		}
		info.hSnap = 1;
		info.vSnap = snap;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DropBox::draw (const UpdateRgn& updateRgn)
{
	if(style.isCustomStyle (Styles::kDropBoxAppearanceColorize))
	{
		GraphicsPort port (this);
		port.fillRect (updateRgn.bounds, getVisualStyle ().getBackBrush ());
	}
	
	SuperClass::draw (updateRgn);
}
