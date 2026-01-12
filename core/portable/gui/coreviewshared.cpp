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
// Filename    : core/portable/gui/coreviewshared.cpp
// Description : Shared between static and dynamic view classes
//
//************************************************************************************************

#define DEBUG_REDRAW (0 && DEBUG)

#include "core/portable/gui/coreviewshared.h"

#include "core/system/coredebug.h"

using namespace Core;
using namespace Portable;

//************************************************************************************************
// DrawEvent
//************************************************************************************************

DrawEvent::DrawEvent (Graphics& graphics, RectRef updateRect, PointRef origin)
: graphics (graphics),
  updateRect (updateRect),
  origin (origin)
 {
 	graphics.setOrigin (origin);
	graphics.setClip (updateRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DrawEvent::DrawEvent (const DrawEvent& e, RectRef subPart)
: graphics (e.graphics),
  updateRect (e.updateRect),
  origin (e.origin)  
{
	origin.offset (subPart.left, subPart.top);	
	updateRect.bound (subPart);
	updateRect.offset (-subPart.left, -subPart.top);
	
	graphics.setOrigin (origin);
	graphics.setClip (updateRect);
}

//************************************************************************************************
// ThemePainterBase
//************************************************************************************************

ThemePainterBase::ThemePainterBase ()
: focusColor (0, 0, 0xFF),
  focusBorder (Skin::kBorderAllEdges),
  focusBorderWeight (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainterBase::drawFocusFrame (Graphics& graphics, RectRef rect)
{
	drawFocusFrame (graphics, rect, focusBorder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainterBase::drawFocusFrame (Graphics& graphics, RectRef rect, int focusBorder)
{
	if(focusBorder & Skin::kBorderLeftEdge)
	{
		Rect leftEdge (rect);
		leftEdge.right = leftEdge.left + focusBorderWeight;
		graphics.fillRect (leftEdge, focusColor);
	}
	if(focusBorder & Skin::kBorderRightEdge)
	{	
		Rect rightEdge (rect);
		rightEdge.left = rightEdge.right - focusBorderWeight;
		graphics.fillRect (rightEdge, focusColor);
	}
	if(focusBorder & Skin::kBorderTopEdge)
	{
		Rect topEdge (rect);
		topEdge.bottom = topEdge.top + focusBorderWeight;
		graphics.fillRect (topEdge, focusColor);
	}
	if(focusBorder & Skin::kBorderBottomEdge)
	{
		Rect bottomEdge (rect);
		bottomEdge.top = bottomEdge.bottom - focusBorderWeight;
		graphics.fillRect (bottomEdge, focusColor);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainterBase::drawBackground (Graphics& graphics, RectRef rect, const Style& style, Bitmap* image)
{
	if(image)
		graphics.drawBitmap (rect.getLeftTop (), *image, rect);
	else
		graphics.fillRect (rect, style.getBackColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainterBase::drawValueBar (Graphics& graphics, RectRef r, int options, float value, const Style& style, Bitmap* image)
{
	bool centered = (options & Skin::kValueBarAppearanceCentered) != 0;
	bool vertical = (options & Skin::kValueBarAppearanceVertical) != 0;
	bool filmstrip = (options & Skin::kValueBarAppearanceFilmstrip) != 0;

	if(filmstrip && image)
	{	
		int height = image->getHeight ();
		int yPos = static_cast<int> (value * height);

		if(yPos < 0)
			yPos = 0;
		else if(yPos > height - r.bottom)
			yPos = height - r.bottom;
		else
			yPos -= yPos % r.bottom;

		graphics.drawBitmap (Point (0, 0), *image, Rect (0, yPos, r.right, yPos + r.bottom));
		return;
	}
	
	static const int kCenteredSpacing = 2;
	
	Rect hiliteRect;
	if(vertical)
	{
		Coord height = r.getHeight ();
		hiliteRect.right = r.getWidth ();
		if(centered)
		{
			Coord hilite = (Coord)((value - .5f) * height);
			
			hiliteRect.bottom = hiliteRect.top = height / 2;
			if(value < 0.5f)
				hiliteRect.bottom = hiliteRect.top - hilite;
			else
				hiliteRect.top = hiliteRect.bottom - hilite;
			
			hiliteRect.top -= (kCenteredSpacing / 2);
			hiliteRect.bottom += (kCenteredSpacing / 2);
		}
		else
		{
			Coord hilite = (Coord)(value * height);
			
			hiliteRect.top = height - hilite;
			hiliteRect.bottom = height;
		}
	}
	else
	{
		hiliteRect.bottom = r.getHeight ();
		Coord width = r.getWidth ();
		if(centered)
		{
			Coord hilite = (Coord)((value - .5f) * width);
			
			hiliteRect.left = hiliteRect.right = width / 2;
			if(value < 0.5f)
				hiliteRect.left = hiliteRect.right + hilite;
			else
				hiliteRect.right = hiliteRect.left + hilite;
			hiliteRect.left -= (kCenteredSpacing / 2);
			hiliteRect.right += (kCenteredSpacing / 2);
		}
		else
			hiliteRect.right = (Coord)(value * width);
		
	}

	if(!hiliteRect.isEmpty ())
	{
		if(image)
			graphics.drawBitmap (hiliteRect.getLeftTop (), *image, hiliteRect);
		else
			graphics.fillRect (hiliteRect, style.getHiliteColor ());
	}
}

//************************************************************************************************
// RootViewBase
//************************************************************************************************

RootViewBase::RootViewBase (RectRef targetSize, BitmapPixelFormat pixelFormat, RenderMode renderMode)
: updateSuspended (false),
  targetSize (targetSize),
  pixelFormat (pixelFormat),
  renderMode (renderMode),
  activeBufferIndex (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootViewBase::redraw ()
{
	ASSERT (!offscreenList.isEmpty ())
	if(offscreenList.isEmpty ())
		return false;

	if(!dirtyRegion.isEmpty ())
	{
		if(renderMode == kFlipMode)
		{
			// flip buffers
			Bitmap* lastOffscreen = offscreenList[activeBufferIndex];
			activeBufferIndex = activeBufferIndex ? 0 : 1;
			Bitmap* newOffscreen = offscreenList[activeBufferIndex];

			// copy last dirty region minus new dirty region
			if(!dirtyRegion.isEqual (lastDirtyRegion))
			{
				if(!lastDirtyRegion.isEmpty ())
				{
					#if DEBUG_REDRAW
					Rect lastDirtyRect = lastDirtyRegion.getBoundingBox ();
					int totalBytesCopied = 0;
					#endif

					if(lastDirtyRegion == targetSize) // copy full frame
						newOffscreen->copyFrom (*lastOffscreen);
					else
					{
						// exclude new dirty region
						lastDirtyRegion.exclude (dirtyRegion);

						ForEachRectFast (lastDirtyRegion, Rect, rect)
							newOffscreen->copyFrom (*lastOffscreen, rect);

							#if DEBUG_REDRAW
							totalBytesCopied += rect.getWidth () * rect.getHeight () * lastOffscreen->accessForRead ().getBytesPerPixel ();
							#endif
						EndFor
					}

					#if DEBUG_REDRAW
					if(!lastDirtyRegion.isEmpty ())
					{
						int bytesCopiedBounding = lastDirtyRect.getWidth () * lastDirtyRect.getHeight () * lastOffscreen->accessForRead ().getBytesPerPixel ();
						if(lastDirtyRegion == targetSize)
							DebugPrintf ("Flip mode: %.3f KB copied (full)\n", float(bytesCopiedBounding)/1024.f);
						else
							DebugPrintf ("Flip mode: %d rect(s), %.3f KB copied (instead of bounding box %.3f KB)\n", lastDirtyRegion.getRects ().count (), float(totalBytesCopied)/1024.f, float(bytesCopiedBounding)/1024.f);
					}
					#endif						
				}

				lastDirtyRegion.copyFrom (dirtyRegion);
			}			
		}

		Bitmap* offscreen = offscreenList[activeBufferIndex];
		switch(offscreen->getFormat ())
		{
		case kBitmapRGBAlpha :
			redrawOffscreen<ColorBitmapRenderer> (offscreen);
			break;

		case kBitmapRGB565 :
			redrawOffscreen<RGB565BitmapRenderer> (offscreen);
			break;

		case kBitmapMonochrome :
			redrawOffscreen<MonoBitmapRenderer> (offscreen);
			break;

		case kBitmapAny :
		case kBitmapRGB :
		default :
			ASSERT (0)
			break;
		}

		dirtyRegion.setEmpty ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RootViewBase::redrawTo (IGraphicsCommandSink& commandSink)
{
	ASSERT (renderMode == kExternalMode)
	if(renderMode != kExternalMode)
		return false;

	if(!dirtyRegion.isEmpty ())
	{
		GraphicsCommandRenderer renderer (commandSink, targetSize, pixelFormat == kBitmapMonochrome);
		ForEachRectFast (dirtyRegion, Rect, r)
			DrawEvent e (renderer, r);
			draw (e);
		EndFor

		dirtyRegion.setEmpty ();
	}
	return true;
}

//************************************************************************************************
// ListViewStyle
//************************************************************************************************

ListViewStyle::ListViewStyle ()
: rowHeight (Skin::kListViewDefaultRowHeight),
  itemInset (Skin::kListViewDefaultItemInset),
  scrollerSize (Skin::kListViewDefaultScrollerSize),
  selectColor (Colors::kLtGray),
  separatorColor (Colors::kGray),
  focusBorder (Skin::kBorderAllEdges)
{}

//************************************************************************************************
// ListViewModelBase
//************************************************************************************************

void ListViewModelBase::drawItem (int index, const DrawInfo& info, bool enabled)
{
	if(!enabled)
		info.graphics.fillRect (info.rect, info.style.getBackColorDisabled ());
	ConstString title (getItemTitle (index));
	if(!title.isEmpty ())
		drawTitle (title, info, enabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewModelBase::drawTitle (CStringPtr title, const DrawInfo& info, bool enabled)
{
	Color textColor = enabled ? (info.selected ? info.style.getTextColorOn () : info.style.getTextColor ()) : info.style.getTextColorDisabled ();
	info.graphics.drawString (info.rect, title, textColor, info.style.getFontName (), info.style.getTextAlign ());
}

//************************************************************************************************
// ListViewPainter
//************************************************************************************************

ListViewPainter::ListViewPainter (ListViewStyle& listStyle)
: clientWidth (0),
  clientHeight (0),
  baseModel (nullptr),
  listStyle (listStyle),
  startIndex (0),
  selectIndex (-1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewPainter::getItemRect (Rect& rect, int index) const
{
	Coord rowHeight = listStyle.getRowHeight ();
	Coord scrollOffset = startIndex * rowHeight;
	
	rect.left = 0;
	rect.right = clientWidth - listStyle.getScrollerSize ();
	rect.top = index * rowHeight - scrollOffset;
	rect.bottom = rect.top + rowHeight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewPainter::getItemIndex (const Point& where) const
{
	return where.y / listStyle.getRowHeight () + startIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewPainter::drawList (const DrawEvent& e, const Style& style)
{
	// Background
	e.graphics.fillRect (e.updateRect, style.getBackColor ());
	
	// Items
	int numItems = baseModel ? baseModel->getItemCount () : 0;
	if(baseModel != nullptr) 
	{
		for(int i = 0; i < numItems; i++)
		{
			Rect itemRect;
			getItemRect (itemRect, i);
			if(itemRect.bottom < e.updateRect.top)
				continue;
			if(itemRect.top > e.updateRect.bottom)
				break;
			
			// Selection
			bool selected = baseModel->isSelectionHandler () ? baseModel->isItemSelected (i) : i == selectIndex;			
			Coord itemInset = listStyle.getItemInset ();
			itemRect.right -= itemInset;
			if(selected)
				e.graphics.fillRect (itemRect, listStyle.getSelectColor ());
			
			// Item
			itemRect.left += itemInset;
			ListViewModelBase::DrawInfo info = {listStyle, e.graphics, itemRect, style, selected};
			baseModel->drawItem (i, info, baseModel->isItemEnabled (i));
		}
	}
	
	// Scrollbar
	float from = 0.f, to = 0.f;
	if(getScrollRange (numItems, from, to))
	{
		Rect barRect;
		barRect.right = clientWidth;
		barRect.left = barRect.right - listStyle.getScrollerSize ();
		barRect.top = (Coord)(from * clientHeight);
		barRect.bottom = (Coord)(to * clientHeight);
		
		e.graphics.fillRect (barRect, style.getHiliteColor ());
	}

	// Border
	Rect r (0, 0, clientWidth, clientHeight);
	e.graphics.drawRect (r, style.getForeColor ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::getScrollRange (int numTotal, float& from, float& to) const
{
	if(numTotal == 0)
		return false;
	
	int numVisible = getNumVisible ();
	if(numVisible >= numTotal)
		return false;
	
	from = (float)startIndex / (float)numTotal;
	to = (float)(startIndex + numVisible) / (float)numTotal;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewPainter::getNumVisible () const
{
	return clientHeight / listStyle.getRowHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewPainter::resetScrollPosition ()
{
	startIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ListViewPainter::getScrollPosition () const
{
	return startIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::scrollTo (int index)
{
	int numTotal = baseModel ? baseModel->getItemCount () : 0;
	int numVisible = getNumVisible ();
	int maxStartIndex = numTotal - numVisible;
	if(index > maxStartIndex)
		index = maxStartIndex;
	
	if(index < 0)
		index = 0;
	
	if(index != startIndex)
	{
		startIndex = index;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::scrollBy (int delta)
{
	return scrollTo (startIndex + delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::makeItemVisible (int index)
{
	if(index >= 0)
	{
		int numTotal = baseModel ? baseModel->getItemCount () : 0;
		int numVisible = getNumVisible ();
		if(numVisible < numTotal)
		{
			if(index < startIndex)
			{
				startIndex = index;
				return true;
			}
			else if(index >= startIndex + numVisible)
			{
				startIndex = index - numVisible + 1;
				return true;
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::selectItem (int index)
{		
	if(baseModel && baseModel->isSelectionHandler ())
		return false;

	int maxIndex = baseModel ? baseModel->getItemCount ()-1 : 0;
	if(index > maxIndex)
		index = -1;
				
	if(index != selectIndex)
	{
		selectIndex = index;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ListViewPainter::resetSelectedItem ()
{
	selectIndex = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ListViewPainter::makeSelectedItemVisible ()
{
	if(baseModel && baseModel->isSelectionHandler ())
	{
		for(int i = 0, count = baseModel->getItemCount (); i < count; i++)
			if(baseModel->isItemSelected (i))
				return makeItemVisible (i);
	}
	else
	{
		if(selectIndex >= 0)
			return makeItemVisible (selectIndex);
	}
	return false;
}
