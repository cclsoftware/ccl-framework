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
// Filename    : ccl/gui/itemviews/itemview.h
// Description : Item View
//
//************************************************************************************************

#ifndef _ccl_itemview_h
#define _ccl_itemview_h

#include "ccl/gui/itemviews/itemviewbase.h"
#include "ccl/gui/theme/visualstyleclass.h"
#include "ccl/public/gui/icommandhandler.h"
#include "ccl/public/gui/framework/itooltip.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

class Image;
class ColumnHeaderList;
class ItemViewController;
class DragSession;

//************************************************************************************************
// ItemStyle
/** Base class for list and tree style. */
//************************************************************************************************

class ItemStyle: public Object
{
public:
	DECLARE_CLASS (ItemStyle, Object)

	ItemStyle ();
	ItemStyle (const ItemStyle&);
	~ItemStyle ();

	PROPERTY_VARIABLE (int, marginH, MarginH)
	PROPERTY_VARIABLE (int, marginV, MarginV)
	PROPERTY_VARIABLE (int, rowHeight, RowHeight)
	PROPERTY_VARIABLE (int, scrollRows, ScrollRows)

	PROPERTY_OBJECT (SolidBrush, backBrush1, BackBrush1)
	PROPERTY_OBJECT (SolidBrush, backBrush2, BackBrush2)
	PROPERTY_OBJECT (SolidBrush, selectBrush, SelectBrush)
	PROPERTY_OBJECT (SolidBrush, textBrush, TextBrush)
	PROPERTY_OBJECT (SolidBrush, deleteTextBrush, DeleteTextBrush)
	PROPERTY_OBJECT (SolidBrush, selectedTextBrush, SelectedTextBrush)
	PROPERTY_OBJECT (Color, iconColor, IconColor)
	PROPERTY_OBJECT (Color, selectedIconColor, SelectedIconColor)
	PROPERTY_OBJECT (Pen, separatorPen, SeparatorPen)
	PROPERTY_BOOL (separatorBeneath, SeparatorBeneath)
	PROPERTY_BOOL (highQualityMode, HighQualityMode)
	PROPERTY_BOOL (vSnapEnabled, VSnapEnabled)
	
	PROPERTY_OBJECT (Pen, focusPen, FocusPen)

	PROPERTY_VARIABLE (int, thumbnailMarginV, ThumbnailMarginV)
	PROPERTY_VARIABLE (int, thumbnailPaddingLeft, ThumbnailPaddingLeft)
	PROPERTY_VARIABLE (int, thumbnailPaddingTop, ThumbnailPaddingTop)
	PROPERTY_VARIABLE (int, thumbnailPaddingBottom, ThumbnailPaddingBottom)
	PROPERTY_VARIABLE (int, thumbnailLimitHeight, ThumbnailLimitHeight)
	PROPERTY_VARIABLE (int, thumbnailLimitWidth, ThumbnailLimitWidth)
	PROPERTY_VARIABLE (float, thumbnailFactor, ThumbnailFactor)
	PROPERTY_OBJECT (Pen, thumbnailFramePen, ThumbnailFramePen)
	PROPERTY_SHARED_AUTO (IImage, selectionBarImage, SelectionBarImage)
	
	void setDefaultIcon (Image* icon, bool open = false);
	Image* getDefaultIcon (bool open = false) const;
	void setBackgroundImage (Image* bg);
	Image* getBackgroundImage () const;
	Image* getDeleteButtonImage () const;

	void setMargin (int m);
	
	struct CustomBackground;
	CustomBackground* getCustomBackground (StringID name, const IVisualStyle& style);
	void discardCustomBackgrounds ();

	virtual void updateStyle (Theme& theme);
	virtual void updateStyle (const VisualStyle& style);

	virtual void zoom (const ItemStyle& original, float zoomFactor);

	Rect getDeleteButtonRect (RectRef itemRect, FontRef font);
	void drawDeleteButton (GraphicsPort& port, RectRef deleteRect, FontRef font);

protected:
	Image* defaultIcon;
	Image* defaultOpenIcon;
	Image* backgroundImage;
	Image* deleteButtonImage;
	ObjectArray customBackgrounds;
};

DECLARE_VISUALSTYLE_CLASS (ItemStyle)

//************************************************************************************************
// ItemView
/** Base class for list and tree views. */
//************************************************************************************************

