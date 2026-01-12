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
// Filename    : ccl/app/controls/colorizedview.h
// Description : Colorized View
//
//************************************************************************************************

#ifndef _colorizedview_h
#define _colorizedview_h

#include "ccl/app/controls/usercontrol.h"

namespace CCL {

interface IColorParam;

//************************************************************************************************
// ColorManipulator
//************************************************************************************************

class ColorManipulator: public Object
{
public:
	DECLARE_CLASS (ColorManipulator, Object)

	ColorManipulator (const IVisualStyle* vs = nullptr);
	
	void adjustColor (Color& color, bool selected);
	void updateSettings (const IVisualStyle& vs);
	void updateSettings () { updateSettings (*visualStyle); }
	const IVisualStyle& getVisualStyle () { return *visualStyle; }
	
protected:
	float saturationWeight;
	float saturationWeightSelected;
	float luminanceWeight;
	float luminanceWeightSelected;
	float saturation;
	float saturationSelected;
	float brightness;
	float brightnessSelected;
	float opacity;
	float opacitySelected;
	Color transparentForeColor;
	Color transparentBackColor;

	struct ColorCache
	{
		bool lookupColor (Color& returnColor, uint32 colorKey);
		void addColor (const Color& cachedColor, uint32 colorKey);
		void removeAll () {cache.removeAll ();}
		
		struct CacheEntry
		{
			CacheEntry () : colorKey (0), cachedColor (0) {}
			CacheEntry (uint32 colorKey, const Color& cachedColor) : colorKey (colorKey), cachedColor (cachedColor) {}
			
			uint32 colorKey;
			uint32 cachedColor;
		};
		
		LinkedList<CacheEntry> cache;
	};
	
	ColorCache normalCache;
	ColorCache selectedCache;
	const IVisualStyle* visualStyle;

	void adjustColor (Color& color, float saturationReference, float brightnessReference, float saturationWeightReference, float luminanceWeightReference, float fixedOpacity);
};

//************************************************************************************************
// ColorizedView
//************************************************************************************************

class ColorizedView: public UserControl,
					 public IBackgroundView
{
public:
	DECLARE_CLASS_ABSTRACT (ColorizedView, UserControl)

	ColorizedView (IColorParam* colorParam, IParameter* selectParam = nullptr, RectRef size = Rect ());
	~ColorizedView ();
	
	template <typename T>
	static void applyConfiguration () { applyConfigurationTo (ccl_typeid<T> ()); }
	static void applyConfigurationTo (MetaClassRef typeId);

	// UserControl
	void attached (IView* parent) override;
	void removed (IView* parent) override;
	void draw (const DrawEvent& event) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IBackgroundView, UserControl)

protected:
	IParameter* selectParam;
	IColorParam* colorParam;
	
	Color color;
	Color selectedColor;
	Pen gradientBorderPen;
	IImage* mask;
	bool colorizeStyle;	
	bool colorsNeedUpdate;
	bool useGradient;
	float radius;

	Rect clipRect;

	SharedPtr<ColorManipulator> manipulator;
	ColorManipulator& getManipulator ();
	ColorManipulator* acquireManipulator ();
	void disposeManipulator ();
	
	virtual void configurationChanged ();
	virtual bool isColorizeEnabled () const;

	void enableUpdates (bool state);
	void drawBackground (IGraphics& graphics, const Rect& updateRect);

	// IBackgroundView
	tbool CCL_API canDrawControlBackground () const override;
	void CCL_API drawControlBackground (IGraphics& graphics, RectRef r, PointRef offset) override;
};

} // namespace CCL

#endif // _colorizedview_h

