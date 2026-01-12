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
// Filename    : ccl/gui/itemviews/itemviewbase.cpp
// Description : Basic Item View
//
//************************************************************************************************

#include "ccl/gui/itemviews/itemviewbase.h"

#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/system/dragndrop.h"

#include "ccl/base/message.h"

#include "ccl/public/base/iformatter.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"
#include "ccl/public/gui/framework/itemviewgeometry.h"

using namespace CCL;

namespace CCL {

//************************************************************************************************
// ItemListSelectionIterator
//************************************************************************************************

class ItemListSelectionIterator: public Unknown,
								 public IItemSelectionIterator
{
public:
	ItemListSelectionIterator (const LinkedList<ItemIndex>& items)
	: iter (items)
	{}

	// IItemSelectionIterator
	tbool CCL_API next (ItemIndex& index) override
	{
		if(iter.done ())
			return false;
		index = iter.next ();
		return true;
	}

	CLASS_INTERFACE (IItemSelectionIterator, Unknown)

private:
	ListIterator<ItemIndex> iter;
};

} // namespace CCL

//************************************************************************************************
// ItemViewBase::ItemDragHandler
//************************************************************************************************

class ItemViewBase::ItemDragHandler: public Unknown,
									 public AbstractDragHandler,
									 public IItemViewDragHandler
{
public:
	ItemDragHandler (ItemViewBase& itemView, int flags, IItemDragVerifier* verifier = nullptr);

	PROPERTY_FLAG (flags, IItemView::kCanDragBetweenItems, canDragBetween)
	PROPERTY_FLAG (flags, IItemView::kCanDragOnItem, canDragOnItem)
	PROPERTY_FLAG (flags, IItemView::kCanDragPrePostItems, canDragPrePostItems)
	PROPERTY_FLAG (flags, IItemView::kDropInsertsData, isDropEnabled)
	PROPERTY_FLAG (flags, IItemView::kDragWithItemIcon, shouldDragWithItemIcon)
	bool needsSprite () const { return canDragBetween () || canDragOnItem (); }

	// IDragHandler
	tbool CCL_API hasVisualFeedback () const override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;
	tbool CCL_API dragLeave (const DragEvent& event) override;

	// IItemViewDragHandler
	tbool CCL_API getTarget (ItemIndex& item, int& relation) override;

	CLASS_INTERFACE2 (IDragHandler, IItemViewDragHandler, Unknown)

protected:
	struct Target
	{
		ItemIndex item;
		int relation;

		Target (int relation = kOnItem)	: item (), relation (relation) {}
		bool isValid () const { return item.isValid () || relation != kOnItem; }
		bool operator == (const Target& t) const { return t.item == item && t.relation == relation; }
		bool operator != (const Target& t) const { return !(t.item == item) || t.relation != relation; }
	};

	ItemViewBase& itemView;
	ItemViewGeometry geometry;
	IItemDragVerifier* verifier;
	int flags;

	SolidDrawable* sDrawable;
	AutoPtr<IDrawable> drawable;
	Target target;

	bool dragOverWithSprite (const DragEvent& event);
};

//************************************************************************************************
// ItemViewBase::ItemDragHandler
//************************************************************************************************

