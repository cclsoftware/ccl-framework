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
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/theme/thememanager.h"

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
// StringTextModel
//************************************************************************************************

class StringTextModel: public Object,
					   public AbstractTextModel
{
public:
	StringTextModel ()
	: wasObserved (false)
	{}

	// ITextModel
	void CCL_API toDisplayString (String& string) const override
	{
		string = getDataString ();
	}

	int CCL_API insertText (int textIndex, StringRef text, ITextModel::EditOptions options) override
	{
		if(textIndex < 0 || textIndex > getDataString ().length ())
		{
			ASSERT (false)
			return 0;
		}

		if(!(options & ITextModel::kMergeUndo))
			saveUndoState ();

		getDataString ().insert (textIndex, text);
		onChanged ();
		return text.length ();
	}

	int CCL_API removeText (int textIndex, int length, ITextModel::EditOptions options) override
	{
		if(!verifyRemoveTextIndex (textIndex, length))
			return 0;

		if(!(options & ITextModel::kMergeUndo))
			saveUndoState ();

		getDataString ().remove (textIndex, length);
		onChanged ();
		return length;
	}

	tbool CCL_API undo () override
	{
		if(undoIndex <= 0)
			return false;

		--undoIndex;
		onChanged ();
		return true;
	}

	tbool CCL_API redo () override
	{
		if(undoIndex >= undoStack.count () - 1)
			return false;

		++undoIndex;
		onChanged ();
		return true;
	}

	void CCL_API fromParamString (StringRef string) override
	{
		if(getDataString () != string)
		{
			undoStack.setCount (0);
			undoIndex = -1;
			getDataString () = string;
			onChanged ();
		}
	}

	void CCL_API toParamString (String& string) const override
	{
		string = getDataString ();
	}

	void CCL_API addObserver (IObserver* observer) override
	{
		wasObserved = true;
		Object::addObserver (observer);
	}

	CLASS_INTERFACE (ITextModel, Object)

protected:
	static bool verifyRemoveTextIndex (int& index, int& length)
	{
		if(length < 0)
		{
			length = -length;
			index -= length;
		}

		if(index < 0)
		{
			ASSERT (false)
			length += index;
			index = 0;
		}

		return length > 0;
	}

	StringRef getDataString () const
	{
		return const_cast<StringTextModel*> (this)->getDataString ();
	}

	String& getDataString ()
	{
		if(undoIndex < 0)
		{
			undoIndex = 0;
			undoStack.add ("");
		}
		return undoStack.at (undoIndex);
	}

	virtual void onChanged ()
	{
		if(wasObserved) // skip useless lookup in SignalHandler if nobody cares (e.g. inside TextBox constructor)
			signal (Message (kChanged));
	}

private:
	Vector<String> undoStack;
	int undoIndex = -1;
	bool wasObserved;

	void saveUndoState ()
	{
		// copy data string since setCount potentially reallocates memory and invalidates the reference
		String dataString (getDataString ());
		undoStack.setCount (++undoIndex);
		undoStack.add (dataString);
	}
};

//************************************************************************************************
// MarkupTextModel
//************************************************************************************************

class MarkupTextModel: public StringTextModel
{
public:
	MarkupTextModel (IView& view)
	: view (view)
	{}

	// ITextModel
	void CCL_API toDisplayString (String& string) const override
	{
		string = getMarkupParser ().getPlainText ();
	}

	void CCL_API updateLayout (ITextLayout& textLayout) override
	{
		TextLayoutBuilder builder (&textLayout);
		getMarkupParser ().applyFormatting (builder);
	}

	int CCL_API insertText (int textIndex, StringRef text, ITextModel::EditOptions options) override
	{
		String escapedText (text);
		getMarkupParser ().escapePlainText (escapedText);
		int markupIndex = getMarkupParser ().getMarkupPosition (textIndex, true);
		StringTextModel::insertText (markupIndex, escapedText, options);
		return text.length ();
	}

