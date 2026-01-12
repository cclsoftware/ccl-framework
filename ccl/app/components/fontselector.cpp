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
// Filename    : ccl/app/components/fontselector.cpp
// Description : Cross-platform Font Selector
//
//************************************************************************************************

#include "ccl/app/components/fontselector.h"

#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/ithememanager.h"
#include "ccl/public/guiservices.h"

#include "ccl/base/message.h"
#include "ccl/base/asyncoperation.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Tags
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Tag
{
	enum FontSelectorTags
	{
		kFontName,
		kFontStyle,
		kFontSize
	};
}

//************************************************************************************************
// FontParamHelper
//************************************************************************************************

FontParamHelper::FontParamHelper (int collectFlags)
: fontTable (Font::collectFonts (collectFlags))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontParamHelper::FontParamHelper (IFontTable* table)
{
	fontTable.share (table);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontParamHelper::selectFont (IParameter* fontParam, IParameter* styleParam, const Font& font) const
{
	UnknownPtr<IListParameter> fontList = fontParam;
	UnknownPtr<IListParameter> styleList = styleParam;
	if(fontList == nullptr || styleList == nullptr)
		return false;

	if(fontList->selectValue (font.getFace ()) == false)
		return false;

	updateStyles (styleParam, fontParam);

	if(styleList->selectValue (font.getStyleName ()) == false)
	{
		// ?? how to translate style to flag ??
		styleParam->setValue (0); // or return false ??
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontParamHelper::updateFonts (IParameter* fontParam) const
{
	UnknownPtr<IListParameter> fontList = fontParam;
	if(fontList && fontTable)
	{
		fontList->removeAll ();
		int fontCount = fontTable->countFonts ();
		for(int i = 0; i < fontCount; i++)
		{
			String fontName;
			if(fontTable->getFontName (fontName, i) == kResultOk)
				fontList->appendString (fontName);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontParamHelper::updateStyles (IParameter* styleParam, const IParameter* fontListParam) const
{
	UnknownPtr<IListParameter> styleList = styleParam;
	if(fontListParam && styleList && fontTable)
	{
		String currentStyle (styleList->getSelectedValue ().asString ());

		int selectedFontIndex = fontListParam->getValue ().asInt ();
		int styleCount = fontTable->countFontStyles (selectedFontIndex);
		styleList->removeAll ();

		for(int i = 0; i < styleCount; i++)
		{
			String styleName;
			if(fontTable->getFontStyleName (styleName, selectedFontIndex, i) == kResultOk)
				styleList->appendString (styleName);
		}

		if(styleList->selectValue (currentStyle) == false)
			styleList->selectValue (CCLSTR ("Regular"));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult FontParamHelper::getExampleText (String& text, IParameter* fontParam) const
{
	if(fontTable == nullptr || fontParam == nullptr)
		return kResultFailed;

	return fontTable->getExampleText (text, fontParam->getValue ().asInt (), 0);
}

//************************************************************************************************
// FontSelectorComponent::ExampleView
//************************************************************************************************

class FontSelectorComponent::ExampleView: public UserControl
{
public:
	DECLARE_CLASS_ABSTRACT (ExampleView, UserControl)

	ExampleView (RectRef size, FontSelectorComponent* component);
	~ExampleView ();

	// UserControl
	void draw (const DrawEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	SharedPtr<FontSelectorComponent> component;
};

//************************************************************************************************
// FontSelectorComponent
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FontSelectorComponent, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

FontSelectorComponent::FontSelectorComponent (int collectFlags)
: Component (CCLSTR ("FontSelector")),
  fontHelper (collectFlags)
{
	makeParams ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontSelectorComponent::FontSelectorComponent (IFontTable* fontTable)
: Component (CCLSTR ("FontSelector")),
  fontHelper (fontTable)
{
	makeParams ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontSelectorComponent::makeParams ()
{
	paramList.addList ("fontName", Tag::kFontName);
	paramList.addList ("fontStyle", Tag::kFontStyle);
	paramList.addInteger (2, 100, "fontSize", Tag::kFontSize)->setValue (20);

	fontHelper.updateFonts (paramList.byTag (Tag::kFontName));
	fontHelper.updateStyles (paramList.byTag (Tag::kFontStyle), paramList.byTag (Tag::kFontName));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* FontSelectorComponent::prepareDialog (const Font& font, StringID _formName, ITheme* theme)
{
	CString formName (_formName);
	if(formName.isEmpty () && theme == nullptr)
	{
		formName = "FontSelectorDialog";
		theme = System::GetThemeManager ().getTheme ("cclgui");	
	}
	else if(theme == nullptr)
		theme = getTheme ();
	
	ASSERT (theme)
	if(theme == nullptr)
		return nullptr;

	selectFont (font);

	IView* view = theme->createView (formName, this->asUnknown ());
	ASSERT (view != nullptr)
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontSelectorComponent::runDialog (Font& font, StringID formName, ITheme* theme)
{
	IView* view = prepareDialog (font, formName, theme);
	if(view)
	{
		int dialogResult = DialogBox ()->runDialog (view, Styles::kWindowCombinedStyleDialog, Styles::kOkayButton | Styles::kCancelButton);	
		if(dialogResult == DialogResult::kOkay)
		{
			getSelectedFont (font);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* FontSelectorComponent::runDialogAsync (const Font& initialFont, StringID formName, ITheme* theme)
{
	IView* view = prepareDialog (initialFont, formName, theme);
	if(view)
	{
		Promise promise (DialogBox ()->runDialogAsync (view, Styles::kWindowCombinedStyleDialog, Styles::kOkayButton|Styles::kCancelButton));
		return return_shared<IAsyncOperation> (promise.then ([] (IAsyncOperation& operation)
		{
			operation.setResult (Variant (operation.getResult ().asInt () == DialogResult::kOkay));
		}));
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontSelectorComponent::getSelectedFont (Font& font)
{
	UnknownPtr<IListParameter> fontName = paramList.byTag (Tag::kFontName);
	UnknownPtr<IListParameter> fontStyle = paramList.byTag (Tag::kFontStyle);
	IParameter* sizeParam = paramList.byTag (Tag::kFontSize);

	font = Font (fontName->getSelectedValue ().asString (),sizeParam->getValue ().asFloat ());
	font.setStyleName (fontStyle->getSelectedValue ().asString ());
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontSelectorComponent::selectFont (const Font& font)
{
	IParameter* sizeParam = paramList.byTag (Tag::kFontSize);
	sizeParam->setValue (font.getSize ());

	return fontHelper.selectFont (paramList.byTag (Tag::kFontName), paramList.byTag (Tag::kFontStyle), font);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool FontSelectorComponent::getExampleStringForSelectedFont (String& string)
{
	IParameter* fontListParam = paramList.byTag (Tag::kFontName);
	if(fontListParam && fontHelper.getExampleText (string, fontListParam) == kResultOk)
		return true;

	auto text = "abcdefghijklmnopqrstuvwxyz\n" "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" "1234567890.:,;(!?)+-*/=";
	string = String (text);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FontSelectorComponent::paramChanged (IParameter* param)
{
	switch(param->getTag ())
	{
	case Tag::kFontName:
		fontHelper.updateStyles (paramList.byTag (Tag::kFontStyle), param);
		signal (Message (kChanged));
		onEditFont ();
		break;

	case Tag::kFontStyle:
	case Tag::kFontSize:
		signal (Message (kChanged));
		onEditFont ();
		break;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API FontSelectorComponent::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "Example")
		return *NEW ExampleView (bounds, this);

	return nullptr;	
}

//************************************************************************************************
// FontSelectorComponent::ExampleView
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FontSelectorComponent::ExampleView, UserControl)

//////////////////////////////////////////////////////////////////////////////////////////////////

FontSelectorComponent::ExampleView::ExampleView (RectRef size, FontSelectorComponent* component)
: component (component)
{
	component->addObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

FontSelectorComponent::ExampleView::~ExampleView ()
{
	component->removeObserver (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API FontSelectorComponent::ExampleView::notify (ISubject* subject, MessageRef msg)
{
	if(subject == component && msg == kChanged)
	{
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void FontSelectorComponent::ExampleView::draw (const DrawEvent& event)
{
	String exampleText;

	Font font;
	component->getSelectedFont (font);
	component->getExampleStringForSelectedFont (exampleText);
	
	Rect r;
	getClientRect (r);
	SolidBrush brush (getVisualStyle ().getTextBrush ());

	TextFormat format (Alignment::kLeftTop);
	event.graphics.drawText (r, exampleText, font, brush, format);
}
