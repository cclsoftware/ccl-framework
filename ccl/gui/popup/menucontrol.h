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
// Filename    : ccl/gui/popup/menucontrol.h
// Description : Menu Control
//
//************************************************************************************************

#ifndef _ccl_menucontrol_h
#define _ccl_menucontrol_h

#include "ccl/gui/popup/menu.h"
#include "ccl/gui/views/scrollview.h"

#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/framework/idleclient.h"

namespace CCL {

class Window;
class PopupSelector;
class PopupSelectorWindow;
class CompactMenuControl;
class MarkupPainter;

//************************************************************************************************
// IMenuControl
/** Internal interface for different MenuControl implementations. */
//************************************************************************************************

interface IMenuControl: IUnknown
{
	virtual PopupSelectorClient* getPopupClient () const = 0;

	virtual MenuItem* getResultItem () const = 0;

	DECLARE_IID (IMenuControl)
};

//************************************************************************************************
// MenuItemPainter
//************************************************************************************************

class MenuItemPainter: public Object
{
public:
	MenuItemPainter ();
	~MenuItemPainter ();

	PROPERTY_OBJECT (Font, font, Font)
	PROPERTY_OBJECT (Font, smallFont, SmallFont)
	PROPERTY_VARIABLE (Color, backColor, BackColor)
	PROPERTY_VARIABLE (Color, secondaryBackColor, SecondaryBackColor)
	PROPERTY_VARIABLE (Color, borderColor, BorderColor)
	PROPERTY_VARIABLE (Color, separatorColor, SeparatorColor)
	PROPERTY_VARIABLE (Color, headerColor, HeaderColor)
	PROPERTY_VARIABLE (Color, selectionBackColor, SelectionColor)
	PROPERTY_VARIABLE (Color, selectionFrameColor, SelectionFrameColor)
	PROPERTY_VARIABLE (Color, textColor, TextColor)
	PROPERTY_VARIABLE (Color, disabledTextColor, DisabledTextColor)
	PROPERTY_VARIABLE (Color, headerTextColor, HeaderTextColor)
	PROPERTY_VARIABLE (Color, selectedTextColor, SelectedTextColor)
	PROPERTY_VARIABLE (Coord, normalIconSize, NormalIconSize)
	PROPERTY_VARIABLE (Coord, largeIconSize, LargeIconSize)
	PROPERTY_VARIABLE (Color, iconColor, IconColor)
	PROPERTY_VARIABLE (Color, selectedIconColor, SelectedIconColor)
	PROPERTY_VARIABLE (Coord, spacing, Spacing)
	PROPERTY_VARIABLE (Coord, separatorSpacing, SeparatorSpacing)
	PROPERTY_VARIABLE (Coord, headerSpacing, HeaderSpacing)
	PROPERTY_VARIABLE (Coord, segmentSpacing, SegmentSpacing)
	PROPERTY_VARIABLE (Coord, segmentMargin, SegmentMargin)
	PROPERTY_VARIABLE (Coord, explicitRowHeight, ExplicitRowHeight)
	PROPERTY_VARIABLE (Coord, menuArrowWidth, MenuArrowWidth)
	PROPERTY_VARIABLE (Coord, closeIconWidth, CloseIconWidth)
	PROPERTY_VARIABLE (Coord, checkMarkWidth, CheckMarkWidth)
	PROPERTY_VARIABLE (Coord, fixedSubMenuWidth, FixedSubMenuWidth)
	PROPERTY_SHARED_AUTO (IImage, checkMarkIcon, CheckMarkIcon)
	PROPERTY_SHARED_AUTO (IImage, selectionBarImage, SelectionBarImage)
	PROPERTY_SHARED_AUTO (IImage, menuArrowIcon, MenuArrowIcon)
	PROPERTY_SHARED_AUTO (IImage, closeIcon, CloseIcon)

	void updateStyle (const IVisualStyle& visualStyle);

	PROPERTY_VARIABLE (Coord, maxViewWidth, MaxViewWidth)
	PROPERTY_VARIABLE (Coord, maxTitleWidth, MaxTitleWidth)
	PROPERTY_VARIABLE (Coord, maxKeyWidth, MaxKeyWidth)
	PROPERTY_BOOL (checkMarkNeeded, CheckMarkNeeded)
	PROPERTY_BOOL (iconSpaceNeeded, iconSpaceNeeded)
	void recalc (const Menu& menu);

	enum MenuVariant { kNormal, kLarge };
	enum ItemType { kRegular, kSeparator, kHeader, kSubMenu, kSplitMenu, kViewItem };

	ItemType getItemType (const MenuItem& item) const;
	String getDisplayTitle (const MenuItem& item) const;
	MenuVariant getMenuVariant (const MenuItem& item) const;

	Rect getItemSize (const MenuItem& item) const;
	Rect getBackButtonSize (RectRef itemSize) const;

