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
// Filename    : ccl/app/components/colorpicker.h
// Description : Color Picker Component
//
//************************************************************************************************

#ifndef _ccl_colorpicker_h
#define _ccl_colorpicker_h

#include "ccl/app/component.h"

#include "ccl/base/singleton.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/gui/graphics/color.h"
#include "ccl/public/gui/framework/popupselectorclient.h"
#include "ccl/public/gui/framework/ipalette.h"

namespace CCL {

class PresetComponent;

//************************************************************************************************
// CustomColorPresets
//************************************************************************************************

class CustomColorPresets: public Object,
						  public Singleton<CustomColorPresets>,
						  public AbstractPresetMediator
{
public:
	DECLARE_CLASS (CustomColorPresets, Object)
	
	CustomColorPresets ();
		
	static const FileType& getFileType ();

	void initializePalette (bool loadDefault = false);
	IColorPalette* getPalette () const;

	void getUserPresetPath (Url& userPath) const;
	void restoreUserPreset ();
	void storeUserPreset ();
	bool restoreLastPreset (UrlRef presetPath); 
	
	// IPresetMediator
	IUnknown* CCL_API getPresetTarget () override;
	tbool CCL_API getPresetMetaInfo (IAttributeList& metaInfo) override;
	
	CLASS_INTERFACE (IPresetMediator, Object)
	
protected:
	bool paletteInitialized;
	String presetCategory;
	String presetClassName;
	
	AutoPtr<IStorable> paletteFile;
	AutoPtr<IColorPalette> palette;
};

//************************************************************************************************
// ColorPicker
//************************************************************************************************

class ColorPicker: public Component,
				   public PopupSelectorClient
{
public:
	DECLARE_CLASS (ColorPicker, Component)
	DECLARE_METHOD_NAMES (ColorPicker)
	
	ColorPicker (IParameter* parameter, bool applyPresetPalette = true);
	~ColorPicker ();
	
	PROPERTY_BOOL (hslDirty, HSLDirty)
	bool isInPickerMode () const;
	bool hasPresets () const;
	bool hasPresetPalette () const;
	
	bool popup (IVisualStyle* popupStyle = nullptr, bool useMousePos = false);

	// Component
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	
	// PopupSelectorClient
	IView* CCL_API createPopupView (SizeLimit& limits) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	Result CCL_API onMouseDown (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onMouseUp (const MouseEvent& event, IWindow& popupWindow) override;
	Result CCL_API onEventProcessed (const GUIEvent& event, IWindow& popupWindow, IView* view) override;
	void CCL_API onPopupClosed (Result result) override;
		
	CLASS_INTERFACE (IPopupSelectorClient, Component)
	
protected:
	IParameter* parameter;
	IColorPaletteModel* paletteModel;
	mutable IPalette* pickerPalette;
	bool deferAcceptOnMouseUp;
	bool shouldEndPreview;
	bool colorWasChangedInPickerMode;
	PresetComponent* presetComponent;
	int currentPaletteCount;

	static bool hslWheelMode;
	static constexpr int kMinColors = 15;
	static constexpr int kMaxColors = 255;
			
	class HSLColorWheel;
	class RGBSlider;
	class ColorPickerHueMouseHandler;
	class ColorPickerSLMouseHandler;

	ColorPicker ();
	void construct (IParameter* parameter = nullptr, bool applyPresetPalette = true);
	
	bool getColorFromHexString (Color& color) const; 
	void syncParametersFromColor (ColorRef color);
	void initializePopup ();
	bool addCurrentColor ();
	bool removeSelectedColor ();
	void resetColors ();
	void restorePreset ();
};

//************************************************************************************************
// ColorPickerDialog
//************************************************************************************************

class ColorPickerDialog
{
public:
	/** Run color picker as modal dialog (desktop platforms only). */
	bool run (Color& color);
};

} // namespace CCL

#endif // _ccl_colorpicker_h
