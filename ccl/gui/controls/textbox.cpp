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
// Filename    : ccl/gui/controls/textbox.cpp
// Description : Text Box
//
//************************************************************************************************

#include "ccl/gui/controls/textbox.h"
#include "ccl/gui/controls/controlaccessibility.h"

#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/graphics/textlayoutbuilder.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/theme/thememanager.h"
#include "ccl/gui/theme/renderer/textboxrenderer.h"

#include "ccl/public/gui/framework/itextmodel.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/icontextmenu.h"

#include "ccl/app/params.h"

namespace CCL {

//************************************************************************************************
// TextBoxAccessibilityProvider
//************************************************************************************************

class TextBoxAccessibilityProvider: public ValueControlAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (TextBoxAccessibilityProvider, ValueControlAccessibilityProvider)

	TextBoxAccessibilityProvider (TextBox& owner);
	
	// ValueControlAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	tbool CCL_API isReadOnly () const override;
	tresult CCL_API setValue (StringRef value) const override;
	tbool CCL_API canIncrement () const override;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("TextBox")
	XSTRING (CopyText, "Copy Text to Clipboard")
END_XSTRINGS

//************************************************************************************************
// TextBox
//************************************************************************************************

BEGIN_STYLEDEF (TextBox::customStyles)
	{"multiline",		Styles::kTextBoxAppearanceMultiLine},
	{"fittext",			Styles::kTextBoxAppearanceFitText},
	{"nocontextmenu",	Styles::kTextBoxBehaviorNoContextMenu},
	{"scaletext",		Styles::kTextBoxAppearanceScaleText},
	{"markup",			Styles::kTextBoxAppearanceMarkupEnabled},
	{"hidetext",		Styles::kTextBoxAppearanceHideText},
	{"nowheel",			Styles::kTextBoxBehaviorNoWheel},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (TextBox, Control)
DEFINE_CLASS_UID (TextBox, 0xc4d27fc9, 0xfa79, 0x422f, 0x8f, 0x1a, 0x97, 0xe5, 0xc8, 0xc0, 0x8a, 0x66)

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TextBox::getPasswordReplacementString ()
{
	return PasswordTextModel::kPasswordReplacementString;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBox::TextBox (const Rect& size, IParameter* param, StyleRef style, StringRef title)
: Control (size, param ? param : (IParameter*)AutoPtr<IParameter> (NEW StringParam), style, title),
  textModel (nullptr),
  labelParam (nullptr),
  colorParam (nullptr),
  showFullTextAsTooltip (-1),
  textTrimMode (Font::kTrimModeDefault),
  explicitMaxWidth (kMaxCoord),
  constructed (false),
  savedAlignment (Alignment::kLeft),
  alignmentInitialized (false)
{
	setWheelEnabled (false);
	createTextModel ();
	constructed = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBox::~TextBox ()
{
	cancelSignals ();

	setLabelParam (nullptr);
	setColorParam (nullptr);
	setTextModel (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API TextBox::getVisualStyle () const
{
	ASSERT (constructed)
	return SuperClass::getVisualStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBox::setParameter (IParameter* param)
{
	setTextModel (nullptr);
	SuperClass::setParameter (param);
	createTextModel ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::createTextModel ()
{
	AutoPtr<IMutableTextModel> model;

	// model provided via parameter
	UnknownPtr<ITextModelProvider> provider (getTextParameter ());
	if(provider)
	{
		UnknownPtr<IMutableTextModel> providedModel (provider->getTextModel ());
		if(providedModel.isValid ())
			model.share (providedModel);
	}

	// model specified by style
	if(!model)
	{
		if(style.isCustomStyle (Styles::kTextBoxAppearanceMarkupEnabled))
			model = NEW MarkupTextModel ();
		else if(style.isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
			model = NEW PasswordTextModel ();
		else
			model = NEW StringTextModel ();
	}

	AutoPtr<ProxyTextModel> textBoxModel = NEW ProxyTextModel (this, model);
	setTextModel (textBoxModel);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TextBox::getTextParameter () const
{
	return getParameter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::modifyDisplayString (String& displayString) const
{
	if(labelParam && !isEditing ())
	{
		String label;
		labelParam->toString (label);
		if(!label.isEmpty ())
		{
			if(!displayString.isEmpty ())
				displayString << " ";
			displayString << label;
		}
	}

	if(privateFlags & kMeasuringFitWidth)
		return;

	if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine) && style.isCustomStyle (Styles::kTextBoxAppearanceFitText))
	{
		String fullText (displayString);

		Coord textSpace = style.isVertical () ? getDisplayHeight () : getDisplayWidth ();
		if(!isEditing () && textSpace > 0)
			Font::collapseString (displayString, textSpace, getVisualStyle ().getTextFont (), textTrimMode);

		// if truncated, set full text as tooltip
		if(showFullTextAsTooltip)
		{
			if(showFullTextAsTooltip == -1) // first time check: an explicit tooltip suppresses this behavior
			{
				showFullTextAsTooltip = tooltip.isEmpty () ? 1 : 0;
				if(!showFullTextAsTooltip)
					return;
			}
	
			const_cast<TextBox*> (this)->setTooltip ((displayString == fullText) ? String::kEmpty : fullText);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableTextModel* TextBox::getTextModel () const
{
	return textModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::setTextModel (IMutableTextModel* model)
{
	if(textModel != model)
	{
		if(model)
		{
			String paramString;
			getTextParameter ()->toString (paramString);
			model->fromParamString (paramString);
		}

		share_and_observe_unknown (this, textModel, model);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TextBox::getLabelParam () const
{
	return labelParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::setLabelParam (IParameter* p)
{
	if(labelParam != p)
		share_and_observe_unknown (this, labelParam, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TextBox::getColorParam () const
{
	return colorParam;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::setColorParam (IParameter* p)
{
	if(colorParam != p)
		share_and_observe_unknown (this, colorParam, p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseCursor* TextBox::getTextCursor () const
{
	IMouseCursor* cursor = nullptr;

	// try application theme
	if(ITheme* appTheme = ThemeManager::instance ().getApplicationTheme ())
		if(&getTheme () != appTheme)
			cursor = appTheme->getThemeCursor (ThemeElements::kTextCursor);

	// fallback to system cursor
	if(cursor == nullptr)
		cursor = getTheme ().getThemeCursor (ThemeElements::kTextCursor);

	return unknown_cast<MouseCursor> (cursor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const ITextLayout* TextBox::getTextLayout () const
{
	if(auto* textBoxRenderer = ccl_cast<TextBoxRenderer> (renderer))
		return textBoxRenderer->getTextLayout ();

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::getText (String& text) const
{
	text.empty ();
	if(IMutableTextModel* model = getTextModel ())
		model->toDisplayString (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TextBox::getDisplayWidth () const
{
	int paddingSpace = padding.left + padding.right;
	if(style.isBorder ())
		return getWidth () - 8 - paddingSpace;
	else
		return getWidth () - paddingSpace;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TextBox::getDisplayHeight () const
{
	int paddingSpace = padding.top + padding.bottom;
	if(style.isBorder ())
		return getHeight () - 4 - paddingSpace;
	else
		return getHeight () - paddingSpace;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline void TextBox::triggerFitSizeCheck ()
{
	if(((sizeMode & kHFitSize) && getWidth () == 0) || (sizeMode & kVFitSize) || isHFitAndFitText ())
		checkFitSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::paramChanged ()
{
	triggerFitSizeCheck ();
	SuperClass::paramChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::attached (View* parent)
{
	SuperClass::attached (parent);
	updatePadding ();
	triggerFitSizeCheck ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();
	updatePadding ();
	if(isAttached ())
	{
		alignmentInitialized = false;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBox::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kChanged)
	{
		IParameter* textParam = getTextParameter ();
		if(textModel && textParam && isEqualUnknown (subject, textParam))
		{
			String string;
			textParam->toString (string);
			textModel->fromParamString (string);
			deferCheckFitSize ();
			SuperClass::notify (subject, msg);
		}
		else if(labelParam && isEqualUnknown (subject, labelParam))
			paramChanged ();
		else if(colorParam && isEqualUnknown (subject, colorParam))
		{
			invalidate ();
			if(renderer)
				renderer->invalidateStyleConditions ();
		}
		else if(textModel && isEqualUnknown (subject, textModel))
			invalidate ();
		else
			SuperClass::notify (subject, msg);
	}
	else if(msg == ITextModel::kRequestLayoutUpdate && isEqualUnknown (subject, textModel))
	{
		invalidate ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBox::setSize (RectRef newSize, tbool invalidate)
{
	if(size != newSize)
	{
		Rect rect = getTextRect (newSize);
		if(auto* textBoxRenderer = ccl_cast<TextBoxRenderer> (renderer))
		{
			textBoxRenderer->setSize (rectIntToF (rect));

			if(invalidate)
				this->invalidate ();
		}
	}

	SuperClass::setSize (newSize, invalidate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::onSize (const Point& delta)
{
	bool multiLine = style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
	if(isAttached ())
	{
		bool mustInvalidate = style.isBorder ();

		// check if resized in text direction
		bool isVertical = style.isVertical ();
		if((isVertical ? delta.y : delta.x) != 0)
		{
			// must invalidate for centered or right aligned text
			if(!mustInvalidate)
			{
				Alignment align = getVisualStyle ().getTextAlignment ();
				mustInvalidate = multiLine || ((align.align & Alignment::kHMask) != Alignment::kLeft);
			}
		}

		if(mustInvalidate)
			invalidate ();
	}

	if(multiLine && (sizeMode & kFitSize) == kVFitSize && delta.x != 0)
	{
		deferCheckFitSize ();  // must calc height for given width and resizeLayout
	}

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* TextBox::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kTextBoxRenderer, visualStyle, this);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::updatePadding ()
{
	getVisualStyle ().getPadding (padding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::calcAutoSize (Rect& r)
{
	if(auto* renderer = ccl_cast<TextBoxRenderer> (getRenderer ()))
		renderer->updateAtoms ();

	const ITextLayout* textLayout = getTextLayout ();
	if(textLayout == nullptr)
	{
		SuperClass::calcAutoSize (r);
		return;
	}

	Rect bounds;
	textLayout->getBounds (bounds);

	if(bounds.getWidth () > explicitMaxWidth)
	{
		bounds.setWidth (explicitMaxWidth);
		if(auto* textBoxRenderer = ccl_cast<TextBoxRenderer> (renderer))
			textBoxRenderer->setSize (rectIntToF (bounds));

		textLayout->getBounds (bounds);
	}

	if(isAttached () == false)
		updatePadding ();

	r.left = 0;
	r.top = 0;
	r.right = 0;
	r.bottom = bounds.getHeight () + padding.top + padding.bottom;

	if(isHFitAndFitText ())
	{
		r.right += getHFitWidth ();
		
		// limit the width (may shrink or grow, but must not exceed hfit or explicitMaxWidth when autosized)
		SizeLimit newLimits (getSizeLimits ());
		newLimits.maxWidth = ccl_min (explicitMaxWidth, r.right);
		SuperClass::setSizeLimits (newLimits);
	}
	else
		r.right += bounds.getWidth () + padding.left + padding.right;

	if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		if(!textLayout->getText ().isEmpty ())
		{
			if(getHeight () != 0)
				r.bottom = getHeight ();
		}
		else
			SuperClass::calcAutoSize (r);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBox::setSizeLimits (const SizeLimit& limits)
{
	explicitMaxWidth = limits.maxWidth;
	SuperClass::setSizeLimits (limits);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::isHFitAndFitText () const
{
	return (sizeMode & kHFitSize) && style.isCustomStyle (Styles::kTextBoxAppearanceFitText);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord TextBox::getHFitWidth () const
{
	Rect measureStringRect;
	String fullText;
	{
		ScopedFlag<kMeasuringFitWidth> scope (ccl_const_cast (this)->privateFlags); // measure the full text, not the collapsed one
		getText (fullText);
	}
	Font::measureString (measureStringRect, fullText, getVisualStyle ().getTextFont ());
	return measureStringRect.getWidth () + padding.left + padding.right;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::tryModelEditText (const MouseEvent& event)
{
	if(auto* textBoxRenderer = ccl_cast<TextBoxRenderer> (renderer))
	{
		IMutableTextModel::InteractionInfo interactionInfo = { this, event };
		if(textBoxRenderer->onTextInteraction (interactionInfo))
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::onMouseDown (const MouseEvent& event)
{
	if(tryModelEditText (event))
		return true;

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::onContextMenu (const ContextMenuEvent& event)
{
	bool result = SuperClass::onContextMenu (event);

	String text;
	getText (text);
	if(style.isCustomStyle (Styles::kTextBoxBehaviorNoContextMenu) == false && text.isEmpty () == false)
	{
		Rect r;
		getRenderer ()->getPartRect (r, kPartTextExtent);
		if(!r.isEmpty () && (event.wasKeyPressed || r.pointInside (event.where)))
		{
			event.contextMenu.addSeparatorItem ();
			event.contextMenu.addCommandItem (XSTR (CopyText), CSTR ("Edit"), CSTR ("Copy"), this);
			result = true; // avoid other items to appear
		}
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextBox::checkCommandCategory (CStringRef category) const
{
	return category == "Edit";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextBox::interpretCommand (const CommandMsg& msg)
{
	if(msg.category == "Edit")
	{
		if(msg.name == "Copy")
		{
			return onEditCopy (msg);
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::onEditCopy (const CommandMsg& msg)
{
	if(!msg.checkOnly ())
	{
		IParameter* textParam = getTextParameter ();
		String text;
		if(textParam)
			textParam->toString (text);

		Clipboard::instance ().setText (text);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect TextBox::getTextRect () const
{
	return getTextRect (getSize ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect TextBox::getTextRect (RectRef size) const
{
	Rect rect (0, 0, size.getWidth (), size.getHeight ());
	if(!rect.isEmpty ())
	{
		if(rect.getWidth () > padding.left + padding.right)
		{
			rect.left += padding.left;
			rect.right -= padding.right;
		}
		if(rect.getHeight () > padding.top + padding.bottom)
		{
			rect.top += padding.top;
			rect.bottom -= padding.bottom;
		}

		if(explicitMaxWidth > 0 && rect.getWidth () > explicitMaxWidth)
			rect.setWidth (explicitMaxWidth);
	}
	else
	{
		if(rect.getWidth () <= 0)
			rect.setWidth (kMaxCoord);
		if(rect.getHeight () <= 0)
			rect.setHeight (kMaxCoord);
	}

	return rect;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::isEditing () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* TextBox::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW TextBoxAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// TextBoxAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (TextBoxAccessibilityProvider, ValueControlAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

TextBoxAccessibilityProvider::TextBoxAccessibilityProvider (TextBox& owner)
: ValueControlAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API TextBoxAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kLabel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextBoxAccessibilityProvider::isReadOnly () const
{
	return true;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API TextBoxAccessibilityProvider::setValue (StringRef value) const
{
	return kResultFailed;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API TextBoxAccessibilityProvider::canIncrement () const
{
	return false;
}
