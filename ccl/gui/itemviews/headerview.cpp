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
// Filename    : ccl/gui/itemviews/headerview.cpp
// Description : Header View
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/itemviews/headerview.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/viewaccessibility.h"
#include "ccl/gui/theme/renderer/headerviewrenderer.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/popup/menu.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/attributes.h"

#include "ccl/public/text/translation.h"

#include "ccl/public/gui/iviewstate.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/gui/framework/themeelements.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_IID_ (IColumnCalculator, 0xd593b197, 0xe793, 0x4419, 0x82, 0x9f, 0x9, 0x2c, 0x38, 0xfa, 0xb0, 0x8c)
DEFINE_IID_ (IHeaderViewRenderer, 0xC119BA7E, 0xC9DC, 0x4924, 0xA4, 0x58, 0xA1, 0xED, 0x2A, 0xA3, 0x99, 0x01)

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace CCL {

enum ColumnActions { kNone, kSize, kMove, kSort };

enum { kSizeHandleWidth = 3, kMinColumnWidth = 5, };

//************************************************************************************************
// ColumnPositionDrawable
//************************************************************************************************

class ColumnPositionDrawable: public Unknown,
							  public AbstractDrawable
{
public:
	ColumnPositionDrawable (Color color)
	: color (color)
	{}

	// IDrawable
	void CCL_API draw (const DrawArgs& args) override
	{
		Rect rect (args.size);
		if(!rect.isEmpty ())
			args.graphics.drawRect (rect, Pen (color, 3));
	}

	CLASS_INTERFACE (IDrawable, Unknown)

protected:
	Color color;
};

//************************************************************************************************
// SizeColumnMouseHandler
//************************************************************************************************

class SizeColumnMouseHandler: public MouseHandler
{
public:
	SizeColumnMouseHandler (HeaderView* headerView, ColumnHeader* column)
	: MouseHandler (headerView, kAutoScrollH),
	  headerView (headerView),
	  column (column),
	  initialWidth (column->getWidth ())
	{
		canEscape (true);
	}

	bool onMove (int moveFlags) override
	{
		headerView->getColumnHeaders ()->setColumnWidth (column, initialWidth + current.where.x - first.where.x);
		return true;
	}

	void onRelease (bool canceled) override
	{
		if(canceled)
			headerView->getColumnHeaders ()->setColumnWidth (column, initialWidth);
		else
			headerView->storeColumnState ();
	}

protected:
	HeaderView* headerView;
	ColumnHeader* column;
	int initialWidth;
};

//************************************************************************************************
// MoveColumnMouseHandler
//************************************************************************************************

class MoveColumnMouseHandler: public MouseHandler
{
public:
	MoveColumnMouseHandler (HeaderView* headerView, ColumnHeader* column)
	: MouseHandler (headerView, kAutoScroll),
	  headerView (headerView),
	  column (column),
	  offset (0)
	{
		canEscape (true);
	}

	void onBegin () override
	{
		Rect columnRect;
		headerView->getColumnRect (columnRect, column);

		// header sprite
		Coord colW = columnRect.getWidth ();
		Coord colH = columnRect.getHeight ();
		AutoPtr<Bitmap> bitmap (NEW Bitmap (colW, colH));
		UnknownPtr<IHeaderViewRenderer> headerRenderer (ccl_as_unknown (headerView->getRenderer ()));
		if(headerRenderer)
		{
			const IVisualStyle& vs = headerView->getVisualStyle ();
			Font font (vs.getTextFont ());
			SolidBrush textBrush (vs.getTextBrush ());

			BitmapGraphicsDevice port (bitmap);
			headerRenderer->drawHeader (headerView, port, Rect (0, 0, colW, colH), column->getTitle (), textBrush, font);
		}
		AutoPtr<IDrawable> drawable = NEW ImageDrawable (bitmap, .8f);
		headerSprite = Sprite::createSprite (headerView, drawable, columnRect);

		// position sprite
		//Color color = headerView->getTheme ().getThemeColor (ThemeElements::kAlphaCursorColor);
		Color color (Colors::kBlue);
		color.setAlphaF (.5f);
		drawable = NEW ColumnPositionDrawable (color);
		positionSprite = Sprite::createSprite (headerView, drawable, columnRect.setWidth (kSizeHandleWidth));

		offset = current.where.x - columnRect.left;
	}

	bool onMove (int moveFlags) override
	{
		if(!headerSprite->isVisible ())
		{
			positionSprite->show ();
			headerSprite->show ();
		}

		headerSprite->moveTo (Point (current.where.x - offset, 0));

		int position = headerView->getVisibleTargetColumnPos (column, current.where.x);
		Rect targetColumnRect;
		headerView->getColumnRect (targetColumnRect, headerView->getColumnHeaders ()->getColumnAtPosition (position, true));
		positionSprite->moveTo (Point (targetColumnRect.left, 0));
		return true;
	}

	void onRelease (bool canceled) override
	{
		headerSprite->hide ();
		positionSprite->hide ();

		if(!canceled)
		{
			int position = headerView->getVisibleTargetColumnPos (column, current.where.x);
			headerView->getColumnHeaders ()->moveColumn (column, position);
			headerView->storeColumnState ();
		}
	}

protected:
	HeaderView* headerView;
	ColumnHeader* column;
	AutoPtr<Sprite> headerSprite;
	AutoPtr<Sprite> positionSprite;
	Coord offset;
};

//************************************************************************************************
// HeaderViewAccessibilityProvider
//************************************************************************************************

class HeaderViewAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (HeaderViewAccessibilityProvider, ViewAccessibilityProvider)

	HeaderViewAccessibilityProvider (HeaderView& headerView);

	void rebuildColumnProviders ();
	
	void getElementName (String& name, int columnIndex) const;
	void getElementBounds (Rect& rect, int columnIndex) const;

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;

private:
	HeaderView& getHeaderView () const;
};

