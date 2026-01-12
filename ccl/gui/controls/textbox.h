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
// Filename    : ccl/gui/controls/textbox.h
// Description : Text Box
//
//************************************************************************************************

#ifndef _ccl_textbox_h
#define _ccl_textbox_h

#include "ccl/gui/controls/control.h"
#include "ccl/gui/graphics/textlayoutbuilder.h"
#include "ccl/gui/theme/renderer/compositedrenderer.h"

#include "ccl/public/gui/icommandhandler.h"

namespace CCL {

interface ITextModel;

//************************************************************************************************
// ITextParamProvider
//************************************************************************************************

interface ITextParamProvider: IUnknown
{
	virtual IParameter* getTextParameter () const = 0;

	DECLARE_IID (ITextParamProvider)
};

//************************************************************************************************
// TextBox
/** A TextBox displays the text representation of a parameter. 
The text is drawn in the "textcolor" of the visual style. 
An optional "backgound" image or "backcolor" of the style is drawn unless the "transparent" option is set.

The optional "labelname" specifies a parameter that gives a label text to be appended to the parameter value (e.g. a unit).

The "texttrimmode" specifies how the text is abbreviated when it doesn't fit in the TextBox.

When there is no "textcolor.bright" style definition, the "colorname" parameter is used as text color.
If "textcolor.bright" is defined the "colorname" parameter is used to keep text readable on varying background colors.
It is used to inform the TextBox about the background color. When the luminance of the TextBox goes below the "textcolor.threshold" value,
it switches the text color to the alternative "textcolor.bright" color.
If this background color is transparent "textcolor.transparent" will be used to draw the text */
//************************************************************************************************

class TextBox: public Control,
			   public ICommandHandler,
			   public ITextParamProvider
{
public:
	DECLARE_CLASS (TextBox, Control)

	TextBox (const Rect& size = Rect (), IParameter* param = nullptr,
			 StyleRef style = 0, StringRef title = nullptr);
	~TextBox ();

	enum TextBoxParts
	{
		kPartNone = 0,
		kPartContentArea = 1,
		kPartTextExtent = 2
	};

	DECLARE_STYLEDEF (customStyles)

	IParameter* getLabelParam () const;
	void setLabelParam (IParameter* labelParam);

	IParameter* getColorParam () const;
	void setColorParam (IParameter* colorParam);

	ITextModel* getTextModel () const;

	Rect getTextRect () const;
	Rect getTextRect (RectRef size) const;

	virtual Coord getDisplayWidth () const;
	virtual Coord getDisplayHeight () const;
	virtual ITextLayout* getTextLayout ();
	virtual StringRef getText ();
	virtual bool isEditing () const;

	PROPERTY_VARIABLE (int, textTrimMode, TextTrimMode)

	Coord getDisplayOffset () { return isEditing () ? displayOffset : 0; }

	static StringRef getPasswordReplacementString ();

	// Control
	const IVisualStyle& CCL_API getVisualStyle () const override;
	void CCL_API setParameter (IParameter* param) override;
	ThemeRenderer* getRenderer () override;
	void CCL_API setSize (RectRef size, tbool invalidate = true) override;
	void onSize (const Point& delta) override;
	void paramChanged () override;
	void calcAutoSize (Rect& r) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onContextMenu (const ContextMenuEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	void attached (View* parent) override;
	void onVisualStyleChanged () override;
	void CCL_API setSizeLimits (const SizeLimit& limits) override;
	AccessibilityProvider* getAccessibilityProvider () override;

	// ITextParamProvider
	IParameter* getTextParameter () const override;

	CLASS_INTERFACE2 (ICommandHandler, ITextParamProvider, Control)

protected:
	AutoPtr<ITextLayout> textLayout;
	ITextModel* textModel;
	TextScaler textScaler;
	String displayedText; // possibly truncated to fit into the box and has the labelParam text prepended
	String plainTextCache; // read-only cache that mirrors textModel.toDisplayString ()
	bool changed;
	IParameter* labelParam;
	IParameter* colorParam;
	mutable int showFullTextAsTooltip;
	bool constructed;

	Rect padding;
	Alignment savedAlignment;
	bool alignmentInitialized;
	Coord explicitMaxWidth;
	Coord displayOffset;

	void setTextModel (ITextModel* model);

	void createTextModel ();
	bool tryModelEditText (const MouseEvent& event);
	void buildFullText (String& text) const;
	void buildText (String& text) const;
	virtual void buildTextLayout ();
	void deferFitSizeCheck ();
	void triggerFitSizeCheck ();
	bool isHFitAndFitText () const;
	virtual Coord getHFitWidth () const;
	
	bool onEditCopy (const CommandMsg&);
	virtual void setChanged ();

	void updatePadding ();
	void updateTextScaler ();
	void resizeLayout (RectRef size);
	MouseCursor* getTextCursor () const;

	// ICommandHandler
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
};

} // namespace CCL

#endif // _ccl_textbox_h