	void drawItem (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state, bool parentOfCurrentSubMenu) const;
	void drawBackButton (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state) const;
	void drawCloseButton (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state) const;

protected:
	MarkupPainter* markupPainter;

	Coord getIconSize (MenuVariant variant) const;
	Coord getItemHeight (MenuVariant variant, ItemType type, const MenuItem& item) const;

	struct ItemMetrics
	{
		ItemType type;

		Coord width;
		Coord height;

		Coord checkPos;
		Coord iconPos;
		Coord iconWidth;
		Coord titlePos;
		Coord keyPos;
		Coord arrowPos;
		Coord rowCount;

		ItemMetrics ()
		: type (kRegular), width (0), height (0), 
		  checkPos (0), iconPos (0), iconWidth (0), titlePos (0), keyPos (0), arrowPos (0), rowCount (1)
		{}
	};

	void getItemMetrics (ItemMetrics& metrics, const MenuItem& item) const;
	Color getItemTextColor (const MenuItem& item, int state) const;
	void drawItemBackground (IGraphics& graphics, RectRef itemSize, const MenuItem& item, int state, bool parentOfCurrentSubMenu) const;
	void drawSeparatorBottom (IGraphics& graphics, RectRef itemSize) const;
	void drawMenuArrow (IGraphics& graphics, RectRef itemSize, bool enabled, int state, bool drawBackArrow = false) const;
};

DECLARE_VISUALSTYLE_CLASS (MenuControl)

//************************************************************************************************
// MenuControl
//************************************************************************************************

class MenuControl: public ScrollView,
				   public IdleClient,
				   public IMenuControl
{
public:
	DECLARE_CLASS (MenuControl, ScrollView)

	MenuControl (Menu* menu = nullptr,
				 VisualStyle* menuStyle = nullptr,
				 View* target = nullptr,
				 StyleRef scrollStyle = StyleFlags (0, Styles::kScrollViewBehaviorAutoHideVButtons));

	~MenuControl ();

	class ClientView;
	class ItemButton;
	class PopupClient;

	VisualStyle* getMenuStyle () const;
	ClientView* getClient () const;
	bool isTopLevel () const;

	bool navigate (const KeyEvent& event);
	void closeAll (bool deferred = false);

	void setResultItem (MenuItem* _resultItem) { resultItem = _resultItem; }
	
	PROPERTY_BY_VALUE (MenuControl*, parentControl, ParentControl)

	void updateSize ();

	// IMenuControl
	PopupSelectorClient* getPopupClient () const override;
	MenuItem* getResultItem () const override { return resultItem; }

	CLASS_INTERFACE2 (ITimerTask, IMenuControl, ScrollView)

protected:
	PopupClient* popupClient;
	ObservedPtr<MenuControl> parentControl;
	SharedPtr<MenuItem> resultItem;
	Point initialMousePos;
	SharedPtr<VisualStyle> menuStyle;

	bool popup (const Point& where, View* view = nullptr);
	void trackItem (Window* window, PointRef mousePos);
	void suspendMouseTracking ();
	void closeAllInternal (KeyState keyState);

	virtual ClientView* getActiveClientView ();
	virtual Window* findActiveMouseWindow ();
	virtual View* findActiveMouseView (Window* mouseWindow, PointRef mousePos);
	
	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IdleClient
	void onIdleTimer () override;
};

//************************************************************************************************
// MenuControl::PopupClient
//************************************************************************************************

class MenuControl::PopupClient: public Object,
								public PopupSelectorClient
{
public:
	DECLARE_CLASS_ABSTRACT (PopupClient, Object)

	PopupClient (MenuControl& control);

	void closeAll (bool deferred = false);

	PROPERTY_BOOL (cancelOnMouseUp, CancelOnMouseUp)

	// PopupSelectorClient
	void CCL_API attached (IWindow& popupWindow) override;
	void CCL_API onPopupClosed (Result result) override;
	bool hasPopupResult () override;
	Result CCL_API onKeyDown (const KeyEvent& event) override;
	Result CCL_API onKeyUp (const KeyEvent& event) override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onMouseUp (const MouseEvent& event, IWindow& popupWindow) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IPopupSelectorClient, Object)

protected:
	MenuControl& control;
};

//************************************************************************************************
// MenuControl::ClientView
//************************************************************************************************

class MenuControl::ClientView: public View
{
public:
	DECLARE_CLASS (ClientView, View)

	ClientView (Menu* menu = nullptr, VisualStyle* menuStyle = nullptr);
	void construct ();

	Menu* getMenu () const;
	MenuItemPainter* getPainter () const;
	virtual void updateSize ();

	MenuItem* getClickedItem () const;
	virtual void setClickedItem (MenuItem* item);

