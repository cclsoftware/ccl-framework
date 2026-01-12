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
// Filename    : ccl/app/controls/itemviewmodel.cpp
// Description : Item View Model
//
//************************************************************************************************

#include "ccl/app/controls/itemviewmodel.h"
#include "ccl/app/utilities/boxedguitypes.h"
#include "ccl/app/params.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/public/gui/iparamobserver.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/ipopupselector.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/framework/usercontrolbase.h"
#include "ccl/public/gui/framework/abstracttouchhandler.h"
#include "ccl/public/system/isignalhandler.h"

#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ItemModelPainter
//************************************************************************************************

static inline Rect drawIconHelper (ItemModelPainter::DrawInfoRef info, IImage* icon, bool enabled, bool fitImage, int margin)
{
	if(icon)
	{
		ITheme& theme = ViewBox (info.view).getTheme ();
		IThemePainter& painter = theme.getPainter ();
		ImageMode mode (0.4f);
	
		if(fitImage)
		{
			Rect iconRect (info.rect);
			if(margin != 0)
				iconRect.contract (margin);
			painter.drawBestMatchingFrame (info.graphics, icon, iconRect, enabled ? nullptr : &mode, info.style.adaptiveColor);
			return iconRect;
		}
		else
		{
			IImage::Selector (icon, ThemeNames::kNormal);
			painter.drawFrameCentered (info.graphics, icon, info.rect, enabled ? nullptr : &mode, info.style.adaptiveColor);
		
			Rect iconRect (0, 0, icon->getWidth (), icon->getHeight ());
			return iconRect.center (info.rect);
		}
	}
	return Rect ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawIcon (DrawInfoRef info, IImage* icon, bool enabled, bool fitImage, int margin)
{
	drawIconHelper (info, icon, enabled, fitImage, margin);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawIconWithOverlay (DrawInfoRef info, IImage* icon, IImage* overlay, bool enabled, bool fitImage, int margin)
{
	Rect iconRect = drawIconHelper (info, icon, enabled, fitImage, margin);

	if(overlay)
		drawOverlay (info, iconRect, overlay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawOverlay (DrawInfoRef info, RectRef iconRect, IImage* overlay)
{
	Rect src (0, 0, overlay->getWidth (), overlay->getHeight ());
	Rect overlayRect (src);
	overlayRect.moveTo (Point (iconRect.right - src.getWidth (), iconRect.bottom - src.getHeight ()));
	info.graphics.drawImage (overlay, src, overlayRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawButtonImage (DrawInfoRef info, IImage* image, bool pressed, bool enabled)
{
	ITheme& theme = ViewBox (info.view).getTheme ();
	IThemePainter& painter = theme.getPainter ();
	
	ImageMode mode (0.4f);
	
	int frameIndex = image->getFrameIndex (pressed ? ThemeNames::kPressed : ThemeNames::kNormal);
	if(frameIndex < 0 && pressed)
		frameIndex = image->getFrameIndex (ThemeNames::kNormalOn);
	
	ASSERT (frameIndex >= 0)
	image->setCurrentFrame (frameIndex);	
	
	painter.drawFrameCentered (info.graphics, image, info.rect, enabled ? nullptr : &mode, info.style.adaptiveColor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::calcCheckBoxRect (Rect& checkRect, DrawInfoRef info, AlignmentRef alignment)
{
	ITheme& theme = ViewBox (info.view).getTheme ();
	Coord size = theme.getThemeMetric (ThemeElements::kCheckBoxSize);
	checkRect (0, 0, size, size);
	
	int hAlign = alignment.getAlignH ();	
	if(hAlign & Alignment::kLeft)
	{
		checkRect.offset (info.rect.left, 0);
		checkRect.centerV (info.rect);
	}
	else if(hAlign & Alignment::kRight)
	{
		checkRect.offset (info.rect.right - size, 0);
		checkRect.centerV (info.rect);
	}
	else
		checkRect.center (info.rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawCheckBox (DrawInfoRef info, bool checked, bool enabled, AlignmentRef alignment)
{
	ITheme& theme = ViewBox (info.view).getTheme ();
	IThemePainter& painter = theme.getPainter ();

	Rect checkRect;
	calcCheckBoxRect (checkRect, info, alignment);

	ThemeElementID element = checked ? ThemeElements::kCheckBoxChecked : ThemeElements::kCheckBoxNormal;
	ThemeElementState state = enabled ? ThemeElements::kNormal : ThemeElements::kDisabled;
	painter.drawElement (info.graphics, checkRect, element, state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::calcButtonRect (Rect& buttonRect, DrawInfoRef info, StringRef title, Coord verticalMargin)
{
	ITheme& theme = ViewBox (info.view).getTheme ();
	Coord spacing = theme.getThemeMetric (ThemeElements::kLayoutSpacing);
	Coord buttonH = theme.getThemeMetric (ThemeElements::kButtonHeight);
	ccl_upper_limit (buttonH, info.rect.getHeight () - (2 * verticalMargin));
	Coord buttonW = theme.getThemeMetric (ThemeElements::kButtonWidth);

	Coord width = Font::getStringWidth (title, info.style.font);
	width += 2 * spacing;
	if(width < buttonW)
		width = buttonW;
	if(width > info.rect.getWidth ())
		width = info.rect.getWidth ();

	buttonRect (0, 0, width, buttonH);
	buttonRect.center (info.rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawButton (DrawInfoRef info, StringRef title, bool enabled, Coord verticalMargin)
{
	ITheme& theme = ViewBox (info.view).getTheme ();
	IThemePainter& painter = theme.getPainter ();

	Rect buttonRect;
	calcButtonRect (buttonRect, info, title, verticalMargin);

	ThemeElementState state = enabled ? ThemeElements::kNormal : ThemeElements::kDisabled;
	painter.drawElement (info.graphics, buttonRect, ThemeElements::kPushButton, state);
	
	if(!title.isEmpty ())
	{
		SolidBrush textBrush (theme.getThemeColor (ThemeElements::kPushButtonTextColor));
		info.graphics.drawString (buttonRect, title, info.style.font, textBrush, Alignment::kCenter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawSelectBoxArrow (DrawInfoRef info, bool enabled, int margin)
{
	const Coord arrowWidth = 8;
	const Coord arrowHeight = 5;

	SolidBrush textBrush = getTextBrush (info, enabled);
	SolidBrush arrowBrush (textBrush.getColor ().scaleAlpha (0.7));

	Point trianglePoints[3] = {{0,0}, {arrowWidth,0}, {arrowWidth/2,arrowHeight}};	
	Coord arrowLeft = info.rect.right - arrowWidth - margin;
	Coord arrowTop = info.rect.top + ((info.rect.getHeight () - arrowHeight + 1) / 2);
	for(Point& tp : trianglePoints)
		tp.offset (arrowLeft, arrowTop);
	info.graphics.fillTriangle (trianglePoints, arrowBrush);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SolidBrush ItemModelPainter::getTextBrush (DrawInfoRef info, bool enabled)
{
	bool selected = (info.state & IItemModel::DrawInfo::kItemSelectedState) ? true : false;
	if(!enabled && selected)
	{
		SolidBrush brush (info.style.textBrush);
		brush.setColor (brush.getColor ().setAlphaF (0.5f));
		return brush;
	}
	return info.style.getTextBrush (enabled);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawTitle (DrawInfoRef info, StringRef title, bool enabled, int fontStyle,
								  AlignmentRef alignment, int trimMode)
{
	Font font (info.style.font);
	font.setStyle (font.getStyle () | fontStyle);

	SolidBrush textBrush (getTextBrush (info, enabled));

	String title2 (title);
	Font::collapseString (title2, info.rect.getWidth (), info.style.font, trimMode);
	info.graphics.drawString (info.rect, title2, font, textBrush, alignment);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawText (DrawInfoRef info, StringRef text, AlignmentRef alignment, bool enabled, int fontStyle, Coord margin)
{
	Font font (info.style.font);
	font.setStyle (font.getStyle () | fontStyle);

	SolidBrush textBrush (getTextBrush (info, enabled));

	TextFormat format (alignment);
	Rect rect (info.rect);
	if(margin != 0)
	{
		if(alignment.align & Alignment::kLeft)
			rect.left += margin;
		else if(alignment.align & Alignment::kRight)
			rect.right -= margin;
	}

	info.graphics.drawText (rect, text, font, textBrush, format);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::calcTitleRects (Rect& titleRect, Rect& subTitleRect, DrawInfoRef info, Coord spacing)
{
	Rect charRect;
	Font::measureString (charRect, CCLSTR ("A"), info.style.font);
	Coord lineHeight = charRect.getHeight () + spacing;

	Rect fullRect (info.rect);
	fullRect.setHeight (2 * lineHeight);
	fullRect.centerV (info.rect);

	titleRect = fullRect;
	titleRect.setHeight (lineHeight);

	subTitleRect = titleRect;
	subTitleRect.offset (0, lineHeight);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawTitleWithSubtitle (DrawInfoRef info, StringRef title, StringRef subTitle, bool enabled, 
											  int fontStyle, Coord lineSpacing, int trimMode)
{
	Font font (info.style.font);
	font.setStyle (font.getStyle () | fontStyle);
	font.isBold (true);

	SolidBrush textBrush (getTextBrush (info, enabled));

	Rect titleRect, subTitleRect;
	calcTitleRects (titleRect, subTitleRect, info, lineSpacing);

	if(!title.isEmpty ())
	{
		String title2 (title);
		Font::collapseString (title2, titleRect.getWidth (), font, trimMode);
		info.graphics.drawString (titleRect, title2, font, textBrush, Alignment::kLeft|Alignment::kVCenter);
	}

	font.isBold (false);

	if(!subTitle.isEmpty ())
	{
		String subTitle2 (subTitle);
		Font::collapseString (subTitle2, subTitleRect.getWidth (), font, trimMode);
		info.graphics.drawString (subTitleRect, subTitle2, font, textBrush, Alignment::kLeft|Alignment::kVCenter);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawVerticalBar (IGraphics& graphics, RectRef _rect, float value, Color backColor, Color hiliteColor, Coord margin)
{
	Rect rect (_rect);
	rect.contract (margin);

	Coord h = (Coord)(value * rect.getHeight ());

	Rect hiliteRect (rect);
	hiliteRect.top = hiliteRect.bottom - h;
	
	Rect backRect (rect);
	backRect.bottom = hiliteRect.top;

	if(!backRect.isEmpty ())
		graphics.fillRect (backRect, SolidBrush (backColor));

	if(!hiliteRect.isEmpty ())
		graphics.fillRect (hiliteRect, SolidBrush (hiliteColor));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModelPainter::drawHorizontalBar (IGraphics& graphics, RectRef _rect, float value, Color backColor, Color hiliteColor, Coord margin)
{
	Rect rect (_rect);
	rect.contract (margin);

	Coord w = (Coord)(value * rect.getWidth ());

	Rect hiliteRect (rect);
	hiliteRect.right = hiliteRect.left + w;
	
	Rect backRect (rect);
	backRect.left = hiliteRect.right;

	if(!backRect.isEmpty ())
		graphics.fillRect (backRect, SolidBrush (backColor));

	if(!hiliteRect.isEmpty ())
		graphics.fillRect (hiliteRect, SolidBrush (hiliteColor));
}

//************************************************************************************************
// ItemModel::EditControlOperation
//************************************************************************************************

class ItemModel::EditControlOperation: public AsyncOperation,
									   public CCL::IParamObserver					  
{
public:
	EditControlOperation (IParameter* param, IView* _view)
	: param (param),
	  view (UnknownPtr<ISubject> (_view))
	{
		retain (); // stay alive until view destroyed

		if(view)
			view->addObserver (this);

		param->connect (this, 0);
		setState (kStarted);
	}

	// IParamObserver
	tbool CCL_API paramChanged (CCL::IParameter* param) override
	{
		if(param == this->param)
		{
			if(getState () == kCompleted)
				setState (kStarted);
			setResult (param->getValue ());
			setState (kCompleted);
		}
		return true;
	}

	void CCL_API notify (ISubject* subject, MessageRef msg) override
	{
		if(subject == view && msg == kDestroyed)
		{
			view->removeObserver (this);
			view = nullptr;

			if(getState () != kCompleted)
				setState (kCanceled);

			release (); 
		}
	}

	void CCL_API paramEdit (IParameter* param, tbool begin) override {}

	CLASS_INTERFACE (IParamObserver, AsyncOperation)

private:
	SharedPtr<IParameter> param;
	ISubject* view;
};

//************************************************************************************************
// ItemModel::SwipeItemsMouseHandler
//************************************************************************************************

class ItemModel::SwipeItemsMouseHandler: public Object,
									     public AbstractMouseHandler,
										 public AbstractItemSelection
{
public:
	SwipeItemsMouseHandler (IItemView* itemView, ItemVisitor* visitor, SwipeMethod method)
	: itemView (itemView), visitor (visitor), method (method)
	{}

	bool onMove (int moveFlags) override
	{
		if(itemView)
		{
			if(method == SwipeMethod::kMultiple)
			{
				Rect rect (first.where, current.where);
				rect.normalize ();
				itemView->findItems (rect, *this); ///< calls select ;-)
			}
			else if(method == SwipeMethod::kSingle)
			{
				ItemIndex index;
				if(itemView->findItem (index, current.where) && index != recent)
				{
					visitor->visit (index);		
					recent = index;
				}
			}
		}
		return true;
	}

	void onRelease (bool canceled) override
	{
		UnknownPtr<IObserver> observer (itemView->getModel ());
		if(observer)
			observer->notify (nullptr, Message (kSwipeEditDone));
	}

	// AbstractItemSelection
	void CCL_API select (ItemIndexRef index) override
	{
		visitor->visit (index);
	}

	CLASS_INTERFACE (IMouseHandler, Object)

private:
	IItemView* itemView;
	AutoPtr<ItemVisitor> visitor;
	SwipeMethod method;
	ItemIndex recent;
};

//************************************************************************************************
// ItemModel::TouchMouseHandler
//************************************************************************************************

class ItemModel::TouchMouseHandler: public Object,
									public AbstractTouchMouseHandler
{
public:
	TouchMouseHandler (IMouseHandler* mouseHandler, IView* view)
	: AbstractTouchMouseHandler (mouseHandler, view)
	{}

	CLASS_INTERFACE (ITouchHandler, Object)
};

//************************************************************************************************
// ItemModel::BoxedEditInfo
//************************************************************************************************

DEFINE_CLASS (ItemModel::BoxedEditInfo, Object)

BEGIN_PROPERTY_NAMES (ItemModel::BoxedEditInfo)
	DEFINE_PROPERTY_NAME ("view")
	DEFINE_PROPERTY_NAME ("rect")
	DEFINE_PROPERTY_NAME ("editEvent")
END_PROPERTY_NAMES (ItemModel::BoxedEditInfo)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemModel::BoxedEditInfo::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "view")
	{
		var = view;
		return true;
	}
	else if(propertyId == "rect")
	{
		CCL_BOX (Boxed::Rect, boxedRect, rect)
		var.takeShared (ccl_as_unknown (boxedRect));
		return true;
	}
	else if(propertyId == "editEvent")
	{
		if(auto mouseEvent = editEvent.as<MouseEvent> ())
		{
			CCL_BOX (Boxed::MouseEvent, boxedEvent, *mouseEvent)
			var.takeShared (ccl_as_unknown (boxedEvent));
		}
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//************************************************************************************************
// ItemModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (ItemModel, Object)
DEFINE_STRINGID_MEMBER_ (ItemModel, kSwipeEditDone, "swipeEditDone")

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ItemModel::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IItemView::kViewAttached || msg == IItemView::kViewFocused)
	{
		UnknownPtr<IItemView> itemView (subject);
		if(itemView)
			makeFirst (itemView);
	}
	else if(msg == IItemView::kViewRemoved)
	{
		UnknownPtr<IItemView> itemView (subject);
		if(itemView)
			makeLast (itemView);	
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModel::invalidate ()
{
	for(IItemView* itemView : getItemViews ())
		ViewBox (itemView).invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModel::invalidateItem (ItemIndexRef index)
{
	for(IItemView* itemView : getItemViews ())
		itemView->invalidateItem (index);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ItemModel::updateColumns ()
{
	for(auto itemView : getItemViews ())
	{
		UnknownPtr<IObserver> observer (itemView);
		if(observer)
			observer->notify (nullptr, Message (kUpdateColumns));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemModel::doPopup (IParameter* param, const EditInfo& info, IVisualStyle* visualStyle)
{
	AutoPtr<IPopupSelector> selector = ccl_new<IPopupSelector> (CCL::ClassID::PopupSelector);
	ASSERT (selector != nullptr)
	if(selector)
	{
		PopupSizeInfo sizeInfo (info.rect.getLeftBottom (), info.view);
		selector->setTheme (&ViewBox (info.view).getTheme ());
		selector->setVisualStyle (visualStyle);
		return selector->popup (param, sizeInfo, MenuPresentation::kTree) != 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemModel::doPopupSlider (IParameter* param, const EditInfo& info, PointRef position, bool horizontal, StringID popupSliderDecorForm)
{
	AutoPtr<IPopupSelector> selector = ccl_new<IPopupSelector> (CCL::ClassID::PopupSelector);
	ASSERT (selector != nullptr)
	if(selector)
	{
		PopupSizeInfo sizeInfo (position, info.view);
		if(horizontal)
			sizeInfo.flags |= PopupSizeInfo::kVCenterRel;
		else
			sizeInfo.flags |= PopupSizeInfo::kHCenterRel;
		selector->setTheme (&ViewBox (info.view).getTheme ());
		selector->setDecor (popupSliderDecorForm, nullptr);
		return selector->popupSlider (param, sizeInfo) != 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemModel::doPopupColorPalette (Color& color, IColorPalette* palette, const EditInfo& info)
{
	ASSERT (palette)
	if(palette == nullptr)
		return false;

	AutoPtr<ColorParam> colorParam = NEW ColorParam;
	colorParam->setPalette (palette);
	colorParam->setColor (color);
					
	if(doPopup (colorParam, info))
	{		
		colorParam->getColor (color);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemModel::setEditControl (ViewBox& editControl, const EditInfo& info)
{
	UnknownPtr<IItemView> itemView (info.view);
	if(itemView)
	{
		auto mouseEvent = info.editEvent.as<MouseEvent> ();
		if(editControl.getClassID () == ClassID::ValueBox && mouseEvent && info.view->detectDrag (*mouseEvent))
		{
			itemView->setEditControl (editControl, true);
	
			MouseEvent e2 (*mouseEvent);
			info.view->clientToWindow (e2.where);
			editControl.windowToClient (e2.where);
			editControl.getChildren ().delegateEvent (e2);
		}	
		else
			itemView->setEditControl (editControl, false);

		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IVisualStyle* ItemModel::createEditStyle ()
{
	AutoPtr<IVisualStyle> editStyle;
	const IVisualStyle& itemViewStyle (ViewBox (getItemView ()).getVisualStyle ());

	editStyle = ccl_new<IVisualStyle> (CCL::ClassID::VisualStyle);
	editStyle->setFont (StyleID::kTextFont, itemViewStyle.getTextFont ());
	editStyle->setOptions (StyleID::kTextAlign, itemViewStyle.getTextAlignment ().align);
	editStyle->setColor (StyleID::kTextColor, itemViewStyle.getTextColor ());
	editStyle->setColor (StyleID::kBackColor, itemViewStyle.getBackColor ());
	
	editStyle->setFont ("titlefont", itemViewStyle.getFont ("titlefont", itemViewStyle.getTextFont ()));
	editStyle->setColor ("titlecolor", itemViewStyle.getColor ("titlecolor", itemViewStyle.getTextColor ()));
	editStyle->setColor ("titlecolor.bright", itemViewStyle.getColor ("titlecolor.bright", Colors::kWhite));
	editStyle->setMetric ("titlecolor.threshold", itemViewStyle.getMetric ("titlecolor.threshold", 0.35f));
	
	return editStyle.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::editString (StringRef initialValue, RectRef rect, const EditInfo& info, IVisualStyle* visualStyle)
{
	return editString (initialValue, rect, &info, nullptr, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::editString (StringRef initialValue, RectRef rect, IItemView* view, IVisualStyle* visualStyle)
{
	return editString (initialValue, rect, nullptr, view, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::editString (StringRef initialValue, RectRef rect, const EditInfo* info, IItemView* itemView, IVisualStyle* visualStyle)
{
	AutoPtr<IParameter> param (NEW StringParam);
	param->setValue (initialValue);

	ControlBox editBox (CCL::ClassID::EditBox, param, rect, StyleFlags (0, CCL::Styles::kBorder));

	return startEditOperation (param, editBox, info, itemView, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::editValue (IParameter* param, const EditInfo& info, IVisualStyle* visualStyle)
{
	const UIDBytes* controllClassID = &CCL::ClassID::ValueBox;
	if(param->getType () != IParameter::Type::kInteger && param->getType () != IParameter::Type::kFloat)
		controllClassID = &CCL::ClassID::EditBox;

	ControlBox editBox (*controllClassID, param, info.rect, StyleFlags (0, CCL::Styles::kBorder));

	return startEditOperation (param, editBox, &info, nullptr, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::startEditOperation (IParameter* param, ViewBox& editControl, const EditInfo& info, IVisualStyle* visualStyle)
{
	return startEditOperation (param, editControl, &info, nullptr, visualStyle);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* ItemModel::startEditOperation (IParameter* param, ViewBox& editControl, const EditInfo* info, IItemView* itemView, IVisualStyle* visualStyle)
{
	ASSERT (param != nullptr);

	if(UnknownPtr<ISubject> paramSubject = param) // the control must not react on setValue (otherwise a text box will reset the selection)
		System::GetSignalHandler ().cancelSignals (paramSubject);

	AutoPtr<IVisualStyle> editStyle;
	if(!visualStyle)
	{
		editStyle = createEditStyle ();
		visualStyle = editStyle;
	}

	editControl.setVisualStyle (visualStyle);

	if(info)
		setEditControl (editControl, *info);
	else
	{
		if(itemView == nullptr)
			itemView = getItemView ();
		if(itemView)
			itemView->setEditControl (editControl);
	}

	return NEW EditControlOperation (param, editControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ItemModel::swipeItems (IView* _itemView, const MouseEvent& mouseEvent, ItemVisitor* itemVisitor, SwipeMethod method)
{
	UnknownPtr<IItemView> itemView (_itemView);
	if(itemView)
	{
		IMouseHandler* mouseHandler = NEW SwipeItemsMouseHandler (itemView, itemVisitor, method);

		itemView->beginMouseHandler (mouseHandler, mouseEvent);
		mouseHandler->trigger (mouseEvent, 0); // initial action
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AbstractTouchHandler* ItemModel::wrapMouseHandler (IMouseHandler* mouseHandler, IView* view)
{
	return NEW TouchMouseHandler (mouseHandler, view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ItemModel)
	DEFINE_METHOD_NAME ("doPopup")
END_METHOD_NAMES (ItemModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ItemModel::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "doPopup")
	{
		UnknownPtr<IParameter> param (msg[0]);
		BoxedEditInfo* editInfo = unknown_cast<BoxedEditInfo> (msg[1]);
		ASSERT (param && editInfo)
		if(param && editInfo)
			doPopup (param, *editInfo);
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}
