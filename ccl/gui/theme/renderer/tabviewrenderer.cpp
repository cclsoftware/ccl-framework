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
// Filename    : ccl/gui/theme/renderer/tabviewrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#include "ccl/gui/theme/renderer/tabviewrenderer.h"

#include "ccl/gui/views/graphicsport.h"
#include "ccl/gui/graphics/graphicspath.h"
#include "ccl/gui/controls/textmodel.h"

#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/iparameter.h"

using namespace CCL;

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////

enum TabMetrics
{
	kTabSlope = 20,
	kTabControl	= 10,
	kMenuTabWidth = 16,
	kContentMargin = 3,
	kTabSpacing = 0,
	kTabMenuSpacing = 0,
	kTabIconSpacing = 0
};

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace AtomName
{
	const CStringPtr kTabBackground = "tab.background";
	const CStringPtr kTabIcon = "tab.icon";
	const CStringPtr kTabValue = "tab.value";
	const CStringPtr kTabMenuIcon = "tab.menuicon";
}

//************************************************************************************************
// TabViewGeometry
/** Encapsulates the differences between horizontal & vertical tabviews. */
//************************************************************************************************

template<int orientation> struct TabViewGeometry;

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct TabViewGeometry<Styles::kHorizontal>
{
	static inline void clientRectToContent (Rect& rect, int tabHeight)	{ rect.top += tabHeight; }
	static inline void clientRectToHeader (Rect& rect, int tabHeight)	{ rect.bottom = rect.top + tabHeight; }
	static inline void headerToFirstTab (Rect& rect, Coord inset, Coord overlap) { rect.top += inset; rect.left += inset + overlap; }

	static inline void setTabWidth (Rect& rect, Coord width)			{ rect.right = rect.left + width; }
	static inline void setTabHeight (Rect& rect, Coord height)			{ rect.bottom = rect.top + height; }
	static inline void toNextTab (Rect& rect, Coord offset = 0)			{ rect.left = rect.right + offset; }
	static inline void offset (Rect& rect, Coord offset)				{ rect.left += offset; rect.right += offset; }
	static inline void offset (Point& p, Coord offset)					{ p.x += offset; }
	static inline bool isInside (PointRef p, RectRef rect)				{ return p.x >= rect.left && p.x <= rect.right; }
	static inline Coord& getStartCoord (Rect& rect)						{ return rect.left; }
	static inline Coord& getEndCoord (Rect& rect)						{ return rect.right; }

	static inline void layoutLabel (RenderAtom& iconAtom, RenderAtom& valueAtom, Transform& labelTransform, RectRef rect, StringRef text, IImage* icon, Coord iconSpacing, bool center);
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<> struct TabViewGeometry<Styles::kVertical>
{
	static inline void clientRectToContent (Rect& rect, int tabHeight)				{ rect.right -= tabHeight; }
	static inline void clientRectToHeader (Rect& rect, int tabHeight)				{ rect.left = rect.right - tabHeight; }
	static inline void headerToFirstTab (Rect& rect, Coord inset, Coord overlap)	{ rect.right -= inset; rect.top += inset + overlap; }

	static inline void setTabWidth (Rect& rect, Coord width)			{ rect.bottom = rect.top + width; }
	static inline void setTabHeight (Rect& rect, Coord height)			{ rect.right = rect.left + height; }
	static inline void toNextTab (Rect& rect, Coord offset = 0)			{ rect.top = rect.bottom + offset; }
	static inline void offset (Rect& rect, Coord offset)				{ rect.top += offset; rect.bottom += offset; }
	static inline void offset (Point& p, Coord offset)					{ p.y += offset; }
	static inline bool isInside (PointRef p, RectRef rect)				{ return p.y >= rect.top && p.y <= rect.bottom; }
	static inline Coord& getStartCoord (Rect& rect)						{ return rect.top; }
	static inline Coord& getEndCoord (Rect& rect)						{ return rect.bottom; }

	static inline void layoutLabel (RenderAtom& iconAtom, RenderAtom& valueAtom, Transform& labelTransform, RectRef rect, StringRef text, IImage* icon, Coord iconSpacing, bool center);
};

} // namespace CCL


