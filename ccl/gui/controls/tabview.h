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
// Filename    : ccl/gui/controls/tabview.h
// Description : Tab View
//
//************************************************************************************************

#ifndef _tabview_h
#define _tabview_h

#include "ccl/gui/controls/control.h"

#include "ccl/public/gui/framework/iitemmodel.h" // IItemDragTarget

namespace CCL {

//************************************************************************************************
// TabView styles
//************************************************************************************************

namespace Styles
{
	enum TabViewStyles
	{
		kTabViewBehaviorCanReorderTabs = 1<<8,		///< tabs can be reorderd by drag & drop
		kTabViewBehaviorCanDragTabData = 1<<9,		///< data associated with a tab can be dragged
		kTabViewBehaviorNoMenu = 1<<10,				///< suppresses the menu button that appears when not all tabs fit in the view
		kTabViewBehaviorExtendTabs = 1<<11,			///< extend tabs to fill the header, if there is more space available
		kTabViewBehaviorTabMenu = 1<<12,			///< the active tab displays a menu icon
		kTabViewBehaviorNoWheel = 1<<13,			///< no mousewheel
		kTabViewBehaviorNoActivateOnHover = 1<<14,	///< do not activate tab when any drag enters view
		kTabViewBehaviorFitAllViews = 1<<15,		///< when autosizing, use largest size of all content views
		kTabViewAppearanceCentered = 1<<16			///< center tabs if possible (no "extendtabs" and combined tab width smaller than view width)
	};
};

//************************************************************************************************
// TabView
/** Shows a list of tab buttons to control a parameter or chose a child view. 
A tab view draws a row of Tab buttons, which behave like radio buttons. 
One button is the active tab, which can be controlled by the parameter.
When there is not enough space for all tabs, a menu button is appended as last button, that gives access to all tabs.

If the tab view has child views, the child view that correspond to the active tab is automatically added 
to the content area of the tab view (remaining height without tabs). This feature is optional, a tabview 
can also be used only for tab buttons, controlling the parameter of a variant view that switches chhild view. */
//************************************************************************************************

class TabView: public Control,
			   public IItemDragTarget
{
public:
	DECLARE_CLASS (TabView, Control)

	TabView (const Rect& size = Rect (), IParameter* param = nullptr, StyleRef style = 0);
	~TabView ();

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_MUTABLE_CSTRING (persistenceID, PersistenceID)

	int countTabs () const;
	void activateTab (int index);
	int getActiveIndex () const;

	StringRef getTabTitle (String& title, int index) const;
	IImage* getTabIcon (int index) const;
	View* getTabView (int index) const;
	int getMouseOverTab () const;
	int getMouseDownTab () const;
	int findTab (const Point& where);

	PROPERTY_VARIABLE (Coord, scrollOffset, ScrollOffset)
	PROPERTY_VARIABLE (Coord, fillWidth, FillWidth)
	PROPERTY_VARIABLE (Coord, centerOffset, CenterOffset)
	PROPERTY_BOOL (menu, Menu)

	enum TabViewParts
	{
		kPartNone     = 0,
		kPartContent  = 1,
		kPartViewSize = 2,
		kPartHeader   = 3,
		kPartFirstTab = 100,
		kPartLastTab  = 200,
		kPartMenuTab  = kPartLastTab,
		kPartTabMenu  = 300,	///< optional menu icon in tab; rectangle inside tab rect (not returned by hitTest)
	};

	// IItemDragTarget
	IDragHandler* CCL_API createDragHandler (int flags = kCanDragBetweenItems, IItemDragVerifier* verifier = nullptr) override;

	// Control
	bool addView (View* view) override;
	bool removeView (View* view) override;
	void onSize (const Point& delta) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	MouseHandler* createMouseHandler (const MouseEvent& event) override;
	IDragHandler* createDragHandler (const DragEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	void calcAutoSize (Rect& r) override;
	void draw (const UpdateRgn& updateRgn) override;
	void paramChanged () override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	ThemeRenderer* getRenderer () override;

	CLASS_INTERFACE (IItemDragTarget, Control)

private:
	friend class TabViewMouseHandler;
	friend class TabViewDragHandlerBase;
	friend class ReorderTabsDragHandler;
	class TouchMouseHandler;
	
	ThemeRenderer* renderer;
	int mouseOverTab;
	int mouseDownTab;
	bool preferIcon;
	LinkedList<View*> views;

	IAttributeList* getViewState (bool create);
	void init (IAttributeList* savedState = nullptr);
	bool mouseDown  (const MouseEvent& event);
	bool mouseUp (const MouseEvent& event);
	void setMouseOver (int tab);
	void setMouseDown (int tab);
	void showMenu (int tabIndex = -1);
	bool canDragTabs (const MouseEvent& event) const;
	void dragTab (int tabIndex);
	void updateStyle ();

	void invalidateHeader ();
	void invalidateTab (int index);
	void getViewSize (Rect& rect);
};

//************************************************************************************************
// ITabViewRenderer
//************************************************************************************************

interface ITabViewRenderer: IUnknown
{
	/** Draw one tab. */
	virtual void drawTab (View* view, GraphicsDevice& port, RectRef r, int tabIndex) = 0;

	DECLARE_IID (ITabViewRenderer)
};

} // namespace CCL 

#endif // _tabview_h