ItemViewBase::ItemDragHandler::ItemDragHandler (ItemViewBase& itemView, int flags, IItemDragVerifier* verifier)
: itemView (itemView),
  verifier (verifier),
  flags (flags),
  sDrawable (nullptr)
{
	if(needsSprite ())
	{
		geometry.setVertical (itemView.getStyle ().isVertical ());
		
		IImage* indicatorImage = itemView.getVisualStyle ().getImage ("indicatorImage");
		
		if(indicatorImage)
		{
			drawable = NEW ImageDrawable (indicatorImage);
			geometry.setIndicatorWidth (itemView.getStyle ().isVertical () ? indicatorImage->getHeight () : indicatorImage->getWidth ());
		}
		else
		{
			Color c (itemView.getTheme ().getThemeColor (ThemeElements::kAlphaCursorColor));
			drawable = sDrawable = NEW SolidDrawable (SolidBrush (c));
			geometry.setIndicatorWidth (2);
		}
		
		sprite = NEW FloatingSprite (&itemView, drawable, Rect ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::ItemDragHandler::getTarget (ItemIndex& item, int& relation)
{
	item = target.item;
	relation = target.relation;

	// simplify: translate kAfterItem to kBeforeItem for index-based ItemIndex
	if(relation == kAfterItem)
	{
		int i = 0;
		if(item.getIndex (i))
		{
			item = ++i;
			relation = kBeforeItem;
		}
	}
	return target.isValid ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::ItemDragHandler::hasVisualFeedback () const
{
	if(shouldDragWithItemIcon ())
		return false;
	else
		return AbstractDragHandler::hasVisualFeedback ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::ItemDragHandler::dragOver (const DragEvent& event)
{
	if(needsSprite ())
		return dragOverWithSprite (event);
	else
	{
		if(isDropEnabled ())
		{
			if(IItemModel* model = itemView.getModel ())
			{
				ItemIndex index (-1);
				int column = -1;
				itemView.findItemCell (index, column, event.where);

				if(!(index == target.item))
				{
					target.item = index;

					if(model->canInsertData (index, column, event.session.getItems (), &event.session, &itemView))
					{
						if(event.session.getResult () == DragSession::kDropNone)
							event.session.setResult (DragSession::kDropCopyReal);
					}
					else
						event.session.setResult (DragSession::kDropNone);
				}
			}
		}
		return AbstractDragHandler::dragOver (event);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewBase::ItemDragHandler::dragOverWithSprite (const DragEvent& event)
{
	bool canDragBetween = this->canDragBetween ();
	bool canDragOnItem  = this->canDragOnItem ();
	bool canDragPrePostItems  = this->canDragPrePostItems ();

	Target newTarget;
	bool upperHalf = false;

	Rect rect;
	if(itemView.findItem (newTarget.item, event.where))
	{
		itemView.getItemRect (rect, newTarget.item);
		newTarget.relation = geometry.getRelation (upperHalf, rect, event.where);
	}
	else if(canDragBetween)
	{
		if(canDragPrePostItems)
		{
			itemView.getItemRect (rect, target.item);
			if(!rect.isEmpty ())
			{
				geometry.getRelation (upperHalf, rect, event.where);
				newTarget.relation = upperHalf ? kBeforeItem : kAfterItem; // before first or after last item
			}
			else
				newTarget.relation = kAfterItem; // after last item
		}
		else
			newTarget.relation = kAfterItem; // after last targetItem
	}
	
	// check flags
	if(newTarget.relation == kOnItem && !canDragOnItem && canDragBetween)
		newTarget.relation = upperHalf ? kBeforeItem : kAfterItem;

	else if((newTarget.relation == kBeforeItem || newTarget.relation == kAfterItem) && !canDragBetween && canDragOnItem)
		newTarget.relation = kOnItem;

	// check verifier
	if(verifier)
	{
		ItemIndex originalItem (newTarget.item);
		if(verifier->verifyTargetItem (newTarget.item, newTarget.relation))
		{
			if(!(newTarget.item == originalItem))
				itemView.getItemRect (rect, newTarget.item);

			if(newTarget.relation == kBeforeOrAfterItem)
				newTarget.relation = upperHalf ? kBeforeItem : kAfterItem;
		}
		else
			newTarget = Target (), rect.setEmpty ();
	}

	if(newTarget != target)
	{
		target = newTarget;

		if(!target.isValid ())
			hideSprite ();
		else
		{
			if(target.relation == kFullView)
			{
				// highlite full view
				itemView.getVisibleClient (rect);
			}
			else
			{
				Rect containerSize;
				itemView.calcAutoSize (containerSize);
				if(itemView.getStyle ().isVertical ())
					containerSize.setWidth (itemView.getWidth ());
				else
					containerSize.setHeight (itemView.getHeight ());

				rect = geometry.calcSpriteSize (containerSize, rect, target.relation);
			}
			
			if(sDrawable)
			{
				Color c (sDrawable->getBrush ().getColor ());
				c.setAlphaF ((target.relation == kOnItem || target.relation == kFullView) ? .4f : .75f);
				sDrawable->setBrush (c);
				sDrawable->takeOpacity ();
			}

			moveSprite (rect);
		}

		itemView.onDragOverItem (event, target.relation == kOnItem ? target.item : ItemIndex ());
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::ItemDragHandler::afterDrop (const DragEvent& event)
{
	if(isDropEnabled ())
	{
		bool dropped = false;

		if(IItemModel* model = itemView.getModel ())
		{
			ItemIndex index (-1);
			int column = -1;
			itemView.findItemCell (index, column, event.where);
			dropped = model->insertData (index, column, event.session.getItems (), &event.session) != 0;
			if(dropped)
			{
				if(event.session.getResult () == DragSession::kDropNone)
					event.session.setResult (DragSession::kDropCopyReal);
			}
		}

		AbstractDragHandler::afterDrop (event);
		return dropped;
	}
	else
		return AbstractDragHandler::afterDrop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::ItemDragHandler::dragLeave (const DragEvent& event)
{
	if(isDropEnabled ())
	{
		// reset target identifier
		event.session.setTargetID (nullptr);
	}
	return AbstractDragHandler::dragLeave (event);
}

//************************************************************************************************
// ItemViewBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ItemViewBase, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewBase::ItemViewBase (const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  model (nullptr),
  selection (nullptr),
  editModeParam (nullptr)
{
	nameNavigator.init (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewBase::onKeyDown (const KeyEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorNoNameNavigation) == false)
	{
		if(!Unicode::isPrintable (event.character))
			return false;

		Variant targetItem;
		if(nameNavigator.onKey (targetItem, event))
		{
			setFocusItem (ItemIndex (targetItem), true);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* ItemViewBase::createTouchHandler (const TouchEvent& event)
{
	if(privateFlags & kTouchHandlerDisabled)
		return nullptr;

	// find item at (main) touch
	bool itemFound = false;
	ItemIndex index (-1);
	int column = -1;
	Point where;
	if(const TouchInfo* touch = event.touches.getTouchInfoByID (event.touchID))
	{
		Point where (touch->where);
		windowToClient (where);
		itemFound = model && findItemCell (index, column, where);
	}

	if(model)
	{
		Rect rect;
		getItemRect (rect, index);
		IItemModel::EditInfo editInfo (makeEditInfo (rect, event));
		if(ITouchHandler* handler = model->createTouchHandler (index, column, editInfo))
			return handler;
	}

	if(itemFound)
	{
		// want to start dragging on long press
		GestureHandler* handler = NEW GestureHandler (this, GestureEvent::kSingleTap);
		if(!style.isCustomStyle (Styles::kItemViewBehaviorNoDrag))
			handler->addRequiredGesture (GestureEvent::kLongPress, GestureEvent::kPriorityHigh);
			
		// also want to drag on swipe: with high priority when specified as style, otherwise only with normal priority in "other" direction
		if(style.isCustomStyle (Styles::kItemViewBehaviorDragSwipeH))
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal, GestureEvent::kPriorityHigh);
		else if(style.isVertical ())
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kHorizontal);
			
		if(style.isCustomStyle (Styles::kItemViewBehaviorDragSwipeV))
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical, GestureEvent::kPriorityHigh);
		else if(style.isHorizontal ())
			handler->addRequiredGesture (GestureEvent::kSwipe|GestureEvent::kVertical);

		return handler;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewBase::~ItemViewBase ()
{
	if(model)
	{
		ISubject::removeObserver (model, this);
		model->viewDetached (this);
		model->release ();
	}

	if(selection)
		selection->release ();

	setEditModeParam (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::attached (View* parent)
{
	SuperClass::attached (parent);

	UnknownPtr<IObserver> observer (model);
	if(observer)
		observer->notify (this, Message (IItemView::kViewAttached));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::removed (View* parent)
{
	UnknownPtr<IObserver> observer (model);
	if(observer)
		observer->notify (this, Message (IItemView::kViewRemoved));

	if(editModeParam)
		editModeParam->setValue (false);

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::setModel (IItemModel* _model)
{
	if(model)
	{
		ISubject::removeObserver (model, this);
		model->viewDetached (this);
		model->release ();
	}

	model = _model;
	
	if(model)
	{
		ISubject::addObserver (model, this);
		model->retain ();
		model->viewAttached (this);
		
		modelChanged (kModelAssigned, ItemIndex ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* CCL_API ItemViewBase::getModel () const
{
	return model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IItemSelection& CCL_API ItemViewBase::getSelection () const
{
	if(!selection)
	{
		selection = model ? model->getSelection () : nullptr;
		if(selection)
			selection->retain ();
		else
			selection = createSelection ();
	}
	return *selection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemSelection* ItemViewBase::createSelection () const
{
	return NEW ItemListSelection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::selectItem (ItemIndexRef index, tbool state)
{
	if(index.isValid () == false)
		return false;

	getSelection ();
	if(state)
	{
		if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
			selectAll (false);
		selection->select (index);
	}
	else
		selection->unselect (index);
	
	if(state)
		onItemFocused (index);

	invalidateItem (index);
	signalSelectionChanged ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
tbool CCL_API ItemViewBase::selectAll (tbool state)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::signalSelectionChanged ()
{
	if(privateFlags & kSuspendSelectSignal)
		return;

	UnknownPtr<IObserver> observer (model);
	if(observer)
		observer->notify (this, Message (IItemView::kSelectionChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::removeItem (ItemIndexRef index)
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::findItems (const Rect& rect, IItemSelection& items) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::findItem (ItemIndex& index, const Point& where) const
{
	ItemListSelection items;
	if(findItems (Rect (where.x, where.y, where.x + 1, where.y + 1), items))
	{
		ForEachItem (items, idx)
			index = idx;
			return true;
		EndFor
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::findItemCell (ItemIndex& row, int& column, const Point& where) const
{
	column = 0;
	return findItem (row, where);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::getItemRect (Rect& rect, ItemIndexRef index, int column) const
{
	rect.setEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::getFocusItem (ItemIndex& index) const
{
	ASSERT (0)
	index = ItemIndex ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::setFocusItem (ItemIndexRef index, tbool selectExclusive)
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::invalidateItem (ItemIndexRef index)
{
	ASSERT (0)
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::makeItemVisible (ItemIndexRef index)
{
	Rect rect;
	if(index.getIndex () >= 0)
	{
		getItemRect (rect, index);
		makeVisible (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::setEditControl (IView* view, tbool directed)
{
	ASSERT (0)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::setEditModeParam (IParameter* parameter)
{
	ISubject::removeObserver (editModeParam, this);
	editModeParam = parameter;
	ISubject::addObserver (editModeParam, this);

	if(editModeParam)
		onEditModeChanged (editModeParam->getValue ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::setEditMode (bool state)
{
	if(state != isEditMode ())
	{
		if(editModeParam)
			editModeParam->setValue (state);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::onEditModeChanged (bool state)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::setDeleteFocusItemMode (bool state, ItemIndexRef item)
{
	isDeleteFocusItemMode (state);
	if(state)
	{
		isEditMode (false);

		ASSERT (item.isValid ())
		setFocusItem (item);
	}

	if(editModeParam)
		editModeParam->setValue (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Font& ItemViewBase::getFont (Font& font) const
{
	return font = getVisualStyle ().getTextFont ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel::EditInfo ItemViewBase::makeEditInfo (RectRef rect, const GUIEvent& editEvent)
{
	Font font;
	getFont (font);
	Brush textBrush (getVisualStyle ().getTextBrush ());
	Brush backBrush (getVisualStyle ().getBackBrush ());
	
	IItemModel::StyleInfo styleInfo = {font, textBrush, backBrush, 0};
	IItemModel::EditInfo editInfo = {this, rect, styleInfo, editEvent};
	return editInfo;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool ItemViewBase::editCell (ItemIndexRef item, int column, RectRef rect, const GUIEvent& editEvent)
{
	privateFlags &= ~kOpenItemCalled;

	IItemModel::EditInfo editInfo (makeEditInfo (rect, editEvent));
	return model->editCell (item, column, editInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewBase::openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect)
{
	privateFlags |= kOpenItemCalled;

	Rect r (rect);
	if(r.isEmpty ())
		getItemRect (r, item);

	IItemModel::EditInfo editInfo (makeEditInfo (r, editEvent));
	return model->openItem (item, column, editInfo) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::beginMouseHandler (IMouseHandler* handler, const MouseEvent& mouseEvent)
{
	MouseHandlerDelegate* handlerDelegate = handler ? NEW MouseHandlerDelegate (this, handler) : nullptr;
	getWindow ()->setMouseHandler (handlerDelegate);
	if(handlerDelegate)
		handlerDelegate->begin (mouseEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ItemViewBase::getDragImageForItem (ItemIndexRef item)
{
	return model ? model->getItemIcon (item) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* CCL_API ItemViewBase::createDragHandler (int flags, IItemDragVerifier* verifier)
{
	return NEW ItemDragHandler (*this, flags, verifier);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::getSizeInfo (SizeInfo& info)
{
	ASSERT (0)
	info = SizeInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::calcAutoSize (Rect& r)
{
	SizeInfo info;
	getSizeInfo (info);
	r (0, 0, info.width, info.height);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::updateSize (bool recalc) 
{
	if(recalc)
	{
		if(!model)
			return;

		getSizeInfo (sizeInfo);
		
		if(hasExplicitSizeLimits ())
		{
			// respect sizeLimits if explicitely given
			const SizeLimit& limits (getSizeLimits ());
			sizeInfo.width  = ccl_bound (sizeInfo.width, limits.minWidth, limits.maxWidth);
			sizeInfo.height = ccl_bound (sizeInfo.height, limits.minHeight, limits.maxHeight);
		}
	}

	if(ScrollView* sv = ScrollView::getScrollView (this))
	{
		Point snap (sizeInfo.hSnap, sizeInfo.vSnap);
		if(snap != sv->getSnap ())
			sv->setSnap (snap);
		
		sv->setTargetSize (Rect (0, 0, sizeInfo.width, sizeInfo.height));
	}
	else
	{
		Rect r (getSize ());
		if(sizeMode & kHFitSize)
			r.setWidth (sizeInfo.width);
		if(sizeMode & kVFitSize)
			r.setHeight (sizeInfo.height);
		setSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemViewBase::notify (ISubject* subject, MessageRef msg)
{
	// When the destroyed message is send, isEqualUnknown is illegal to call
	if(msg != kDestroyed && isEqualUnknown (model, subject))
	{
		if(msg == IItemModel::kItemAdded)
			modelChanged (kItemAdded, ItemIndex (msg[0]));
		else if(msg == IItemModel::kItemRemoved)
			modelChanged (kItemRemoved, ItemIndex (msg[0]));
		else if(msg == IItemModel::kItemModified)
			modelChanged (kItemModified, ItemIndex (msg[0]));
		else if(msg == kChanged)
			modelChanged (kModelChanged, ItemIndex ());
	}
	else if(msg == kChanged && UnknownPtr<IParameter> (subject) == editModeParam)
	{
		if(editModeParam->getValue ())
		{
			if(!isDeleteFocusItemMode ())
				isEditMode (true);
		}
		else
		{
			isEditMode (false);
			isDeleteFocusItemMode (false);
		}
		onEditModeChanged (isEditMode ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::modelChanged (int changeType, ItemIndexRef item)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::onItemFocused (ItemIndexRef item)
{
	if(model)
		model->onItemFocused (item);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewBase::onDragEnter (const DragEvent& event)
{
	if(model)
	{
		ItemIndex index (-1);
		int column = -1;
		findItemCell (index, column, event.where);

		if(model->canInsertData (index, column, event.session.getItems (), &event.session, this))
		{
			if(event.session.getResult () == DragSession::kDropNone)
				event.session.setResult (DragSession::kDropCopyReal);

			if(IDragHandler* dragHandler = event.session.getDragHandler ())
				dragHandler->dragEnter (event);
			else
				// default behavior: no visual feedback + call IItemModel::insertData() on drop
				event.session.setDragHandler (AutoPtr<IDragHandler> (NEW ItemDragHandler (*this, IItemView::kDropInsertsData)));

			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::getStartItem (Variant& item, String& name)
{
	ItemIndex index;
	getFocusItem (index);

	// get item after focus, or first item if none
	if(getNextItem (index))
	{
		index.toVariant (item);
		if(model)
			model->getItemTitle (name, index);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::getNextItem (Variant& item, String& name)
{
	ItemIndex index (item);
	if(getNextItem (index))
	{
		index.toVariant (item);
		if(model)
			model->getItemTitle (name, index);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemViewBase::getNextItem (ItemIndex& item, bool forNavigation)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(style.isCustomStyle (Styles::kItemViewAppearanceRedrawOnResize|Styles::kItemViewBehaviorSelectFullWidth))
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::onDragOverItem (const DragEvent& event, ItemIndexRef index)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemViewBase::disableTouchHandler (bool disable)
{
	if(disable)
		privateFlags |= kTouchHandlerDisabled;
	else
		privateFlags &= ~kTouchHandlerDisabled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ItemViewBase)
	DEFINE_METHOD_ARGR ("selectItem", "index, state=true", "bool")
	DEFINE_METHOD_ARGR ("getFocusItem", "", "ItemIndex")
	DEFINE_METHOD_ARGR ("setFocusItem", "index, selectExclusive=true", "ItemIndex")
	DEFINE_METHOD_ARGR ("invalidateItem", "index", "bool")
END_METHOD_NAMES (ItemViewBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewBase::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "selectItem")
	{
		ItemIndex index;
		index.fromVariant (msg[0]);
		bool state = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		returnValue = selectItem (index, state);
		return true;
	}
	else if(msg == "getFocusItem")
	{
		ItemIndex index;
		getFocusItem (index);
		index.toVariant (returnValue);
		return true;
	}
	else if(msg == "setFocusItem")
	{
		ItemIndex index;
		index.fromVariant (msg[0]);
		bool selectExclusive = msg.getArgCount () > 1 ? msg[1].asBool () : true;
		returnValue = setFocusItem (index, selectExclusive);
		return true;
	}
	else if(msg == "invalidateItem")
	{
		ItemIndex index;
		index.fromVariant (msg[0]);
		returnValue = invalidateItem (index);
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ItemControlBase
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ItemControlBase, ScrollView)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemControlBase::ItemControlBase (const Rect& size, ItemViewBase* itemView, StyleRef scrollViewStyle)
: ScrollView (size, itemView, scrollViewStyle)
{
	ASSERT (itemView)
	if((scrollViewStyle.custom & Styles::kScrollViewBehaviorCenterTarget) == 0)
		style.custom |= Styles::kScrollViewBehaviorExtendTarget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemViewBase* ItemControlBase::getItemView () const
{
	return (ItemViewBase*)target;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ItemControlBase::queryInterface (UIDRef iid, void** ptr)
{
	// delegate to ItemView
	if(iid == ccl_iid<IItemView> ())
		if(ItemViewBase* itemView = getItemView ())
			return itemView->queryInterface (iid, ptr);

	return ScrollView::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::setTheme (Theme* theme)
{
	SuperClass::setTheme (theme);

	if(ItemViewBase* itemView = getItemView ())
		itemView->setTheme (theme);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::setStyle (StyleRef newStyle)
{
	StyleFlags s (newStyle);
	s.custom |= Styles::kScrollViewBehaviorExtendTarget;
	SuperClass::setStyle (s);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	if(ItemViewBase* itemView = getItemView ())
		itemView->setVisualStyle (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::setName (StringRef name)
{
	SuperClass::setName (name);

	if(ItemViewBase* itemView = getItemView ())
		itemView->setName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::setTitle (StringRef title)
{
	SuperClass::setTitle (title);

	if(ItemViewBase* itemView = getItemView ())
		itemView->setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControlBase::onSize (const Point& delta)
{
	SuperClass::onSize (delta);

	if(ItemViewBase* itemView = getItemView ())
		itemView->updateSize (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemControlBase::setZoomFactor (float factor)
{
	SuperClass::setZoomFactor (factor);

	if(ItemViewBase* itemView = getItemView ())
		itemView->setZoomFactor (factor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemControlBase::takeFocus (tbool directed)
{
	if(ItemViewBase* itemView = getItemView ())
		return itemView->takeFocus (directed);

	return SuperClass::takeFocus (directed);
}

//************************************************************************************************
// ItemListSelection
//************************************************************************************************

DEFINE_CLASS (ItemListSelection, Object)
DEFINE_CLASS_UID (ItemListSelection, 0x7764797A, 0xB532, 0x48E3, 0x98, 0x1A, 0x22, 0x74, 0x91, 0x70, 0x0C, 0x61)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemListSelection::ItemListSelection ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemListSelection::ItemListSelection (const ItemListSelection& selection)
{
	ForEachItem (selection, idx)
		items.append (idx);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemListSelection::clone (IItemSelection*& selection) const
{
	selection = NEW ItemListSelection (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemListSelection::isEmpty () const
{
	return items.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemListSelection::isMultiple () const
{
	return items.isMultiple ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemListSelection::isSelected (ItemIndexRef index) const
{
	return items.contains (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemSelectionIterator* CCL_API ItemListSelection::newIterator () const
{
	return NEW ItemListSelectionIterator (items);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemListSelection::select (ItemIndexRef index)
{
	ASSERT (!items.contains (index))
	items.append (index); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemListSelection::unselect (ItemIndexRef index)
{
	return items.remove (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemListSelection::unselectAll ()
{
	items.removeAll ();
}

//************************************************************************************************
// ParamItemModel
//************************************************************************************************

DEFINE_CLASS (ParamItemModel, Object)
DEFINE_CLASS_UID (ParamItemModel, 0xe760eb99, 0xccef, 0x4be4, 0xac, 0x79, 0xb, 0x58, 0xb2, 0xdf, 0x29, 0x65)

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamItemModel::ParamItemModel (StringID name, IParameter* parameter)
: source (parameter)
{
	ISubject::addObserver (source, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ParamItemModel::~ParamItemModel ()
{
	ISubject::removeObserver (source, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ParamItemModel::countFlatItems ()
{
	if(source)
		return source->getMax ().asInt () - source->getMin ().asInt () + 1;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::getItemTitle (String& title, ItemIndexRef index)
{
	if(source == nullptr)
		return false;

	source->getString (title, index.getIndex ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::onItemFocused (ItemIndexRef index)
{
	if(source == nullptr)
		return false;
	source->setValue (index.getIndex (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::openItem (ItemIndexRef index, int column, const EditInfo& info)
{
	if(source == nullptr)
		return false;
	source->setValue (index.getIndex (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::editCell (ItemIndexRef index, int column, const EditInfo& info)
{
	if(source == nullptr)
		return false;

	// ignore other gesture than single tap
	if(auto* gestureEvent = info.editEvent.as<GestureEvent> ())
		if(gestureEvent->getType () != GestureEvent::kSingleTap || gestureEvent->getState () != GestureEvent::kBegin)
			return false;

	source->setValue (index.getIndex (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemSelection* CCL_API ParamItemModel::getSelection ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ParamItemModel::clone (IItemSelection*& selection) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::isEmpty () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::isMultiple () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::isSelected (ItemIndexRef index) const
{
	if(source)
		return source->getValue ().asInt () == index.getIndex ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemSelectionIterator* CCL_API ParamItemModel::newIterator () const
{	
	class ParamItemIterator: public Unknown,
						     public IItemSelectionIterator
	{
	public:
		ParamItemIterator (IParameter* p)
		: source (p)
		{}

		// IItemSelectionIterator
		tbool CCL_API next (ItemIndex& index) override
		{
			if(source)
			{
				index = ItemIndex (source->getValue ().asInt ());
				source = nullptr;
				return true;
			}			
			return false;
		}

		CLASS_INTERFACE (IItemSelectionIterator, Unknown)
	private:
		SharedPtr<IParameter> source;
	};

	return NEW ParamItemIterator (source);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ParamItemModel::select (ItemIndexRef index)
{
	if(source)
		source->setValue (index.getIndex (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ParamItemModel::unselect (ItemIndexRef index)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ParamItemModel::unselectAll ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ParamItemModel::notify (ISubject* subject, MessageRef msg)
{
	if(isEqualUnknown (source, subject))
		signal (msg);

	else if(msg == IItemView::kViewAttached)
	{
		UnknownPtr<IItemView> itemView (subject);
		if(itemView && source)
		{
			ItemIndex idx (source->getValue ().asInt ());
			itemView->makeItemVisible (idx);
		}
	}
}