	virtual void setCurrentItem (ItemButton* button, bool keyNavigation);
	ItemButton* getCurrentItem () const;
	ItemButton* getOpenSubMenuItem ();
	ItemButton* findSubMenuItem (StringRef name) const;

	virtual void checkSubMenus ();
	virtual Coord getMaxControlHeight () const;
	
	enum Direction
	{
		kMoveLeft,
		kMoveUp,
		kMoveRight,
		kMoveDown,
	};
	
	ItemButton* getNextSelectableItem (Direction direction);
	bool selectNextItem (bool keyNavigation);

	MenuControl* getRootMenuControl ();

	// View
	void draw (const UpdateRgn& updateRgn) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;

protected:
	virtual ItemButton* createItemButton (MenuItem* item);
	
	SharedPtr<Menu> menu;
	AutoPtr<MenuItemPainter> painter;
	SharedPtr<ItemButton> currentItem;
	SharedPtr<MenuItem> clickedItem;
	int64 nextSubMenuCheck;
	bool wasKeyNavigation;
	int pageBreakIndex;

	PROPERTY_VARIABLE (Coord, margin, Margin)
};

//************************************************************************************************
// MenuControl::ItemButton
//************************************************************************************************

class MenuControl::ItemButton: public View
{
public:
	DECLARE_CLASS (ItemButton, View)

	ItemButton (MenuItemPainter* painter = nullptr, MenuItem* item = nullptr);

	MenuItem* getItem () const;

	virtual bool isClickable () const;
	bool canOpenSubMenu () const;
	void select ();
	
	virtual bool popupSubMenu (bool keyNavigation = false);
	virtual void closeSubMenu ();
	bool isSubMenuOpen () const;
	MenuControl* getSubMenuControl () const;
	void setSubMenuControl (MenuControl* control);

	virtual bool onNavigate (const KeyEvent& event);

	// View
	void calcAutoSize (Rect& r) override;
	void draw (const UpdateRgn& updateRgn) override;
	bool onMouseDown (const MouseEvent& event) override;
	AccessibilityProvider* getAccessibilityProvider () override;

protected:
	SharedPtr<MenuItemPainter> painter;
	SharedPtr<MenuItem> item;
	MenuControl* subMenuControl;
	PopupSelector* subPopupSelector;

	void closeSubMenuInternal ();
};

//************************************************************************************************
// CompactMenuContainer
//************************************************************************************************

class CompactMenuContainer: public View,
							public IMenuControl
{
public:
	DECLARE_CLASS_ABSTRACT (CompactMenuContainer, View)

	CompactMenuContainer (Menu* menu = nullptr, VisualStyle* menuStyle = nullptr, int maxColumns = 2);

	PROPERTY_VARIABLE (int, maxColumns, MaxColumns)

	static int getTotalColumnsInMenu (const Menu& menu); ///< independent of view state

	CompactMenuControl* getControl (int columnIndex) const;

	int countColumns () const;	///< curent number open sub menu columns
	int getFirstVisibleColumn () const;
	int getBackButtonColumn () const;
	void addColumn (CompactMenuControl* control);
	void removeColumn (int index = -1); ///< -1: deepest

	CompactMenuControl* createMenuControl (Menu* menu, CompactMenuControl* parentControl);

	// IMenuControl
	PopupSelectorClient* getPopupClient () const override;
	MenuItem* getResultItem () const override;

	// View
	void attached (View* parent) override;
	void onSize (const Point& delta) override;

	CLASS_INTERFACE (IMenuControl, View)

private:
	SharedPtr<VisualStyle> menuStyle;
	Rect availableScreenSize;
	Point minColumnSize;
	Point requestedSize;
	bool unifyColumnWidth;
	bool needsCloseButton;
	bool wasAttached;

	class ColumnSizeHelper;

	static Rect getAvailableScreenSize ();

	Coord getMinColumnWidth () const;
	void removeColumnViewInternal (CompactMenuControl* control);
	void layoutColumns ();
	void updateBackButton ();
};

//************************************************************************************************
// CompactMenuControl
//************************************************************************************************

class CompactMenuControl: public MenuControl
{
public:
	DECLARE_CLASS (CompactMenuControl, MenuControl)

	CompactMenuControl (Menu* menu = nullptr, VisualStyle* menuStyle = nullptr);

	class ClientView;
	class ItemButton;
	class HeaderButton;
	class BackButton;
	class CloseButton;

	enum HeaderType { kHeaderNone, kHeaderCloseButton, kHeaderBackButton, kHeaderBackAndCloseButton };

	ClientView* getCompactClient () const;
	CompactMenuContainer* getContainer () const;

	// MenuControl
	Window* findActiveMouseWindow () override;
	MenuControl::ClientView* getActiveClientView () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
};

//************************************************************************************************
// CompactMenuControl::ClientView
//************************************************************************************************