//************************************************************************************************
// ColumnHeaderAccessibilityProvider
//************************************************************************************************

class ColumnHeaderAccessibilityProvider: public AccessibilityProvider
{
public:
	ColumnHeaderAccessibilityProvider (HeaderViewAccessibilityProvider& parent, int index);

	// AccessibilityProvider
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	void CCL_API getElementName (String& name) const override;
	AccessibilityElementRole CCL_API getElementRole () const override;
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override;
	View* getView () const override;

private:
	HeaderViewAccessibilityProvider& parent;
	int index;
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("HeaderView")
	XSTRING (FitColumn, "Size Column to Fit")
	XSTRING (FitAllColumns, "Size All Columns to Fit")
	XSTRING (ResetColumns, "Reset All Columns")
END_XSTRINGS

//************************************************************************************************
// ColumnHeader
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ColumnHeader, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader::ColumnHeader (int _width, StringRef title, StringID columnId, int _minWidth, int flags)
: width (_width),
  minWidth (_minWidth),
  defaultWidth (-1),
  title (title),
  columnId (columnId),
  flags (flags),
  index (0)
{
	if(minWidth == 0)
		minWidth = width;
	else if(width != IColumnHeaderList::kAutoWidth)
		ccl_lower_limit (width, minWidth);
	defaultWidth = width;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader::ColumnHeader (const ColumnHeader& h)
: width (h.width),
  minWidth (h.minWidth),
  defaultWidth (h.defaultWidth),
  title (h.title),
  columnId (h.columnId),
  flags (h.flags),
  index (h.index)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeader::compare (const Object& obj) const
{
	const ColumnHeader& h = static_cast<const ColumnHeader&> (obj);
	return index - h.index;
}

//************************************************************************************************
// ColumnHeaderList
//************************************************************************************************

DEFINE_CLASS (ColumnHeaderList, Object)
DEFINE_CLASS_UID (ColumnHeaderList, 0xE0C5B54B, 0xBAA3, 0x4DAA, 0xBE, 0x2D, 0xE1, 0x4C, 0xB5, 0x0D, 0x56, 0x13)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeaderList::ColumnHeaderList ()
{
	columns.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& ColumnHeaderList::getColumns ()
{
	return columns;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::copyFrom (const IColumnHeaderList& _other)
{
	columns.removeAll ();
	indexColumns.removeAll ();

	ColumnHeaderList* other = unknown_cast<ColumnHeaderList> (&_other);
	if(other)
		ArrayForEachFast (other->getColumns (), ColumnHeader, c)
			ColumnHeader* c2 = NEW ColumnHeader (*c);
			columns.add (c2);
			indexColumns.addSorted (c2);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ColumnHeaderList::getColumnCount () const
{ 
	return columns.count (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::getCount (bool visible) const
{ 
	if(visible)
	{
		int count = 0;
		ArrayForEachFast (columns, ColumnHeader, c)
			if(!c->isHidden ())
				count++;
		EndFor
		return count;
	}
	else
		return columns.count (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::removeAll ()
{
	columns.removeAll ();
	indexColumns.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader* ColumnHeaderList::getColumnByIndex (int columnIndex) const
{
#if 1
	return (ColumnHeader*)indexColumns.at (columnIndex);
#else
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getIndex () == columnIndex)
			return c;
	EndFor
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader* ColumnHeaderList::getColumnAtPosition (int position, bool visible) const
{
	if(visible)
	{
		int i = 0;
		ArrayForEachFast (columns, ColumnHeader, c)
			if(c->isHidden ())
				continue;
			if(i == position)
				return c;
			i++;
		EndFor
		return nullptr;
	}
	else
		return (ColumnHeader*)columns.at (position);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader* ColumnHeaderList::getColumnWithID (StringID id) const
{
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getID () == id)
			return c;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::getFlatPositionFromVisible (int position) const
{
	int flatIndex = 0, visibleIndex = 0;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(!c->isHidden ())
		{
			if(visibleIndex == position)
				return flatIndex;
			visibleIndex++;
		}
		flatIndex++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::getVisiblePositionFromFlat (int position) const
{
	int flatIndex = 0, visibleIndex = -1;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(!c->isHidden ())
			visibleIndex++;

		if(flatIndex == position)
			return visibleIndex;

		flatIndex++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::getColumnRange (Coord& left, Coord& right, int columnIndex) const
{
	Coord columnStart = 0;

	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->isHidden ())
			continue;

		if(c->getIndex () == columnIndex)
		{
			left = columnStart;
			right = columnStart + c->getWidth ();
			return;
		}
		columnStart += c->getWidth ();
	EndFor

	left = right = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::getColumnIndex (Coord x) const
{
	Coord columnEnd = 0;

	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->isHidden ())
			continue;

		columnEnd += c->getWidth ();
		if(x < columnEnd)
			return c->getIndex (); 
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::columnIndexToPosition (int columnIndex, bool visible) const
{
	int pos = 0;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(visible && c->isHidden ())
			continue;

		if(c->getIndex () == columnIndex)
			return pos;
		pos++;
	EndFor
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::addColumn (int width, StringRef title, StringID id, int minWidth, int flags)
{
	ColumnHeader* column = NEW ColumnHeader (width, title, id, minWidth, flags);
	column->setIndex (columns.count ());
	columns.add (column);
	indexColumns.add (column);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API ColumnHeaderList::getColumnID (int columnIndex) const
{
	ColumnHeader* column = getColumnByIndex (columnIndex);
	if(column)
		return column->getID ();
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ColumnHeaderList::getTotalWidth () const
{
	int w = 0;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(!c->isHidden ())
			w += c->canFill () ? c->getMinWidth () : c->getWidth ();
	EndFor
	return w;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColumnHeaderList::canHideColumns () const
{
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->canHide ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColumnHeaderList::canFitColumns (bool& multiple) const
{
	int count = 0;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->isHidden ())
			continue;
		if(c->canFit ())
			count++;
	EndFor
	multiple = count > 1;
	return count > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ColumnHeaderList::canResetColumns () const
{
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->isHidden () || !c->canResize ())
			continue;
		if(c->getWidth () != c->getDefaultWidth ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::setColumnMinWidth (ColumnHeader* column, int minWidth)
{
	column->setMinWidth (minWidth);
	if(column->getDefaultWidth () < minWidth)
		column->setDefaultWidth (minWidth);
	if(column->getWidth () < minWidth)
	{
		column->setWidth (minWidth);
		signal (Message (kColumnRectsChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::setColumnWidth (ColumnHeader* column, int width)
{
	ccl_lower_limit (width, ccl_max ((int)kMinColumnWidth, column->getMinWidth ()));
	column->setWidth (width);

	signal (Message (kColumnRectsChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::setColumnWidth (StringID column, int width)
{
	ColumnHeader* columnHeader = getColumnWithID (column);
	if(columnHeader)
		setColumnWidth (columnHeader, width);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::hideColumn (StringID column, tbool state)
{
	ColumnHeader* columnHeader = getColumnWithID (column);
	if(columnHeader)
		columnHeader->isHidden (state != 0);	
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderList::moveColumn (StringID column, int newVisiblePosition)
{
	ColumnHeader* columnHeader = getColumnWithID (column);
	if(columnHeader)
		moveColumn (columnHeader, newVisiblePosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::moveColumn (ColumnHeader* column, int newVisiblePosition)
{
	int oldPosition = columns.index (column);
	int newPosition = getFlatPositionFromVisible (newVisiblePosition);

	if(oldPosition >= 0 && newPosition >= 0 && oldPosition != newPosition)
	{
		columns.remove (column);
		if(oldPosition < newPosition)
			newPosition--;
		columns.insertAt (newPosition, column);

		signal (Message (kColumnRectsChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::showColumn (ColumnHeader* column, bool state)
{
	bool hidden = !state;
	if(column->isHidden () != hidden)
	{	
		column->isHidden (hidden);

		signal (Message (kChanged));
		signal (Message (kColumnRectsChanged));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::setSortColumn (StringID id, bool upwards, bool signalNeeded)
{
	bool wasUpwards = false;
	CString oldId = getSortColumn (wasUpwards);
	if(oldId == id && wasUpwards == upwards)
		return;

	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getID () == id && c->canSort ())
		{
			c->isSortedUp (upwards);
			c->isSortedDown (!upwards);
		}
		else
		{
			c->isSortedUp (false);
			c->isSortedDown (false);
		}
	EndFor

	if(signalNeeded)
		signal (Message (kSortColumnChanged));
	else
		signal (Message (kChanged)); // force at least invalidation of HeaderView
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID ColumnHeaderList::getSortColumn (bool& upwards) const
{
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->isSorted ())
		{
			upwards = c->isSortedUp ();
			return c->getID ();
		}
	EndFor
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColumnHeaderList::setColumnDataAt (int columnIndex, VariantRef data)
{
	if(ColumnHeader* column = getColumnByIndex (columnIndex))
	{
		column->setUserData (data);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColumnHeaderList::getColumnDataAt (Variant& data, int columnIndex) const
{
	if(ColumnHeader* column = getColumnByIndex (columnIndex))
	{
		data = column->getUserData ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::storeState (IAttributeList& _a) const
{
	_a.removeAll ();
	AttributeAccessor a (_a);

	// save column order
	MutableCString orderString;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getID ().isEmpty ())
			continue;
		orderString += c->getID ();
		orderString += ";";
	EndFor
	if(!orderString.isEmpty ())
		a.set ("columnOrder", orderString);

	// save sort column
	bool upwards = false;
	StringID sortID = getSortColumn (upwards);
	if(!sortID.isEmpty ())
	{
		a.set ("sortColumn", sortID);
		a.set ("sortUpwards", upwards);
	}

	// save individual column state
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getID ().isEmpty ())
			continue;
		if(c->canResize ())
		{
			// TODO: only if different from default width?
			MutableCString id = c->getID ();
			id += ".width";
			a.set (id, c->getWidth ());
		}
		if(c->canHide () && c->isHidden ())
		{
			MutableCString id = c->getID ();
			id += ".hidden";
			a.set (id, true);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ColumnHeaderList::restoreState (const IAttributeList& _a)
{
	AttributeReadAccessor a (_a);

	bool anyMovable = false;
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->canMove ())
		{
			anyMovable = true;
			break;
		}
	EndFor

	if(anyMovable)
	{
		// restore column order
		String orderString = a.getString ("columnOrder");
		if(!orderString.isEmpty ())
		{
			// TODO: try to keep original order for non-movable columns!
			ObjectArray orderedColumns;
			ForEachStringToken (orderString, CCLSTR (";"), id)
				if(ColumnHeader* c = getColumnWithID (MutableCString (id)))
					if(!orderedColumns.contains (c))
						orderedColumns.add (c);
			EndFor

			// add missing columns
			if(orderedColumns.count () != columns.count ())
			{
				for(int idx = 0; idx < columns.count (); idx++)
				{
					ColumnHeader* c = (ColumnHeader*) columns.at (idx);
					if(!orderedColumns.contains (c))
					{
						if(orderedColumns.insertAt (idx, c) == false)
							orderedColumns.add (c);				
					}				
				}
			}

			columns.objectCleanup (false);
			columns.removeAll ();
			columns.add (orderedColumns);
			columns.objectCleanup (true);
		}
	}

	// restore individual column state
	ArrayForEachFast (columns, ColumnHeader, c)
		if(c->getID ().isEmpty ())
			continue;
		if(c->canResize ())
		{
			MutableCString id = c->getID ();
			id += ".width";
			int width = 0;
			if(a.getInt (width, id))
				c->setWidth (ccl_bound<int> (width, c->getMinWidth (), kMaxCoord));
		}
		if(c->canHide ())
		{
			MutableCString id = c->getID ();
			id += ".hidden";
			c->isHidden (a.getBool (id));
		}
	EndFor

	// restore sort column
	MutableCString sortID = a.getCString ("sortColumn");
	if(!sortID.isEmpty ())
	{
		bool upwards = a.getBool ("sortUpwards");
		setSortColumn (sortID, upwards);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ColumnHeaderList)
	DEFINE_METHOD_ARGS ("addColumn", "width, title, id, minWidth, flags")
END_METHOD_NAMES (ColumnHeaderList)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColumnHeaderList::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "addColumn")
	{
		MutableCString id;
		if(msg.getArgCount () > 2)
			id = msg[2].asString ();
		int minWidth = 0;
		if(msg.getArgCount () > 3)
			minWidth = msg[3].asInt ();
		int flags = 0;
		if(msg.getArgCount () > 4)
			flags = msg[4].asInt ();

		addColumn (msg[0].asInt (), msg[1].asString (), id, minWidth, flags);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// HeaderView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (HeaderView, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderView::HeaderView (ColumnHeaderList* _columnList)
: columnList (nullptr),
  renderer (nullptr)
{
	if(_columnList)
		setColumnHeaders (_columnList);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderView::~HeaderView ()
{
	setColumnHeaders (nullptr);

	if(renderer)
		renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::calcAutoSize (Rect& r)
{
	Coord defaultHeight = getTheme ().getThemeMetric (ThemeElements::kHeaderHeight);
	int h = getVisualStyle ().getMetric ("headerHeight", defaultHeight);
	int w = columnList ? columnList->getTotalWidth () : 0;
	r (0, 0, w, h);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::setColumnHeaders (ColumnHeaderList* list)
{
	if(columnList != list)
	{
		share_and_observe (this, columnList, list);
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeaderList* HeaderView::getColumnHeaders () const 
{ 
	return columnList; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* HeaderView::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kHeaderViewRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAttributeList* HeaderView::getViewState (bool create)
{
	if(!persistenceID.isEmpty ())
		if(ILayoutStateProvider* provider = GetViewInterfaceUpwards<ILayoutStateProvider> (this))
			return provider->getLayoutState (persistenceID, create);

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::storeColumnState ()
{
	if(columnList)
		if(IAttributeList* a = getViewState (true))
			columnList->storeState (*a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::restoreColumnState ()
{
	if(columnList)
		if(IAttributeList* a = getViewState (false))
			columnList->restoreState (*a);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::attached (View* parent)
{
	SuperClass::attached (parent);

	restoreColumnState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::removed (View* parent)
{
	storeColumnState ();

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::draw (const UpdateRgn& updateRgn)
{
	getRenderer ();
	if(renderer)
		renderer->draw (this, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::getColumnRect (Rect& rect, ColumnHeader* column)
{
	Coord left  = 0;
	ArrayForEachFast (columnList->getColumns (), ColumnHeader, c)
		if(c->isHidden ())
			continue;

		if(c == column)
		{
			rect (left, 0, left + c->getWidth (), getHeight ());
			return true;
		}
		left += c->getWidth ();
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int HeaderView::getVisibleTargetColumnPos (ColumnHeader* column, Coord x)
{
	int oldPos = columnList->getVisiblePositionFromFlat (columnList->getColumns ().index (column));

	int pos = 0;
	Coord left  = 0;
	ArrayForEachFast (columnList->getColumns (), ColumnHeader, c)
		if(c->isHidden ())
			continue;

		Coord right = left + c->getWidth ();

		if((x < 0 || left < x) && x < right)
		{
			if(pos != oldPos && x > (left + right)/2)
				pos++;
			return pos;
		}
		left = right;
		pos++;
	EndFor
	return pos - 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::getColumnAction (const MouseEvent& event, ColumnHeader*& column, int& action)
{
	Coord left  = 0;
	ArrayForEachFast (columnList->getColumns (), ColumnHeader, c)
		if(c->isHidden ())
			continue;

		Coord right = left + c->getWidth ();

		if(event.where.x >= right - kSizeHandleWidth && event.where.x <= right + kSizeHandleWidth)
		{
			if(c->canResize ())
			{
				column = c;
				action = kSize;
				return true;
			}
		}
		else if(left < event.where.x && event.where.x < right)
		{
			if(c->canSort ())
			{
				bool shouldMove = c->canMove () && event.eventType == MouseEvent::kMouseDown && detectDrag (event);
				if(shouldMove == false)
				{	
					column = c;
					action = kSort;
					return true;
				}
			}

			if(c->canMove ())
			{
				column = c;
				action = kMove;
				return true;
			}
		}
		left = right;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeader* HeaderView::findColumnAt (const Point& where) const
{
	Coord height = getHeight ();
	Coord left  = 0;
	ArrayForEachFast (columnList->getColumns (), ColumnHeader, c)
		if(c->isHidden ())
			continue;

		Coord right = left + c->getWidth ();

		Rect columnRect (left, 0, right, height);
		if(columnRect.pointInside (where))
			return c;

		left = right;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* HeaderView::createMouseHandler (const MouseEvent& event)
{
	ColumnHeader* column = nullptr;
	int action = 0;
	if(getColumnAction (event, column, action))
	{
		switch(action)
		{
		case kSize:	
			if(column->canFit () && detectDoubleClick (event))
			{
				onFitColumn (CommandMsg ("View", "Fit Column"), column->asUnknown ());
				return NEW NullMouseHandler (this);
			}
			return NEW SizeColumnMouseHandler (this, column);

		case kMove:
			return NEW MoveColumnMouseHandler (this, column);
		
		case kSort :
			{
				bool upwards = false;
				if(column->isSorted ()) // already sort column => toggle direction
					upwards = !column->isSortedUp ();

				columnList->setSortColumn (column->getID (), upwards);
				storeColumnState ();
			}
			return NEW NullMouseHandler (this);

		default:
			break;
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onMouseEnter (const MouseEvent& event)
{
	return onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onMouseMove (const MouseEvent& event)
{
	ColumnHeader* column = nullptr;
	int action = kNone;
	getColumnAction (event, column, action);

	IMouseCursor* cursor = nullptr;
	if(action == kSize)
		cursor = getTheme ().getThemeCursor (ThemeElements::kSizeHorizontalCursor);
	setCursor (cursor);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onMouseLeave (const MouseEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::onSize (const Point& delta)
{
	// invalidate empty area
	Rect r;
	getClientRect (r);
	if(columnList)
		r.left = columnList->getTotalWidth ();
	invalidate (r);

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(!visualStyle || visualStyle->hasReferences (event.scheme))
		safe_release (renderer);

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HeaderView::notify (ISubject* subject, MessageRef msg)
{
	if(columnList && subject == columnList)
	{
		if(isAccessibilityEnabled ())
			if(HeaderViewAccessibilityProvider* provider = ccl_cast<HeaderViewAccessibilityProvider> (accessibilityProvider))
				provider->rebuildColumnProviders ();
		invalidate ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onContextMenu (const ContextMenuEvent& event)
{
	if(!columnList)
		return false;

	bool result = false;
	IContextMenu& contextMenu = event.contextMenu;

	bool canFitMultiple = false;
	if(columnList->canFitColumns (canFitMultiple))
	{		
		ColumnHeader* c = !event.wasKeyPressed ? findColumnAt (event.where) : nullptr;
		if(c || canFitMultiple)
		{
			if(c != nullptr)
				contextMenu.addCommandItem (XSTR (FitColumn), "View", "Fit Column", 
						CommandDelegate<HeaderView>::make (this, &HeaderView::onFitColumn, c->asUnknown ()));

			if(canFitMultiple)
				contextMenu.addCommandItem (XSTR (FitAllColumns), "View", "Fit All Columns", 
						CommandDelegate<HeaderView>::make (this, &HeaderView::onFitColumn, 0));

			result = true;
		}
	}

	if(columnList->canResetColumns ())
	{
		contextMenu.addCommandItem (XSTR (ResetColumns), "View", "Reset Columns", 
				CommandDelegate<HeaderView>::make (this, &HeaderView::onResetColumns, 0));
		result = true;
	}

	if(columnList->canHideColumns ())
	{
		contextMenu.addSeparatorItem ();

		ArrayForEachFast (columnList->getColumns (), ColumnHeader, c)
			if(c->getTitle ().isEmpty ()) // ignore columns without title
			{
				ASSERT (c->canHide () == false)
				continue;
			}

			contextMenu.addCommandItem (c->getTitle (), "View", "Setup Column", 
										CommandDelegate<HeaderView>::make (this, &HeaderView::onSetupColumn, c->asUnknown ()));
		EndFor
		result = true;
	}
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onSetupColumn (CmdArgs args, VariantRef data)
{
	ColumnHeader* column = unknown_cast<ColumnHeader> (data);
	ASSERT (column)
	if(!column)
		return false;

	bool canHide = column->canHide ();

	if(args.checkOnly ())
	{
		if(MenuItem* menuItem = unknown_cast<MenuItem> (args.invoker))
			menuItem->check (!column->isHidden ());
	}
	else 
	{
		if(canHide)
		{
			columnList->showColumn (column, column->isHidden ());
			storeColumnState ();
		}
	}
	return canHide;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onFitColumn (CmdArgs args, VariantRef data)
{
	IColumnCalculator* calculator = columnList ? columnList->getColumnCalculator () : nullptr;
	if(!calculator)
		return false;

	if(args.checkOnly ())
		return true;

	ColumnHeader* column = unknown_cast<ColumnHeader> (data);
	if(column && !column->isHidden () && column->canResize ())
	{
		int width = 0;
		if(calculator->calcColumnWidth (width, column->getIndex ()) && width > 0)
		{
			columnList->setColumnWidth (column, width);
			storeColumnState ();
		}
	}
	else // all columns
	{
		ArrayForEach (columnList->getColumns (), ColumnHeader, c)
			if(c->isHidden ())
				continue;
			if(!c->canResize ())
				continue;

			int width = 0;
			if(calculator->calcColumnWidth (width, c->getIndex ()) && width > 0)
				columnList->setColumnWidth (c, width);
		EndFor
		
		storeColumnState ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool HeaderView::onResetColumns (CmdArgs args, VariantRef data)
{
	if(!args.checkOnly ())
	{
		ArrayForEach (columnList->getColumns (), ColumnHeader, c)
			if(c->isHidden ())
				continue;
			if(!c->canResize ())
				continue;

			if(c->getWidth () != c->getDefaultWidth ())
				columnList->setColumnWidth (c, c->getDefaultWidth ());
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* HeaderView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW HeaderViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// HeaderViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (HeaderViewAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderViewAccessibilityProvider::HeaderViewAccessibilityProvider (HeaderView& headerView)
: ViewAccessibilityProvider (headerView)
{
	rebuildColumnProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewAccessibilityProvider::rebuildColumnProviders ()
{
	ArrayForEachReverse (getChildren (), AccessibilityProvider, item)
		removeChildProvider (item);
	EndFor
	
	ASSERT (AccessibilityManager::isEnabled ())
	
	int visibleColumns = 1;
	ColumnHeaderList* headers = getHeaderView ().getColumnHeaders ();
	if(headers == nullptr)
		return;

	for(int i = 0; i < headers->getCount (false); i++)
	{
		AutoPtr<ColumnHeaderAccessibilityProvider> child = NEW ColumnHeaderAccessibilityProvider (*this, i);
		addChildProvider (child);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API HeaderViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kHeader;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewAccessibilityProvider::getElementName (String& name, int columnIndex) const
{
	ColumnHeaderList* headers = getHeaderView ().getColumnHeaders ();
	if(headers == nullptr)
		return;
	
	ColumnHeader* header = headers->getColumnByIndex (columnIndex);
	if(header)
		name = header->getTitle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void HeaderViewAccessibilityProvider::getElementBounds (Rect& rect, int columnIndex) const
{
	ColumnHeaderList* headers = getHeaderView ().getColumnHeaders ();
	if(headers == nullptr)
		return;
	
	ColumnHeader* header = headers->getColumnByIndex (columnIndex);
	if(header)
	{
		getHeaderView ().getColumnRect (rect, header);
		Rect clipping;
		getHeaderView ().getVisibleClient (clipping);
		rect.bound (clipping);
		Point screenOffset;
		getHeaderView ().clientToScreen (screenOffset);
		rect.offset (screenOffset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HeaderView& HeaderViewAccessibilityProvider::getHeaderView () const
{
	return static_cast<HeaderView&> (view);
}

//************************************************************************************************
// ColumnHeaderAccessibilityProvider
//************************************************************************************************

ColumnHeaderAccessibilityProvider::ColumnHeaderAccessibilityProvider (HeaderViewAccessibilityProvider& parent, int index)
: parent (parent),
  index (index)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ColumnHeaderAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	if(direction == AccessibilityDirection::kParent)
		return &parent;
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColumnHeaderAccessibilityProvider::getElementName (String& name) const
{
	parent.getElementName (name, index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ColumnHeaderAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kHeaderItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ColumnHeaderAccessibilityProvider::getElementBounds (Rect& b, AccessibilityCoordSpace space) const
{
	parent.getElementBounds (b, index);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ColumnHeaderAccessibilityProvider::getView () const
{
	return parent.getView ();
}
