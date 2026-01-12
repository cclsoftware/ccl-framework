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
// Filename    : ccl/gui/controls/label.h
// Description : Label class
//
//************************************************************************************************

#ifndef _ccl_label_h
#define _ccl_label_h

#include "ccl/gui/views/view.h"

#include "ccl/public/gui/graphics/itextlayout.h"

namespace CCL {

//************************************************************************************************
// Label Styles
//************************************************************************************************

namespace Styles 
{
	enum LabelStyles
	{
		kLabelMultiLine = 1<<0,		///< multiline label
		kLabelMarkupEnabled = 1<<1,	///< interpret text markup
		kLabelColorize = 1<<2,		///< fill with "backcolor" (TextBox look)
	};
}

//************************************************************************************************
// Label
/** A Label draws a static text. 
The text can be arranged in a single line or broken into multiple lines (option "multiline"). */
//************************************************************************************************

class Label: public View
{
public:
	DECLARE_CLASS (Label, View)

	Label (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~Label ();

	DECLARE_STYLEDEF (customStyles)

	ITextLayout* getTextLayout ();

	// View
	void setTitle (StringRef title) override;
	void calcAutoSize (Rect& r) override;
	void calcSizeLimits () override;
	void onSize (const Point& delta) override;
	void onVisualStyleChanged () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	AccessibilityProvider* getAccessibilityProvider () override;
	const IVisualStyle& CCL_API getVisualStyle () const override;

protected:
	ThemeRenderer* renderer;
	AutoPtr<ITextLayout> textLayout;

	ThemeRenderer* getRenderer ();
	Rect getTextRect () const;
};

//************************************************************************************************
// Heading
/** A special label used for headings. */
//************************************************************************************************

class Heading: public Label
{
public:
	DECLARE_CLASS (Heading, Label)

	using Label::Label;
};

} // namespace CCL

#endif // _ccl_label_h
