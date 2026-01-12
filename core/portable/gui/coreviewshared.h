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
// Filename    : core/portable/gui/coreviewshared.h
// Description : Shared between static and dynamic view classes
//
//************************************************************************************************

#ifndef _coreviewshared_h
#define _coreviewshared_h

#include "core/portable/gui/coregraphics.h"

#include "core/public/corevector.h"
#include "core/public/gui/corerectlist.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// DrawEvent
/** \ingroup core_gui */
//************************************************************************************************

struct DrawEvent
{
	Graphics& graphics;
	Rect updateRect;
	Point origin;
	
	DrawEvent (Graphics& graphics, RectRef updateRect, PointRef origin = Point ());
	DrawEvent (const DrawEvent& e, RectRef subPart);
};

//************************************************************************************************
// Style
/** \ingroup core_gui */
//************************************************************************************************

class Style
{
public:
	Style ()
	: backColor (Colors::kWhite),
	  backColorDisabled (Colors::kWhite),
	  foreColor (Colors::kBlack),
	  foreColorDisabled (Colors::kBlack),
	  textColor (Colors::kBlack),
	  textColorOn (Colors::kBlack),
	  textColorDisabled (Colors::kBlack),
	  hiliteColor (Colors::kLtGray),
	  textAlign (Alignment::kLeftCenter)
	{}

	PROPERTY_VARIABLE (Color, backColor, BackColor)
	PROPERTY_VARIABLE (Color, backColorDisabled, BackColorDisabled)
	PROPERTY_VARIABLE (Color, foreColor, ForeColor)
	PROPERTY_VARIABLE (Color, foreColorDisabled, ForeColorDisabled)
	PROPERTY_VARIABLE (Color, textColor, TextColor)
	PROPERTY_VARIABLE (Color, textColorOn, TextColorOn)
	PROPERTY_VARIABLE (Color, textColorDisabled, TextColorDisabled)
	PROPERTY_VARIABLE (Color, hiliteColor, HiliteColor)
	
	PROPERTY_CSTRING_BUFFER (32, fontName, FontName)
	PROPERTY_VARIABLE (int, textAlign, TextAlign)
};

//************************************************************************************************
// ThemePainterBase
/** \ingroup core_gui */
//************************************************************************************************

class ThemePainterBase
{
public:
	ThemePainterBase ();

	PROPERTY_VARIABLE (Color, focusColor, FocusColor)
	PROPERTY_VARIABLE (Skin::BorderStyles, focusBorder, FocusBorder)
	PROPERTY_VARIABLE (int, focusBorderWeight, FocusBorderWeight)

	void drawFocusFrame (Graphics& graphics, RectRef rect);
	void drawFocusFrame (Graphics& graphics, RectRef rect, int focusBorder);
	
	void drawBackground (Graphics& graphics, RectRef rect, const Style& style, Bitmap* image = nullptr);
	void drawValueBar (Graphics& graphics, RectRef rect, int options, float value, const Style& style, Bitmap* image = nullptr);
};

//************************************************************************************************
// TViewBase
/** \ingroup core_gui */
//************************************************************************************************

template<class VC>
class TViewBase
{
public:
	TViewBase (RectRef size = Rect ());
	
	PROPERTY_CSTRING_BUFFER (64, name, Name)
	PROPERTY_POINTER (VC, parent, Parent)

	void clientToRoot (Point& p) const;
	void rootToClient (Point& p) const;

	RectRef getSize () const;
	virtual void setSize (RectRef newSize);

	Rect& getClientRect (Rect& r) const;

	void invalidate ();
	virtual void invalidate (RectRef rect);

	const Style& getStyle () const;
	virtual void setStyle (Style* style);

	virtual void draw (const DrawEvent& e);

	PROPERTY_FLAG (options, kWantsFocus, wantsFocus)
	bool hasFocus () const;

	virtual void onFocus (bool state);

protected:
	PROPERTY_FLAG (options, kIsFocused, isFocused)

	enum Options
	{
		kWantsFocus = 1<<1,
		kIsFocused = 1<<2,
		kLastViewBaseFlag = 2
	};

	Rect size;
	Style* style;
	int options;
};

//************************************************************************************************
// ContainerViewBase
/** \ingroup core_gui */
//************************************************************************************************

template <class VC>
class ContainerViewBase: public VC
{
public:
	ContainerViewBase (RectRef size = Rect ());

