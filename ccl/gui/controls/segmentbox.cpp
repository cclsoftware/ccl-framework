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
// Filename    : ccl/gui/controls/segmentbox.cpp
// Description : Segment Box
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/controls/segmentbox.h"
#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/windows/window.h"
#include "ccl/gui/graphics/imaging/bitmap.h"

#include "ccl/gui/system/clipboard.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/gui/graphics/itextlayout.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/icontextmenu.h"
#include "ccl/public/math/mathprimitives.h"

using namespace CCL;

#if 0 && DEBUG
#define LOG_SEGMENTS Debugger::printf ("active: %d, typed: [%2d, %2d, %2d, %2d, %2d] %s\n", activePart, segmentValues[0], segmentValues[1], segmentValues[2], segmentValues[3], segmentValues[4], inserting ? "inserting" : "");
#else
#define LOG_SEGMENTS
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("SegmentBox")
	XSTRING (CopyText, "Copy Text to Clipboard")
END_XSTRINGS

//************************************************************************************************
// SegmentBoxDragHandler
//************************************************************************************************

class SegmentBoxDragHandler: public MouseHandler
{
public:
	SegmentBoxDragHandler (SegmentBox* segmentBox, int part)
	: MouseHandler (segmentBox),
	  part (part)
	{
		checkKeys (true);
	}

	// MouseHandler
	void onBegin () override
	{
		SegmentBox* segmentBox = (SegmentBox*)view;
		segmentBox->getParameter ()->beginEdit ();
	}

	void onRelease (bool) override
	{
		SegmentBox* segmentBox = (SegmentBox*)view;
		segmentBox->getParameter ()->endEdit ();
		segmentBox->setDragging (false);
		segmentBox->killFocus ();
		view->updateClient ();
	}

	bool onMove (int moveFlags) override
	{
		float delta = float ((first.where.y - current.where.y) + (current.where.x - first.where.x));

		((SegmentBox*)view)->incrementPart (part, ccl_to_int (delta));
		view->updateClient ();

		first = current;

		return true;
	}

protected:
	bool wasFine;
	int part;
};

//************************************************************************************************
// SegmentBox
//************************************************************************************************

BEGIN_VISUALSTYLE_CLASS (SegmentBox, VisualStyle, "SegmentBox")
	ADD_VISUALSTYLE_COLOR  ("state1")				///< alternative text color when the parameter's visual state is 1
	ADD_VISUALSTYLE_COLOR  ("state2")				///< alternative text color when the parameter's visual state is 2
	ADD_VISUALSTYLE_COLOR  ("state3")				///< alternative text color when the parameter's visual state is 3
END_VISUALSTYLE_CLASS (SegmentBox)

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (SegmentBox, Control)
DEFINE_CLASS_UID (SegmentBox, 0xA6888AE3, 0x9501, 0x4DA8, 0xA8, 0x1D, 0x6D, 0x28, 0xE2, 0x98, 0xE1, 0x61)

static StringRef signStr = CCLSTR("-");

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (SegmentBox::customStyles)
	{"static",			Styles::kSegmentBoxBehaviorStatic},
	{"separatedigits",	Styles::kSegmentBoxAppearanceSeparateDigits},
	{"scaletext",		Styles::kSegmentBoxAppearanceScaleText},
	{"nocontextmenu",	Styles::kSegmentBoxBehaviorNoContextMenu},
END_STYLEDEF

//////////////////////////////////////////////////////////////////////////////////////////////////

