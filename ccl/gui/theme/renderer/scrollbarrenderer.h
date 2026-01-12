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
// Filename    : ccl/gui/theme/renderer/scrollbarrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_scrollbarrenderer_h
#define _ccl_scrollbarrenderer_h

#include "ccl/gui/theme/themerenderer.h"

#include "ccl/public/gui/framework/styleflags.h"

namespace CCL {

class ScrollBar;
class ScrollButton;
class Image;
struct ScrollBarDrawState;

struct ScrollBarImages
{
	SharedPtr<IImage> buttonUp;
	SharedPtr<IImage> buttonDown;
	SharedPtr<IImage> thumb;
	SharedPtr<IImage> back;
};

//************************************************************************************************
// ScrollBarRenderer
//************************************************************************************************

class ScrollBarRenderer: public ThemeRenderer
{	
public:
	ScrollBarRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

	bool getDrawState (ScrollBar* scrollBar, ScrollBarDrawState& state, ScrollBarImages& images) const;
	void loadImages ();

protected:
	bool imagesLoaded;
	ScrollBarImages vImages;
	ScrollBarImages hImages;
	ScrollBarImages vSmallImages;
	ScrollBarImages hSmallImages;
	float inset;
	bool clipBackground;

	ScrollBarImages& getImages (StyleRef style);
};

DECLARE_VISUALSTYLE_CLASS (ScrollBar)

//************************************************************************************************
// ScrollButtonRenderer
//************************************************************************************************

class ScrollButtonRenderer: public ThemeRenderer
{	
public:
	ScrollButtonRenderer (VisualStyle* visualStyle);

	static void drawTriangleIcon (IGraphics& port, RectRef clientRect, int orientation); // see Alignment

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	void loadImages (ScrollButton* view);
	void drawTriangle (IGraphics& port, RectRef clientRect, ScrollButton* scrollButton);

	bool imagesLoaded;
	SharedPtr<IImage> buttonImage;
	SharedPtr<IImage> icon;
};

DECLARE_VISUALSTYLE_CLASS (ScrollButton)

//************************************************************************************************
// PageControlRenderer
//************************************************************************************************

class PageControlRenderer: public ThemeRenderer
{	
public:
	PageControlRenderer (VisualStyle* visualStyle);

	// ThemeRenderer
	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	SharedPtr<IImage> background;
	SharedPtr<IImage> dotImage;
	Coord spacing;
	bool imagesLoaded;

	void loadImages ();
};

DECLARE_VISUALSTYLE_CLASS (PageControl)

} // namespace CCL

#endif // _ccl_scrollbarrenderer_h