	virtual void addView (VC* view);
	virtual void removeView (VC* view);

	const ConstVector<VC*>& getChildren () const;

	// VC
	void draw (const DrawEvent& e) override;

protected:
	static const int kMaxViewCount = 55;
	typedef FixedSizeVector<VC*, kMaxViewCount> ViewChildren;

	ViewChildren children;
};

//************************************************************************************************
// RootViewBase
/** \ingroup core_gui */
//************************************************************************************************

class RootViewBase
{
public:
	enum RenderMode
	{
		kExternalMode,	///< render to external sink (no offscreen)
		kOffscreenMode,	///< use single offscreen
		kFlipMode		///< use double-buffering scheme
	};

	typedef Core::RectList<5> RectList;

	RootViewBase (RectRef targetSize = Rect (), BitmapPixelFormat pixelFormat = kBitmapRGBAlpha, RenderMode renderMode = kOffscreenMode);
	virtual ~RootViewBase () {}

	PROPERTY_BOOL (updateSuspended, UpdateSuspended)

	BitmapPixelFormat getFormat () const;
	RenderMode getRenderMode () const;
	const RectList& getDirtyRegion () const;

	const void* getNextRenderBuffer () const;
	const Bitmap* getActiveBufferBitmap () const;
	const BitmapData* accessForRead () const;

	bool redraw ();
	bool redrawTo (IGraphicsCommandSink& commandSink);
	
	virtual void draw (const DrawEvent& e) = 0;
	
protected:
	Rect targetSize;
	BitmapPixelFormat pixelFormat;
	RenderMode renderMode;
	FixedSizeVector<Bitmap*, 2> offscreenList;
	int activeBufferIndex;
	RectList dirtyRegion;
	RectList lastDirtyRegion;

	void addDirtyRect (RectRef _rect);

	template <class RendererClass>
	void redrawOffscreen (Bitmap* offscreen);
};

//************************************************************************************************
// ListViewStyle
/** \ingroup core_gui */
//************************************************************************************************

class ListViewStyle
{
public:
	ListViewStyle ();

	PROPERTY_VARIABLE (Coord, rowHeight, RowHeight)
	PROPERTY_VARIABLE (Coord, itemInset, ItemInset)
	PROPERTY_VARIABLE (Coord, scrollerSize, ScrollerSize)
	PROPERTY_VARIABLE (Color, selectColor, SelectColor)
	PROPERTY_VARIABLE (Color, separatorColor, SeparatorColor)
	PROPERTY_VARIABLE (int, focusBorder, FocusBorder)
};

//************************************************************************************************
// ListViewModelBase
/** \ingroup core_gui */
//************************************************************************************************

class ListViewModelBase
{
public:
	virtual int getItemCount () const = 0;
	virtual CStringPtr getItemTitle (int index) const = 0;
	
	virtual bool isSelectionHandler () const { return false; }
	virtual bool isItemSelected (int index) const { return false; }
	virtual bool canSelectItem (int index) const { return true; }
	virtual bool isItemEnabled (int index) const { return true; }

	struct DrawInfo
	{
		const ListViewStyle& list;
		Graphics& graphics;
		RectRef rect;
		const Style& style;
		bool selected;
	};

	virtual void drawItem (int index, const DrawInfo& info, bool enabled = true);

protected:
	void drawTitle (CStringPtr title, const DrawInfo& info, bool enabled = true);
};

//************************************************************************************************
// ListViewPainter
/** \ingroup core_gui */
//************************************************************************************************

class ListViewPainter
{
public:
	ListViewPainter (ListViewStyle& listStyle);

	PROPERTY_VARIABLE (Coord, clientWidth, ClientWidth)
	PROPERTY_VARIABLE (Coord, clientHeight, ClientHeight)
	PROPERTY_POINTER (ListViewModelBase, baseModel, BaseModel)

	void getItemRect (Rect& rect, int index) const;
	int getItemIndex (const Point& where) const;

	int getScrollPosition () const;
	void resetScrollPosition ();
	bool scrollTo (int index);
	bool scrollBy (int delta);
	bool makeItemVisible (int index);

	bool selectItem (int index);
	void resetSelectedItem ();
	bool makeSelectedItemVisible ();
	
	void drawList (const DrawEvent& e, const Style& style);

protected:
	ListViewStyle& listStyle;
	int startIndex;
	int selectIndex;