	int CCL_API removeText (int textIndex, int length, ITextModel::EditOptions options) override
	{
		if(!verifyRemoveTextIndex (textIndex, length))
			return 0;

		int markupStart = getMarkupParser ().getMarkupPosition (textIndex, false);
		int markupEnd = getMarkupParser ().getMarkupPosition (textIndex + length, true);
		int markupLength = markupEnd - markupStart;
		StringTextModel::removeText (markupStart, markupLength, options);
		return length;
	}

private:
	mutable AutoPtr<MarkupParser> markupParser;
	mutable bool markupDirty = false;
	IView& view;

	MarkupParser& getMarkupParser () const
	{
		if(markupParser == nullptr)
			markupParser = NEW MarkupParser (getDataString (), view.getVisualStyle ());
		else if(markupDirty)
			markupParser->parse (getDataString ());

		markupDirty = false;

		return *markupParser;
	}

	// StringTextModel
	void onChanged () override
	{
		markupDirty = true;
		StringTextModel::onChanged ();
	}
};

//************************************************************************************************
// PasswordTextModel
//************************************************************************************************

class PasswordTextModel: public StringTextModel
{
public:
	// ITextModel
	void CCL_API toDisplayString (String& string) const override
	{
		String blindText (kPasswordReplacementString, getDataString ().length ());
		string = blindText;
	}

	void CCL_API copyText (String& text, int textIndex = 0, int length = -1) const override
	{
		// not allowed
	}

public:
	static const String kPasswordReplacementString;
};

const String PasswordTextModel::kPasswordReplacementString (Text::kUTF8, u8"\u25CF");

//************************************************************************************************
// TextBox
//************************************************************************************************