//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kHorizontal>::layoutLabel (RenderAtom& iconAtom, RenderAtom& valueAtom, Transform& labelTransform, RectRef rect, StringRef text, IImage* icon, Coord iconSpacing, bool center)
{
	labelTransform = Transform ();

	Rect labelRect (rect);
	if(icon)
	{
		Rect iconRect (0, 0, icon->getWidth (), icon->getHeight ());
		if(text.isEmpty ())
			iconRect.center (rect);
		else
		{
			iconRect.centerV (rect);
			iconRect.offset (rect.left + (kTabControl / 2));
		}

		iconAtom.setSize (rectIntToF (iconRect));
		labelRect.left = iconRect.right + iconSpacing;
	}

	if(!text.isEmpty ())
	{
		RectF textBounds;
		valueAtom.setSize (rectIntToF (labelRect));
		valueAtom.updateTextSize (textBounds);

		float top = labelRect.top + (labelRect.getHeight () - textBounds.getHeight ()) / 2.f;
		float left = center ? (labelRect.left + (labelRect.getWidth () - textBounds.getWidth ()) / 2.f) : (float)labelRect.left;
		valueAtom.setSize (RectF (left, top, left + textBounds.getWidth (), top + textBounds.getHeight ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TabViewGeometry<Styles::kVertical>::layoutLabel (RenderAtom& iconAtom, RenderAtom& valueAtom, Transform& labelTransform, RectRef rect, StringRef text, IImage* icon, Coord iconSpacing, bool center)
{
	ASSERT (icon == nullptr) // not implemented!
	iconAtom.isHidden (true);

	labelTransform = Transform ();
	labelTransform.translate ((float)rect.right, (float)rect.top);
	labelTransform.rotate (Math::degreesToRad (90.f));

	if(text.isEmpty ())
		return;

	Rect r (0, 0, rect.getHeight (), rect.getWidth ());

	RectF textBounds;
	valueAtom.setSize (rectIntToF (r));
	valueAtom.updateTextSize (textBounds);

	float top = (r.getHeight () - textBounds.getHeight ()) / 2.f;
	float left = center ? (r.getWidth () - textBounds.getWidth ()) / 2.f : 0.f;
	valueAtom.setSize (RectF (left, top, left + textBounds.getWidth (), top + textBounds.getHeight ()));
}

//************************************************************************************************
// TabViewRenderer::TabItemRenderer
/** Draws one tab. Its atoms are kept from one appearance of the tab to the next, so they animate. */
//************************************************************************************************

class TabViewRenderer::TabItemRenderer: public AtomRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (TabItemRenderer, AtomRenderer)

	TabItemRenderer (TabViewRenderer& tabViewRenderer);
	~TabItemRenderer ();

	enum TabAtomTags
	{
		kBackAtom,
		kIconAtom,
		kValueAtom,
		kTabMenuAtom
	};

	void setAppearance (TabKind kind, int state, IImage* icon); ///< icon: the one the tab shows, nullptr for none
	void setLabel (StringRef label, bool center);
	void setSize (RectRef size);
	void draw (IGraphics& graphics, const UpdateRgn& updateRgn);

	// AtomRenderer
	void draw (const UpdateRgn& updateRgn) override;
	void updateAtoms () override;
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	bool getPartRect (Rect& rect, int partCode) override;
	void invalidate (const RenderAtom& atom) override;

protected:
	TabViewRenderer& tabViewRenderer;
	RenderAtom backAtom;
	RenderAtom iconAtom;
	RenderAtom valueAtom;
	RenderAtom tabMenuAtom;
	StringTextModel* textModel;
	String label;
	bool centered;
	Rect size;
	Transform labelTransform;
	TabKind kind;
	int state;
	SharedPtr<IImage> icon;

	#if LEGACY_STYLES_SUPPORTED
	void applyLegacyStyle ();
	#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (TabViewRenderer::TabItemRenderer, AtomRenderer)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::TabItemRenderer::TabItemRenderer (TabViewRenderer& _tabViewRenderer)
: AtomRenderer (_tabViewRenderer.getVisualStyle (), _tabViewRenderer.getOwner ()),
  tabViewRenderer (_tabViewRenderer),
  backAtom (AtomName::kTabBackground, kBackAtom),
  iconAtom (AtomName::kTabIcon, kIconAtom),
  valueAtom (AtomName::kTabValue, kValueAtom),
  tabMenuAtom (AtomName::kTabMenuIcon, kTabMenuAtom),
  textModel (nullptr),
  centered (true),
  kind (kSingleTab),
  state (0)
{
	textModel = NEW StringTextModel ();
	valueAtom.setTextModel (textModel);
	valueAtom.setTextFont (visualStyle->getTextFont ());

	addAtom (backAtom);
	addAtom (iconAtom);
	addAtom (valueAtom);
	addAtom (tabMenuAtom);

	backAtom.isContentFill (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::TabItemRenderer::~TabItemRenderer ()
{
	safe_release (textModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::setAppearance (TabKind _kind, int _state, IImage* _icon)
{
	if(_kind == kind && _state == state && _icon == icon)
		return;

	kind = _kind;
	state = _state;
	icon = _icon;
	invalidateStyleConditions ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::setLabel (StringRef _label, bool center)
{
	centered = center;
	if(_label == label)
		return;

	label = _label;
	textModel->fromParamString (label);
	valueAtom.needsTextLayoutRebuild (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::setSize (RectRef _size)
{
	size = _size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	backAtom.setSize (rectIntToF (size));

	RectRef padding = tabViewRenderer.padding;
	Rect labelRect (size);
	labelRect.left += padding.left;
	labelRect.top += padding.top;
	labelRect.right -= padding.right;
	labelRect.bottom -= padding.bottom;

	const Image* menuImage = tabMenuAtom.getImage ();
	bool hasTabMenu = menuImage && (state & kTabStateOn) && owner->getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu);
	tabMenuAtom.isHidden (!hasTabMenu);
	if(hasTabMenu)
	{
		Rect menuRect (0, 0, menuImage->getWidth (), menuImage->getHeight ());
		menuRect.offset (labelRect.right - menuRect.right);
		menuRect.centerV (labelRect);
		tabMenuAtom.setSize (rectIntToF (menuRect));
		labelRect.right = menuRect.left - tabViewRenderer.tabMenuSpacing;
	}

	// the size of a shared image can depend on its frame
	iconAtom.isHidden (!icon.isValid ());
	if(icon)
		icon->setCurrentFrame (iconAtom.getImageFrame ());

	bool centerText = centered && !icon.isValid ();
	Alignment alignment (centerText ? Alignment::kCenter : Alignment::kLeftCenter);
	if(valueAtom.getTextFormat ().getAlignment ().align != alignment.align)
	{
		valueAtom.setTextFormat (TextFormat (alignment));
		valueAtom.needsTextLayoutRebuild (true);
	}

	valueAtom.isHidden (label.isEmpty ());

	if(owner->getStyle ().isVertical ())
		TabViewGeometry<Styles::kVertical>::layoutLabel (iconAtom, valueAtom, labelTransform, labelRect, label, icon, tabViewRenderer.iconSpacing, centerText);
	else
		TabViewGeometry<Styles::kHorizontal>::layoutLabel (iconAtom, valueAtom, labelTransform, labelRect, label, icon, tabViewRenderer.iconSpacing, centerText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::draw (IGraphics& graphics, const UpdateRgn& updateRgn)
{
	updateAtoms ();

	backAtom.draw (graphics, updateRgn);
	tabMenuAtom.draw (graphics, updateRgn);
	iconAtom.draw (graphics, updateRgn);

	TransformSetter transformSetter (graphics, labelTransform);
	valueAtom.draw (graphics, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort port (owner);
	draw (port, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewRenderer::TabItemRenderer::getPartRect (Rect& rect, int partCode)
{
	RenderAtom* atom = findAtom (partCode);
	if(atom == nullptr || atom->isHidden ())
		return false;

	rect = rectFToInt (atom->getSize ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	setTabConditions (conditionContext, kind, state);

	SuperClass::applyVisualStyle (transaction);

	int styleFrame = backAtom.getImageFrame ();

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	applyLegacyStyle ();
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	// named frames of the tab image follow the tab state, "imageframe" only applies to an image without them
	int frameIndex = getTabImageFrame (backAtom.getImage (), state);
	backAtom.setImageFrame (frameIndex >= 0 ? frameIndex : styleFrame);

	// the TabViewRenderer decides about the image, as the icon takes up space within the tab
	iconAtom.setImage (unknown_cast<Image> (icon));

	auto getIconFrameName = [this] ()
	{
		if(state & kTabStateOn)
			return ThemeNames::kNormalOn;
		if(state & kTabStatePressed)
			return ThemeNames::kPressed;
		if(state & kTabStateMouseOver)
			return ThemeNames::kMouseOver;
		return ThemeNames::kNormal;
	};

	// the first frame stands in for a missing one
	if(Image* iconImage = iconAtom.getImage ())
		iconAtom.setImageFrame (ccl_max (0, iconImage->getFrameIndex (getIconFrameName ())));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::invalidate (const RenderAtom& atom)
{
	// An atom that is scaled or offset draws beyond its own rect, and in a vertical TabView the title is
	// laid out in the coordinates of the rotated tab - so the whole tab is drawn again.
	SuperClass::invalidate (backAtom);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::TabItemRenderer::applyLegacyStyle ()
{
	// A tab used to take its image from "first", "middle", "last" or "single" depending on its
	// position within the row, and the menu button from "menuBackground".
	IImage* image = tabViewRenderer.middleButton;
	switch(kind)
	{
	case kLeftTab :
		if(tabViewRenderer.firstButton)
			image = tabViewRenderer.firstButton;
		break;

	case kRightTab :
		if(tabViewRenderer.lastButton)
			image = tabViewRenderer.lastButton;
		break;

	case kSingleTab :
		if(tabViewRenderer.singleButton)
			image = tabViewRenderer.singleButton;
		break;

	case kMenuTab :
		// without an image of its own the menu button looks like the last tab
		if(tabViewRenderer.menuBackground)
			image = tabViewRenderer.menuBackground;
		else if(tabViewRenderer.lastButton)
			image = tabViewRenderer.lastButton;
		break;

	case kMiddleTab :
		image = tabViewRenderer.middleButton;
		break;
	}

	if(image)
		backAtom.setImage (unknown_cast<Image> (image));

	// "activetextcolor" was the title color of the active tab, "textcolor" the one of all others
	Brush textBrush (visualStyle->getTextBrush ());
	Color textColor;
	bool hasLegacyTextColor = visualStyle->getGradient (StyleID::kTextColor) || visualStyle->getColor (textColor, StyleID::kTextColor);
	if((state & kTabStateOn) && visualStyle->getColor (textColor, LegacyStyleID::kTabViewActiveTextColor))
	{
		textBrush = SolidBrush (textColor);
		hasLegacyTextColor = true;
	}

	if(hasLegacyTextColor || !valueAtom.getBrush ().isVisible ())
		valueAtom.setBrush (textBrush);

	// "tabMenuIcon" is the menu icon the active tab displays
	if(IImage* legacyTabMenuIcon = visualStyle->getImage (LegacyStyleID::kTabViewTabMenuIcon))
		tabMenuAtom.setImage (unknown_cast<Image> (legacyTabMenuIcon));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif // LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

//************************************************************************************************
// TabViewRenderer
//************************************************************************************************

DEFINE_TYPELIB_ENTRY_WITH_PARENT (TabView, VisualStyle, "TabViewStyle", VISUALSTYLES_TYPELIB)
BEGIN_TYPELIB_MEMBERS (TabView)
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kBackground)	///< the content area next to the tabs
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kHeader)		///< the strip behind the tabs
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kTabBackground)	///< the background of a tab - the "left", "middle", "right" and "menu" style conditions select its position within the row (a single tab has none of them), "on", "mouseover", "pressed" and "disabled" its state, the first three also the named frame of its image
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kTabIcon)			///< the icon of a tab - a tab without an icon of its own (like the menu button) shows the image of this atom, resolved with the "middle" or "menu" condition alone
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kTabValue)			///< the title of a tab
	ADD_TYPELIB_MEMBER_RENDERATOM (AtomName::kTabMenuIcon)		///< the menu icon the active tab displays ("tabmenu" option)
	ADD_TYPELIB_MEMBER_METRIC (StyleID::TabView::kTabHeight)		///< height of the tabs
	ADD_TYPELIB_MEMBER_METRIC (StyleID::TabView::kTabSpacing)		///< spacing between the tabs
	ADD_TYPELIB_MEMBER_METRIC (StyleID::TabView::kTabMenuSpacing)	///< distance between the title and the menu icon of the active tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kMargin)					///< distance between the edges of the content area and the child view of the active tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::TabView::kTabPreferIcon)	///< a tab that has an icon shows only that icon
	ADD_TYPELIB_MEMBER_METRIC (StyleID::BIV::kIconValueSpacing)		///< distance between the icon and the title of a tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kExtendedPopup)				///< displays the menu as an extended menu instead of the default tree menu
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPadding)					///< padding for the icon and the title of a tab, used if one of the paddings for left, top, right, bottom is not specified
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingLeft)				///< left padding for the icon and the title of a tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingTop)				///< top padding for the icon and the title of a tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingRight)				///< right padding for the icon and the title of a tab
	ADD_TYPELIB_MEMBER_METRIC (StyleID::kPaddingBottom)				///< bottom padding for the icon and the title of a tab
END_TYPELIB_MEMBERS (TabView)

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::TabViewRenderer (VisualStyle* visualStyle, View* owner)
: ControlRenderer (visualStyle, owner),
  iconSpacing (kTabIconSpacing),
  tabSpacing (kTabSpacing),
  tabMenuSpacing (kTabMenuSpacing),
  tabSlope (kTabSlope),
  tabExtraWidth (2 * kTabSlope),
  tabHeight (kTabSlope),
  contentMargin (kContentMargin),
  inset (0),
  menuTabWidth (kMenuTabWidth),
  backgroundAtom (AtomName::kBackground),
  headerAtom (AtomName::kHeader),
  menuItem (nullptr)
{
	tabItems.objectCleanup (true);

	addAtom (backgroundAtom);
	addAtom (headerAtom);

	backgroundAtom.isContentFill (true);
	headerAtom.isContentFill (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::~TabViewRenderer ()
{
	safe_release (menuItem);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::updateAtoms ()
{
	SuperClass::updateAtoms ();

	if(owner == nullptr)
		return;

	Rect headerRect;
	owner->getClientRect (headerRect);
	Rect contentRect (headerRect);

	if(owner->getStyle ().isVertical ())
	{
		TabViewGeometry<Styles::kVertical>::clientRectToHeader (headerRect, tabHeight);
		TabViewGeometry<Styles::kVertical>::clientRectToContent (contentRect, tabHeight);
	}
	else
	{
		TabViewGeometry<Styles::kHorizontal>::clientRectToHeader (headerRect, tabHeight);
		TabViewGeometry<Styles::kHorizontal>::clientRectToContent (contentRect, tabHeight);
	}

	headerAtom.setSize (rectIntToF (headerRect));
	backgroundAtom.setSize (rectIntToF (contentRect));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::applyVisualStyle (RenderAtomTransaction& transaction)
{
	SuperClass::applyVisualStyle (transaction);

	// Renderer properties
	visualStyle->getPadding (padding);
	tabHeight = visualStyle->getMetric<int> (StyleID::TabView::kTabHeight, kTabSlope);
	tabSpacing = visualStyle->getMetric<int> (StyleID::TabView::kTabSpacing, kTabSpacing);
	tabMenuSpacing = visualStyle->getMetric<int> (StyleID::TabView::kTabMenuSpacing, kTabMenuSpacing);
	contentMargin = visualStyle->getMetric<int> (StyleID::kMargin, kContentMargin);

	iconSpacing = visualStyle->getMetric<int> (StyleID::BIV::kIconValueSpacing, kTabIconSpacing);

	RenderAtom tabMenuAtom (AtomName::kTabMenuIcon);
	tabMenuIcon = tabMenuAtom.getStyleImage (*visualStyle);

	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// Legacy renderer and atom properties
	applyLegacyVisualStyle ();
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif

	// without a tab image the tabs use the sloped geometry: they overlap and are inset from the header edge
	RenderAtom backAtom (AtomName::kTabBackground);
	RenderAtom iconAtom (AtomName::kTabIcon);
	StyleConditionContext tabConditions;
	setTabConditions (tabConditions, kMiddleTab, 0);
	bool hasTabImage = backAtom.getStyleImage (*visualStyle, &tabConditions) != nullptr;
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	hasTabImage = hasTabImage || middleButton != nullptr;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
	tabSlope = hasTabImage ? 0 : kTabSlope;
	tabExtraWidth = hasTabImage ? kTabControl : 2 * kTabSlope;
	inset = hasTabImage ? 0 : 2;

	// the icon of the style takes up space within a tab, so it does not depend on the state of a tab either
	styleTabIcon = iconAtom.getStyleImage (*visualStyle, &tabConditions);

	// The menu button is as wide as the image that draws it.
	setTabConditions (tabConditions, kMenuTab, 0);
	IImage* menuTabImage = backAtom.getStyleImage (*visualStyle, &tabConditions);
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	if(menuBackground)
		menuTabImage = menuBackground;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
	menuTabWidth = menuTabImage ? menuTabImage->getWidth () : kMenuTabWidth;

	menuTabIcon = iconAtom.getStyleImage (*visualStyle, &tabConditions);
	#if LEGACY_STYLES_SUPPORTED
	BEGIN_LEGACY_STYLES_ENABLED_BLOCK
	// "menuIcon" is the icon of the menu button
	if(menuIcon)
		menuTabIcon = menuIcon;
	END_LEGACY_STYLES_ENABLED_BLOCK
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#if LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::applyLegacyVisualStyle ()
{
	firstButton = visualStyle->getImage (LegacyStyleID::kFirstImage);
	middleButton = visualStyle->getImage (LegacyStyleID::kMiddleImage);
	lastButton = visualStyle->getImage (LegacyStyleID::kLastImage);
	singleButton = visualStyle->getImage (LegacyStyleID::kSingleImage);
	menuIcon = visualStyle->getImage (LegacyStyleID::kTabViewMenuIcon);
	menuBackground = visualStyle->getImage (LegacyStyleID::kTabViewMenuBackground);

	// "background" and "headerBackground" are the legacy IDs of the images of the "background" and "header" atoms
	if(IImage* legacyBackground = visualStyle->getBackgroundImage ())
		backgroundAtom.setImage (unknown_cast<Image> (legacyBackground));

	if(IImage* legacyHeaderBackground = visualStyle->getImage (LegacyStyleID::kTabViewHeaderBackground))
		headerAtom.setImage (unknown_cast<Image> (legacyHeaderBackground));

	if(IImage* legacyTabMenuIcon = visualStyle->getImage (LegacyStyleID::kTabViewTabMenuIcon))
		tabMenuIcon = legacyTabMenuIcon;

	// The content area used to be framed in "bordercolor" - and in black when the style named no
	// border color - whenever no image covered it.
	if(backgroundAtom.getImage () == nullptr)
	{
		Pen borderPen (backgroundAtom.getBorderPen ());
		Color legacyBorderColor (Colors::kBlack);
		if(visualStyle->getColor (legacyBorderColor, LegacyStyleID::kBorderColor) || !borderPen.isVisible ())
		{
			borderPen.setColor (legacyBorderColor);
			backgroundAtom.setBorderPen (borderPen);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
#endif // LEGACY_STYLES_SUPPORTED
//////////////////////////////////////////////////////////////////////////////////////////////////
// static
int TabViewRenderer::getTabImageFrame (IImage* image, int state)
{
	if(image == nullptr)
		return -1;

	int frameIndex = -1;
	if(state & kTabStateOn)
	{
		if(state & kTabStateMouseOver)
			frameIndex = image->getFrameIndex (ThemeNames::kMouseOverOn);

		if(state & kTabStatePressed)
			frameIndex = image->getFrameIndex (ThemeNames::kPressedOn);

		if(frameIndex < 0)
			frameIndex = image->getFrameIndex (ThemeNames::kNormalOn);
		if(frameIndex < 0)
			frameIndex = image->getFrameIndex (ThemeNames::kPressedOn);
		if(frameIndex < 0)
			frameIndex = image->getFrameIndex (ThemeNames::kPressed);
	}
	else
	{
		if(state & kTabStateMouseOver)
			frameIndex = image->getFrameIndex (ThemeNames::kMouseOver);

		if(state & kTabStatePressed)
			frameIndex = image->getFrameIndex (ThemeNames::kPressed);

		if(frameIndex < 0)
			frameIndex = image->getFrameIndex (ThemeNames::kNormal);
	}

	return frameIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// static
void TabViewRenderer::setTabConditions (StyleConditionContext& context, TabKind kind, int state)
{
	context.setCondition (ThemeNames::kLeft, kind == kLeftTab);
	context.setCondition (ThemeNames::kMiddle, kind == kMiddleTab);
	context.setCondition (ThemeNames::kRight, kind == kRightTab);
	context.setCondition (ThemeNames::kMenu, kind == kMenuTab);
	context.setCondition (ThemeNames::kOn, (state & kTabStateOn) != 0);
	context.setCondition (ThemeNames::kMouseOver, (state & kTabStateMouseOver) != 0);
	context.setCondition (ThemeNames::kPressed, (state & kTabStatePressed) != 0);
	context.setCondition (ThemeNames::kDisabled, (state & kTabStateDisabled) != 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* TabViewRenderer::getDisplayedTabIcon (TabView* tabView, int index) const
{
	if(index == TabView::kPartMenuTab - TabView::kPartFirstTab)
		return menuTabIcon;

	if(IImage* icon = tabView->getTabIcon (index))
		return icon;

	return styleTabIcon;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabViewRenderer::getTabState (TabView* tabView, int index, bool active) const
{
	int state = active ? kTabStateOn : 0;
	if(tabView->getMouseOverTab () == index)
		state |= kTabStateMouseOver;
	if(tabView->getMouseDownTab () == index)
		state |= kTabStatePressed;
	if(!tabView->isEnabled ())
		state |= kTabStateDisabled;

	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TabViewRenderer::TabItemRenderer& TabViewRenderer::getTabItem (int index)
{
	if(index == TabView::kPartMenuTab - TabView::kPartFirstTab)
	{
		if(menuItem == nullptr)
			menuItem = NEW TabItemRenderer (*this);

		return *menuItem;
	}

	while(tabItems.count () <= index)
		tabItems.add (NEW TabItemRenderer (*this));

	return *static_cast<TabItemRenderer*> (tabItems.at (index));
}


//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TabViewRenderer::calcTabWidth (TabView* tabView, int index, FontRef font)
{
	String title;
	tabView->getTabTitle (title, index);
	return calcTabWidth (tabView, index, title, getDisplayedTabIcon (tabView, index), font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TabViewRenderer::calcTabWidth (TabView* tabView, int index, StringRef label, IImage* icon, FontRef font)
{
	Coord width = 0;
	if(!label.isEmpty ())
		width = Font::getStringWidth (label, font);
	if(icon)
		width += icon->getWidth ();
	if(tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorTabMenu) && tabMenuIcon && index == tabView->getActiveIndex ())
		width += tabMenuIcon->getWidth () + tabMenuSpacing;

	Coord fillWidth = tabView->getFillWidth ();
	if(fillWidth > 0)
	{
		int numTabs = tabView->countTabs ();
		Coord w = fillWidth / numTabs;
		Coord remainder = fillWidth - w * numTabs;
		width += w;
		if(index < remainder)
			width++;
	}

	return width + padding.left + padding.right + iconSpacing;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::drawTab (GraphicsDevice& port, RectRef r, int tabIndex)
{
	// This is called from outside the drawing loop for the image of a tab that is being reordered.
	updateAtoms ();

	TabView* tabView = static_cast<TabView*> (owner);
	Font textFont (visualStyle->getTextFont ());

	UpdateRgn updateRgn (r);
	TabDrawArgs args (tabView, port, updateRgn, textFont);

	String label;
	tabView->getTabTitle (label, tabIndex);
	Font::collapseString (label, r.getWidth (), args.font, Font::kTrimModeRight);

	IImage* icon = getDisplayedTabIcon (tabView, tabIndex);
	bool active = tabIndex == tabView->getActiveIndex ();

	// a renderer of its own keeps the tab in the TabView from changing its appearance
	AutoPtr<TabItemRenderer> item = NEW TabItemRenderer (*this);
	drawTabItem (args, *item, r, kSingleTab, tabIndex, active, label, icon, true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::draw (const UpdateRgn& updateRgn)
{
	updateAtoms ();

	if(owner->getStyle ().isVertical ())
		draw<TabViewGeometry<Styles::kVertical> > (updateRgn);
	else
		draw<TabViewGeometry<Styles::kHorizontal> > (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TabViewRenderer::hitTest (PointRef where, Point* clickOffset)
{
	updateAtoms ();

	if(owner->getStyle ().isVertical ())
		return hitTest<TabViewGeometry<Styles::kVertical> > (where, clickOffset);
	else
		return hitTest<TabViewGeometry<Styles::kHorizontal> > (where, clickOffset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TabViewRenderer::getPartRect (Rect& rect, int partCode)
{
	updateAtoms ();

	if(owner->getStyle ().isVertical ())
		return getPartRect<TabViewGeometry<Styles::kVertical> > (rect, partCode);
	else
		return getPartRect<TabViewGeometry<Styles::kHorizontal> > (rect, partCode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
void TabViewRenderer::draw (const UpdateRgn& updateRgn)
{
	TabView* tabView = static_cast<TabView*> (owner);
	GraphicsPort port (owner);
	Font textFont (visualStyle->getTextFont ());

	TabDrawArgs args (tabView, port, updateRgn, textFont);

	Rect clientRect;
	owner->getClientRect (clientRect);

	// header - updateAtoms () has sized the atom
	Rect header (clientRect);
	Geometry::clientRectToHeader (header, tabHeight);
	if(header.getWidth () > 5 && header.getHeight () > 5)
		headerAtom.draw (port, updateRgn);

	// background - updateAtoms () has sized the atom
	Rect content (clientRect);
	Geometry::clientRectToContent (content, tabHeight);
	if(content.getWidth () > 5 && content.getHeight () > 5)
		backgroundAtom.draw (port, updateRgn);

	int activeIndex = tabView->getActiveIndex ();
	int numTabs = tabView->countTabs ();

	// drop the renderers of the tabs that no longer exist
	for(int i = tabItems.count () - 1; i >= numTabs; i--)
	{
		Object* item = tabItems.at (i);
		tabItems.removeAt (i);
		item->release ();
	}

	if(numTabs <= 0)
		return;

	Coord tabsEndCoord = Geometry::getEndCoord (header);

	// running tab
	String label;
	TabKind tabKind = kLeftTab;
	Rect tabRect = header;
	Geometry::headerToFirstTab (tabRect, inset, 0);

	// active tab
	String activeTabLabel;
	IImage* activeTabIcon = nullptr;
	TabKind activeTabKind = kLeftTab;
	Rect activeTabRect;

	// menu tab
	Rect menuRect;
	Coord menuStart = kMaxCoord;

	// reset menu & scroll offset for calculation
	tabView->setMenu (false);
	tabView->setScrollOffset (0);
	tabView->setFillWidth (0);

	// check if menu required
	Rect requiredSize;
	calcViewSize<Geometry> (tabView, requiredSize);
	Coord remaingWidth = header.getWidth () - requiredSize.getWidth ();//todo: Geometry
	bool isOverflow = remaingWidth < 0;
	bool canHaveMenu = !tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorNoMenu);
	bool hasMenu = canHaveMenu && isOverflow;
	tabView->setMenu (hasMenu);

	// extend tabs to fill header
	if(remaingWidth > 0)
	{
		if(tabView->getStyle ().isCustomStyle (Styles::kTabViewBehaviorExtendTabs))
			tabView->setFillWidth (remaingWidth);
		else if(tabView->getStyle ().isCustomStyle (Styles::kTabViewAppearanceCentered))
			tabView->setCenterOffset (remaingWidth / 2);
	}

	// use scroll offset to make the active tab completely visible
	Coord scrollOffset = 0;
	getPartRect (activeTabRect, TabView::kPartFirstTab + activeIndex);
	if(activeIndex != numTabs - 1)
		activeTabRect.right -= tabSpacing;

	if(isOverflow)
	{
		if(hasMenu)
		{
			// calc menu rect
			getPartRect (menuRect, TabView::kPartMenuTab);
			menuStart = Geometry::getStartCoord (menuRect);
			tabsEndCoord = menuStart;
		}

		Coord overlap = Geometry::getEndCoord (activeTabRect) - tabsEndCoord;
		if(overlap > 0 && numTabs > 1)
		{
			scrollOffset = overlap;
			tabView->setScrollOffset (scrollOffset);

			// recalc active rect
			getPartRect (activeTabRect, TabView::kPartFirstTab + activeIndex);
		}
	}

	Geometry::offset (tabRect, -scrollOffset);
	Geometry::offset (tabRect, tabView->getCenterOffset ());

	if(numTabs == 1 && !hasMenu)
		tabKind = kSingleTab;

	for(int i = 0; i < numTabs; i++)
	{
		if(Geometry::getStartCoord (tabRect) >= tabsEndCoord)
			break;

		Coord currentTabSpacing = (i == numTabs - 1) ? 0 : tabSpacing;

		tabView->getTabTitle (label, i);
		IImage* icon = getDisplayedTabIcon (tabView, i);
		Coord width = calcTabWidth (tabView, i, label, icon, args.font);
		width += tabExtraWidth;
		Geometry::setTabWidth (tabRect, width - currentTabSpacing);

		// skip tab if completly beyond left edge
		bool beyondLeft = Geometry::getEndCoord (tabRect) < 0;
		bool collapsed = false;
		if(!beyondLeft)
		{
			if(Geometry::getStartCoord (tabRect) < 0)
			{
				// overflow on left edge...
				Geometry::getStartCoord (tabRect) = 0;
				width = Geometry::getEndCoord (tabRect) - Geometry::getStartCoord (header);
				Font::collapseString (label, width, args.font, Font::kTrimModeRight);
				if(icon)
				{
					label = String::kEmpty;
					if(width < icon->getWidth ())
						icon = nullptr;
				}
				else
					collapsed = true;
			}
			else
			{
				Coord overflow = Geometry::getEndCoord (tabRect) - tabsEndCoord;
				if(overflow > 0)
				{
					// overflow on right edge
					width -= overflow;
					Geometry::setTabWidth (tabRect, width);
					Font::collapseString (label, width, args.font, Font::kTrimModeRight);
					if(icon)
					{
						label = String::kEmpty;
						if(width < icon->getWidth ())
							icon = nullptr;
					}
					else
						collapsed = true;

				}
			}

			if(i == activeIndex)
			{
				activeTabKind = tabKind;
				activeTabLabel = label;
				activeTabIcon = icon;
			}
			else
				drawTabItem (args, getTabItem (i), tabRect, tabKind, i, false, label, icon, !collapsed);
		}

		Geometry::toNextTab (tabRect, - tabSlope + currentTabSpacing);

		if(!beyondLeft)
			tabKind = (hasMenu || i < numTabs - 2) ? kMiddleTab : kRightTab;
	}


	if(activeIndex >= 0)
		drawTabItem (args, getTabItem (activeIndex), activeTabRect, activeTabKind, activeIndex, true, activeTabLabel, activeTabIcon, true);

	if(hasMenu)
	{
		int menuIndex = TabView::kPartMenuTab - TabView::kPartFirstTab;
		drawTabItem (args, getTabItem (menuIndex), menuRect, kMenuTab, menuIndex, false, String::kEmpty, getDisplayedTabIcon (tabView, menuIndex), true);
	}

	owner->View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TabViewRenderer::drawTabItem (TabDrawArgs& args, TabItemRenderer& item, RectRef tabRect, TabKind kind, int index, bool active, StringRef label, IImage* icon, bool center)
{
	item.setAppearance (kind, getTabState (args.tabView, index, active), icon);
	item.setLabel (label, center);
	item.setSize (tabRect);
	item.draw (args.port, args.updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
int TabViewRenderer::hitTest (PointRef where, Point* clickOffset)
{
	TabView* tabView = static_cast<TabView*> (owner);
	int tab = findTab<Geometry> (tabView, where);
	if(tab >= 0)
	{
		tab += TabView::kPartFirstTab;
		if(tab <= TabView::kPartLastTab)
			return tab;
	}

	Rect client;
	tabView->getClientRect (client);

	Rect rect (client);
	rect.contract (inset);
	Geometry::clientRectToContent (rect, tabHeight);
	if(rect.pointInside (where))
		return TabView::kPartContent;

	rect = client;
	Geometry::clientRectToHeader (rect, tabHeight);
	if(rect.pointInside (where))
		return TabView::kPartHeader;

	return TabView::kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
bool TabViewRenderer::getPartRect (Rect& rect, int partCode)
{
	TabView* tabView = static_cast<TabView*> (owner);
	if(partCode == TabView::kPartContent)
	{
		owner->getClientRect (rect);
		rect.contract (contentMargin);
		Geometry::clientRectToContent (rect, tabHeight);
		return true;
	}

	if(partCode == TabView::kPartViewSize)
	{
		calcViewSize<Geometry> (tabView, rect);
		return true;
	}

	if(partCode == TabView::kPartHeader)
	{
		owner->getClientRect (rect);
		Geometry::clientRectToHeader (rect, tabHeight);
		return true;
	}

	if(partCode == TabView::kPartMenuTab)
	{
		if(tabView->isMenu ())
		{
			owner->getClientRect (rect);
			Geometry::clientRectToHeader (rect, tabHeight);
			Geometry::headerToFirstTab (rect, inset, 0);
			Geometry::toNextTab (rect, -menuTabWidth);
			return true;
		}
		return false;
	}

	if(partCode >= TabView::kPartFirstTab && partCode <= TabView::kPartLastTab)
	{
		Rect r;
		owner->getClientRect (r);
		Geometry::clientRectToHeader (r, tabHeight);
		Geometry::headerToFirstTab (r, inset, 0);
		Geometry::offset (r, - tabView->getScrollOffset ());

		Font tabFont (visualStyle->getTextFont ());

		int numTabs = tabView->countTabs ();
		for(int i = 0; i < numTabs; i++)
		{
			Coord width = calcTabWidth (tabView, i, tabFont);
			Geometry::setTabWidth (r, width + tabExtraWidth);
			if(i == partCode - TabView::kPartFirstTab)
			{
				Geometry::offset (r, tabView->getCenterOffset ());
				rect = r;
				return true;
			}
			Geometry::toNextTab (r, -tabSlope);
		}
	}

	if(partCode >= TabView::kPartTabMenu)
	{
		int tabIndex = partCode - TabView::kPartTabMenu;
		if(tabIndex < tabItems.count ())
			return getTabItem (tabIndex).getPartRect (rect, TabItemRenderer::kTabMenuAtom);
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
int TabViewRenderer::findTab (TabView* tabView, PointRef where)
{
	Rect rect;
	tabView->getClientRect (rect);
//	rect.contract (inset);

	Geometry::clientRectToHeader (rect, tabHeight);
	if(!rect.pointInside (where))
		return -1;

	if(tabView->isMenu () && where.x > Geometry::getEndCoord (rect) - menuTabWidth)
		return TabView::kPartMenuTab - TabView::kPartFirstTab;

	Point p (where);
	Geometry::offset (p, tabView->getScrollOffset ());
	Geometry::offset (rect, tabView->getCenterOffset ());

	if(tabSlope != 0)
	{
		rect.contract (1);
		Geometry::headerToFirstTab (rect, inset, tabSlope / 2); // half width of the overlapping curves
	}

	Font tabFont (visualStyle->getTextFont ());
	int numTabs = tabView->countTabs ();
	for(int i = 0; i < numTabs; i++)
	{
		Coord width = calcTabWidth (tabView, i, tabFont);
		Geometry::setTabWidth (rect, width + tabExtraWidth);

		if(Geometry::isInside (p, rect))
			return i;

		Geometry::toNextTab (rect);
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class Geometry>
void TabViewRenderer::calcViewSize (TabView* tabView, Rect& r)
{
	Rect rect;
	Geometry::setTabWidth (rect, 0);
	Geometry::setTabHeight (rect, tabHeight);

	Font tabFont (visualStyle->getTextFont ());

	int numTabs = tabView->countTabs ();
	for(int i = 0; i < numTabs; i++)
	{
		Coord width = calcTabWidth (tabView, i, tabFont);
		Geometry::setTabWidth (rect, width + tabExtraWidth);
		if(i < (numTabs - 1))
			Geometry::toNextTab (rect, -tabSlope);
	}

	r = rect;
	r.left = 0;
	r.top = 0;
}