class CompactMenuControl::ClientView: public MenuControl::ClientView
{
public:
	DECLARE_CLASS (ClientView, MenuControl::ClientView)

	ClientView (Menu* menu = nullptr, VisualStyle* menuStyle = nullptr);

	void initWithParent (const ClientView& parentView);

	PROPERTY_VARIABLE (int, depth, Depth) ///< column index of this menu
	
	PROPERTY_VARIABLE (Coord, maxWidth, MaxWidth)
	PROPERTY_VARIABLE (Point, minSize, MinSize)
	PROPERTY_VARIABLE (Coord, minColumnHeight, MinColumnHeight)
	void setMinWidth (Coord minWidth);
	void setMinHeight (Coord minHeight);

	CompactMenuContainer* getContainer () const;
	CompactMenuControl* getCompactControl () const;
	ClientView* getSubClient () const;
	ClientView* getParentClient () const;
	ClientView* getActiveClientView () const;

	void updateHeader (CompactMenuControl::HeaderType type);
	void closeDeepestMenu ();

	bool isUpdatingSize () const;

	// MenuControl::ClientView
	void setClickedItem (MenuItem* item) override;
	void checkSubMenus () override;
	void updateSize () override;
	void setCurrentItem (MenuControl::ItemButton* button, bool keyNavigation) override;
	void onChildSized (View* child, const Point& delta) override;

protected:
	bool inUpdateSize;

	MenuControl::ItemButton* createItemButton (MenuItem* item) override;
};

//************************************************************************************************
// CompactMenuControl::ItemButton
//************************************************************************************************

class CompactMenuControl::ItemButton: public MenuControl::ItemButton
{
public:
	DECLARE_CLASS (ItemButton, MenuControl::ItemButton)

	ItemButton (MenuItemPainter* painter = nullptr, MenuItem* item = nullptr);

	// MenuControl::ItemButton
	bool popupSubMenu (bool keyNavigation = false) override;
	void closeSubMenu () override;
	bool onMouseDown (const MouseEvent& event) override;
};

//************************************************************************************************
// CompactMenuControl::HeaderButton
//************************************************************************************************

class CompactMenuControl::HeaderButton: public CompactMenuControl::ItemButton
{
public:
	DECLARE_CLASS_ABSTRACT (HeaderButton, ItemButton)

	HeaderButton (MenuItemPainter* painter = nullptr);

	// CompactMenuControl::ItemButton
	bool isClickable () const override;
	void draw (const UpdateRgn& updateRgn) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove  (const MouseEvent& event) override;
	bool onMouseLeave (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;

private:
	bool isActive;

	void checkActiveArea (PointRef position);

	virtual Rect getActiveArea (RectRef itemSize) const;
	virtual void drawButton (IGraphics& graphics, RectRef rect, int state) = 0;
	virtual void push () = 0;
};

//************************************************************************************************
// CompactMenuControl::BackButton
//************************************************************************************************

class CompactMenuControl::BackButton: public CompactMenuControl::HeaderButton
{
public:
	DECLARE_CLASS_ABSTRACT (BackButton, HeaderButton)

	BackButton (MenuItemPainter* painter = nullptr, Menu* menu = nullptr);

private:
	// CompactMenuControl::HeaderButton
	Rect getActiveArea (RectRef itemSize) const override;
	bool onNavigate (const KeyEvent& event) override;
	void drawButton (IGraphics& graphics, RectRef rect, int state) override;
	void push () override;
};

//************************************************************************************************
// CompactMenuControl::CloseButton
//************************************************************************************************

class CompactMenuControl::CloseButton: public CompactMenuControl::HeaderButton
{
public:
	DECLARE_CLASS_ABSTRACT (CloseButton, HeaderButton)

	CloseButton (MenuItemPainter* painter = nullptr);

private:
	// CompactMenuControl::HeaderButton
	void drawButton (IGraphics& graphics, RectRef rect, int state) override;
	void push () override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline MenuItemPainter* MenuControl::ClientView::getPainter () const
{ return painter; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline MenuItem* MenuControl::ClientView::getClickedItem () const
{ return clickedItem; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CompactMenuControl* CompactMenuControl::ClientView::getCompactControl () const
{ return getParent<CompactMenuControl> (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CompactMenuContainer* CompactMenuControl::ClientView::getContainer () const
{ return getParent<CompactMenuContainer> (); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool CompactMenuControl::ClientView::isUpdatingSize () const
{ return inUpdateSize; }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CompactMenuControl::ClientView* CompactMenuControl::getCompactClient () const
{
	ASSERT (!getClient () || ccl_cast<ClientView> (getClient ()))
	return static_cast<ClientView*> (getClient ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline CompactMenuContainer* CompactMenuControl::getContainer () const
{ return getParent<CompactMenuContainer> (); }

} // namespace CCL

#endif // _ccl_menucontrol_h