	bool getScrollRange (int numItems, float& from, float& to) const;
	int getNumVisible () const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// RootViewBase implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

inline BitmapPixelFormat RootViewBase::getFormat () const
{
	return pixelFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline RootViewBase::RenderMode RootViewBase::getRenderMode () const
{
	return renderMode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const RootViewBase::RectList& RootViewBase::getDirtyRegion () const
{
	return dirtyRegion; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const void* RootViewBase::getNextRenderBuffer () const
{
	if(offscreenList.isEmpty ())
		return nullptr;
	if(renderMode == kFlipMode)
		return offscreenList[activeBufferIndex ? 0 : 1]->accessForRead ().scan0;
	else
		return offscreenList[0]->accessForRead ().scan0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const Bitmap* RootViewBase::getActiveBufferBitmap () const
{
	return	!offscreenList.isEmpty () ? 
			offscreenList[activeBufferIndex] : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const BitmapData* RootViewBase::accessForRead () const
{
	return	!offscreenList.isEmpty () ? 
			&offscreenList[activeBufferIndex]->accessForRead () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void RootViewBase::addDirtyRect (RectRef _rect)
{
	Rect rect (_rect);
	rect.bound (targetSize);
	if(!rect.isEmpty ())
	{
		#if (0 && DEBUG)
		dumpRect (rect, " --- dirty rect join");
		#endif		
		dirtyRegion.join (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class RendererClass>
void RootViewBase::redrawOffscreen (Bitmap* offscreen)
{
	RendererClass renderer (*offscreen);
	ForEachRectFast (dirtyRegion, Rect, rect)
		#if (0 && DEBUG)
		dumpRect (rect, " --- draw dirty rect");
		#endif
		DrawEvent e (renderer, rect);
		draw (e);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ContainerViewBase implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
ContainerViewBase<VC>::ContainerViewBase (RectRef size)
: VC (size)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void ContainerViewBase<VC>::addView (VC* view)
{
	ASSERT (view != nullptr && view->getParent () == nullptr)
	ASSERT (children.count () < kMaxViewCount)
	view->setParent (this);
	children.add (view);
	VC::invalidate ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void ContainerViewBase<VC>::removeView (VC* view)
{
	ASSERT (view != nullptr && view->getParent () == this)
	view->setParent (nullptr);
	children.remove (view);
	VC::invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
const ConstVector<VC*>& ContainerViewBase<VC>::getChildren () const
{
	return children; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void ContainerViewBase<VC>::draw (const DrawEvent& e)
{
	VectorForEachFast (children, VC*, view)
		if(e.updateRect.intersect (view->getSize ()))
		{
			DrawEvent e2 (e, view->getSize ());
			if(!e2.updateRect.isEmpty ())
				view->draw (e2);
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TViewBase implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
TViewBase<VC>::TViewBase (RectRef size)
: parent (nullptr),
  size (size),
  style (nullptr),
  options (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::clientToRoot (Point& p) const
{
	p.x += size.left;
	p.y += size.top;
	
	if(parent)
		parent->clientToRoot (p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::rootToClient (Point& p) const
{
	Point offset;
	clientToRoot (offset);
	
	p.x -= offset.x;
	p.y -= offset.y;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
template <class VC>
RectRef TViewBase<VC>::getSize () const
{
	return size; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::setSize (RectRef newSize)
{
	if(newSize != size)
	{
		invalidate ();
		size = newSize;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
Rect& TViewBase<VC>::getClientRect (Rect& r) const
{
	r.left = r.top = 0;
	r.right = size.getWidth ();
	r.bottom = size.getHeight ();
	return r;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::invalidate ()
{
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::invalidate (RectRef rect)
{
	if(parent == nullptr)
		return;
		
	Rect r;
	getClientRect (r);
	if(r.bound (rect))
	{
		r.offset (size.left, size.top);
		parent->invalidate (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
const Style& TViewBase<VC>::getStyle () const
{
	static const Style kEmptyStyle;
	return style ? *style : kEmptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::setStyle (Style* _style)
{
	style = _style;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::draw (const DrawEvent& e)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
bool TViewBase<VC>::hasFocus () const
{
	return isFocused ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class VC>
void TViewBase<VC>::onFocus (bool state)
{
	ASSERT (wantsFocus () == true)
	if(state != isFocused ())
	{
		isFocused (state);
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace Portable
} // namespace Core

#endif // _coreviewshared_h