SegmentBox::SegmentBox (const Rect& size, IParameter* param, StyleRef style)
: Control (size, param, style),
  inserting (false),
  dragging (false),
  activePart (0),
  charWidth (0),
  delimiterWidth (0),
  leftMargin (0),
  oldSign (0xFF),
  oldId (-1),
  oldVisualState (-1),
  cachedBitmap (nullptr),
  contentScaleFactor (1.f),
  cachedFontSize (0)
{
	::memset (segmentValues, 0, sizeof(segmentValues));
	::memset (sizes, 0, sizeof(sizes));
	::memset (delimiter, 0, sizeof(delimiter));
	::memset (oldParts, -1, sizeof(oldParts));
	wantsFocus (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SegmentBox::~SegmentBox ()
{
	safe_release (cachedBitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::attached (View* parent)
{
	contentScaleFactor = getWindow ()->getContentScaleFactor ();

	SuperClass::attached (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::removed (View* parent)
{
	SuperClass::removed (parent);

	safe_release (cachedBitmap);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	contentScaleFactor = event.contentScaleFactor;
	safe_release (cachedBitmap);

	SuperClass::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* SegmentBox::getCachedBitmap (const Point& size, bool& contentLost)
{
	if(cachedBitmap && cachedBitmap->getWidth () == size.x && cachedBitmap->getHeight () == size.y)
	{
		contentLost = false;
		return cachedBitmap;
	}

	contentLost = true;
	safe_release (cachedBitmap);

	if(size.x > 0 && size.y > 0)
		cachedBitmap = NEW Bitmap (size.x, size.y, Bitmap::kRGB, contentScaleFactor);

	return cachedBitmap;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const IVisualStyle& CCL_API SegmentBox::getVisualStyle () const
{
	// TODO: clean this up by creating a renderer for SegmentBox!
	if(visualStyle)
		return *visualStyle;

	if(VisualStyle* standardStyle = getTheme ().getStandardStyle (ThemePainter::kSegmentBoxStyle))
	{
		const_cast<SegmentBox*> (this)->setVisualStyle (standardStyle);
		return *standardStyle;
	}
	return VisualStyle::emptyStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::checkState (IParamSplitter* splitter)
{
	bool needsSizeInfoUpdate = false;
	if(style.isCustomStyle (Styles::kSegmentBoxAppearanceScaleText))
		needsSizeInfoUpdate = scaleTextFont (splitter);
	
	if(splitter->getSplitterID () != oldId || needsSizeInfoUpdate)
	{
		oldId = splitter->getSplitterID ();

		updateSizeInfo (splitter);
		return true;
	}
	else if(param && param->getVisualState () != oldVisualState)
	{
	 	oldVisualState = param->getVisualState ();
		return true;
	}
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::updateSizeInfo (CCL::IParamSplitter *splitter)
{
	charWidth = getCharWidth ();
	delimiterWidth = getDelimiterWidth ();
	
	splitter->getPartSizes (sizes, kMaxNumParts);
	splitter->getDelimiter (delimiter, kMaxNumParts);
	
	int stringWidth = charWidth;
	for(int i = 0, count = splitter->countParts (); i < count; i++)
	{
		stringWidth += charWidth * sizes[i];
		if(i < count - 1)
			stringWidth += delimiterWidth;
	}
	
	const IVisualStyle& vs = getVisualStyle ();
	if(vs.getTextAlignment ().getAlignH () == Alignment::kHCenter)
		leftMargin = (getWidth () - stringWidth) / 2;
	else if(vs.getTextAlignment ().getAlignH () == Alignment::kRight)
		leftMargin = getWidth () - stringWidth;
	else
		leftMargin = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::scaleTextFont (IParamSplitter* splitter)
{
	Rect r;
	getVisibleClient (r);	
	if(r == cachedRect && splitter->getSplitterID () == oldId)
		return false;
	
	Font font = getVisualStyle ().getTextFont ();
	
	String zeroString = CCLSTR("-0"); // additional character to compensate raw charWidth estimation
	static StringRef delimiterString = CCLSTR(";");
	int sizes[kMaxNumParts];
	splitter->getPartSizes (sizes, kMaxNumParts);
	
	for(int i = 0, count = splitter->countParts (); i < count; i++)
	{
		zeroString.appendIntValue (0, sizes[i]);
		if(i < count - 1)
			zeroString.append (delimiterString);
	}
	
	float fontSize = font.getSize ();
	bool shouldGrow = (r.getHeight () > (2 * fontSize));
	static constexpr float kMinimalFontSize = 6.f;
	float maximalFontSize = (r.getHeight () * 0.75f);

	while(true)
	{
		
		Coord stringWidth = Font::getStringWidth (zeroString, font);
		if(fontSize <= kMinimalFontSize)
			break;
		if(r.getWidth () > stringWidth)
			break;
		 
		fontSize -= 1;
		font.setSize (fontSize);
		shouldGrow = false;
	}

	if(shouldGrow)
	{
		while(true)
		{
			fontSize += 1;
			font.setSize (fontSize);
			
			Coord stringWidth = Font::getStringWidth (zeroString, font);
			if(fontSize > maximalFontSize)
				break;
			if(r.getWidth () < stringWidth)
				break;
		}
		fontSize -= 1;
		font.setSize (fontSize);
	}
	
	cachedRect = r;
	cachedFontSize = fontSize;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::getPartValues (IParamSplitter& splitter, PartValues& parts)
{
	splitter.getParts (parts.values, parts.sign, kMaxNumParts);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::setPartValues (IParamSplitter& splitter, const PartValues& parts)
{
	splitter.setParts (parts.values, parts.sign, kMaxNumParts);	
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::drawBackground (IGraphics& graphics, RectRef r)
{
	const IVisualStyle& vs = getVisualStyle ();

	IImage* background = vs.getImage (StyleID::kBackground);
	if(background)
	{
		if(style.isDirectUpdate ())
			graphics.drawImage (background, r, r); // do not stretch!
		else
		{
			Rect src (0, 0, background->getWidth (), background->getHeight ());
			graphics.drawImage (background, src, r);
		}
	}
	else
		graphics.fillRect (r, vs.getBackBrush ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::adjustTextColor (SolidBrush& textBrush, Pen& textPen,const IVisualStyle& vs)
{
	int state = param->getVisualState ();
	if(state > 0)
	{
		MutableCString colorName;
		colorName.appendFormat ("state%d", state);
		textBrush.setColor (vs.getColor (colorName, textBrush.getColor ()));
		textPen.setColor (textBrush.getColor ());
	}
	else if(!isEnabled ())
		textBrush.setColor (vs.getColor ("textcolor.disabled", textBrush.getColor ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::drawOneSegment (IGraphics& port, RectRef size, StringRef segment, FontRef font, SolidBrushRef textBrush)
{
	if(style.isCustomStyle (Styles::kSegmentBoxAppearanceSeparateDigits))
	{
		Rect r (size);
		r.setWidth (charWidth);
		int numChars = segment.length ();
		ASSERT (size.getWidth () == numChars * charWidth);
		for(int i = 0; i < numChars; i++)
		{
			port.drawString (r, segment.subString (i, 1), font, textBrush, Alignment::kLeftCenter);
			r.offset (charWidth, 0);
		}
	}
	else
		port.drawString (size, segment, font, textBrush, Alignment::kCenter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::drawSegments (IGraphics& port, RectRef _rect)
{
	Rect rect (_rect);
	const IVisualStyle& vs = getVisualStyle ();

	if(style.isOpaque () || style.isDirectUpdate ())
		drawBackground (port, rect);

	if(style.isBorder ())
		port.drawRect (rect, vs.getForePen ());

	getDrawRect (rect);

	SolidBrush textBrush (vs.getTextBrush ());
	Pen textPen (textBrush.getColor ());
	
	String text;
	if(param)
	{
		adjustTextColor (textBrush, textPen, vs);

		UnknownPtr<IParamSplitter> splitter = param;
		if(splitter != nullptr)
		{
			checkState (splitter);
			
			Font font = vs.getTextFont ();
			if(cachedFontSize > 0)
				font.setSize (cachedFontSize);

			rect.offset (leftMargin, 0);

			int numberOfParts = splitter->countParts ();
			PartValues parts;
			getPartValues (*splitter, parts);

			Rect size (rect);

			size.right = size.left + charWidth;
			if(parts.sign < 0)
				port.drawString (size, signStr, font, textBrush, Alignment::kCenter);
			size.left = size.right;
			oldSign = parts.sign;

			Rect focusRect;
			for(int i = 0; i < numberOfParts; i++)
			{
				size.right = size.left + sizes[i] * charWidth;
				//port.drawRect (size, pen);
				if(inserting && segmentValues[i] >= 0)
				{
					String partsStr;
					partsStr.appendIntValue (segmentValues[i], sizes[i]);

					SolidBrush gray (Colors::kGray);
					port.fillRect (size, gray);
					drawOneSegment (port, size, partsStr, font, textBrush);
				}
				else
				{
					String partsStr;
					partsStr.appendIntValue (parts.values[i], sizes[i]);

					drawOneSegment (port, size, partsStr, font, textBrush);

					if(isFocused () && i == activePart)
						focusRect = size;
				}
				oldParts[i] = parts.values[i];
				size.left = size.right;

				if(i < numberOfParts - 1)
				{
					char str[] = { static_cast<char>(delimiter[i]), 0 };
					String delimStr (str);

					size.right += delimiterWidth;

					if(!(isFocused () && i == activePart))
						port.drawString (size, delimStr, font, textBrush, Alignment::kCenter);

					//port.drawRect (size, pen);
					size.left = size.right;
				}
			}

			if(!focusRect.isEmpty ())
			{
				focusRect.left--;
				focusRect.right++;
				port.drawRect (focusRect, textPen);

				if(0 && !inserting && !dragging)
				{
					Font small;
					small.setSize (font.getSize () / 2);

					focusRect.left = focusRect.right;
					focusRect.right = focusRect.left + delimiterWidth * 2;
					port.fillRect (focusRect, textBrush);

					focusRect.bottom = focusRect.top + focusRect.getHeight () / 2;
					port.drawString (focusRect, CCLSTR("+"), small, vs.getBackBrush (), Alignment::kCenter);

					focusRect.top = focusRect.bottom;
					focusRect.bottom = size.bottom;
					port.drawString (focusRect, CCLSTR("-"), small, vs.getBackBrush (), Alignment::kCenter);
				}
			}
		}
		else
		{
			oldId = -1;
			param->toString (text);
			port.drawString (rect, text, vs.getTextFont (), textBrush, vs.getTextAlignment ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::draw (const UpdateRgn& updateRgn)
{
	GraphicsPort port (this);
	Rect rect;
	getClientRect (rect);

	if(style.isDirectUpdate ())
	{
		// draw into bitmap here because update mechanism will only update parts of it!
		bool contentLost = true;
		if(Bitmap* bitmap = getCachedBitmap (rect.getSize (), contentLost)) // otherwise size is null
		{
			// render segments
			{
				BitmapGraphicsDevice graphics (bitmap);
				drawSegments (graphics, rect);
			}

			// transfer to screen
			//ImageMode mode (ImageMode::kInterpolationPixelQuality);
			port.drawImage (bitmap, rect, rect);//, &mode);
		}
	}
	else
		drawSegments (port, rect);

	View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::updateClient ()
{
	if(!hasBeenDrawn ())
		return;

	WindowUpdateInfo updateInfo;
	Window* window = getWindowForUpdate (updateInfo);
	if(window == nullptr)
		return;

	if(updateInfo.collectUpdates)
	{
		invalidate ();
		return;
	}

	bool partUpdate = true;
	UnknownPtr<IParamSplitter> splitter = param;
	if(splitter)
	{
		// meaning of parts has changed
		if(checkState (splitter))
			partUpdate = false;

		// avoid flicker with single part
		if(splitter->countParts () <= 1)
			partUpdate = false;
	}
	else // no parts
		partUpdate = false;

	bool done = false;
	if(partUpdate && style.isDirectUpdate () && !inserting && !isFocused ())
	{
		Rect rect;
		getClientRect (rect);

		bool contentLost = true;
		if(Bitmap* bitmap = getCachedBitmap (rect.getSize (), contentLost)) // otherwise size is null
			if(contentLost == false)
			{
				// update segments
				{
					BitmapGraphicsDevice graphics (bitmap);
					updateSegments (graphics, rect);
				}

				// transfer to screen
				GraphicsPort port (this);
				//ImageMode mode (ImageMode::kInterpolationPixelQuality);
				port.drawImage (bitmap, rect, rect);//, &mode);
				updateInfo.addDirtyRect (rect);
				done = true;
			}
	}

	if(done == false)
		Control::updateClient ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::updateSegments (IGraphics& port, RectRef _rect)
{
	Rect rect (_rect);
	getDrawRect (rect);

	rect.offset (leftMargin, 0);

	const IVisualStyle& vs = getVisualStyle ();

	SolidBrush textBrush (vs.getTextBrush ());
	Pen textPen (vs.getTextColor ());
	
	String text;
	if(param)
	{
		adjustTextColor (textBrush, textPen, vs);

		UnknownPtr<IParamSplitter> splitter = param;
		if(splitter != nullptr)
		{
			Font font = vs.getTextFont ();
			if(cachedFontSize > 0)
				font.setSize (cachedFontSize);
			
			int numberOfParts = splitter->countParts ();
			PartValues parts;
			getPartValues (*splitter, parts);

			Rect size (rect);
			size.right = size.left + charWidth;
			if(parts.sign != oldSign)
			{
				drawBackground (port, size);

				if(parts.sign < 0)
					port.drawString (size, signStr, font, textBrush, Alignment::kCenter);

				oldSign = parts.sign;
			}

			size.left = size.right;

			for(int i = 0; i < numberOfParts; i++)
			{
				size.right = size.left + sizes[i] * charWidth;
				if(parts.values[i] != oldParts[i])
				{
					String partsStr;
					partsStr.appendIntValue (parts.values[i], sizes[i]);

					drawBackground (port, size);

					drawOneSegment (port, size, partsStr, font, textBrush);
					oldParts[i] = parts.values[i];
				}

				size.left = size.right;

				if(i < numberOfParts - 1)
					size.left = size.right + delimiterWidth;
			}
		}
		else
		{
			param->toString (text);
			drawBackground (port, rect);
			port.drawString (rect, text, vs.getTextFont (), textBrush);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::onFocus (const FocusEvent& event)
{
	if(style.isCustomStyle (Styles::kSegmentBoxBehaviorStatic))
		return true;

	if(event.eventType == FocusEvent::kSetFocus)
	{
		isFocused (true);
		invalidate ();
	}
	else
	{
		if(inserting)
		{
			inserting = false;

			UnknownPtr<IParamSplitter> splitter = param;
			if(splitter)
			{
				PartValues parts;
				getPartValues (*splitter, parts);			
				for(int i = 0; i < kMaxNumParts; i++)
					if(segmentValues[i] >= 0)
						parts.values [i] = segmentValues[i];
				setPartValues (*splitter, parts);
			}
		}

		isFocused (false);
		activePart = -1;
		invalidate ();
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* SegmentBox::createMouseHandler (const MouseEvent& event)
{
	if(style.isCustomStyle (Styles::kSegmentBoxBehaviorStatic))
		return nullptr;

	int oldActivePart = activePart;
	int part = findRect (event.where);
	if(part == kPartSign)
	{
		param->setValue (-param->getValue ().asDouble (), true);
	}
	else if(part >= kPartSegment0)
	{
		activePart = (short)(part - kPartSegment0);
		LOG_SEGMENTS

		invalidate ();

		if(!inserting)
		{
			if(detectDrag (event))
			{
				dragging = true;
				return NEW SegmentBoxDragHandler (this, activePart);
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::onMouseWheel (const MouseWheelEvent& event)
{
	if(style.isCustomStyle (Styles::kSegmentBoxBehaviorStatic))
		return true;

	int part = findRect (event.where);
	if(part >= kPartSegment0)
	{
		float delta = event.delta;
		if(event.isAxisInverted ())
			delta *= -1;
		
		if(ccl_abs (delta) < 1.f)
			delta = 1.f * ccl_sign (delta);		

		incrementPart (part - kPartSegment0, ccl_to_int (delta));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::onKeyDown (const KeyEvent& event)
{
	if(style.isCustomStyle (Styles::kSegmentBoxBehaviorStatic))
		return false;

	UnknownPtr<IParamSplitter> splitter = param;
	if(splitter == nullptr)
		return false;

	bool result = true;
	switch(event.vKey)
	{
	case VKey::kEscape :
		if(inserting)
			inserting = false;
		killFocus ();
		break;
	case VKey::kReturn :
	case VKey::kEnter :
		killFocus ();
		return false;
	case VKey::kTab :
		if(!advance ((event.state.getModifiers () & KeyState::kShift) != 0))
			return false;
		break;
	case VKey::kLeft :
		advance (true);
		break;
	case VKey::kRight :
		advance (false);
		break;
	case VKey::kUp :
	case VKey::kDown :
		incrementPart (activePart, event.vKey == VKey::kDown ? -1 : 1);
		break;
	case VKey::kDelete :
		deleteCharacter (false);
		break;
	case VKey::kBackspace :
		deleteCharacter (true);
		break;
	default:
		{
			switch(event.character)
			{
			case '-' :
				toggleSign ();
				break;

			case '.' :
			case ',' :
			case ':' :
			case ';' :
				if(inserting && activePart == splitter->countParts () - 1)
					shiftUp ();
				else
					advance ((event.state.getModifiers () & KeyState::kShift) != 0);
				break;
			case '0' :
			case '1' :
			case '2' :
			case '3' :
			case '4' :
			case '5' :
			case '6' :
			case '7' :
			case '8' :
			case '9' :
				insertCharacter (event.character);
				break;

			case 'c' :
				if(event.state.getModifiers () & KeyState::kCommand)
				{
					String text;
					if(param)
						param->toString (text);
					Clipboard::instance ().setText (text);
				}
				break;
			case 'v' :
				if(event.state.getModifiers () & KeyState::kCommand)
				{
					String text;
					Clipboard::instance ().getText (text);
					if(param)
						param->fromString (text, true);
				}
				break;

			default :
				result = false;
			}
		}
	}

	invalidate ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::onKeyUp (const KeyEvent& event)
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::advance (bool back)
{
	UnknownPtr<IParamSplitter> splitter = param;
	int maxPart = splitter ? splitter->countParts () - 1 : 0;

	short oldActivePart = activePart;
	if(back)
		activePart--;
	else
		activePart++;

	if(activePart < 0)
		activePart = 0;
	else if(activePart > maxPart)
		activePart = (short)maxPart;

	LOG_SEGMENTS
	return activePart != oldActivePart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::incrementPart (int part, int amount)
{
	if(inserting == false)
	{
		UnknownPtr<IParamSplitter> splitter = param;
		if(splitter == nullptr)
			return;

		splitter->incrementPart (part, amount);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::deleteCharacter (bool back)
{
	if(inserting)
		segmentValues[activePart] = segmentValues[activePart] / 10;
	LOG_SEGMENTS
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::insertCharacter (short character)
{
	UnknownPtr<IParamSplitter> splitter = param;
	if(splitter)
	{
		int sizes[kMaxNumParts];
		splitter->getPartSizes (sizes, kMaxNumParts);

		if(inserting == false)
		{
			inserting = true;
			for(int i = 0; i < kMaxNumParts; i++)
				segmentValues[i] = -1;
		}

		// always insert into the active part
		if(segmentValues[activePart] < 0)
			segmentValues[activePart] = 0;

		// if part has already received the maximum digits, restart from 0 with first digit
		if(ccl_digits_of (segmentValues[activePart]) >= sizes[activePart])
			segmentValues[activePart] = 0;

		segmentValues[activePart] = segmentValues[activePart] * 10 + character - '0';

		// advance to next part if active part has received the maximum digits
		if(ccl_digits_of (segmentValues[activePart]) >= sizes[activePart])
			advance (false);

		LOG_SEGMENTS
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::shiftUp ()
{
	int i = 0;
	for( ; i < 3; i++)
		segmentValues[i] = segmentValues[i + 1];
	segmentValues[i] = 0;
	LOG_SEGMENTS
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SegmentBox::getActivePart ()
{
	return activePart;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::toggleSign ()
{
	UnknownPtr<IParamSplitter> splitter = param;
	if(splitter == nullptr)
		return;

	PartValues parts;
	getPartValues (*splitter, parts);

	if(inserting)
	{
		inserting = false;
		for(int i = 0; i < kMaxNumParts; i++)
			if(segmentValues[i] >= 0)
				parts.values[i] = segmentValues[i];
	}
	parts.sign = -parts.sign;
	setPartValues (*splitter, parts);
	LOG_SEGMENTS
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SegmentBox::getDrawRect (Rect& size)
{
	getClientRect (size);
	size.contract (1);

	Rect padding;
	getVisualStyle ().getPadding (padding);

	size.left += padding.left;
	size.right -= padding.right;
	size.top += padding.top;
	size.bottom -= padding.bottom;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SegmentBox::findRect (const Point& where)
{
	UnknownPtr<IParamSplitter> splitter = param;
	if(splitter != nullptr)
	{
		Rect size;
		getDrawRect (size);
		size.offset (leftMargin, 0);

		int numberOfParts = splitter->countParts ();

		size.right = size.left + charWidth;
		if(size.pointInside (where))
			return kPartSign;
		size.left = size.right;

		for(int i = 0; i < numberOfParts; i++)
		{
			size.right = size.left + sizes[i] * charWidth;
			if(i < numberOfParts - 1)
				size.right += delimiterWidth;

			if(size.pointInside (where))
				return kPartSegment0 + i;

			size.left = size.right;
		}
	}
	return kPartNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SegmentBox::getCharWidth ()
{
	static StringRef zero = CCLSTR("0000000000");

	const IVisualStyle& vs = getVisualStyle ();
	Font font = vs.getTextFont ();
	
	if(cachedFontSize > 0)
		font.setSize (cachedFontSize);
	
	Rect charSize;
	Font::measureString (charSize, zero, font, ITextLayout::kNoMargin);
	return (int)floor (((float)charSize.right / 10.f) + 0.5f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int SegmentBox::getDelimiterWidth ()
{
	static StringRef zero = CCLSTR(";;;;;;;;;;");

	const IVisualStyle& vs = getVisualStyle ();
	Font font = vs.getTextFont ();
	
	if(cachedFontSize > 0)
		font.setSize (cachedFontSize);

	Rect charSize;
	Font::measureString (charSize, zero, font, ITextLayout::kNoMargin);
	return (int)floor (((float)charSize.right / 10.f) + 0.5f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SegmentBox::onContextMenu (const ContextMenuEvent& event)
{
	bool result = SuperClass::onContextMenu (event);

	if(!style.isCustomStyle (Styles::kSegmentBoxBehaviorNoContextMenu))
	{
		event.contextMenu.addSeparatorItem ();
		event.contextMenu.addCommandItem (XSTR (CopyText), CSTR ("Edit"), CSTR ("Copy"), this);
		result = true;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SegmentBox::checkCommandCategory (CStringRef category) const
{
	return category == "Edit";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SegmentBox::interpretCommand (const CommandMsg& msg)
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

bool SegmentBox::onEditCopy (const CommandMsg& msg)
{
	if(!msg.checkOnly ())
	{
		String text;
		if(param)
			param->toString (text);
		Clipboard::instance ().setText (text);
	}
	return true;
}
