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
// Description : ScrollBar Renderer
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

//************************************************************************************************
// ScrollBarImages
//************************************************************************************************

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
	ScrollBarRenderer (VisualStyle* visualStyle, View* owner);

	bool getDrawState (ScrollBar* scrollBar, ScrollBarDrawState& state, ScrollBarImages& images) const;
	void loadImages ();

	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

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

DECLARE_TYPELIB_ENTRY (ScrollBar)

//************************************************************************************************
// ScrollButtonRenderer
//************************************************************************************************

class ScrollButtonRenderer: public ThemeRenderer
{	
public:
	ScrollButtonRenderer (VisualStyle* visualStyle, View* owner);

	static void drawTriangleIcon (IGraphics& port, RectRef clientRect, int orientation); // see Alignment

	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

protected:
	void loadImages (ScrollButton* view);
	void drawTriangle (IGraphics& port, RectRef clientRect, ScrollButton* scrollButton);

	bool imagesLoaded;
	SharedPtr<IImage> buttonImage;
	SharedPtr<IImage> icon;
};

DECLARE_TYPELIB_ENTRY (ScrollButton)

//************************************************************************************************
// PageControlRenderer
//************************************************************************************************

class PageControlRenderer: public ThemeRenderer
{	
public:
	PageControlRenderer (VisualStyle* visualStyle, View* owner);

	// ThemeRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

protected:
	SharedPtr<IImage> background;
	SharedPtr<IImage> dotImage;
	Coord spacing;
	bool imagesLoaded;

	void loadImages ();
};

DECLARE_TYPELIB_ENTRY (PageControl)

} // namespace CCL

#endif // _ccl_scrollbarrenderer_h
