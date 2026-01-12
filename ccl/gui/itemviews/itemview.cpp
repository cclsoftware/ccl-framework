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
// Filename    : ccl/gui/controls/itemviews/itemview.cpp
// Description : Item View
//
//************************************************************************************************

#include "ccl/gui/itemviews/itemview.h"
#include "ccl/gui/itemviews/headerview.h"
#include "ccl/gui/itemviews/itemviewaccessibility.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/keyevent.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/graphics/imaging/image.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/touch/touchhandler.h"
#include "ccl/gui/views/sprite.h"
#include "ccl/gui/views/focusnavigator.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/graphics/imaging/multiimage.h"
#include "ccl/gui/graphics/imaging/coloredbitmap.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/gui/commanddispatch.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

namespace CCL {

//************************************************************************************************
// ItemView::ColumnCalculator
//************************************************************************************************

class ItemView::ColumnCalculator: public Object,
								  public IColumnCalculator
{
public:
	ColumnCalculator (ItemView* itemView);

	PROPERTY_POINTER (ItemView, itemView, ItemView)

	// IColumnCalculator
	tbool CCL_API calcColumnWidth (int& width, int columnIndex) override;
	
	CLASS_INTERFACE (IColumnCalculator, Object)
};								  

//************************************************************************************************
// DrawItemSelectionHandler
//************************************************************************************************

class DrawItemSelectionHandler: public MouseHandler
{
public:
	DrawItemSelectionHandler (ItemView* view);

