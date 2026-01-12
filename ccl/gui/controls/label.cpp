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
// Filename    : ccl/gui/controls/label.cpp
// Description : Label class
//
//************************************************************************************************

#include "ccl/gui/controls/label.h"

#include "ccl/gui/views/viewaccessibility.h"

#include "ccl/gui/graphics/textlayoutbuilder.h"
#include "ccl/base/message.h"

#include "ccl/gui/theme/renderer/labelrenderer.h"
#include "ccl/gui/theme/visualstyle.h"
#include "ccl/gui/layout/layoutprimitives.h"
#include "ccl/gui/graphics/nativegraphics.h"

using namespace CCL;

//************************************************************************************************
// LabelAccessibilityProvider
//************************************************************************************************

class LabelAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (LabelAccessibilityProvider, ViewAccessibilityProvider)

	LabelAccessibilityProvider (Label& owner);

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
};

//************************************************************************************************
// LabelAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (LabelAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

LabelAccessibilityProvider::LabelAccessibilityProvider (Label& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API LabelAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kLabel;
}

//************************************************************************************************
// Label
//************************************************************************************************

BEGIN_STYLEDEF (Label::customStyles)
	{"multiline", Styles::kLabelMultiLine},
	{"markup",	  Styles::kLabelMarkupEnabled},
	{"colorize",  Styles::kLabelColorize},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Label, View)
DEFINE_CLASS_UID (Label, 0x909eabe6, 0x47e3, 0x4eab, 0xa3, 0x80, 0xc5, 0x2b, 0xec, 0x9e, 0xf1, 0xa5)

//////////////////////////////////////////////////////////////////////////////////////////////////

Label::Label (const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  renderer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Label::~Label ()
{
	if(renderer)
		renderer->release ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* Label::getRenderer ()
{	
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kLabelRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& Label::getVisualStyle () const
{
	if(visualStyle)
		return *visualStyle;
	
	// need this for calculating auto-size with correct visual style
	if(ThemeRenderer* r = const_cast<Label*> (this)->getRenderer ())
		if(VisualStyle* rvs = r->getVisualStyle ())
			return *rvs;

	return SuperClass::getVisualStyle ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Rect Label::getTextRect () const
{
	bool vertical = style.isVertical ();
	return Rect (0, 0, vertical ? getHeight () : getWidth (), vertical ? getWidth () : getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* Label::getTextLayout ()
{
	if(!title.isEmpty ())
	{
		if(textLayout == nullptr)
		{
			textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
			
			bool multiline = style.isCustomStyle (Styles::kLabelMultiLine);
			Rect rect = getTextRect ();
			const IVisualStyle& vs = getVisualStyle ();
			Font font (vs.getTextFont ().zoom (getZoomFactor ()));

			if(style.isCustomStyle (Styles::kLabelMarkupEnabled))
			{
				MarkupParser parser (title, vs);
				StringRef text = parser.getPlainText ();
				textLayout->construct (text, rect.getWidth (), rect.getHeight (), font,
									   multiline ? ITextLayout::kMultiLine : ITextLayout::kSingleLine,
									   vs.getTextFormat ());
				TextLayoutBuilder builder (textLayout);
				parser.applyFormatting (builder);
			}
			else
			{
				textLayout->construct (title, rect.getWidth (), rect.getHeight (), font, 
									   multiline ? ITextLayout::kMultiLine : ITextLayout::kSingleLine,
									   vs.getTextFormat ());				
			}
		}
	}
	return textLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::calcAutoSize (Rect& r)
{
	if(!title.isEmpty ())
	{
		const IVisualStyle& vs = getVisualStyle ();
		Font font = vs.getTextFont ().zoom (getZoomFactor ());
		if(style.isCustomStyle (Styles::kLabelMultiLine))
		{
			Coord lineWidth = getWidth ();
			if(lineWidth <= 0)
				lineWidth = 100;
			Font::measureText (r, lineWidth, title, font, getVisualStyle ().getTextFormat ());
		}
		else
			Font::measureString (r, title, font);

		Rect padding;
		vs.getPadding (padding);
		
		ccl_lower_limit (padding.top, 2);
		ccl_lower_limit (padding.bottom, 2);

		padding.zoom (getZoomFactor ());
		
		r.right  += padding.left + padding.right;
		r.bottom += padding.top + padding.bottom;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::calcSizeLimits ()
{
	if(style.isCustomStyle (Styles::kLabelMultiLine))
	{
		if(!LayoutPrimitives::calcMultiLineLimits (sizeLimits, this))
			sizeLimits.setUnlimited ();
	}
	else
	{
		if(!LayoutPrimitives::calcTitleLimits (sizeLimits, this))
			sizeLimits.setUnlimited ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::onSize (const Point& delta)
{
	if(textLayout)
	{
		Rect rect = getTextRect ();
		if(textLayout->resize (rect.getWidth (), rect.getHeight ()) != kResultOk)
			textLayout.release ();
	}

	if(delta.x != 0)
	{
		bool multiLine = style.isCustomStyle (Styles::kLabelMultiLine);

		// must invalidate for centered or right aligned text, or multiline text
		if(multiLine || (getVisualStyle ().getTextAlignment ().align & Alignment::kHMask) != Alignment::kLeft)
			invalidate ();

		if(multiLine && (sizeMode & kFitSize) == kVFitSize)
			(NEW Message ("checkFitSize"))->post (this); // must calc height for given width
	}

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::onVisualStyleChanged ()
{
	// discard renderer & layout
	safe_release (renderer);
	textLayout.release ();

	SuperClass::onVisualStyleChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(!visualStyle || visualStyle->hasReferences (event.scheme))
		safe_release (renderer);

	textLayout.release ();

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::setTitle (StringRef title)
{
	textLayout.release ();

	SuperClass::setTitle (title);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Label::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "checkFitSize")
		checkFitSize ();
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Label::draw (const UpdateRgn& updateRgn)
{
	getRenderer ()->draw (this, updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* Label::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW LabelAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// Heading
//************************************************************************************************

DEFINE_CLASS (Heading, Label)
DEFINE_CLASS_UID (Heading, 0x34fe453c, 0x15f8, 0x450d, 0xbe, 0xaf, 0xfb, 0x76, 0x15, 0x7c, 0xba, 0x54)
