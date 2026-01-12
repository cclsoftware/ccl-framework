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
// Filename    : ccl/gui/popup/palettepopup.h
// Description : Palette Popup Selector
//
//************************************************************************************************

#ifndef _ccl_palettepopup_h
#define _ccl_palettepopup_h

#include "ccl/gui/popup/itemviewpopup.h"

#include "ccl/public/gui/framework/ipalette.h"

#include "ccl/public/gui/iparamobserver.h"

namespace CCL {

//************************************************************************************************
// PaletteModel
/** ItemModel for a ListView that allows selecting elements from a palette. */
//************************************************************************************************

class PaletteModel: public Object,
					public ItemViewObserver<AbstractItemModel>,
					public IPaletteItemModel
{
public:
	DECLARE_CLASS (PaletteModel, Object)

	PaletteModel (IPalette* palette = nullptr, IParameter* param = nullptr, IParamPreviewHandler* previewHandler = nullptr);
	~PaletteModel ();
	 
	IPalette* getPalette () const;
	virtual void setPalette (IPalette* _palette);

	PROPERTY_POINTER (IParameter, param, Parameter)
	PROPERTY_POINTER (IParamPreviewHandler, previewHandler, PreviewHandler)

	PROPERTY_VARIABLE (int, columns, Columns)
	PROPERTY_VARIABLE (int, cellW, CellWidth)
	PROPERTY_VARIABLE (int, cellH, CellHeight)

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	// IPaletteItemModel
	void CCL_API initModel (IPalette* palette, IParameter* param, IParamPreviewHandler* previewHandler = nullptr) override;
	int CCL_API getFocusIndex () const override;
	void CCL_API setFocusIndex (int index) override;
	IItemView* CCL_API getItemView () const override;
	void CCL_API finishPreview () override;
	
	// IItemModel
	int CCL_API countFlatItems () override;
	IImage* CCL_API getItemIcon (ItemIndexRef index) override;
	tbool CCL_API drawIconOverlay (ItemIndexRef index, const DrawInfo& info) override;
	tbool CCL_API getItemTooltip (String& tooltip, ItemIndexRef index, int column) override;
	tbool CCL_API onItemFocused (ItemIndexRef index) override;
	void CCL_API viewAttached (IItemView* itemView) override;

	CLASS_INTERFACE2 (IItemModel, IPaletteItemModel, Object)
	
protected:
	void triggerPreviewHandler (ItemIndexRef index);
	
private:
	AutoPtr<IImage> icon;
	IPalette* palette;
	ParamPreviewEvent previewEvent;
	int initialFocusIndex;
	
	void drawFocusOverlay (RectRef rect, IGraphics& graphics, const IVisualStyle& style);
};

//************************************************************************************************
// ColorPaletteModel
/** ItemModel for a ListView that allows manipulation of its color palette. */
//************************************************************************************************

class ColorPaletteModel: public PaletteModel,
						 public IColorPaletteModel
{
public:
	ColorPaletteModel (IPalette* palette = nullptr, IParameter* param = nullptr, IParamPreviewHandler* previewHandler = nullptr);
	
	DECLARE_CLASS (ColorPaletteModel, PaletteModel)

	// PaletteModel
	tbool CCL_API onItemFocused (ItemIndexRef item) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	// IColorPaletteModel
	void CCL_API addColor (ColorRef color, int index = -1) override;
	void CCL_API removeColor (int index = -1) override;
	Color CCL_API getFocusColor () const override;
	void CCL_API setFocusColor (ColorRef color) override;
	
	CLASS_INTERFACE (IColorPaletteModel, PaletteModel)
	
private:	
	IColorPalette* getColorPalette () const;
};

//************************************************************************************************
// PalettePopup
//************************************************************************************************

class PalettePopup: public ListViewPopup
{
public:
	PalettePopup (IParameter* param);

	void setVisualStyle (VisualStyle* visualStyle) override;
	
protected:
	AutoPtr<PaletteModel> paletteModel;

	// ListViewPopup
	IItemModel* getItemModel () override;
	VisualStyle* getVisualStyle (Theme& theme) override;
	void onItemViewCreated () override;
	void CCL_API onPopupClosed (Result result) override;
};

} // namespace CCL

#endif // _ccl_palettepopup_h
