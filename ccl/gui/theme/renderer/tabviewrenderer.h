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
// Filename    : ccl/gui/theme/renderer/tabviewrenderer.h
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_tabviewrenderer_h
#define _ccl_tabviewrenderer_h

#include "ccl/gui/theme/atomrenderer.h"

#include "ccl/gui/controls/tabview.h"

namespace CCL {

//************************************************************************************************
// TabViewRenderer
/** Draws the tabs of a TabView, the strip behind them and the content area next to them.

The "background" atom draws the content area, the "header" atom the strip behind the tabs. A tab is
drawn by the "tab.background", "tab.icon", "tab.value" and "tab.menuicon" (menu icon of the active tab)
atoms. Every tab has a TabItemRenderer of its own that holds these atoms, so they animate when the
visual style defines animations for them.

The style conditions of a tab are "left", "middle", "right" and "menu" for its position - a single tab
has none of them - and "on", "mouseover", "pressed" and "disabled" for its state. */
//************************************************************************************************

class TabViewRenderer: public ControlRenderer,
					   public ITabViewRenderer
{
public:
	TabViewRenderer (VisualStyle* visualStyle, View* owner);
	~TabViewRenderer ();

	// ControlRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;
	void updateAtoms () override;
	void applyVisualStyle (RenderAtomTransaction& transaction) override;

	// ITabViewRenderer
	void drawTab (GraphicsDevice& port, RectRef r, int tabIndex) override;

	CLASS_INTERFACE (ITabViewRenderer, ControlRenderer)

protected:
	/** The position a tab has within the row of them - in a vertical TabView, the left tab is the top one. */
	enum TabKind { kLeftTab, kMiddleTab, kRightTab, kSingleTab, kMenuTab };

	/** The state a tab is in - combined with the TabKind it identifies the appearance of a tab. */
	enum TabState
	{
		kTabStateOn			= 1<<0,
		kTabStateMouseOver	= 1<<1,
		kTabStatePressed	= 1<<2,
		kTabStateDisabled	= 1<<3
	};

	class TabItemRenderer;

	#if LEGACY_STYLES_SUPPORTED
	SharedPtr<IImage> firstButton;
	SharedPtr<IImage> middleButton;
	SharedPtr<IImage> lastButton;
	SharedPtr<IImage> singleButton;
	SharedPtr<IImage> menuIcon;
	SharedPtr<IImage> menuBackground;
	#endif

	SharedPtr<IImage> tabMenuIcon;	///< the menu icon of the active tab - kept for the width it takes away from the title
	SharedPtr<IImage> styleTabIcon;	///< the icon of a tab that has none of its own
	SharedPtr<IImage> menuTabIcon;	///< the icon the style gives the menu button (legacy: "menuIcon")

	int iconSpacing;
	int tabSpacing;
	int tabMenuSpacing;
	int tabSlope;			///< the width the sloped tabs of a TabView without tab images overlap each other by - 0 when it has them
	int tabExtraWidth;		///< the width a tab takes on top of its icon, its title and its padding
	int tabHeight;
	int contentMargin;		///< "margin" - the distance between the edges of the content area and the child view of the active tab
	int inset;
	int menuTabWidth;
	Rect padding;

	RenderAtom backgroundAtom;
	RenderAtom headerAtom;

	ObjectArray tabItems;			///< the renderers of the tabs, by tab index
	TabItemRenderer* menuItem;		///< the renderer of the menu button

	struct TabDrawArgs
	{
		TabView* tabView;
		GraphicsDevice& port;
		const UpdateRgn& updateRgn;
		FontRef font;

		TabDrawArgs (TabView* tabView, GraphicsDevice& port, const UpdateRgn& updateRgn, FontRef font)
		: tabView (tabView), port (port), updateRgn (updateRgn), font (font)
		{}
	};

	Coord calcTabWidth (TabView* tabView, int index, FontRef font);
	Coord calcTabWidth (TabView* tabView, int index, StringRef label, IImage* icon, FontRef font);

	TabItemRenderer& getTabItem (int index); ///< the renderer of the tab - the one of the menu button for the index TabView::kPartMenuTab - TabView::kPartFirstTab
	IImage* getDisplayedTabIcon (TabView* tabView, int index) const; ///< the icon of the tab, or the one of the style if it has none
	int getTabState (TabView* tabView, int index, bool active) const;
	static void setTabConditions (StyleConditionContext& context, TabKind kind, int state);
	static int getTabImageFrame (IImage* image, int state); ///< the named frame of a tab image for the given state, -1 if it has none
	void drawTabItem (TabDrawArgs& args, TabItemRenderer& item, RectRef tabRect, TabKind kind, int index, bool active, StringRef label, IImage* icon, bool center);

	#if LEGACY_STYLES_SUPPORTED
	void applyLegacyVisualStyle ();
	#endif

	template<class Geometry> void draw (const UpdateRgn& updateRgn);
	template<class Geometry> int hitTest (PointRef where, Point* clickOffset);
	template<class Geometry> bool getPartRect (Rect& rect, int partCode);
	template<class Geometry> int findTab (TabView* tabView, PointRef where);
	template<class Geometry> void calcViewSize (TabView* tabView, Rect& rect);
};

DECLARE_TYPELIB_ENTRY (TabView)

} // namespace CCL

#endif // _ccl_tabviewrenderer_h
