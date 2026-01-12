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
// Filename    : ccl/gui/theme/theme.cpp
// Description : Theme class
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/theme/theme.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/theme/palette.h"

#include "ccl/gui/theme/renderer/buttonrenderer.h"
#include "ccl/gui/theme/renderer/comboboxrenderer.h"
#include "ccl/gui/theme/renderer/dividerrenderer.h"
#include "ccl/gui/theme/renderer/valuebarrenderer.h"
#include "ccl/gui/theme/renderer/scrollbarrenderer.h"
#include "ccl/gui/theme/renderer/selectboxrenderer.h"
#include "ccl/gui/theme/renderer/sliderrenderer.h"
#include "ccl/gui/theme/renderer/tabviewrenderer.h"
#include "ccl/gui/theme/renderer/headerviewrenderer.h"
#include "ccl/gui/theme/renderer/textboxrenderer.h"
#include "ccl/gui/theme/renderer/backgroundrenderer.h"
#include "ccl/gui/theme/renderer/dialoggrouprenderer.h"
#include "ccl/gui/theme/renderer/labelrenderer.h"
#include "ccl/gui/theme/renderer/knobrenderer.h"
#include "ccl/gui/theme/renderer/vectorpadrenderer.h"
#include "ccl/gui/theme/renderer/trivectorpadrenderer.h"
#include "ccl/gui/theme/renderer/valueboxrenderer.h"
#include "ccl/gui/theme/renderer/updownboxrenderer.h"
#include "ccl/gui/theme/renderer/scrollpickerrenderer.h"
#include "ccl/gui/theme/renderer/menubarrenderer.h"

#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/base/storage/configuration.h"

using namespace CCL;
using namespace ThemeElements;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Metrics
//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Theme::metricNames[kNumMetrics] =
{
	"Margin",					// kLayoutMargin
	"Spacing",					// kLayoutSpacing
	"ButtonWidth",				// kButtonWidth
	"ButtonHeight",				// kButtonHeight
	"TextBoxHeight",			// kTextBoxHeight
	"CheckBoxSize",				// kCheckBoxSize
	"ScrollBarSize",			// kScrollBarSize
	"SliderHandleSize",			// kSliderHandleSize
	"DividerSize",				// kDividerSize
	"DividerOutreach",			// kDividerOutreach
	"HeaderHeight",		    	// kHeaderHeight
	"Border",					// kBorder
	"SystemStatusBarHeight",	// kSystemStatusBarHeight
	"SystemNavigationBarHeight",// kSystemNavigationBarHeight
	"SystemMarginLeft",			// kSystemMarginLeft
	"SystemMarginRight",		// kSystemMarginRight
	"TitleBarHeight"			// kTitleBarHeight
};