class ItemView: public ItemViewBase,
				public IEditControlHost
{
public:
	DECLARE_CLASS (ItemView, ItemViewBase)
	DECLARE_STYLEDEF (customStyles)

	ItemView (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~ItemView ();

	ItemStyle& getItemStyle () const;
	int getRowsPerPage () const;

	PROPERTY_SHARED_AUTO (VisualStyle, headerViewStyle, HeaderViewStyle)
	ColumnHeaderList* getColumnHeaders () const;
	void setColumnHeaders (ColumnHeaderList* list);
	void updateColumns ();

	// ItemViewBase
	void CCL_API setModel (IItemModel* model) override;
	void CCL_API setZoomFactor (float factor) override;
	void CCL_API makeItemVisible (ItemIndexRef index) override;
	void CCL_API setEditControl (IView* view, tbool directed = true) override;
	tbool CCL_API findItemCell (ItemIndex& idx, int& column, const Point& where) const override;
	Font& getFont (Font& font) const override;

	// View
	void attached (View* parent) override;
	void onVisualStyleChanged () override;
	const IVisualStyle& CCL_API getVisualStyle () const override;
	void setTheme (Theme* theme) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onMouseEnter (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onTrackTooltip (const TooltipEvent& event) override;
	IUnknown* CCL_API getController () const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	bool onGesture (const GestureEvent& event) override;
	void onSize (const Point& delta) override;
	void makeAccessibilityProvider (bool state) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	
	// IEditControlHost
	tbool CCL_API onEditNavigation (const KeyEvent& event, IView* control) override;
	void CCL_API onEditControlLostFocus (IView* control) override;

	// Command methods
	virtual bool onEditCut (const CommandMsg&);
	virtual bool onEditCopy (const CommandMsg&);
	virtual bool onEditPaste (const CommandMsg&);
	virtual bool onEditDelete (const CommandMsg&);

	void drawSolidBackground (const UpdateRgn& updateRgn);
	void drawAlternatingBackground (const UpdateRgn& updateRgn);
	void drawThumbnail (GraphicsPort& port, Image& thumbnailImage, PointRef pos);
	virtual Coord determineRowHeight (ItemIndexRef itemIndex);

	CLASS_INTERFACE (IEditControlHost, ItemViewBase)

protected:
	ItemStyle* itemStyle;
	ItemStyle* savedStyle;
	ItemIndex tooltipItem;
	int tooltipColumn;
	ColumnHeaderList* columnList;
	mutable AutoPtr<IUnknown> controller;
	View* editControl;

	void setItemStyle (ItemStyle* itemStyle);	///< view takes ownership!

	class ColumnCalculator;
	friend class ItemViewController;
	enum NavigationMode
	{
		kSelect,
		kSelectExtend,
		kSelectExtendAdd,
		kSkip
	};

	friend class ItemControl;
	enum PrivateFlags
	{
		kHasAlternatingBackground	= 1<<(kItemViewBaseLastPrivateFlag + 1),
		kHasCustomBackgrounds		= 1<<(kItemViewBaseLastPrivateFlag + 2)
	};
	PROPERTY_FLAG (privateFlags, kHasAlternatingBackground, hasAlternatingBackground)
	PROPERTY_FLAG (privateFlags, kHasCustomBackgrounds, hasCustomBackgrounds)

	NavigationMode validateNavigationMode (NavigationMode mode) const;

	/// to be implemented by subclass
	virtual Point getBackgroundOffset () const;
	virtual bool getAnchorItem (ItemIndex& index) const;
	virtual bool setAnchorItem (ItemIndexRef index);
	virtual bool selectRange (ItemIndexRef fromIndex, ItemIndexRef toIndex);
	virtual bool navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly);
	virtual int getItemHeight (ItemIndexRef index) const;
	virtual int getItemRow (ItemIndexRef index) const;
	virtual int getColumnIndex (PointRef where);
	virtual bool getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn);
	virtual int getLogicalColumnIndex (PointRef where); ///< column showing item data (not column in icon layout)
	virtual ColumnHeaderList* getVisibleColumnList () const;
	virtual int getStandardStyleIndex () const;

	bool getFirstCommandItem (ItemIndex& item);
	int toModelColumnIndex (int column) const; ///< visible column position to index used by item model (when columns have been hidden/moved by user)

	/// to be used by subclass
	void doSelection (ItemIndexRef clickedItem, const MouseEvent& event);
	void doSelection (ItemIndexRef clickedItem, const GestureEvent& event);
	bool tryRubberSelection (const MouseEvent& event);
	bool tryDrag (const MouseEvent& event);
	bool dragItems (DragSession& session, int inputDevice);
	bool shouldDrawFocus () const;
	void drawFocusRect (GraphicsPort& port, RectRef rect);
	ItemStyle::CustomBackground* getCustomBackground (StringID name);
	IImage* getThumbnail (ItemIndexRef index) const;
	Coord getThumbnailAreaHeight (IImage* image) const;
	
	// ItemViewBase
	void modelChanged (int changeType, ItemIndexRef item) override;
	void onEditModeChanged (bool state) override;
	bool openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect = Rect ()) override;
	tbool editCell (ItemIndexRef item, int column, RectRef rect, const GUIEvent& editEvent) override;
	IImage* getDragImageForItem (ItemIndexRef item) override;

private:
	NavigationMode getNavigationMode (const KeyEvent& event);

	void updateItemStyle ();
	void autoSizeColumns ();
	bool getLimitedThumbnailSize (Rect& limitedSize, IImage* image) const;
};

//************************************************************************************************
// ItemControl
/** Base class for scrollable list and tree views. */
//************************************************************************************************

class ItemControl: public ItemControlBase
{
public:
	DECLARE_CLASS (ItemControl, ItemControlBase)

	ItemControl (const Rect& size = Rect (), 
				 ItemView* itemView = nullptr,
				 StyleRef scrollViewStyle = Styles::kScrollViewAppearanceScrollBars);

	void setHeaderViewStyle (VisualStyle* visualStyle);

	// ScrollView
	float getScrollSpeedV () const override;
	void drawBackground (const UpdateRgn& updateRgn) override;
};

//************************************************************************************************
// ItemViewController
/** Controller that handles commands for item views. */
//************************************************************************************************

class ItemViewController: public Unknown,
						  public ICommandHandler
{
public:
	ItemViewController (ItemView* view);

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;

	CLASS_INTERFACE (ICommandHandler, Unknown)

protected:
	ItemView* view;
};

//************************************************************************************************
// ItemStyle::CustomBackground
//************************************************************************************************

struct ItemStyle::CustomBackground: public Object
{
	MutableCString name;
	SolidBrush* brush[2];
	Pen* separatorPen;
	Coord rowHeight;
	Coord iconSize;
	const Font* textFont;
	
	CustomBackground (StringID name);
	~CustomBackground ();
};

} // namespace CCL

#endif // _ccl_itemview_h