BEGIN_STYLEDEF (TextBox::customStyles)
	{"multiline",		Styles::kTextBoxAppearanceMultiLine},
	{"fittext",			Styles::kTextBoxAppearanceFitText},
	{"nocontextmenu",	Styles::kTextBoxBehaviorNoContextMenu},
	{"scaletext",		Styles::kTextBoxAppearanceScaleText},
	{"markup",			Styles::kTextBoxAppearanceMarkupEnabled},
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
  changed (true),
  textModel (nullptr),
  labelParam (nullptr),
  colorParam (nullptr),
  showFullTextAsTooltip (-1),
  textTrimMode (Font::kTrimModeDefault),
  explicitMaxWidth (kMaxCoord),
  displayOffset (0),
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
	AutoPtr<ITextModel> model;

	// model provided via parameter
	UnknownPtr<ITextModelProvider> provider (getTextParameter ());
	if(provider)
		model.share (provider->getTextModel ());

	// model specified by style
	if(!model)
	{
		if(style.isCustomStyle (Styles::kTextBoxAppearanceMarkupEnabled))
			model = NEW MarkupTextModel (*this);
		else if(style.isCustomStyle (Styles::kTextBoxBehaviorPasswordEdit))
			model = NEW PasswordTextModel ();
		else
			model = NEW StringTextModel ();
	}

	setTextModel (model);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IParameter* TextBox::getTextParameter () const
{
	return getParameter ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextModel* TextBox::getTextModel () const
{
	return textModel;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::setTextModel (ITextModel* model)
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

void TextBox::buildTextLayout ()
{
	const IVisualStyle& vs = getVisualStyle ();
	bool scaleText = style.isCustomStyle (Styles::kTextBoxAppearanceScaleText);
	bool multiline = style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
	bool markup = style.isCustomStyle (Styles::kTextBoxAppearanceMarkupEnabled);

	ASSERT (textModel)
	String displayText (getText ());

	Rect rect (getTextRect ());
	Font font (vs.getTextFont ().zoom (getZoomFactor ()));
	TextFormat textFormat (vs.getTextFormat ());

	if(!alignmentInitialized)
	{
		savedAlignment = vs.getTextFormat ().getAlignment ();
		alignmentInitialized = true;
	}

	textFormat.setAlignment (savedAlignment);

	if(scaleText && rect.getSize () != Point (kMaxCoord, kMaxCoord) && !isEditing ())
		textScaler.scaleTextFont (font, rect, displayText, markup ? TextScaler::kMarkupText : 0);

	if(textModel)
	{
		textLayout = NativeGraphicsEngine::instance ().createTextLayout ();
		textLayout->construct (displayText, rect.getWidth (), rect.getHeight (), font, multiline ? ITextLayout::kMultiLine : ITextLayout::kSingleLine, textFormat);
		textModel->updateLayout (*textLayout);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::buildFullText (String& text) const
{
	text = plainTextCache;
	if(labelParam)
	{
		String label;
		labelParam->toString (label);
		if(!label.isEmpty ())
		{
			if(!text.isEmpty ())
				text << " ";
			text << label;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::buildText (String& text) const
{
	if(!style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine) && style.isCustomStyle (Styles::kTextBoxAppearanceFitText))
	{
		String fullText (text);

		Coord textSpace = style.isVertical () ? getDisplayHeight () : getDisplayWidth ();
		
		if(style.isCustomStyle (Styles::kTextBoxAppearanceScaleText))
		{
			static constexpr float kFitTextFactorAfterScaling = 1.16f;
			textSpace = ccl_to_int (textSpace * kFitTextFactorAfterScaling);
		}

		if(!isEditing () && textSpace > 0)
			Font::collapseString (text, textSpace, getVisualStyle ().getTextFont (), textTrimMode);

		// if truncated, set full text as tooltip
		if(showFullTextAsTooltip)
		{
			if(showFullTextAsTooltip == -1) // first time check: an explicit tooltip suppresses this behavior
			{
				showFullTextAsTooltip = tooltip.isEmpty () ? 1 : 0;
				if(!showFullTextAsTooltip)
					return;
			}
	
			const_cast<TextBox*> (this)->setTooltip ((text == fullText) ? String::kEmpty : fullText);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITextLayout* TextBox::getTextLayout ()
{
	if(changed)
	{
		String oldDisplayedText ((sizeMode & kHFitSize) ? displayedText : String::kEmpty); // only needed for kHFitSize

		buildTextLayout ();
		if(textLayout)
		{
			changed = false;
			if((sizeMode & kHFitSize) && displayedText != oldDisplayedText)
				deferFitSizeCheck ();
		}
	}
	return textLayout;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef TextBox::getText ()
{
	if(changed)
	{
		ASSERT (textModel)
		if(textModel)
			textModel->toDisplayString (plainTextCache);

		String oldDisplayedText ((sizeMode & kHFitSize) ? displayedText : String::kEmpty); // only needed for kHFitSize

		buildFullText (displayedText);
		buildText (displayedText);
		if((sizeMode & kHFitSize) && displayedText != oldDisplayedText)
			deferFitSizeCheck ();
	}

	return displayedText;
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
	setChanged ();
	triggerFitSizeCheck ();
	SuperClass::paramChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::attached (View* parent)
{
	SuperClass::attached (parent);
	updatePadding ();
	updateTextScaler ();
	triggerFitSizeCheck ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();
	updatePadding ();
	updateTextScaler ();
	if(isAttached ())
	{
		alignmentInitialized = false;
		buildTextLayout ();
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
			SuperClass::notify (subject, msg);
		}
		else if(labelParam && isEqualUnknown (subject, labelParam))
			paramChanged ();
		else if(colorParam && isEqualUnknown (subject, colorParam))
			invalidate ();
		else if(textModel && isEqualUnknown (subject, textModel))
		{
			textLayout.release (); // let model create new layout on next getTextLayout / buildTextLayout
			setChanged ();
			invalidate ();
		}
		else
			SuperClass::notify (subject, msg);
	}
	else if(msg == ITextModel::kRequestLayoutUpdate && isEqualUnknown (subject, textModel))
	{
		if(textLayout)
		{
			// let model update the existing layout
			textModel->updateLayout (*textLayout);
			invalidate ();
		}
	}
	else if(msg == "checkFitSize")
	{
		if(style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		{
			Point oldSize (getSize ().getSize ());
			checkFitSize ();
			if(getSize ().getSize () != oldSize)
			{
				Rect rect = getTextRect ();
				resizeLayout (rect);
			}
		}
		else
			checkFitSize ();
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::deferFitSizeCheck ()
{
	(NEW Message ("checkFitSize"))->post (this, -1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API TextBox::setSize (RectRef newSize, tbool invalidate)
{
	if(invalidate && size != newSize)
	{
		Rect rect = getTextRect (newSize);
		resizeLayout (rect);
	}
	
	SuperClass::setSize (newSize, invalidate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::onSize (const Point& delta)
{
	bool multiLine = style.isCustomStyle (Styles::kTextBoxAppearanceMultiLine);
	bool fitText = style.isCustomStyle (Styles::kTextBoxAppearanceFitText);
	bool scaleText = style.isCustomStyle (Styles::kTextBoxAppearanceScaleText);

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

			// the collapsed text may change
			if(!multiLine && (fitText || scaleText))
				setChanged ();
		}

		if(mustInvalidate)
			invalidate ();
	}
	else if(fitText || scaleText)
		setChanged ();

	if(multiLine && (sizeMode & kFitSize) == kVFitSize && delta.x != 0)
	{
		deferFitSizeCheck ();  // must calc height for given width and resizeLayout
	}

	SuperClass::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ThemeRenderer* TextBox::getRenderer ()
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kTextBoxRenderer, visualStyle);
	return renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::updatePadding ()
{
	getVisualStyle ().getPadding (padding);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::updateTextScaler ()
{
	textScaler.setExplicitMaximalFontSize (getVisualStyle ().getMetric<float> ("scaletext.maxfont", 100));
	textScaler.setExplicitMinimalFontSize (getVisualStyle ().getMetric<float> ("scaletext.minfont", 6));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::resizeLayout (RectRef size)
{
	if(textLayout == nullptr || changed)
		return;

	if(textLayout->resize (size.getWidth (), size.getHeight ()) != kResultOk)
		setChanged ();

	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextBox::calcAutoSize (Rect& r)
{
	if(getTextLayout () == nullptr) // creates text layout if changed
	{
		SuperClass::calcAutoSize (r);
		return;
	}

	Rect bounds;
	textLayout->getBounds (bounds);

	if(bounds.getWidth () > explicitMaxWidth)
	{
		bounds.setWidth (explicitMaxWidth);
		resizeLayout (bounds);
		textLayout->getBounds (bounds);
	}

	if(isAttached () == false)
		updatePadding ();

	// add kSpaceForPlatformPadding to calculated width,
	// in order to compensate for potential padding on platforms
	// i.e. Skia uses kPaddingLeft, kPaddingRight = 2;
	static constexpr int kSpaceForPlatformPadding = 4;
	
	r.left = 0;
	r.top = 0;
	r.right = kSpaceForPlatformPadding;
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
		if(!plainTextCache.isEmpty ())
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
	buildFullText (fullText);
	Font::measureString (measureStringRect, fullText, getVisualStyle ().getTextFont ());
	return measureStringRect.getWidth () + padding.left + padding.right;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextBox::tryModelEditText (const MouseEvent& event)
{
	if(textModel && textLayout)
	{
		ITextModel::InteractionInfo interactionInfo = { this, event };
		if(textModel->onTextInteraction (*textLayout, interactionInfo))
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

	if(style.isCustomStyle (Styles::kTextBoxBehaviorNoContextMenu) == false && getText ().isEmpty () == false)
	{
		Rect r;
		getRenderer ()->getPartRect (this, kPartTextExtent, r);
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

void TextBox::setChanged ()
{
	changed = true;
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