	// MouseHandler
	void onBegin () override;
	bool onMove (int moveFlags) override;
	void onRelease (bool canceled) override;

protected:
	ItemView* itemView;
	Sprite* sprite;
	UnknownPtr<IItemSelection> oldSelection;
	ItemIndex newFocusItem;
};

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Edit")
	XSTRING (Delete, "Delete")
END_XSTRINGS

//////////////////////////////////////////////////////////////////////////////////////////////////
// Commands
//////////////////////////////////////////////////////////////////////////////////////////////////

REGISTER_COMMAND ("Navigation", "Left")
REGISTER_COMMAND ("Navigation", "Left Extend")
REGISTER_COMMAND ("Navigation", "Left Extend Add")
REGISTER_COMMAND ("Navigation", "Left Skip")

REGISTER_COMMAND ("Navigation", "Right")
REGISTER_COMMAND ("Navigation", "Right Extend")
REGISTER_COMMAND ("Navigation", "Right Extend Add")
REGISTER_COMMAND ("Navigation", "Right Skip")

REGISTER_COMMAND ("Navigation", "Up")
REGISTER_COMMAND ("Navigation", "Up Extend")
REGISTER_COMMAND ("Navigation", "Up Extend Add")
REGISTER_COMMAND ("Navigation", "Up Skip")

REGISTER_COMMAND ("Navigation", "Down")
REGISTER_COMMAND ("Navigation", "Down Extend")
REGISTER_COMMAND ("Navigation", "Down Extend Add")
REGISTER_COMMAND ("Navigation", "Down Skip")

REGISTER_COMMAND ("Navigation", "Start")
REGISTER_COMMAND ("Navigation", "Start Extend")
REGISTER_COMMAND ("Navigation", "Start Extend Add")
REGISTER_COMMAND ("Navigation", "Start Skip")

REGISTER_COMMAND ("Navigation", "End")
REGISTER_COMMAND ("Navigation", "End Extend")
REGISTER_COMMAND ("Navigation", "End Extend Add")
REGISTER_COMMAND ("Navigation", "End Skip")

REGISTER_COMMAND ("Navigation", "Page Up")
REGISTER_COMMAND ("Navigation", "Page Up Extend")
REGISTER_COMMAND ("Navigation", "Page Up Extend Add")
REGISTER_COMMAND ("Navigation", "Page Up Skip")

REGISTER_COMMAND ("Navigation", "Page Down")
REGISTER_COMMAND ("Navigation", "Page Down Extend")
REGISTER_COMMAND ("Navigation", "Page Down Extend Add")
REGISTER_COMMAND ("Navigation", "Page Down Skip")

REGISTER_COMMAND ("Edit", "Cut")
REGISTER_COMMAND ("Edit", "Copy")
REGISTER_COMMAND ("Edit", "Paste")
REGISTER_COMMAND ("Edit", "Delete")

REGISTER_COMMAND ("Edit", "Select All")
REGISTER_COMMAND ("Edit", "Deselect All")

//************************************************************************************************
// ItemViewController
//************************************************************************************************

ItemViewController::ItemViewController (ItemView* view)
: view (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewController::checkCommandCategory (CStringRef category) const
{
	return true; // we don't know the categories handled by the model
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#define INTERPRET_DIRECTION_CMDS(rows,cols,dir)\
	if(msg.name == dir)					return view->navigate (rows, cols, view->validateNavigationMode (ItemView::kSelect),			msg.checkOnly ());\
	if(msg.name == dir" Extend")		return view->navigate (rows, cols, view->validateNavigationMode (ItemView::kSelectExtend),		msg.checkOnly ());\
	if(msg.name == dir" Extend Add")	return view->navigate (rows, cols, view->validateNavigationMode (ItemView::kSelectExtendAdd),	msg.checkOnly ());\
	if(msg.name == dir" Skip")			return view->navigate (rows, cols, view->validateNavigationMode (ItemView::kSkip),				msg.checkOnly ());

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemViewController::interpretCommand (const CommandMsg& msg)
{
	// try model first to allow overriding the built-in commands below
	IItemModel* model =	view->getModel ();
	if(model)
	{
		ItemIndex commandItem;
		view->getFirstCommandItem (commandItem);
		if(model->interpretCommand (msg, commandItem, view->getSelection ()))
			return true;
	}

	if(msg.category == "Navigation")
	{
		INTERPRET_DIRECTION_CMDS (0, -1, "Left")
		INTERPRET_DIRECTION_CMDS (0, +1, "Right")
		INTERPRET_DIRECTION_CMDS (-1, 0, "Up")
		INTERPRET_DIRECTION_CMDS (+1, 0, "Down")

		INTERPRET_DIRECTION_CMDS (- view->getRowsPerPage (), 0, "Page Up")
		INTERPRET_DIRECTION_CMDS (  view->getRowsPerPage (), 0, "Page Down")

		INTERPRET_DIRECTION_CMDS (NumericLimits::kMinInt, 0, "Start")
		INTERPRET_DIRECTION_CMDS (NumericLimits::kMaxInt, 0, "End")

		if(view->getStyle ().isCustomStyle (Styles::kItemViewBehaviorColumnFocus))
		{
			// column navigation with Tab / Shift+Tab
			if(msg.name == "Focus Next")
				return view->navigate (0, 1, ItemView::kSelect, msg.checkOnly ());
			else if(msg.name == "Focus Previous")
				return view->navigate (0, -1, ItemView::kSelect, msg.checkOnly ());
		}

		if(msg.name == "Back")
		{
			// leave edit mode
			if(view->isEditMode ())
				view->setEditMode (false);
		}
	}
	else if(msg.category == "Edit")
	{
		if(msg.name == "Cut")
		{
			if(view->onEditCut (msg))
				return true;
		}
		else if(msg.name == "Copy")
		{
			if(view->onEditCopy (msg))
				return true;
		}
		else if(msg.name == "Paste")
		{
			if(view->onEditPaste (msg))
				return true;
		}
		else if(msg.name == "Delete")
		{
			if(view->onEditDelete (msg))
				return true;
		}
		else if(view->getStyle ().isCustomStyle (Styles::kItemViewBehaviorSelection))
		{
			if(msg.name == "Select All")
			{
				if(!msg.checkOnly ())
					view->selectAll (true);
				return true;
			}
			else if(msg.name == "Deselect All")
			{
				if(!msg.checkOnly ())
					view->selectAll (false);
				return true;
			}
		}
	}
	return false;
}

//************************************************************************************************
// DrawItemSelectionHandler
//************************************************************************************************

DrawItemSelectionHandler::DrawItemSelectionHandler (ItemView* view)
: MouseHandler (view, kCanEscape|kCheckKeys|kAutoScroll),
  itemView (view),
  sprite (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DrawItemSelectionHandler::onBegin ()
{
	ASSERT (sprite == nullptr)
	if(!sprite)
	{
		ITheme& theme = getView ()->getTheme ();
		Color color = theme.getThemeColor (ThemeElements::kAlphaSelectionColor);
		color.setAlphaF (0.5f);
		AutoPtr<IDrawable> shape = NEW SolidDrawable (color);
		sprite = Sprite::createSprite (getView (), shape, Rect ());
		sprite->takeOpacity (shape);
		sprite->show ();
	}

	// save current selection
	oldSelection.release ();
	if(itemView)
	{
		IItemSelection* selectionCopy = nullptr;
		itemView->getSelection ().clone (selectionCopy);
		if(selectionCopy)
		{
			oldSelection = selectionCopy;
			selectionCopy->release ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DrawItemSelectionHandler::onMove (int moveFlags)
{
	Rect r;
	r.left = first.where.x;
	r.top = first.where.y;
	r.right = current.where.x;
	r.bottom = current.where.y;
	r.normalize ();

	if(sprite)
		sprite->move (r);

	newFocusItem = ItemIndex ();

	if(itemView)
	{
		int modifiers = current.keys.getModifiers ();
		bool keep   = false;
		bool toggle = false;
		if(modifiers & KeyState::kShift)
			keep = true;
		else if(modifiers & KeyState::kCommand)
			keep = toggle = true;

		itemView->selectAll (false);
		if(keep)
		{
			// select all previously selected again
			if(oldSelection)
				ForEachItem ((*oldSelection), idx)
					itemView->selectItem (idx, true);
				EndFor
		}

		ItemListSelection mouseItems;
		if(itemView->findItems (r, mouseItems))
		{
			ForEachItem (mouseItems, idx)
				if(keep)
				{
					if(oldSelection && oldSelection->isSelected (idx))
					{
						if(toggle)
							itemView->selectItem (idx, false);
						continue;
					}
				}
				itemView->selectItem (idx, true);

				if(!newFocusItem.isValid ())
					newFocusItem = idx;
			EndFor
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DrawItemSelectionHandler::onRelease (bool canceled)
{
	if(itemView)
	{
		if(canceled)
		{
			itemView->selectAll (false);

			// select all previously selected again
			if(oldSelection)
				ForEachItem ((*oldSelection), idx)
					itemView->selectItem (idx, true);
				EndFor
		}
		else
		{
			if(newFocusItem.isValid ())
				itemView->setFocusItem (newFocusItem, false);
		}
	}
	
	Rect rect;
	if(sprite)
	{
		rect = sprite->getSize ();
		sprite->hide ();
		sprite->release ();
		sprite = nullptr;
	}

	MouseHandler::onRelease (canceled);
}

//************************************************************************************************
// ItemView::ColumnCalculator
//************************************************************************************************

ItemView::ColumnCalculator::ColumnCalculator (ItemView* itemView)
: itemView (itemView) 
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemView::ColumnCalculator::calcColumnWidth (int& width, int columnIndex)
{
	IItemModel* model = itemView->getModel ();
	if(!model)
		return false;

	Font font;
	itemView->getFont (font);
	IItemModel::StyleInfo styleInfo = {font, itemView->getItemStyle ().getTextBrush (), itemView->getItemStyle ().getBackBrush1 (), 0};

	int maxWidth = 0;
	for(int i = 0, count = model->countFlatItems (); i < count; i++)
	{
		Rect size;
		if(model->measureCellContent (size, ItemIndex (i), columnIndex, styleInfo))
		{
			int w = size.getWidth ();
			if(w > maxWidth)
				maxWidth = w;
		}
	}

	static constexpr int kMinimalColumnSpacing = 3;
	width = maxWidth + kMinimalColumnSpacing;
	return maxWidth > 0;
}

//************************************************************************************************
// ItemStyle::CustomBackground
//************************************************************************************************

ItemStyle::CustomBackground::CustomBackground (StringID name)
: name (name), separatorPen (nullptr), rowHeight (-1), iconSize (-1), textFont (nullptr)
{
	brush[0] = nullptr;
	brush[1] = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::CustomBackground::~CustomBackground ()
{
	delete brush[0];
	delete brush[1];
	delete separatorPen;
}

//************************************************************************************************
// ItemStyle
/** Common visual style attributes inherited by ListViewStyle and TreeViewStyle. */
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ItemStyle, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_VISUALSTYLE_CLASS (ItemStyle, VisualStyle, "ItemViewStyle")
	ADD_VISUALSTYLE_IMAGE  ("folderIcon")				///< icon for folder items (closed)
	ADD_VISUALSTYLE_IMAGE  ("openFolderIcon")			///< icon for open folder items
	ADD_VISUALSTYLE_IMAGE  ("scrolling.background")		///< background for the whole view
	ADD_VISUALSTYLE_COLOR  ("selectionColor")			///< used to fill a rectangle around selected items
	ADD_VISUALSTYLE_COLOR  ("selectedtextcolor")		///< used instead of "textcolor" for selected items
	ADD_VISUALSTYLE_COLOR  ("iconcolor")				///< color to colorize icons of unselected items
	ADD_VISUALSTYLE_COLOR  ("selectediconcolor")		///< color to colorize icons of selected items
	ADD_VISUALSTYLE_COLOR  ("backcolor2")				///< used to draw alternating backgrounds, the other color is "backcolor"
	ADD_VISUALSTYLE_COLOR  ("separatorcolor")			///< color of separating line between rows
	ADD_VISUALSTYLE_COLOR  ("menu.separatorcolor")		///< color of separating line for menu separators
	ADD_VISUALSTYLE_METRIC ("margin")					///< margin (in pixels) of itemview content
	ADD_VISUALSTYLE_METRIC ("rowHeight")				///< height of rows (in pixels)
	ADD_VISUALSTYLE_METRIC ("scrollRows")				///< how many rows should scroll on mousewheel
	ADD_VISUALSTYLE_METRIC ("thumbnailMarginV")			///< additional vertical margin for itemContent if thumbnails available
	ADD_VISUALSTYLE_METRIC ("thumbnailPadding.left")	///< left padding for thumbnails
	ADD_VISUALSTYLE_METRIC ("thumbnailPadding.top")		///< top padding for thumbnails
	ADD_VISUALSTYLE_METRIC ("thumbnailPadding.bottom")	///< bottom padding for thumbnails
	ADD_VISUALSTYLE_METRIC ("thumbnailLimit.height")	///< thumbnails will be scaled down to (at least) this limit
	ADD_VISUALSTYLE_METRIC ("thumbnailLimit.width")		///< thumbnails will be scaled down to (at least) this limit
	ADD_VISUALSTYLE_METRIC ("thumbnailFactor")			///< thumbnails exceeding the thumbnail limits will be scaled with this factor
	ADD_VISUALSTYLE_COLOR  ("thumbnailFrameColor")		///< color of separating frame around thumbnails
	ADD_VISUALSTYLE_METRIC ("columnWidth.xxx")			///< overrides the default width (in pixels) for the column with name "xxx"
	ADD_VISUALSTYLE_COLOR ("xxx.backcolor")				///< custom background color for items of kind "xxx"
	ADD_VISUALSTYLE_COLOR ("xxx.backcolor2")			///< custom alternating background color for items of kind "xxx"
	ADD_VISUALSTYLE_COLOR ("xxx.separatorcolor")		///< custom separator line color for items of kind "xxx"
	ADD_VISUALSTYLE_METRIC ("xxx.rowHeight")			///< custom rowHeight for items of kind "xxx"
	ADD_VISUALSTYLE_METRIC ("separatorBeneath")			///< draw one pixel separator beneath items (on the bottom border - not below)
	ADD_VISUALSTYLE_METRIC ("highQualityMode")			///< set highQualityMode for image interpolations
	ADD_VISUALSTYLE_METRIC ("vSnapEnabled")				///< enable snapping to itemSize after scrolling
END_VISUALSTYLE_CLASS (ItemStyle)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::ItemStyle ()
: marginH (4),
  marginV (4),
  rowHeight (18),
  scrollRows (1),
  selectBrush (Colors::kLtGray),
  textBrush (Colors::kBlack),
  selectedTextBrush (Colors::kBlack),
  iconColor (Color (0,0,0,0)),
  selectedIconColor (iconColor),
  deleteTextBrush (Colors::kBlack),
  separatorPen (Color (0,0,0,0)),
  focusPen (Colors::kBlack),
  defaultIcon (nullptr),
  defaultOpenIcon (nullptr),
  backgroundImage (nullptr),
  deleteButtonImage (nullptr),
  thumbnailMarginV (0),
  thumbnailPaddingTop (2),
  thumbnailPaddingLeft (0),
  thumbnailPaddingBottom (2),
  thumbnailLimitHeight (100),
  thumbnailLimitWidth (200),
  thumbnailFactor (0.25f),
  thumbnailFramePen (Colors::kBlack),
  separatorBeneath (false),
  highQualityMode (false),
  vSnapEnabled (false)
{
	customBackgrounds.objectCleanup (true);

	IImage* folderIcon = FrameworkTheme::instance ().getImage (ThemeNames::kItemViewFolderIcon);
	setDefaultIcon (unknown_cast<Image> (folderIcon), false);
	
	IImage* openFolderIcon = FrameworkTheme::instance ().getImage (ThemeNames::kItemViewFolderIconOpen);
	setDefaultIcon (unknown_cast<Image> (openFolderIcon), true);

	setSelectBrush (SolidBrush (FrameworkTheme::instance ().getThemeColor (ThemeElements::kSelectionColor)));
	setBackBrush1 (SolidBrush (FrameworkTheme::instance ().getThemeColor (ThemeElements::kListViewBackColor)));
	setBackBrush2 (SolidBrush (FrameworkTheme::instance ().getThemeColor (ThemeElements::kListViewBackColor)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::ItemStyle (const ItemStyle& t)
: marginH (t.marginH),
  marginV (t.marginV),
  rowHeight (t.rowHeight),
  scrollRows (t.scrollRows),
  selectBrush (t.selectBrush),
  textBrush (t.textBrush),
  selectedTextBrush (t.selectedTextBrush),
  selectedIconColor (t.selectedIconColor),
  iconColor (t.iconColor),
  deleteTextBrush (t.deleteTextBrush),
  separatorPen (t.separatorPen),
  defaultIcon (t.defaultIcon),
  defaultOpenIcon (t.defaultOpenIcon),
  backgroundImage (t.backgroundImage),
  deleteButtonImage (t.deleteButtonImage),
  thumbnailPaddingLeft (t.thumbnailPaddingLeft),
  thumbnailPaddingTop (t.thumbnailPaddingTop),
  thumbnailPaddingBottom (t.thumbnailPaddingBottom),
  thumbnailLimitHeight (t.thumbnailLimitHeight),
  thumbnailLimitWidth (t.thumbnailLimitWidth),
  thumbnailFactor (t.thumbnailFactor),
  thumbnailFramePen (t.thumbnailFramePen),
  separatorBeneath (false),
  highQualityMode (false),
  vSnapEnabled (false)
{
	if(defaultIcon)
		defaultIcon->retain ();
	if(defaultOpenIcon)
		defaultOpenIcon->retain ();
	if(backgroundImage)
		backgroundImage->retain ();
	if(deleteButtonImage)
		deleteButtonImage->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::~ItemStyle ()
{
	if(defaultIcon)
		defaultIcon->release ();
	if(defaultOpenIcon)
		defaultOpenIcon->release ();
	if(backgroundImage)
		backgroundImage->release ();
	if(deleteButtonImage)
		deleteButtonImage->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::setDefaultIcon (Image* icon, bool open)
{
	if(open)
		take_shared (defaultOpenIcon, icon);
	else
		take_shared (defaultIcon, icon);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ItemStyle::getDefaultIcon (bool open) const
{
	return open ? defaultOpenIcon : defaultIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::setBackgroundImage (Image* bg)
{
	take_shared (backgroundImage, bg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ItemStyle::getBackgroundImage () const
{
	return backgroundImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Image* ItemStyle::getDeleteButtonImage () const
{
	return deleteButtonImage;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::setMargin (int m)
{
	marginH = marginV = m;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::updateStyle (Theme& theme)
{
	if(getSelectBrush () == SolidBrush (Colors::kLtGray)) // update default selectBrush
		setSelectBrush (SolidBrush (theme.getThemeColor (ThemeElements::kSelectionColor)));

	Image* icon = unknown_cast<Image> (theme.getImage (ThemeNames::kItemViewFolderIcon));
	if(icon)
		setDefaultIcon (icon, false);

	icon = unknown_cast<Image> (theme.getImage (ThemeNames::kItemViewFolderIconOpen));
	if(icon)
		setDefaultIcon (icon, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::updateStyle (const VisualStyle& style)
{
	int margin = (int)style.getMetric ("margin", (VisualStyle::Metric)marginH);
	marginH = (int)style.getMetric ("marginH", (VisualStyle::Metric)margin);
	marginV = (int)style.getMetric ("marginV", (VisualStyle::Metric)margin);
	rowHeight = (int)style.getMetric ("rowHeight", (VisualStyle::Metric)rowHeight);
	scrollRows = (int)style.getMetric ("scrollRows", (VisualStyle::Metric)scrollRows);

	setSelectBrush (style.getColor ("selectionColor", getSelectBrush ().getColor ()));

	setTextBrush (style.getTextBrush ());
	setSelectedTextBrush (style.getColor ("selectedtextcolor", textBrush.getColor ()));
	setIconColor (style.getColor ("iconcolor", Colors::kTransparentBlack));
	setSelectedIconColor (style.getColor ("selectediconcolor", getIconColor ()));
	Color defaultColor = backBrush1.getColor ();
	setBackBrush1 (style.getColor ("backcolor", defaultColor));
	defaultColor = backBrush1.getColor ();
	setBackBrush2 (style.getColor ("backcolor2", defaultColor));
	setDeleteTextBrush (style.getColor ("deleteButtonColor", deleteTextBrush.getColor ()));
	setSeparatorPen (style.getColor ("separatorcolor", Colors::kTransparentBlack));
	setFocusPen (Pen (style.getColor ("focusColor", focusPen.getColor ())));

	setSeparatorBeneath (style.getMetric ("separatorBeneath", false));
	setHighQualityMode (style.getMetric ("highQualityMode", false));
	setVSnapEnabled (style.getMetric ("vSnap", false));

	thumbnailMarginV = style.getMetric<int> ("thumbnailMarginV", thumbnailMarginV);
	thumbnailPaddingLeft = style.getMetric<int> ("thumbnailPadding.left", thumbnailPaddingLeft);
	thumbnailPaddingTop = style.getMetric<int> ("thumbnailPadding.top", thumbnailPaddingTop);
	thumbnailPaddingBottom = style.getMetric<int> ("thumbnailPadding.bottom", thumbnailPaddingBottom);
	thumbnailLimitHeight = style.getMetric<int> ("thumbnailLimit.height", thumbnailLimitHeight);
	thumbnailLimitWidth = style.getMetric<int> ("thumbnailLimit.width", thumbnailLimitWidth);
	thumbnailFactor = style.getMetric<float> ("thumbnailFactor", thumbnailFactor);
	thumbnailFramePen.setColor (style.getColor ("thumbnailFrameColor", thumbnailFramePen.getColor ()));

	Image* icon = unknown_cast<Image> (style.getImage ("folderIcon"));
	if(!icon)
		icon = unknown_cast<Image> (style.getImage ("defaultIcon")); // fallback is the generic default icon
	if(icon)
		setDefaultIcon (icon, false);

	icon = unknown_cast<Image> (style.getImage ("openFolderIcon"));
	if(icon)
		setDefaultIcon (icon, true);

	if(Image* bg = unknown_cast<Image> (style.getImage ("scrolling.background")))
		setBackgroundImage (bg);

	setSelectionBarImage (style.getImage ("selectionbarimage"));
	
	take_shared (deleteButtonImage, unknown_cast<Image> (style.getImage ("deleteButton")));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::CustomBackground* ItemStyle::getCustomBackground (StringID name, const IVisualStyle& style)
{
	if(name.isEmpty ())
		return nullptr;

	ArrayForEachFast (customBackgrounds, CustomBackground, bg)
		if(bg->name == name)
			return bg;
	EndFor

	CustomBackground* bg = NEW CustomBackground (name);
	customBackgrounds.add (bg);

	Color transparent (0, 0, 0, 0);

	MutableCString customName (name);
	customName += CSTR (".backcolor");
	Color c = style.getColor (customName, transparent);
	if(c != transparent)
		bg->brush[0] = NEW SolidBrush (c);

	customName = name;
	customName += CSTR (".backcolor2");
	c = style.getColor (customName, c); // fallback to ".backColor" if only one is specified
	if(c != transparent)
		bg->brush[1] = NEW SolidBrush (c);

	customName = name;
	customName += CSTR (".separatorcolor");
	c = style.getColor (customName, transparent);
	if(c != transparent)
		bg->separatorPen = NEW Pen (c);

	customName = name;
	customName += CSTR (".rowHeight");
	bg->rowHeight = style.getMetric (customName, getRowHeight ());
	
	customName = name;
	customName += CSTR (".iconSize");
	bg->iconSize = style.getMetric (customName, -1);
		
	customName = name;
	customName += CSTR (".textfont");
	FontRef customFont = style.getFont (customName);
	if(!customFont.isEqual (Font::getDefaultFont ())) // ignore fallback font!
		bg->textFont = &customFont;
	
	return bg;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::discardCustomBackgrounds ()
{
	customBackgrounds.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::zoom (const ItemStyle& original, float zoomFactor)
{
	setMarginH (int (zoomFactor * original.getMarginH ()));
	setMarginV (int (zoomFactor * original.getMarginV ()));
	setRowHeight (int (zoomFactor * original.getRowHeight ()));

	discardCustomBackgrounds (); // custom rowHeights are not zoomed so far, but (outdated) default heights might be stored
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect ItemStyle::getDeleteButtonRect (RectRef itemRect, FontRef font)
{	
	Rect textSize;
	Font::measureString (textSize, XSTR (Delete), font);
	Coord width = textSize.getWidth () + 2 * marginH;

	Rect deleteRect (itemRect);
	deleteRect.left = ccl_max (0, itemRect.right - width);
	return deleteRect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemStyle::drawDeleteButton (GraphicsPort& port, RectRef deleteRect, FontRef font)
{
	if(deleteButtonImage)
		port.drawImage (deleteButtonImage, Rect (0, 0, deleteButtonImage->getSize ()), deleteRect);

	port.drawString (deleteRect, XSTR (Delete), font, getDeleteTextBrush (), Alignment::kCenter);
}

//************************************************************************************************
// ItemView
//************************************************************************************************

BEGIN_STYLEDEF (ItemView::customStyles)
	{"header",				Styles::kItemViewAppearanceHeader},
	{"selection",			Styles::kItemViewBehaviorSelection},
	{"exclusive",			Styles::kItemViewBehaviorSelectExclusive},
	{"autoselect",			Styles::kItemViewBehaviorAutoSelect},
	{"nodrag",				Styles::kItemViewBehaviorNoDrag},
	{"norubber",			Styles::kItemViewBehaviorNoRubberband},
	{"nodoubleclick",		Styles::kItemViewBehaviorNoDoubleClick},
	{"nofocus",				Styles::kItemViewAppearanceNoFocusRect},
	{"columnfocus",			Styles::kItemViewBehaviorColumnFocus},
	{"focusselectable",		Styles::kItemViewBehaviorFocusSelectable},
	{"selectfullwidth",		Styles::kItemViewBehaviorSelectFullWidth},
	{"resizeredraw",		Styles::kItemViewAppearanceRedrawOnResize},
	{"simplemouse",			Styles::kItemViewBehaviorSimpleMouse},
	{"swallowalphachars",	Styles::kItemViewBehaviorSwallowAlphaChars},
	{"thumbnails",			Styles::kItemViewAppearanceThumbnails},
	{"dragswipeh",			Styles::kItemViewBehaviorDragSwipeH},
	{"dragswipev",			Styles::kItemViewBehaviorDragSwipeV},
	{"nounselect",			Styles::kItemViewBehaviorNoUnselect},
	{"nonamenavigation",	Styles::kItemViewBehaviorNoNameNavigation},
	{"nocontextmenu",		Styles::kItemViewBehaviorNoContextMenu},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ItemView, ItemViewBase)

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemView::ItemView (const Rect& size, StyleRef style, StringRef title)
: ItemViewBase (size, style, title),
  itemStyle (nullptr),
  savedStyle (nullptr),
  columnList (nullptr),
  tooltipColumn (-1),
  editControl (nullptr)
{
	wantsFocus (true);
	isTooltipTrackingEnabled (true);

	if(this->style.isCustomStyle (Styles::kItemViewBehaviorAutoSelect|Styles::kItemViewBehaviorSelectExclusive))
		this->style.setCustomStyle (Styles::kItemViewBehaviorSelection);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemView::~ItemView ()
{
	cancelSignals ();

	setEditControl (nullptr);

	if(itemStyle)
		itemStyle->release ();

	if(savedStyle)
		savedStyle->release ();

	share_and_observe<ColumnHeaderList> (this, columnList, nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API ItemView::getController () const
{
	if(!controller)
	{
		ItemViewController* c = NEW ItemViewController (const_cast<ItemView*> (this));
		controller = static_cast<ICommandHandler*> (c);
	}
	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle& ItemView::getItemStyle () const
{
	ASSERT (itemStyle != nullptr)
	return *itemStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::setItemStyle (ItemStyle* _itemStyle)
{
	ASSERT (itemStyle == nullptr && savedStyle == nullptr)
	ASSERT (zoomFactor == 1.f)

	itemStyle = _itemStyle;
	savedStyle = (ItemStyle*)itemStyle->clone ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::updateItemStyle ()
{
	hasAlternatingBackground (false);
	hasCustomBackgrounds (false);
	if(visualStyle)
	{
		if(itemStyle)
		{
			itemStyle->discardCustomBackgrounds ();
			itemStyle->updateStyle (*visualStyle);
			if(zoomFactor != 1.f)
				itemStyle->zoom (*savedStyle, zoomFactor);
			
			if(itemStyle->getBackBrush1 ().getColor () != itemStyle->getBackBrush2 ().getColor ())
				hasAlternatingBackground (true);

			hasCustomBackgrounds (visualStyle->getMetric<bool> ("customBackgrounds", false));
		}
		if(savedStyle)
		{
			savedStyle->discardCustomBackgrounds ();
			savedStyle->updateStyle (*visualStyle);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	updateItemStyle ();

	if(isAttached ())
		updateSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API ItemView::getVisualStyle () const
{
	if(visualStyle)
		return *visualStyle;

	if(VisualStyle* standardStyle = getTheme ().getStandardStyle (getStandardStyleIndex ()))
	{
		const_cast<ItemView*> (this)->setVisualStyle (standardStyle);
		return *standardStyle;
	}
	return VisualStyle::emptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getStandardStyleIndex () const
{
	return ThemePainter::kListViewStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::setTheme (Theme* theme)
{
	SuperClass::setTheme (theme);

	if(theme)
	{
		if(itemStyle)
		{
			itemStyle->updateStyle (*theme);
			if(zoomFactor != 1.f && savedStyle)
				itemStyle->zoom (*savedStyle, zoomFactor);
		}
		if(savedStyle)
			savedStyle->updateStyle (*theme);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(visualStyle && visualStyle->hasReferences (event.scheme))
		updateItemStyle ();

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Font& ItemView::getFont (Font& font) const
{
	font = getVisualStyle ().getTextFont ();
	if(zoomFactor != 1.f)
		font.setSize (font.getSize () * zoomFactor);
	return font;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemStyle::CustomBackground* ItemView::getCustomBackground (StringID name)
{
	return getItemStyle ().getCustomBackground (name, getVisualStyle ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeaderList* ItemView::getColumnHeaders () const
{
	return columnList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColumnHeaderList* ItemView::getVisibleColumnList () const
{
	return columnList;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::setColumnHeaders (ColumnHeaderList* list)
{
	share_and_observe (this, columnList, list);

	if(columnList)
	{
		onEditModeChanged (isEditMode ());

		// get column widths from style
		if(visualStyle)
			for(int i = 0, num = columnList->getCount (false); i < num; i++)
				if(ColumnHeader* column = columnList->getColumnByIndex (i))
					if(!column->getID ().isEmpty ())
					{
						MutableCString columnWidthName ("columnWidth.");
						columnWidthName += column->getID ();
						column->setWidth (visualStyle->getMetric (columnWidthName, column->getWidth ()));
					}

		 columnList->signal (Message (IColumnHeaderList::kColumnRectsChanged));
	}

	invalidate ();

	ScrollView* sv = ScrollView::getScrollView (this);
	if(sv)
	{
		if(columnList && style.isCustomStyle (Styles::kItemViewAppearanceHeader))
		{
			HeaderView* headerView = NEW HeaderView (columnList);
			if(theme)
				headerView->setTheme (theme);

			if(!sv->getPersistenceID ().isEmpty ())
			{
				MutableCString headerID (sv->getPersistenceID ());
				headerID += ".HeaderView";
				headerView->setPersistenceID (headerID);
			}

			if(headerViewStyle)
				headerView->setVisualStyle (headerViewStyle);

			headerView->autoSize ();
			sv->setHeader (headerView);
		}
		else
			sv->setHeader (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::updateColumns ()
{
	if(parent)
	{
		if(model)
		{
			AutoPtr<ColumnHeaderList> list = NEW ColumnHeaderList;
			if(model->createColumnHeaders (*list))
			{
				setColumnHeaders (list);
				
				AutoPtr<IColumnCalculator> calculator (NEW ColumnCalculator (this));
				list->setColumnCalculator (calculator);

				autoSizeColumns ();
				
				// init sort column
				tbool upwards = false;
				MutableCString columnID;
				if(model->getSortColumnID (columnID, upwards))
					list->setSortColumn (columnID, upwards != 0, false);
				return;
			}
		}
		setColumnHeaders (nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::autoSizeColumns ()
{
	// auto size columns where required
	if(!columnList)
		return;
	
	int autoFillColumnCount = 0;
	Coord totalColumnUpdateWidth = 0;
	IColumnCalculator* calculator = columnList->getColumnCalculator ();
	
	ArrayForEachFast (columnList->getColumns (), ColumnHeader, column)
		if(column->canFill () && !column->isHidden ())		// fill property set...
		{
			autoFillColumnCount += 1;
		}
		else if(column->getWidth () == IColumnHeaderList::kAutoWidth && calculator) // ...or autoWidth wanted
		{
			int width = 0;
			if(calculator->calcColumnWidth (width, column->getIndex ()) && width > 0)
			{
				width += 2;
				Coord newWidth = column->getMinWidth () == 0 ? width : ccl_max (width, column->getMinWidth ());
				column->setWidth (newWidth);
				
				if(!column->isHidden ())
					totalColumnUpdateWidth += newWidth;
			}
		}
		else
		{
			if(!column->isHidden ())
				totalColumnUpdateWidth += column->getWidth ();
		}
	EndFor
	
	if(autoFillColumnCount > 0)
	{
		ArrayForEachFast (columnList->getColumns (), ColumnHeader, column)
			if(column->canFill () && !column->isHidden ())
			{
				Coord autoFillWidth = ccl_max (0, (getWidth () - totalColumnUpdateWidth) / autoFillColumnCount);
				if(autoFillWidth > column->getMinWidth ())
				{
					column->setWidth (autoFillWidth);
					totalColumnUpdateWidth += autoFillWidth;
				}
				else
				{
					column->setWidth (column->getMinWidth ());
					totalColumnUpdateWidth += column->getMinWidth ();
				}
				autoFillColumnCount -= 1;
			}
		EndFor
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ItemView::getThumbnail (ItemIndexRef index) const
{
	if(style.isCustomStyle (Styles::kItemViewAppearanceThumbnails))
		return model->getItemThumbnail (index);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* ItemView::getDragImageForItem (ItemIndexRef itemIndex)
{
	if(IImage* image = getThumbnail (itemIndex))
		return image;

	return SuperClass::getDragImageForItem (itemIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::getLimitedThumbnailSize (Rect& limitedSize, IImage* image) const
{
	bool resized = false;
	Coord imageWidth = image->getWidth ();
	Coord imageHeight = image->getHeight ();
	
	limitedSize.setWidth (imageWidth);
	limitedSize.setHeight (imageHeight);
	
	Coord heightLimit = getItemStyle ().getThumbnailLimitHeight ();
	Coord widthLimit = getItemStyle ().getThumbnailLimitWidth ();
	
	if((imageHeight > (heightLimit * 1.5)) && (imageWidth > (widthLimit * 1.5)))
	{
		// assume thumbnail has orignal size
		float thumbnailFactor = getItemStyle ().getThumbnailFactor ();
		imageHeight *= thumbnailFactor;
		imageWidth *= thumbnailFactor;
		
		limitedSize.setWidth (imageWidth);
		limitedSize.setHeight (imageHeight);
		resized = true;
	}

	if((imageHeight > heightLimit) || (imageWidth > widthLimit)) // still too big?
	{
		Coord derivedWidth = (heightLimit / (float)imageHeight) * imageWidth;
		if(derivedWidth > widthLimit)
		{
			limitedSize.setWidth (widthLimit);
			limitedSize.setHeight ((widthLimit / (float)imageWidth) * imageHeight);
		}
		else
		{
			limitedSize.setWidth (derivedWidth);
			limitedSize.setHeight (heightLimit);
		}
		resized = true;
	}
	
	return resized;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ItemView::getThumbnailAreaHeight (IImage* image) const
{
	const ItemStyle& style = getItemStyle ();
	Rect limitedSize;
	getLimitedThumbnailSize (limitedSize, image);
	return limitedSize.getHeight () + style.getThumbnailMarginV () + style.getThumbnailPaddingTop () + style.getThumbnailPaddingBottom ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord ItemView::determineRowHeight (ItemIndexRef itemIndex)
{
	ItemStyle::CustomBackground* bg = getCustomBackground (getModel ()->getItemBackground (itemIndex));

	Coord rowHeight = (bg && bg->rowHeight >= 0)
		? bg->rowHeight
		: getItemStyle ().getRowHeight ();

	if(style.isCustomStyle (Styles::kItemViewAppearanceThumbnails))
		if(IImage* image = getThumbnail (itemIndex))
			rowHeight += getThumbnailAreaHeight (image);

	if(model)
	{
		Font font;
		getFont (font);
		IItemModel::StyleInfo styleInfo = {font, getItemStyle ().getTextBrush (), getItemStyle ().getBackBrush1 (), 0};

		int numColumns = 1;
		if(columnList)
			numColumns = columnList->getCount (false);
		
		for(int columnIndex = 0; columnIndex < numColumns; columnIndex++)
		{
			Rect size;
			if(model->measureCellContent (size, itemIndex, columnIndex, styleInfo))
			{
				int h = size.getHeight ();
				if(h > rowHeight)
					rowHeight = h;
			}
		}
	}

	return rowHeight;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getRowsPerPage () const
{
	Coord itemH = getItemHeight (ItemIndex ());
	if(itemH == 0)
		return 1;
	Rect r;
	getVisibleClient (r);
	return r.getHeight () / itemH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point ItemView::getBackgroundOffset () const
{
	const ItemStyle& style = getItemStyle ();
	return Point (style.getMarginH (), style.getMarginV ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ItemView::NavigationMode ItemView::validateNavigationMode (NavigationMode mode) const
{
	switch(mode)
	{
	case kSelectExtend:
	case kSelectExtendAdd:
		if(getStyle ().isCustomStyle (Styles::kItemViewBehaviorSelectExclusive)) // extending does not make sense for exclusive selection
			mode = kSelect;
		break;

	case kSkip:
		if(getStyle ().isCustomStyle (Styles::kItemViewAppearanceNoFocusRect)) // skipping (navigating focus while keeping selection) does not make sense when the focus is not visible
			mode = kSelect;
		break;
	}
	return mode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::getAnchorItem (ItemIndex& index) const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::setAnchorItem (ItemIndexRef index)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::selectRange (ItemIndexRef fromIndex, ItemIndexRef toIndex)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::navigate (int32 rows, int32 columns, NavigationMode navigationMode, bool checkOnly)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getItemHeight (ItemIndexRef index) const
{
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getItemRow (ItemIndexRef index) const
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::getEditContext (ItemIndex& item, Rect& cellRect, int& editColumn)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::makeItemVisible (ItemIndexRef index)
{
	SuperClass::makeItemVisible (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::setModel (IItemModel* _model)
{
	ItemViewBase::setModel (_model);

	updateColumns ();

	if(isAttached ())
		invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::setZoomFactor (float factor)
{
	if(factor != zoomFactor)
	{
		SuperClass::setZoomFactor (factor);

		ASSERT (savedStyle)
		getItemStyle ().zoom (*savedStyle, zoomFactor);

		updateSize ();
		updateClient ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::setEditControl (IView* view, tbool directed)
{
	bool controlHadFocus = false;

	if(editControl)
	{
		Window* window = getWindow ();
		controlHadFocus = window && (window->getFocusView () == editControl);

		editControl->removeObserver (this);

		removeView (editControl);
		editControl->release ();
	}

	editControl = unknown_cast<View> (view);

	if(editControl)
	{
		if(!editControl->hasVisualStyle ())
		{
			const IVisualStyle& vs = getVisualStyle ();
			AutoPtr<VisualStyle> style (NEW VisualStyle);
			style->setFont (StyleID::kTextFont, vs.getTextFont ());
			style->setColor (StyleID::kTextColor, vs.getTextColor ());
			style->setColor (StyleID::kBackColor, vs.getBackColor ());
			editControl->setVisualStyle (style);
		}

		addView (editControl);
		editControl->addObserver (this);
		
		IView* focusView = FocusNavigator::instance ().getFirst (editControl);
		if(!focusView)
			focusView = editControl;

		if(Window* w = getWindow ())
			w->setFocusView (unknown_cast<View> (focusView), directed);
	}
	else if(controlHadFocus)
		takeFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemView::findItemCell (ItemIndex& idx, int& column, const Point& where) const
{
	if(findItem (idx, where))
	{
		column = getColumnHeaders () ? getColumnHeaders ()->getColumnIndex (where.x) : 0;
		return true;
	}
	return false;	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::attached (View* parent)
{
	SuperClass::attached (parent);

	getVisualStyle (); // trigger updating itemStyle

	updateSize ();

	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();

	#if CCL_PLATFORM_DESKTOP
	if(getStyle ().isCustomStyle (Styles::kItemViewBehaviorAutoSelect))
	{
		// trigger intial mouse move
		KeyState keys;
		GUI.getKeyState (keys);
		Point p;
		GUI.getMousePosition (p);
		screenToClient (p);
		Rect clientRect;
		getVisibleClient (clientRect);
		if(clientRect.pointInside (p))
			onMouseMove (MouseEvent (MouseEvent::kMouseMove, p, keys));
	}
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::modelChanged (int changeType, ItemIndexRef item)
{
	switch(changeType)
	{
	case kItemModified:
		invalidateItem (item);
		return;
		
	case kModelChanged:
		if(selection)
			selectAll (false);
		CCL_FALLTHROUGH
		
	default: // kItemAdded, kItemRemoved, kModelAssigned
		
		if(visualStyle) // only updateSize when visualStyle is already set...
			updateSize (); // ...or we might cache wrong default sizes
		invalidate ();
	}
	
	if(isAccessibilityEnabled ())
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::onEditModeChanged (bool state)
{
	if(columnList)
	{
		ArrayForEachFast (columnList->getColumns (), ColumnHeader, column)
			if(column->isEditMode ())
				columnList->showColumn (column, state);
		EndFor
	}
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::toModelColumnIndex (int column) const
{
	ColumnHeaderList* columns = getVisibleColumnList ();
	if(ColumnHeader* c = columns ? columns->getColumnAtPosition (column, false) : nullptr)
		return c->getIndex ();
	else
		return column;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::openItem (ItemIndexRef item, int column, const GUIEvent& editEvent, RectRef rect)
{
	int columnIndex = toModelColumnIndex (column);
	return SuperClass::openItem (item, columnIndex, editEvent, rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool ItemView::editCell (ItemIndexRef item, int column, RectRef rect, const GUIEvent& editEvent)
{
	int columnIndex = toModelColumnIndex (column);
	return SuperClass::editCell (item, columnIndex, rect, editEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::shouldDrawFocus () const
{
	return isFocused () && !style.isCustomStyle (Styles::kItemViewAppearanceNoFocusRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::drawFocusRect (GraphicsPort& port, RectRef rect)
{
	if(shouldDrawFocus ())
		port.drawRect (rect, getItemStyle ().getFocusPen ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onFocus (const FocusEvent& event)
{
	isFocused (event.eventType == FocusEvent::kSetFocus);

	if(isFocused ())
	{
		UnknownPtr<IObserver> observer (model);
		if(observer)
			observer->notify (this, Message (IItemView::kViewFocused));	
	}

	ItemIndex focusItem;
	if(getFocusItem (focusItem))
		invalidateItem (focusItem);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::getFirstCommandItem (ItemIndex& item)
{
	// return the focus item if it's selected, otherwise the first selected item
	const IItemSelection& selection = getSelection ();
	if(!selection.isEmpty ())
	{
		if(getFocusItem (item) && getSelection ().isSelected (item))
			return true;

		ForEachItem (selection, index)
			item = index;
			return true;
		EndFor
	}
	item = ItemIndex ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onContextMenu (const ContextMenuEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorNoContextMenu))
		return false;
	
	ItemIndex commandItem;
	if(getFirstCommandItem (commandItem))
	{
		if(event.wasKeyPressed)
		{
			// adjust menu postion
			Rect r;
			getItemRect (r, commandItem);
			event.setPosition (r.getLeftTop ().offset (getItemStyle ().getMarginH (), r.getHeight ()/2));
		}
	}
	else // try item at mouse
		findItem (commandItem, event.where);
	return model && model->appendItemMenu (event.contextMenu, commandItem, getSelection ()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getColumnIndex (PointRef where)
{
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::drawSolidBackground (const UpdateRgn& updateRgn)
{
	GraphicsPort port (this);
	ItemStyle& itemStyle = getItemStyle ();
	port.fillRect (updateRgn.bounds, itemStyle.getBackBrush1 ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::drawAlternatingBackground (const UpdateRgn& updateRgn)
{
	// draw background rows in alternating colors
	GraphicsPort port (this);
	ItemStyle& itemStyle = getItemStyle ();
	SolidBrush brushes[2] = { itemStyle.getBackBrush1 (), itemStyle.getBackBrush2 () };
	
	const Pen* separatorPen = &itemStyle.getSeparatorPen ();
	if(separatorPen->getColor () == Colors::kTransparentBlack)
		separatorPen = nullptr;
	
	Rect r (updateRgn.bounds);
	Rect line (r);
	int brushIndex = 0;
	Coord defaultH = getItemHeight (ItemIndex ());

	// find first item in update rect
	ItemIndex itemIndex;
	if(findItem (itemIndex, updateRgn.bounds.getLeftTop ()))
	{
		Rect itemRect;
		getItemRect (itemRect, itemIndex);
		r.top = itemRect.top;
		r.bottom = itemRect.bottom;

		int startRow = getItemRow (itemIndex);
		brushIndex = startRow % 2;

		// draw rows while items are visible
		do
		{
			Brush* brush = &brushes[brushIndex];
			const Pen* pen = separatorPen;
			if(ItemStyle::CustomBackground* bg = getCustomBackground (model->getItemBackground (itemIndex)))
			{
				if(Brush* itemBrush = bg->brush[brushIndex])
					brush = itemBrush;
				if(bg->separatorPen)
					pen = bg->separatorPen;
			}

			r.setHeight (getItemHeight (itemIndex));

			port.fillRect (r, *brush);
			if(pen)
			{
				if(itemStyle.isSeparatorBeneath ())
					line.top = r.bottom - 1;
				else
					line.top = r.top;
				
				line.bottom = line.top + 1;
				port.drawRect (line, *pen);
			}

			brushIndex = 1 - brushIndex;
			r.top = r.bottom;

			if(!getNextItem (itemIndex, false))
				break;
		} while(r.top < updateRgn.bounds.bottom);
	}
	else
	{
		// start coord beyond last item, start from content end
		SizeInfo sizeInfo;
		getSizeInfo (sizeInfo);
		r.top = sizeInfo.height - getItemStyle ().getMarginV ();

		int rowsToSkip = (updateRgn.bounds.top - r.top) / defaultH;
		if(rowsToSkip > 0)
		{
			r.top += rowsToSkip * defaultH;
			brushIndex = rowsToSkip % 2;
		}
	}

	// draw remaining dummy rows in default height
	while(r.top < updateRgn.bounds.bottom)
	{
		r.setHeight (defaultH);

		port.fillRect (r, brushes[brushIndex]);
		if(separatorPen)
		{
			if(itemStyle.isSeparatorBeneath ())
				line.top = r.bottom - 1;
			else
				line.top = r.top;
			
			line.bottom = line.top + 1;
			port.drawRect (line, *separatorPen);
		}

		brushIndex = 1 - brushIndex;
		r.top = r.bottom;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::drawThumbnail (GraphicsPort& port, Image& thumbnailImage, PointRef pos)
{
	Rect iconRect;
	thumbnailImage.getSize (iconRect);
	Rect dstRect;
	getLimitedThumbnailSize (dstRect, thumbnailImage.getOriginal ());
	dstRect.moveTo (pos);
	
	ImageMode mode (1.0f, ImageMode::kInterpolationHighQuality);
	port.drawImage (&thumbnailImage, iconRect, dstRect, &mode);

	if(getItemStyle ().getThumbnailFramePen ().getColor ().alpha != 0x00)
	{
		port.drawRect (dstRect, getItemStyle ().getThumbnailFramePen ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onKeyDown (const KeyEvent& event)
{
	ItemIndex item;
	Rect cellRect;
	int editColumn = -1;
	if(getEditContext (item, cellRect, editColumn))
	{
		if(event.vKey == VKey::kReturn)
			if(openItem (item, editColumn, event, cellRect))
				return true;
	}

	bool result = SuperClass::onKeyDown (event);
	
	if(style.isCustomStyle (Styles::kItemViewBehaviorSwallowAlphaChars))
	{
	   if(Unicode::isAlpha (event.character) && !event.state.isSet (KeyState::kCommand|KeyState::kOption|KeyState::kControl))
			result = true;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemView::onEditNavigation (const KeyEvent& event, IView* control)
{
	if(editControl)
	{
		int32 rows = 0;
		int32 cols = 0;

		switch(event.vKey)
		{
			case VKey::kEscape :
			case VKey::kEnter :
			case VKey::kReturn :
				takeFocus (); // transfer focus to itemview before it gets lost completely
				return true;

			case VKey::kUp :		rows = -1; break;
			case VKey::kDown :		rows = +1; break;
			case VKey::kLeft :		cols = -1; break;
			case VKey::kRight :		cols = +1; break;
			case VKey::kPageUp :	rows = -getRowsPerPage (); break;
			case VKey::kPageDown :	rows =  getRowsPerPage (); break;
			case VKey::kTab :		(style.isCustomStyle (Styles::kItemViewBehaviorColumnFocus) ? cols : rows) = event.state.isSet (KeyState::kShift) ? - 1:  +1; break;
		}

		if(rows != 0 || cols != 0)
		{
			AutoPtr<Boxed::KeyEvent> e (NEW Boxed::KeyEvent (event));
			(NEW Message ("editNavigation", Variant (e->asUnknown (), true), rows, cols))->post (this);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onMouseEnter (const MouseEvent& event)
{
	return style.isCustomStyle (Styles::kItemViewBehaviorAutoSelect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onMouseMove (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorAutoSelect))
	{
		ItemIndex item;
		if(findItem (item, event.where))
		{
			if(model->canSelectItem (item))
				setFocusItem (item, true);
			else
				setFocusItem (-1, true);
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ItemView::getLogicalColumnIndex (PointRef where)
{
	return getColumnIndex (where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onTrackTooltip (const TooltipEvent& event)
{
	if(event.eventType == TooltipEvent::kHide)
		return true;

	ItemIndex item;
	if(model && findItem (item, event.where))
	{
		int column = getLogicalColumnIndex (event.where);

		switch(event.eventType)
		{
			case TooltipEvent::kShow:
			case TooltipEvent::kMove:
			{
				bool moved = (event.eventType == TooltipEvent::kMove && !(item == tooltipItem && column == tooltipColumn));
				bool textChanged = false;

				tooltipItem = item;
				tooltipColumn = column;

				String tip;
				if(model->getItemTooltip (tip, item, toModelColumnIndex (tooltipColumn)))
				{
					textChanged = tip != event.tooltip.getText ();
					if(moved || textChanged)
					{
						if(textChanged)
							event.tooltip.setText (tip);
						if(moved)
							event.tooltip.moveToMouse ();
						event.tooltip.setDuration (ITooltipPopup::kDefaultDuration);
						event.tooltip.show ();
					}
					return true;
				}
				break;
			}
		}
	}
	event.tooltip.hide ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onEditCut (const CommandMsg& args)
{
	return onEditCopy (args) && onEditDelete (args);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onEditCopy (const CommandMsg& args)
{
	bool result = model && !getSelection ().isEmpty ();
	if(result && !args.checkOnly ())
	{
		AutoPtr<UnknownList> copyList (NEW UnknownList);
		ForEachItem (getSelection (), idx)
			IUnknown* obj = model->createDragSessionData (idx);
			if(obj)
				copyList->add (obj);
		EndFor
		if(!copyList->isEmpty ())
		{
			copyList->retain ();
			System::GetClipboard ().setContent ((IUnknownList*)copyList);
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onEditPaste (const CommandMsg& args)
{
	IUnknown* toPaste = System::GetClipboard ().getContent ();
	if(toPaste && model)
	{
		UnknownPtr<IUnknownList> list (toPaste);
		if(list)
		{
			ItemIndex itemIndex (-1);
			getFocusItem (itemIndex);
			int column = -1 ;

			if(args.checkOnly () && model->canInsertData (itemIndex, column, *list, nullptr, this))
				return true;
			else if(model->insertData (itemIndex, column, *list, nullptr))
				return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onEditDelete (const CommandMsg& args)
{
	bool result = false;
	if(args.checkOnly ())
	{
		ForEachItem (getSelection (), idx)
			if(model->canRemoveItem (idx))
				return true;
		EndFor
	}
	else
	{
		ForEachItem (getSelection (), idx)
			if(model->removeItem (idx))
			{				
				selectItem (idx, false); 
				result = true;
			}
		EndFor
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::doSelection (ItemIndexRef clickedItem, const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelection))
	{
		ScopedFlag<kSuspendSelectSignal> guard (privateFlags);

		int modifiers = event.keys.getModifiers ();
		if(modifiers & KeyState::kShift)
		{
			// select from anchorItem to clickedItem
			if((modifiers & KeyState::kCommand) == 0)
				selectAll (false);

			if(style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive))
				setAnchorItem (clickedItem);

			ItemIndex anchorItem;
			if(getAnchorItem (anchorItem))
				selectRange (anchorItem, clickedItem);
		}
		else
		{
			setAnchorItem (clickedItem);

			if((modifiers & KeyState::kCommand) || (style.isCustomStyle (Styles::kItemViewBehaviorNoUnselect) && style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive) == false)) // toggle clicked item
				selectItem (clickedItem, !getSelection ().isSelected (clickedItem));
			else // simple click: select this item exclusively
			{
				selectAll (false);
				selectItem (clickedItem, true);
			}
		}

		privateFlags &= ~kSuspendSelectSignal;
		signalSelectionChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::doSelection (ItemIndexRef clickedItem, const GestureEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelection))
	{
		ScopedFlag<kSuspendSelectSignal> guard (privateFlags);
		
		ItemIndex oldIndex;
		getAnchorItem (oldIndex);
		
		bool wantsImmediateToggleTap = style.isCustomStyle (Styles::kItemViewBehaviorNoUnselect) && style.isCustomStyle (Styles::kItemViewBehaviorSelectExclusive) == false;
		
		CCL_PRINTF ("Event Type %d, State %d\n", event.getType (), event.getState ())
		
		if(wantsImmediateToggleTap && event.getState () != GestureEvent::kPossible) // immediate single tap is a possible double tap
			if(event.getType () == GestureEvent::kSingleTap && oldIndex == clickedItem) // single tap on same item
				return;
		
		setAnchorItem (clickedItem);
		
		// toggle tapped item...
		if(wantsImmediateToggleTap)
			selectItem (clickedItem, !getSelection ().isSelected (clickedItem));
		else // ...or select exclusively
		{
			selectAll (false);
			selectItem (clickedItem, true);
		}
		
		privateFlags &= ~kSuspendSelectSignal;
		signalSelectionChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::tryRubberSelection (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorSelection) && !style.isCustomStyle (Styles::kItemViewBehaviorNoRubberband))
	{
		GUI.flushUpdates (false);
		if(detectDrag (event))
		{
			// let the mouse draw a selection rect
			MouseHandler* handler = NEW DrawItemSelectionHandler (this);
			handler->begin (event);
			getWindow ()->setMouseHandler (handler);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::tryDrag (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kItemViewBehaviorNoDrag))
		return false;

	GUI.flushUpdates (false);
	if(detectDrag (event))
	{	
		AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
		IImage* dragImage = nullptr;
		ItemIndex firstItem;

		ForEachItem (getSelection (), idx);
			IUnknown* obj = model->createDragSessionData (idx);
			if(obj)
			{
				session->getItems ().add (obj, false); // owned by drag session!
				if(!dragImage)
					dragImage = getDragImageForItem (idx); /// using icon of first item (hmm...)
				if(!firstItem.isValid ())
					firstItem = idx;
			}
		EndFor

		if(!dragImage && firstItem.isValid () && model->isItemFolder (firstItem))
			dragImage = getItemStyle ().getDefaultIcon ();
		session->setDragImage (dragImage, getItemStyle ().getBackBrush1 ().getColor ());
		
		return dragItems (*session, IDragSession::kMouseInput);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::dragItems (DragSession& session, int inputDevice)
{
	if(!session.getItems ().isEmpty ())
	{
		session.setInputDevice (inputDevice);

		UnknownPtr<IObserver> observer = model;
		if(observer)
			observer->notify (this, Message (IItemView::kDragSessionStart, session.asUnknown ()));
		
		Promise p (session.dragAsync ());
		p.then ([&, observer] (IAsyncOperation& operation)
		{
			if(observer)
				observer->notify (this, Message (IItemView::kDragSessionDone, session.asUnknown ()));
		});	
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::onEditControlLostFocus (IView* control)
{
	if(editControl && (control == editControl || editControl->isChildView (control, true)))
		(NEW Message ("killEditControl", (IView*)editControl))->post (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemView::notify (ISubject* subject, MessageRef msg)
{
	if(msg == ColumnHeaderList::kSortColumnChanged)
	{
		// *** Sorting changed by HeaderView ***
		if(columnList && subject == columnList)
		{
			UnknownPtr<IObserver> observer (model);
			if(observer)
			{
				bool upwards = false;
				CString columnID = columnList->getSortColumn (upwards);
				observer->notify (this, Message (ColumnHeaderList::kSortColumnChanged, columnID.str (), upwards));
			}

			// check if model accepted new sorting
			if(model)
			{
				tbool upwards = false;
				MutableCString columnID;
				model->getSortColumnID (columnID, upwards);
				columnList->setSortColumn (columnID, upwards != 0, false);
			}
		}
		// *** Sorting changed by model ***
		else if(model && isEqualUnknown (model, subject))
		{
			if(columnList)
			{
				tbool upwards = false;
				MutableCString columnID;
				model->getSortColumnID (columnID, upwards);
				columnList->setSortColumn (columnID, upwards != 0, false);
			}
		}
	}
	else if((columnList && subject == columnList) || msg == "updateSize")
	{
		updateSize ();
		invalidate ();

		if(msg == ColumnHeaderList::kColumnRectsChanged) // forward to model
			if(UnknownPtr<IObserver> observer = model)
				observer->notify (this, msg);
	}
	else if(msg == "killEditControl")
	{
		if(unknown_cast<View> (msg[0]) == editControl)
			setEditControl (nullptr);
	}
	else if(msg == "editNavigation")
	{
		Boxed::KeyEvent* key = unknown_cast<Boxed::KeyEvent> (msg[0]);
		int rows = msg[1];
		int cols = msg[2];

		if(navigate (rows, cols, kSelect, false) && key)
		{
			ItemIndex item;
			Rect cellRect;
			int editColumn;
			if(getEditContext (item, cellRect, editColumn))
			{
				System::GetSignalHandler ().flush (); // let possible scrolling happen immediately

				setEditControl (nullptr);

				if(privateFlags & kOpenItemCalled)
					openItem (item, editColumn, *key, cellRect);
				else
					editCell (item, editColumn, cellRect, *key);
			}
		}
	}
	else if(msg == IItemModel::kUpdateColumns)
	{
		updateColumns ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemView::onGesture (const GestureEvent& event)
{
	ItemIndex index;
	if(event.getType () == GestureEvent::kSingleTap)
    {
		if(model && findItem (index, event.where))
			doSelection (index, event);
	}
	else if((event.getType () == GestureEvent::kLongPress  || event.getType () == GestureEvent::kSwipe) && event.getState () == GestureEvent::kBegin)
    {
    	if(getStyle ().isCustomStyle (Styles::kItemViewBehaviorNoDrag) == false)
			if(model && findItem (index, event.where))
			{
				// select item if not already selected
				if(!getSelection ().isSelected (index))
					doSelection (index, event);

				AutoPtr<DragSession> session (DragSession::create (this->asUnknown ()));
				IImage* dragImage = nullptr;
				if(IUnknown* obj = model->createDragSessionData (index))
				{
					session->getItems ().add (obj, false); // owned by drag session!
					dragImage = getDragImageForItem (index);
				}

				if(!dragImage && model->isItemFolder (index))
					dragImage = getItemStyle ().getDefaultIcon ();
				session->setDragImage (dragImage, getItemStyle ().getBackBrush1 ().getColor ());

				return dragItems (*session, IDragSession::kTouchInput);
			}
    }

    return SuperClass::onGesture (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	autoSizeColumns ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemView::makeAccessibilityProvider (bool state)
{
	SuperClass::makeAccessibilityProvider (state);
	if(state)
		if(ItemViewAccessibilityProvider* provider = ccl_cast<ItemViewAccessibilityProvider> (accessibilityProvider))
			provider->rebuildItemProviders ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ItemView::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ItemViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// ItemControl
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ItemControl, ItemControlBase)

////////////////////////////////////////////////////////////////////////////////////////////////////

ItemControl::ItemControl (const Rect& size, ItemView* itemView, StyleRef scrollViewStyle)
: ItemControlBase (size, itemView, scrollViewStyle)
{
	if(itemView)
		itemView->updateColumns (); 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControl::setHeaderViewStyle (VisualStyle* visualStyle)
{
	if(ItemView* itemView = ccl_cast<ItemView> (getItemView ()))
		itemView->setHeaderViewStyle (visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float ItemControl::getScrollSpeedV () const
{
	if(ItemView* itemView = ccl_cast<ItemView> (getItemView ()))
	{
		ItemStyle& style = itemView->getItemStyle ();
		return float(style.getScrollRows ()) * style.getRowHeight ();
	}
	return SuperClass::getScrollSpeedV ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemControl::drawBackground (const UpdateRgn& updateRgn)
{
	if(style.isTransparent ())
		return;

	if(ItemView* itemView = (ItemView*)getItemView ())
	{
		//itemView->getVisualStyle (); // trigger updating itemStyle => can cause invalidate() during redraw, moved to ItemView::attached()! 
		if(style.isBorder ())
			SuperClass::drawBackground (updateRgn);

		UpdateRgn clipRegion (updateRgn, clipView->getSize ());
		UpdateRgn targetRegion (clipRegion, itemView->getSize ());
		if(!targetRegion.isEmpty ())
		{
			if(itemView->hasAlternatingBackground () || itemView->hasCustomBackgrounds ())
				itemView->drawAlternatingBackground (targetRegion);
			else
				itemView->drawSolidBackground (targetRegion);
		}
		return;
	}
	SuperClass::drawBackground (updateRgn);
}
