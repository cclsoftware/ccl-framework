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
// Filename    : ccl/gui/popup/palettepopup.cpp
// Description : Palette Popup Selector
//
//************************************************************************************************

#include "ccl/base/message.h"

#include "ccl/gui/popup/palettepopup.h"

#include "ccl/gui/itemviews/listview.h"

#include "ccl/gui/theme/palette.h"

#include "ccl/public/gui/iparameter.h"


using namespace CCL;

//************************************************************************************************
// PaletteModel
//************************************************************************************************

DEFINE_CLASS (PaletteModel, Object)
DEFINE_CLASS_UID (PaletteModel, 0xF6951DE4, 0x4EAB, 0x4854, 0xB5, 0x47, 0x5B, 0x34, 0x1D, 0x5A, 0x82, 0x9B)

//////////////////////////////////////////////////////////////////////////////////////////////////

PaletteModel::PaletteModel (IPalette* palette, IParameter* param, IParamPreviewHandler* previewHandler)
: columns (1),
  cellW (34),
  cellH (34),
  param (param),
  previewHandler (previewHandler),
  initialFocusIndex (-1),
  palette (nullptr)
{
	setPalette (palette);

	if(palette)
		palette->getDimensions (columns, cellW, cellH);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PaletteModel::~PaletteModel ()
{
	setPalette (nullptr);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

IPalette* PaletteModel::getPalette () const
{
	return palette; 
}
 
//////////////////////////////////////////////////////////////////////////////////////////////////

void PaletteModel::setPalette (IPalette* _palette)
{
	share_and_observe_unknown (this, palette, _palette);
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteModel::initModel (IPalette* palette, IParameter* param, IParamPreviewHandler* previewHandler)
{
	setParameter (param);
	setPreviewHandler (previewHandler);
	setPalette (palette);

	if(palette)
		palette->getDimensions (columns, cellW, cellH);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PaletteModel::countFlatItems () 
{ 
	return palette ? palette->getCount () : 0; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IImage* CCL_API PaletteModel::getItemIcon (ItemIndexRef index) 
{ 
	const IVisualStyle* style = nullptr;
	if(View* view = unknown_cast<View> (getItemView ()))
		style = &view->getVisualStyle ();
	ASSERT (style != nullptr)
	if(style == nullptr)
		style = &VisualStyle::emptyStyle;

	if(palette)
		icon = palette->createIcon (index.getIndex (), cellW - 2, cellH - 2, *style);
	return icon; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PaletteModel::drawIconOverlay (ItemIndexRef index, const DrawInfo& info)
{
	ItemView* itemView = unknown_cast<ItemView> (getItemView ());
	ItemIndex focusItem;
	bool isFocusItem = (itemView && itemView->getFocusItem (focusItem) && focusItem == index);
	if(isFocusItem)
		drawFocusOverlay (info.rect, info.graphics, itemView->getVisualStyle ());
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PaletteModel::drawFocusOverlay (RectRef rect, IGraphics& graphics, const IVisualStyle& style)
{
	// use fallback color in case no dedicated "focuscolor" is set
	ColorRef color = style.getColor ("focuscolor", style.getColor ("selectionColor", Colors::kBlack));
	
	Rect cellRect (rect);
	cellRect.contract (style.getMetric ("cellmargin", 0));
	Coord radius = style.getMetric ("cellradius", 0);
	if(radius > 0)
		graphics.drawRoundRect (cellRect, radius, radius, Pen (color));
	else
		graphics.drawRect (cellRect, Pen (color));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PaletteModel::getItemTooltip (String& tooltip, ItemIndexRef index, int column)
{
	if(palette)
		if(palette->getTitle (tooltip, index.getIndex ()))
			return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteModel::viewAttached (IItemView* itemView)
{
	if(itemView && initialFocusIndex >= 0)
		itemView->setFocusItem (initialFocusIndex);

	ItemViewObserver<AbstractItemModel>::viewAttached (itemView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PaletteModel::onItemFocused (ItemIndexRef item) 
{
	triggerPreviewHandler (item);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteModel::setFocusIndex (int index)
{
	IItemView* itemView = getItemView ();
	if(itemView)
		itemView->setFocusItem (index);
	else
		initialFocusIndex = index;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PaletteModel::getFocusIndex () const
{
	IItemView* itemView = getItemView ();
	if(itemView)
	{
		ItemIndex focusItem;
		if(itemView->getFocusItem (focusItem))
			return focusItem.getIndex ();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemView* CCL_API PaletteModel::getItemView () const
{
	return ItemViewObserver::getItemView ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, getPalette ()))
		{
			if(UnknownPtr<IView> itemView = getItemView ())
			{
				Rect rect;
				itemView->getVisibleClient (rect);
				itemView->invalidate (rect);
			}
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PaletteModel::finishPreview ()
{
	if(previewHandler)
	{
		previewEvent.type = ParamPreviewEvent::kCancel;
		//previewEvent.value = palette->getAt (getFocusIndex ());
		previewHandler->paramPreview (param, previewEvent);
		previewEvent.handlerData.clear ();
	}
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

void PaletteModel::triggerPreviewHandler (ItemIndexRef item)
{
	if(previewHandler)
	{
		int index = item.getIndex ();
		if(index >= 0 && index < palette->getCount ())
		{
			previewEvent.type = ParamPreviewEvent::kChange;
			previewEvent.value = palette->getAt (index);
			previewHandler->paramPreview (param, previewEvent);
		}
	}
}

//************************************************************************************************
// ColorPaletteModel
//************************************************************************************************

DEFINE_CLASS (ColorPaletteModel, PaletteModel)
DEFINE_CLASS_UID (ColorPaletteModel, 0x60EDF04B, 0x5A5B, 0x433D, 0x90, 0xCF, 0x3B, 0x64, 0x8D, 0x07, 0x4E, 0x46)

//////////////////////////////////////////////////////////////////////////////////////////////////

ColorPaletteModel::ColorPaletteModel (IPalette* palette, IParameter* param, IParamPreviewHandler* previewHandler)
: PaletteModel (palette, param, previewHandler)
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ColorPaletteModel::onItemFocused (ItemIndexRef item) 
{
	tbool result = SuperClass::onItemFocused (item);
	
	signal (Message (kFocusColorChanged));					
	
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPaletteModel::addColor (ColorRef color, int index)
{
	int insertionIndex = (index == -1) ? getColorPalette ()->getCount () : index;
	getColorPalette ()->setColors (&color, 1, insertionIndex);
	setFocusIndex (insertionIndex);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPaletteModel::removeColor (int index)
{
	int removeIndex = (index == -1) ? getFocusIndex () : index;
	getColorPalette ()->removeColors (removeIndex);
	removeIndex = ccl_min (removeIndex, getColorPalette ()->getCount () - 1);
	setFocusIndex (removeIndex);
	
	// explicitly invalidate itemview when focusIndex was the same
	if(UnknownPtr<IView> itemView = getItemView ())
	{
		Rect rect;
		itemView->getVisibleClient (rect);
		itemView->invalidate (rect);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPaletteModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		if(isEqualUnknown (subject, getPalette ()))
		{
			if(UnknownPtr<IView> itemView = getItemView ())
			{
				// color palettes autoSize vertically
				itemView->autoSize (false, true);
	
				Rect rect;
				itemView->getVisibleClient (rect);
				itemView->invalidate (rect);
			}
		}
	}
	else	
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Color CCL_API ColorPaletteModel::getFocusColor () const
{
	return getColorPalette ()->getColorAt (getFocusIndex ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ColorPaletteModel::setFocusColor (ColorRef color)
{
	getColorPalette ()->setColors (&color, 1, getFocusIndex ());
	
	if(UnknownPtr<IView> itemView = getItemView ())
	{
		Rect rect;
		itemView->getVisibleClient (rect);
		itemView->invalidate (rect);
	
		ItemIndex focusItem;
		if(getItemView ()->getFocusItem (focusItem))
			triggerPreviewHandler (focusItem);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IColorPalette* ColorPaletteModel::getColorPalette () const
{
	return UnknownPtr<IColorPalette> (getPalette ());
}

//************************************************************************************************
// PalettePopup
//************************************************************************************************

PalettePopup::PalettePopup (IParameter* param)
{
	ASSERT (param != nullptr)

	// resolve to original for preview to work correctly
	if(IParameter* original = param->getOriginal ())
		param = original;

	listViewType = Styles::kListViewIcons;
	scrollStyle.common |= Styles::kTransparent;

	UnknownPtr<IPaletteProvider> provider (param);
	IPalette* palette = provider ? provider->getPalette () : nullptr;
	ASSERT (palette != nullptr)

	UnknownPtr<IParamPreviewHandler> previewHandler (param->getController ());
	paletteModel = NEW PaletteModel (palette, param, previewHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IItemModel* PalettePopup::getItemModel ()
{
	return paletteModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PalettePopup::setVisualStyle (VisualStyle* vs)
{
	ListViewPopup::setVisualStyle (vs);
	if(vs)
	{
		paletteModel->setCellWidth (vs->getMetric<int> ("popup.cellWidth", paletteModel->getCellWidth ()));
		paletteModel->setCellHeight (vs->getMetric<int> ("popup.cellHeight", paletteModel->getCellHeight ()));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* PalettePopup::getVisualStyle (Theme& theme)
{
	if(visualStyle)
		return visualStyle;

	return theme.getStandardStyle (ThemePainter::kPalettePopupStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PalettePopup::onItemViewCreated ()
{
	ListViewPopup::onItemViewCreated ();

	if(IPalette* palette = paletteModel->getPalette ())
	{
		Variant value (paletteModel->getParameter ()->getValue ());
		int index = palette->getIndex (value);
		paletteModel->setFocusIndex (index);

		int columns = ccl_min (paletteModel->getColumns (), palette->getCount ());
		Coord cellW = paletteModel->getCellWidth ();
		Coord cellH = paletteModel->getCellHeight ();

		Rect itemSize (0, 0, cellW, cellH);

		ListView* listView = unknown_cast<ListView> (itemView);

		const IVisualStyle& listVisualStyle = listView->getVisualStyle ();
		listView->isTooltipTrackingEnabled (listVisualStyle.getMetric ("showtooltip", true));
		
		if(listVisualStyle.getImage ("icons.focusframe") == nullptr)
			View::StyleModifier (*listView).setCustomStyle (Styles::kItemViewAppearanceNoFocusRect, true);
			
		View::StyleModifier (*listView).setCustomStyle (Styles::kItemViewBehaviorSelection, false);
		View::StyleModifier (*listView).setCustomStyle (Styles::kListViewBehaviorSwipeToFocus, true);


		ListStyle& listStyle = listView->getListStyle ();
		listStyle.setMargin (0);
		listStyle.setItemSize (Styles::kListViewIcons, itemSize.getSize ());
		
		// dataSize == itemSize or is using an optional fill factor
		float iconFillSize = listVisualStyle.getMetric ("popup.fill.icon", listVisualStyle.getMetric ("fill.icon", 0.f));
		Point dataSize (itemSize.getWidth (), itemSize.getHeight ());
		float iconResize = iconFillSize * ccl_min (itemSize.getWidth (), itemSize.getHeight ());
		float resizeRatio = (iconResize > 0.f) ? (iconResize / ccl_max (itemSize.getWidth (), itemSize.getHeight ())) : 1;
		dataSize *= resizeRatio;
		Point iconPos = itemSize.getLeftTop () + ((itemSize.getSize () - dataSize) * .5f);
		Rect dataRect (iconPos.x, iconPos.y, dataSize);
		listStyle.setDataRect (Styles::kListViewIcons, dataRect);
		
		listStyle.setTextRect (Styles::kListViewIcons, Rect ());
		listStyle.setRowHeight (cellH);
	
		ScrollView* scrollView = ScrollView::getScrollView (unknown_cast<View> (itemView));
		Coord border = listVisualStyle.getMetric ("border", 0);
		
		// resize to get the requested number of columns
		Rect size (border, border, cellW * columns + border, kMaxCoord);
		scrollView->setSizeMode (0);
		scrollView->setSize (size);
		scrollView->autoSize (false, true);
	};
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PalettePopup::onPopupClosed (Result result)
{
	ListViewPopup::onPopupClosed (result);
	paletteModel->finishPreview ();

	if(result == IPopupSelectorClient::kOkay)
		if(IPalette* palette = paletteModel->getPalette ())
		{
			int index = paletteModel->getFocusIndex ();
			Variant value (palette->getAt (index));
			IParameter* param = paletteModel->getParameter ();
			param->beginEdit ();
			param->setValue (value, false);
			param->performUpdate (); // trigger update even if color has not changed
			param->endEdit ();
		}
}
