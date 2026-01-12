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
// Filename    : ccl/app/components/fontselector.h
// Description : Cross-platform Font Selector
//
//************************************************************************************************

#ifndef _ccl_fontselector_h
#define _ccl_fontselector_h

#include "ccl/app/component.h"

namespace CCL {

class Font;
interface IFontTable;
interface IAsyncOperation;

//************************************************************************************************
// FontParamHelper
//************************************************************************************************

class FontParamHelper
{
public:
	FontParamHelper (int collectFlags = 0);
	FontParamHelper (IFontTable* fontTable); // shared

	bool selectFont (IParameter* fontParam, IParameter* styleParam, const Font& font) const;
	void updateFonts (IParameter* fontParam) const;
	void updateStyles (IParameter* styleParam, const IParameter* fontParam) const;
	tresult getExampleText (String& text, IParameter* fontParam) const;

protected:
	AutoPtr<IFontTable> fontTable;
};

//************************************************************************************************
// FontSelectorComponent
//************************************************************************************************

class FontSelectorComponent: public Component
{
public:
	DECLARE_CLASS (FontSelectorComponent, Component)

	FontSelectorComponent (int collectFlags = 0);
	FontSelectorComponent (IFontTable* fontTable);

	bool runDialog (Font& font, StringID formName = nullptr, ITheme* theme = nullptr);
	IAsyncOperation* runDialogAsync (const Font& initialFont, StringID formName = nullptr, ITheme* theme = nullptr); // use getSelectedFont for result

	bool selectFont (const Font& font);
	bool getSelectedFont (Font& font);
	bool getExampleStringForSelectedFont (String& string);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;

protected:	
	virtual void onEditFont () {} // subclass can override

private:
	class ExampleView;

	FontParamHelper fontHelper;

	void makeParams ();
	IView* prepareDialog (const Font& font, StringID formName, ITheme* theme);
};

} // namespace

#endif // _ccl_fontselector_h
