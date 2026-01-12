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
// Filename    : ccl/gui/theme/theme.h
// Description : Theme class
//
//************************************************************************************************

#ifndef _ccl_theme_h
#define _ccl_theme_h

#include "ccl/gui/theme/visualstyle.h"

#include "ccl/public/gui/framework/itheme.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectlist.h"

namespace CCL {

class MouseCursor;
class ThemePainter;
class ThemeRenderer;
class NativeGraphicsDevice;
class VisualStyleClass;

//************************************************************************************************
// ThemeStatics
//************************************************************************************************

class ThemeStatics: public Object,
					public IThemeStatics,
					public Singleton<ThemeStatics>
{
public:
	DECLARE_CLASS (ThemeStatics, Object)
	
	// IThemeStatics
	CStringPtr CCL_API getThemeMetricName (ThemeMetricID which) const override;
	CStringPtr CCL_API getThemeColorName (ThemeColorID which) const override;
	CStringPtr CCL_API getThemeFontName (ThemeFontID which) const override;
	CStringPtr CCL_API getThemeCursorName (ThemeCursorID which) const override;
	const IVisualStyle& CCL_API getGlobalStyle () const override;
	
	CLASS_INTERFACE (IThemeStatics, Object)
};

//************************************************************************************************
// Theme
//************************************************************************************************

class Theme: public Object,
			 public ITheme
{
public:
	DECLARE_CLASS (Theme, Object)
	DECLARE_METHOD_NAMES (Theme)

	Theme ();
	~Theme ();

	static const VisualStyle& getGlobalStyle ();
	static void resetSharedStyles ();

	void setMetric (ThemeMetricID which, int value); ///< delegated to global style
	void setColor (ThemeColorID which, Color color); ///< delegated to global style
	void setFont (ThemeFontID which, FontRef font); ///< delegated to global style

	void resetStyles ();
	void setStyle (StringID styleName, VisualStyle* newStyle, bool* replaced = nullptr); ///< style is shared by theme!
	void setCursor (StringID name, MouseCursor* newCursor);	 ///< cursor is shared by theme!
	void setCursor (ThemeCursorID which, MouseCursor* newCursor); ///< cursor is shared by theme!

	VisualStyle* lookupStyle (StringID styleName) const;

	VisualStyle* getStandardStyle (int which); ///< delegated to painter
	virtual ThemeRenderer* createRenderer (int which, VisualStyle* visualStyle) const; ///< create renderer via painter

	virtual void getVariables (IAttributeList& list) const;

	class ZoomFactorScope;
	virtual void setZoomFactor (float factor);
	virtual float getZoomFactor () const;

	// ITheme
	StringID CCL_API getThemeID () const override;
	int CCL_API getThemeMetric (ThemeMetricID which) override;
	ColorRef CCL_API getThemeColor (ThemeColorID which) override;
	FontRef CCL_API getThemeFont (ThemeFontID which) override;
	IMouseCursor* CCL_API getThemeCursor (ThemeCursorID which) override;
	const IVisualStyle& CCL_API getStyle (StringID name) override;
	IUnknown* CCL_API getResource (StringID name) override;
	IGradient* CCL_API getGradient (StringID name) override;
	IImage* CCL_API getImage (StringID name) override;
	IMouseCursor* CCL_API getCursor (StringID name) override;
	IThemePainter& CCL_API getPainter () override;
	IThemeStatics& CCL_API getStatics () override;
	IView* CCL_API createView (StringID name, IUnknown* controller, IAttributeList* arguments = nullptr) override;
	
	CLASS_INTERFACE (ITheme, Object)

protected:
	friend class ThemeStatics;
	friend class ThemePainter;
	friend class VisualStyleClass;

	ThemePainter* painter; ///< painting methods
	ObjectList styles; ///< list of named styles
	ObjectList cursors; ///< list of mouse cursors

	static CStringPtr kGlobalStyleName;
	static VisualStyle globalStyle;	///< global theme colors and metrics
	static ObjectList sharedStyles;

	static CStringPtr metricNames[ThemeElements::kNumMetrics];
	static int defaultMetrics[ThemeElements::kNumMetrics];
	static CStringPtr colorNames[ThemeElements::kNumColors];
	static Color getDefaultColor (ThemeColorID which);
	static CStringPtr fontNames[ThemeElements::kNumFonts];
	static CStringPtr cursorNames[ThemeElements::kNumCursors];

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// Theme::ZoomFactorScope
//************************************************************************************************

class Theme::ZoomFactorScope
{
public:
	ZoomFactorScope (Theme& theme, float zoomFactor)
	: theme (theme),
	  oldZoomFactor (theme.getZoomFactor ())
	{ theme.setZoomFactor (zoomFactor);	}

	~ZoomFactorScope ()
	{ theme.setZoomFactor (oldZoomFactor); }

private:
	Theme& theme;
	float oldZoomFactor;
};

//************************************************************************************************
// ThemePainter
//************************************************************************************************

class ThemePainter: public Object,
					public IThemePainter
{
public:
	DECLARE_CLASS (ThemePainter, Object)

	ThemePainter (Theme* theme = nullptr);

	enum RendererAndStandardStyles
	{
		kSliderRenderer,
		kScrollBarRenderer,
		kScrollButtonRenderer,
		kPageControlRenderer,
		kValueBarRenderer,
		kProgressBarRenderer,
		kTextBoxRenderer,
		kEditBoxRenderer,
		kComboBoxRenderer,
		kSelectBoxRenderer,
		kButtonRenderer,
		kTabViewRenderer,
		kHeaderViewRenderer,
		kDividerRenderer,
		kBackgroundRenderer,
		kDialogGroupRenderer,
		kLabelRenderer,
		kCheckBoxRenderer,
		kRadioButtonRenderer,
		kKnobRenderer,
		kVectorPadRenderer,
		kTriVectorPadRenderer,
		kValueBoxRenderer,
		kUpDownButtonRenderer,
		kRangeSliderRenderer,
		kScrollPickerRenderer,
		kMenuBarRenderer,
		
		// the following are styles only with no renderer
		kListViewStyle,
		kTreeViewStyle,
		kPopupMenuStyle,
		kPopupMenuLargeStyle,
		kPalettePopupStyle,
		kMenuControlStyle,
		kContextMenuStyle,
		kPerspectiveSwitcherStyle,
		kSegmentBoxStyle,

		kHeading1Style,
		kHeading2Style,
		kHeading3Style,
		kHeading4Style,
		kHeading5Style,
		kHeading6Style,
		kLastHeadingStyle = kHeading6Style,
		
		kNumStandardStyles
	};

	static CStringPtr kStandardPrefix;
	static const int kStandardPrefixLength;

	ThemeRenderer* createRenderer (int which, VisualStyle* visualStyle) const;

	// IThemePainter
	tresult CCL_API drawElement (IGraphics& graphics, RectRef rect, ThemeElementID id, ThemeElementState state) override;
	tresult CCL_API drawBestMatchingFrame (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode = nullptr, ColorRef contextColor = 0, tbool scaleAlways = false) const override;
	tresult CCL_API drawFrameCentered (IGraphics& graphics, IImage* image, RectRef rect, const ImageMode* mode = nullptr, ColorRef contextColor = 0) const override;

	CLASS_INTERFACE (IThemePainter, Object)

	static void setStandardStyle (int which, VisualStyle* visualStyle, Theme* theme);
	static VisualStyle* getStandardStyle (int which);
	static void resetStandardStyles ();

protected:
	friend class Theme;
	friend class VisualStyleClass;

	Theme* theme;

	static CStringPtr themeImageNames[ThemeElements::kNumThemeElements];
	static CStringPtr uniqueImageNames[];
	static const int kUniqueImageCount;
	static SharedPtr<IImage> themeImages[ThemeElements::kNumThemeElements];

	static CStringPtr standardStyleNames[kNumStandardStyles];
	static SharedPtr<VisualStyle> standardStyles[kNumStandardStyles];

	static CStringPtr stateNames[ThemeElements::kNumElementStates];
	static CStringPtr stateNamesOn[ThemeElements::kNumElementStates];

	static void setStandardElementImage (ThemeElementID id, Theme* theme);
};

//************************************************************************************************
// NativeThemePainter
//************************************************************************************************

class NativeThemePainter: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (NativeThemePainter, Object)

	static NativeThemePainter& instance ();	///< external!

	virtual bool getSystemColor (Color& color, ThemeColorID which) const;
	virtual bool getSystemFont (Font& font, ThemeFontID which) const;
	virtual bool getSystemMetric (int& metric, ThemeMetricID which) const;
};

//************************************************************************************************
// FrameworkTheme
/** Access to built-in theme. */
//************************************************************************************************

class FrameworkTheme
{
public:
	static Theme& instance ();
};

} // namespace CCL

#endif // _ccl_theme_h