int Theme::defaultMetrics[kNumMetrics] =
{
	10,	// kLayoutMargin
	8,	// kLayoutSpacing
	75,	// kButtonWidth
	23,	// kButtonHeight
	20,	// kTextBoxHeight
	16, // kCheckBoxSize
	16, // kScrollBarSize
	16,	// kSliderHandleSize
	6,	// kDividerSize
	3,	// kDividerOutreach
	14,	// kHeaderHeight
	4,	// kBorder
	0,	// kSystemStatusBarHeight,
	0,	// kSystemNavigationBarHeight
	0,	// kSystemMarginLeft
	0,	// kSystemMarginRight
	24	// kTitleBarHeight
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Standard Renderer Names
//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr ThemePainter::kStandardPrefix = "Standard.";
const int ThemePainter::kStandardPrefixLength = 9;
CStringPtr ThemePainter::standardStyleNames[ThemePainter::kNumStandardStyles] = 
{
	"Slider",
	"ScrollBar",
	"ScrollButton",
	"PageControl",
	"ValueBar",
	"ProgressBar",
	"TextBox",
	"EditBox",
	"ComboBox",
	"SelectBox",
	"Button",
	"TabView",
	"HeaderView",
	"Divider",
	"WindowBackground",
	"DialogGroup",
	"Label",
	"CheckBox",
	"RadioButton",
	"Knob",
	"VectorPad",
	"TriVectorPad",
	"ValueBox",
	"UpDownBox",
	"RangeSlider",
	"ScrollPicker",
	"MenuBarControl",
	"ListView",
	"TreeView",
	"PopupMenu",
	"PopupMenuLarge",
	"PalettePopup",
	"MenuControl",
	"ContextMenu",
	"PerspectiveSwitcher",
	"SegmentBox",
	"H1",
	"H2",
	"H3",
	"H4",
	"H5",
	"H6"
};

CStringPtr ThemePainter::themeImageNames[kNumThemeElements] = 
	{"Button", "Button", "CheckBox", "CheckBox", "RadioButton", "RadioButton", "TreeViewExpandButton", "TreeViewExpandButton"};
CStringPtr ThemePainter::uniqueImageNames[] = // used by skin type library
	{"Button", "CheckBox", "RadioButton", "TreeViewExpandButton"};
const int ThemePainter::kUniqueImageCount = ARRAY_COUNT(ThemePainter::uniqueImageNames);

// keep in sync with ThemeNames definitions in themeelements.h!
CStringPtr ThemePainter::stateNames[kNumElementStates] = 
	{"normal", "pressed", "mouseover", "disabled", "focus"};
CStringPtr ThemePainter::stateNamesOn[kNumElementStates] = 
	{"normalOn", "pressedOn", "mouseoverOn", "disabledOn", "focus"};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Colors
//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Theme::colorNames[kNumColors] =
{
	"SelectionColor",		// kSelectionColor
	"SelectionTextColor",	// kSelectionTextColor
	"AlphaSelectionColor",	// kAlphaSelectionColor
	"AlphaCursorColor",		// kAlphaCursorColor
	"HyperlinkColor",		// kHyperlinkColor
	"TooltipBackColor",		// kTooltipBackColor
	"TooltipTextColor",		// kTooltipTextColor
	"ListViewBackColor",	// kListViewBackColor
	"PushButtonTextColor"	// kPushButtonTextColor
};

Color Theme::getDefaultColor (ThemeColorID which)
{
	Color color;
	switch(which)
	{
	case kSelectionColor      : color = Colors::kGray; break;
	case kSelectionTextColor  : color = Colors::kBlack; break;
	case kAlphaSelectionColor : color = Color (Colors::kBlue).setAlphaF (.25f); break;
	case kAlphaCursorColor    : color = Color (Colors::kBlue).setAlphaF (.75f); break;
	case kHyperlinkColor      : color = Colors::kBlue; break;
	case kTooltipBackColor    : color = Colors::kWhite; break;
	case kTooltipTextColor    : color = Colors::kBlack; break;
	case kListViewBackColor   : color = Colors::kWhite; break;
	case kPushButtonTextColor : color = Colors::kBlack; break;
	}
	return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Cursors
//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Theme::cursorNames[kNumCursors] =
{
	"ArrowCursor",					// kArrowCursor
	"WaitCursor",					// kWaitCursor
	"CrosshairCursor",				// kCrosshairCursor
	"PointhandCursor",				// kPointhandCursor
	"SizeHorizontalCursor",			// kSizeHorizontalCursor
	"SizeVerticalCursor",			// kSizeVerticalCursor
	"SizeLeftUpRightDownCursor",	// kSizeLeftUpRightDownCursor
	"SizeLeftDownRightUpCursor",	// kSizeLeftDownRightUpCursor
	"TextCursor",					// kTextCursor

	"SizeUpCursor",					// kSizeUpCursor
	"SizeRightCursor",				// kSizeRightCursor
	"SizeDownCursor",				// kSizeDownCursor
	"SizeLeftCursor",				// kSizeLeftCursor
	"SizeLeftUpCursor",				// kSizeLeftUpCursor
	"SizeLeftDownCursor",			// kSizeLeftDownCursor
	"SizeRightUpCursor",			// kSizeRightUpCursor
	"SizeRightDownCursor",			// kSizeRightDownCursor

	"CopyCursor",					// kCopyCursor
	"NoDropCursor",					// kNoDropCursor
	"GrabCursor",					// kGrabCursor
	"GrabbingCursor",				// kGrabbingCursor
	"ZoomInCursor",					// kZoomInCursor
	"ZoomOutCursor"					// kZoomOutCursor
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Theme Fonts
//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr Theme::fontNames[kNumFonts] =
{
	"MenuFont"	// kMenuFont
};

//************************************************************************************************
// ThemeStatics
//************************************************************************************************

DEFINE_SINGLETON_CLASS (ThemeStatics, Object)
DEFINE_CLASS_UID (ThemeStatics, 0x7d5878ad, 0xc251, 0x4c2c, 0xa4, 0x3d, 0x68, 0xf2, 0x3a, 0x18, 0x36, 0xfb)
DEFINE_SINGLETON (ThemeStatics)

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ThemeStatics::getThemeMetricName (ThemeMetricID which) const
{
	return Theme::metricNames[which];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ThemeStatics::getThemeColorName (ThemeColorID which) const
{
	return Theme::colorNames[which];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ThemeStatics::getThemeFontName (ThemeFontID which) const
{
	return Theme::fontNames[which];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr CCL_API ThemeStatics::getThemeCursorName (ThemeCursorID which) const
{
	return Theme::cursorNames[which];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API ThemeStatics::getGlobalStyle () const
{
	return Theme::getGlobalStyle ();
}

//************************************************************************************************
// Theme
//************************************************************************************************

CStringPtr Theme::kGlobalStyleName = ".ThemeElements"; // starts with dot to be first in lexical sorting

VisualStyle Theme::globalStyle;
ObjectList Theme::sharedStyles;

//////////////////////////////////////////////////////////////////////////////////////////////////

const VisualStyle& Theme::getGlobalStyle ()
{
	return globalStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::resetSharedStyles ()
{
	sharedStyles.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (Theme, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme::Theme ()
{
	ColorPalette::linkColorPalette (); // force linkage

	painter = NEW ThemePainter (this);

	styles.objectCleanup (true);
	cursors.objectCleanup (true);

	static bool initDone = false;
	if(!initDone)
	{
		initDone = true;

		sharedStyles.objectCleanup (true);

		// init metrics
		for(int i = 0; i < kNumMetrics; i++)
			globalStyle.setMetric (metricNames[i], (VisualStyle::Metric)defaultMetrics[i]);

		// init colors
		for(int i = 0; i < kNumColors; i++)
			globalStyle.setColor (colorNames[i], getDefaultColor (i));
	}

	// init cursors
	for(int i = 0; i < kNumCursors; i++)
	{
		if(MouseCursor* cursor = MouseCursor::createCursor (i))
		{
			cursor->setName (cursorNames[i]);
			cursors.add (cursor);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme::~Theme ()
{
	if(painter)
		painter->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Theme::createRenderer (int which, VisualStyle* visualStyle) const
{
	return painter->createRenderer (which, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setMetric (ThemeMetricID which, int value)
{
	globalStyle.setMetric (metricNames[which], (VisualStyle::Metric)value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setColor (ThemeColorID which, Color color)
{
	globalStyle.setColor (colorNames[which], color);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setFont (ThemeFontID which, FontRef font)
{
	globalStyle.setFont (fontNames[which], font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* Theme::getStandardStyle (int which)
{
	return ThemePainter::getStandardStyle (which);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::resetStyles ()
{
	styles.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

static void updateStyleInList (Container& list, StringID styleName, VisualStyle* newStyle, bool* replaced)
{
	VisualStyle* oldStyle = nullptr;
	ForEach (list, VisualStyle, s)
		if(s->getName () == styleName)
		{
			if(s == newStyle)  // FIX ME: avoid updating the same style multiple times!
				return;

			oldStyle = s;
			break;
		}
	EndFor

	if(oldStyle)
	{
		if(replaced)
			*replaced = true;
		list.remove (oldStyle);
		oldStyle->release ();
	}

	list.add (newStyle);
	newStyle->setName (styleName);
	newStyle->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setStyle (StringID styleName, VisualStyle* newStyle, bool* replaced)
{
	ASSERT (styleName.isEmpty () == false)
	if(styleName.isEmpty ())
		return;

	if(styleName == kGlobalStyleName)
		globalStyle.merge (*newStyle);
	else if(styleName.startsWith (ThemePainter::kStandardPrefix))
	{
		// try standard styles
		MutableCString name (styleName.subString (ThemePainter::kStandardPrefixLength));
		for(int i = 0; i < ThemePainter::kNumStandardStyles; i++)
		{
			if(name == ThemePainter::standardStyleNames[i])
			{
				ThemePainter::setStandardStyle (i, newStyle, this);
				return;
			}
		}

		// keep as shared style
		updateStyleInList (sharedStyles, styleName, newStyle, replaced);
	}
	else
		updateStyleInList (styles, styleName, newStyle, replaced);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* Theme::lookupStyle (StringID styleName) const
{
	ForEach (styles, VisualStyle, style)
		if(style->getName () == styleName)
			return style;
	EndFor

	if(styleName.startsWith (ThemePainter::kStandardPrefix))
	{
		// try standard styles
		MutableCString name (styleName.subString (ThemePainter::kStandardPrefixLength));
		for(int i = 0; i < ThemePainter::kNumStandardStyles; i++)
		{
			if(name == ThemePainter::standardStyleNames[i])
				return ThemePainter::getStandardStyle (i);
		}

		// try shared styles
		ForEach (sharedStyles, VisualStyle, style)
			if(style->getName () == styleName)
				return style;
		EndFor
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setCursor (StringID name, MouseCursor* newCursor)
{
	MouseCursor* oldCursor = unknown_cast<MouseCursor> (getCursor (name));
	if(oldCursor)
	{
		cursors.remove (oldCursor);
		oldCursor->release ();
	}

	cursors.add (newCursor);
	newCursor->setName (name);
	newCursor->retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setCursor (ThemeCursorID which, MouseCursor* newCursor)
{
	setCursor (cursorNames[which], newCursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// ITheme methods
//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API Theme::getThemeID () const
{
	return CString::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API Theme::getThemeMetric (ThemeMetricID which)
{
	return (int)globalStyle.getMetric (metricNames[which]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorRef CCL_API Theme::getThemeColor (ThemeColorID which)
{ 
	return globalStyle.getColor (colorNames[which]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontRef CCL_API Theme::getThemeFont (ThemeFontID which)
{
	return globalStyle.getFont (fontNames[which]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseCursor* CCL_API Theme::getThemeCursor (ThemeCursorID which)
{
	return getCursor (cursorNames[which]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API Theme::getStyle (StringID name)
{ 
	if(!name.isEmpty ())
	{
		VisualStyle* style = lookupStyle (name);
		if(style)
			return *style;
	}
	return VisualStyle::emptyStyle; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API Theme::getResource (StringID name)
{
	// implemented in derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IGradient* CCL_API Theme::getGradient (StringID name)
{
	// implemented in derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API Theme::getImage (StringID name)
{
	// implemented in derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMouseCursor* CCL_API Theme::getCursor (StringID name)
{
	ForEach (cursors, MouseCursor, cursor)
		if(cursor->getName () == name)
			return cursor;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThemePainter& CCL_API Theme::getPainter ()
{
	ASSERT (painter != nullptr)
	return *painter;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IThemeStatics& CCL_API Theme::getStatics ()
{
	return ThemeStatics::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API Theme::createView (StringID name, IUnknown* controller, IAttributeList* arguments)
{
	// implemented in derived class!
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::setZoomFactor (float factor)
{
	CCL_NOT_IMPL ("Theme::setZoomFactor")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float Theme::getZoomFactor () const
{
	return 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Theme::getVariables (IAttributeList& list) const
{
	CCL_NOT_IMPL ("Theme::getVariables")
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// IObject methods
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Theme)
	DEFINE_METHOD_NAME ("getImage")
	DEFINE_METHOD_NAME ("getStyle")
END_METHOD_NAMES (Theme)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Theme::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getImage")
	{
		MutableCString name (msg[0].asString ());
		returnValue.takeShared (getImage (name));
		return true;
	}
	else if(msg == "getStyle")
	{
		MutableCString name (msg[0].asString ());
		IVisualStyle& style = const_cast<IVisualStyle&> (getStyle (name));
		returnValue.takeShared (&style);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ThemeRenderer
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ThemeRenderer, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemeRenderer::update (View* view, const UpdateInfo& info)
{
	Rect r; 
	view->getClientRect (r);
	view->View::updateClient (r); 
}

//************************************************************************************************
// ThemePainter
//************************************************************************************************

SharedPtr<VisualStyle> ThemePainter::standardStyles[kNumStandardStyles];
SharedPtr<IImage> ThemePainter::themeImages[ThemeElements::kNumThemeElements];

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* ThemePainter::getStandardStyle (int which)
{
	return standardStyles[which];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainter::setStandardStyle (int which, VisualStyle* visualStyle, Theme* theme)
{
	standardStyles[which] = visualStyle;

	// init images used by drawElement()...
	switch(which)
	{
	case kButtonRenderer :
		setStandardElementImage (kPushButton, theme);
		setStandardElementImage (kPushButtonOn, theme);
		break;

	case kCheckBoxRenderer :
		setStandardElementImage (kCheckBoxNormal, theme);
		setStandardElementImage (kCheckBoxChecked, theme);
		break;

	case kRadioButtonRenderer :
		setStandardElementImage (kRadioButtonNormal, theme);
		setStandardElementImage (kRadioButtonChecked, theme);
		break;

	case kTreeViewStyle :
		setStandardElementImage (kTreeViewExpandButton, theme);
		setStandardElementImage (kTreeViewExpandButtonOn, theme);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainter::resetStandardStyles ()
{
	for(int i = 0; i < kNumStandardStyles; i++)
		standardStyles[i].release ();

	for(int i = 0; i < ThemeElements::kNumThemeElements; i++)
		themeImages[i].release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ThemePainter::setStandardElementImage (ThemeElementID id, Theme* theme)
{
	themeImages[id] = theme->getImage (themeImageNames[id]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (ThemePainter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemePainter::ThemePainter (Theme* theme)
: theme (theme)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* ThemePainter::createRenderer (int which, VisualStyle* visualStyle) const
{
	ThemeRenderer* renderer = nullptr;

	if(visualStyle == nullptr)
		visualStyle = standardStyles[which];

	if(visualStyle == nullptr)
		visualStyle = &Theme::globalStyle;

	switch(which)
	{
	case kSliderRenderer :
		return NEW SliderRenderer (visualStyle);
	case kScrollBarRenderer :
		return NEW ScrollBarRenderer (visualStyle);
	case kScrollButtonRenderer :
		return NEW ScrollButtonRenderer (visualStyle);
	case kPageControlRenderer :
		return NEW PageControlRenderer (visualStyle);
	case kValueBarRenderer :
		return NEW ValueBarRenderer (visualStyle);
	case kProgressBarRenderer :
		return NEW ProgressBarRenderer (visualStyle);
	case kTextBoxRenderer :
		return NEW TextBoxRenderer (visualStyle);
	case kEditBoxRenderer :
		return NEW EditBoxRenderer (visualStyle);
	case kComboBoxRenderer :
		return NEW ComboBoxRenderer (visualStyle);
	case kSelectBoxRenderer :
		return NEW SelectBoxRenderer (visualStyle);
	case kButtonRenderer :
		return NEW ButtonRenderer (visualStyle);
	case kTabViewRenderer:
		return NEW TabViewRenderer (visualStyle);
	case kHeaderViewRenderer :
		return NEW HeaderViewRenderer (visualStyle);
	case kDividerRenderer :
		return NEW DividerRenderer (visualStyle);
	case kBackgroundRenderer :
		return NEW BackgroundRenderer (visualStyle);
	case kDialogGroupRenderer :
		return NEW DialogGroupRenderer (visualStyle);
	case kLabelRenderer :
		return NEW LabelRenderer (visualStyle);
	case kCheckBoxRenderer :
		return NEW CheckBoxRenderer (visualStyle);
	case kRadioButtonRenderer:
		return NEW RadioButtonRenderer (visualStyle);
	case kKnobRenderer:
		return NEW KnobRenderer (visualStyle);
	case kVectorPadRenderer :
		return NEW VectorPadRenderer (visualStyle);
	case kTriVectorPadRenderer:
		return NEW TriVectorPadRenderer (visualStyle);
	case kValueBoxRenderer :
		return NEW ValueBoxRenderer (visualStyle);
	case kUpDownButtonRenderer :
		return NEW UpDownButtonRenderer (visualStyle);
	case kRangeSliderRenderer :
		return NEW RangeSliderRenderer (visualStyle);
	case kScrollPickerRenderer :
		return NEW ScrollPickerRenderer (visualStyle);
	case kMenuBarRenderer :
		return NEW MenuBarRenderer (visualStyle);
	}

	CCL_NOT_IMPL ("Renderer not found!")
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemePainter::drawElement (IGraphics& graphics, RectRef rect, ThemeElementID id, ThemeElementState state)
{
	IImage* image = themeImages[id];
	if(image)
	{
		bool isOn = id == kPushButtonOn || id == kCheckBoxChecked || id == kRadioButtonChecked || id == kTreeViewExpandButtonOn;
		bool stretch = id == kPushButton || id == kPushButtonOn;
		CStringPtr stateName = isOn ? stateNamesOn[state] : stateNames[state];

		int index = image->getFrameIndex (stateName);
		if(index < 0)
			index = image->getFrameIndex (isOn ? stateNamesOn[ThemeElements::kNormal] : stateNames[ThemeElements::kNormal]);

		if(id == kTreeViewExpandButton || id == kTreeViewExpandButtonOn)
			ImageResolutionSelector::draw (graphics, image, rect, ImageResolutionSelector::kAllowZoom, index);
		else
		{
			image->setCurrentFrame (index);
			Rect src (0, 0, image->getWidth (), image->getHeight ());

			if(stretch)
				graphics.drawImage (image, src, rect);
			else
			{
				Rect dst (src);
				dst.center (rect);
				graphics.drawImage (image, src, dst);
			}
		}
		return kResultOk;
	}
	else
	{
		CCL_DEBUGGER ("Theme image not found!\n")
		return kResultFailed;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemePainter::drawBestMatchingFrame (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode, ColorRef contextColor, tbool scaleAlways) const
{
	Image* drawable = unknown_cast<Image> (image);
	ImageResolutionSelector s (drawable, rect, (scaleAlways != 0) ? ImageResolutionSelector::kAllowZoom : 0);
	
	if(drawable->getIsAdaptive () || drawable->getIsTemplate ())
		image = ModifiedImageCache::instance ().lookup (s.bestImage, contextColor);
	else
		image = s.bestImage;
	
	graphics.drawImage (image, s.srcRect, s.dstRect, mode);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ThemePainter::drawFrameCentered (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode, ColorRef contextColor) const
{
	Image* drawable = unknown_cast<Image> (image);
	if(drawable->getIsAdaptive () || drawable->getIsTemplate ())
		image = ModifiedImageCache::instance ().lookup (image, contextColor);
	
	Rect src (0, 0, image->getWidth (), image->getHeight ());
	Rect dst (src);
	dst.center (rect);
	
	graphics.drawImage (image, src, dst, mode);
	return kResultOk;
}

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (NativeThemePainter, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeThemePainter::getSystemColor (Color& color, ThemeColorID which) const
{
	CCL_NOT_IMPL ("NativeThemePainter::getSystemColor")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeThemePainter::getSystemFont (Font& font, ThemeFontID which) const
{
	CCL_NOT_IMPL ("NativeThemePainter::getSystemFont")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool NativeThemePainter::getSystemMetric (int& metric, ThemeMetricID which) const
{
	// status bar can be forced via cclgui.config for platforms without a system status bar
	if(which == ThemeElements::kSystemStatusBarHeight)
		return Configuration::Registry::instance ().getValue (metric, "GUI.Theme", "SystemStatusBarHeight");

	if(which == ThemeElements::kSystemNavigationBarHeight)
		return Configuration::Registry::instance ().getValue (metric, "GUI.Theme", "SystemNavigationBarHeight");

	if(which == ThemeElements::kSystemMarginLeft)
		return Configuration::Registry::instance ().getValue (metric, "GUI.Theme", "SystemMarginLeft");

	if(which == ThemeElements::kSystemMarginRight)
		return Configuration::Registry::instance ().getValue (metric, "GUI.Theme", "SystemMarginRight");
	
	CCL_NOT_IMPL ("NativeThemePainter::getSystemMetric")
	return false;
}
