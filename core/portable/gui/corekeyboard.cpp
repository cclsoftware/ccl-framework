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
// Filename    : core/portable/gui/corekeyboard.cpp
// Description : Touch Screen Keyboard
//
//************************************************************************************************

#include "core/portable/gui/corekeyboard.h"

#include "core/system/coretime.h"
#include "corefont.h"

namespace Core {
namespace Skin {

//////////////////////////////////////////////////////////////////////////////////////////////////
// US-ASCII Keyboard Layout Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace KeyboardLayout
{
	enum SpecialKeys
	{
		kUnused = 0,
		kBackspace,
		kEnter,
		kShift,
		kMode,
		kSpace,
		kCancel,
		kSpecialKeyCount
	};
	
	static bool isSpecialKey (char keyCode)
	{
		return keyCode > kUnused && keyCode < kSpecialKeyCount;
	}

	static const int kRowCount = 4;
	static const int keysPerRow[kRowCount] =
	{
		11,
		10,
		11,
		4
	};
	
	static const char layoutLetters[] =
	{
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', kBackspace,
		'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', kEnter,
		kShift, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', kShift,	
		kMode, kSpace, kMode, kCancel		
	};
	
	static const char layoutNumbers[] =
	{
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', kBackspace,
		'-', '/', ':', ';', '(', ')', '$', '&', '@', kEnter,
		kShift, '.', ',', '?', '!', '\'', '\"', kUnused, kUnused, kUnused, kShift,
		kMode, kSpace, kMode, kCancel
	};
	
	static const char layoutSymbols[] =
	{
		'[', ']', '{', '}', '#', '%', '^', '*', '+', '=', kBackspace,
		'_', '\\', '|', '~', '<', '>', '$', '&', '@', kEnter,
		kShift, '.', ',', '?', '!', '\'', '\"', kUnused, kUnused, kUnused, kShift,
		kMode, kSpace, kMode, kCancel
	};

	static const char* getLayout (Mode mode)
	{
		switch(mode)
		{
		case kNumbers : return layoutNumbers;
		case kSymbols : return layoutSymbols;
		default : return layoutLetters;
		}
	}

	static const struct { char keyCode; float weight; } keyWeights[] =
	{
		{kEnter, 1.5f},
		{kMode, 1.5f},
		{kCancel, 1.5f},
		{kSpace, 6.5f}
	};
	
	static float getKeyWeight (char keyCode)
	{
		if(isSpecialKey (keyCode))
			for(int i = 0; i < ARRAY_COUNT (keyWeights); i++)
				if(keyWeights[i].keyCode == keyCode)
					return keyWeights[i].weight;
		return 1.f;
	}
	
	static void getKeyLabel (CString32& label, char keyCode, Mode mode)
	{
		label.empty ();
		if(isSpecialKey (keyCode))
			switch(keyCode)
			{
			case kBackspace : label = "Back"; break;
			case kEnter : label = "Enter"; break;
			case kShift : label = mode == kNumbers ? "#+=" : mode == kSymbols ? "123" : "Shift"; break;
			case kMode : label = mode == kNumbers || mode == kSymbols ? "ABC" : ".?123"; break;		
			case kSpace : label = " "; break;
			case kCancel : label = "Cancel"; break;
			}
		else
			switch(keyCode)
			{
			case ',' :
				if(mode == kLetters) label = "!\n,";
				break;
			case '.' :
				if(mode == kLetters) label = "?\n.";
				break;
			}
	
		if(label.isEmpty ())
			label += keyCode;
	}

	static char getCharacter (char keyCode, bool shiftEnabled)
	{
		char c = keyCode;
		if(shiftEnabled == true)
			switch(keyCode)
			{
			case ',' : c = '!'; break;
			case '.' : c = '?'; break;
			}
		else
			c = (char)::tolower (c);
		return c;
	}
}

} // namespace Skin
} // namespace Core

using namespace Core;
using namespace Portable;

//************************************************************************************************
// TouchKeyboard
//************************************************************************************************

TouchKeyboard::TouchKeyboard (RectRef size, IKeyboardInputReceiver* receiver)
: View (size),
  receiver (receiver),
  keySpacing (4),
  rowSpacing (8),
  keyWidth (38),
  keyColor (0xd3, 0xd3, 0xd6),
  pressedKeyColor (0xe8, 0xe8, 0xeb),
  keyColor2 (0x8b, 0x8a, 0x8a),
  pressedKeyColor2 (0xaa, 0xaa, 0xaa),
  mode (Skin::KeyboardLayout::kLetters),
  shiftEnabled (false),
  previousShiftState (false),
  capitalizationMode (Skin::KeyboardCapitalization::kNone)
{
	makeLayout (Skin::KeyboardLayout::kLetters);

	wantsFocus (true);
	wantsTouch (true);
}
  
//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::setAttributes (const Attributes& a)
{
	View::setAttributes (a);

	keySpacing = ViewAttributes::getInt (a, "keyspacing", keySpacing);
	rowSpacing = ViewAttributes::getInt (a, "rowspacing", rowSpacing);
	keyWidth = ViewAttributes::getInt (a, "keywidth", keyWidth);
	
	keyColor = ViewAttributes::getColor (a, "keycolor", keyColor);
	pressedKeyColor = ViewAttributes::getColor (a, "pressedkeycolor", pressedKeyColor);
	keyColor2 = ViewAttributes::getColor (a, "keycolor2", keyColor2);
	pressedKeyColor2 = ViewAttributes::getColor (a, "pressedkeycolor2", pressedKeyColor2);

	normalBack = ViewAttributes::getBitmap (a, "normalback");
	backspaceBack = ViewAttributes::getBitmap (a, "backspaceback");
	enterBack = ViewAttributes::getBitmap (a, "enterback");
	shiftBack = ViewAttributes::getBitmap (a, "shiftback");
	spaceBack = ViewAttributes::getBitmap (a, "spaceback");

	backspaceIcon = ViewAttributes::getBitmap (a, "backspaceicon");
	shiftIcon = ViewAttributes::getBitmap (a, "shifticon");
	
	makeLayout (Skin::KeyboardLayout::kLetters);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringPtr TouchKeyboard::getConnectionType () const
{
	return kKeyboardInputType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::connect (void* object)
{
	setReceiver (reinterpret_cast<IKeyboardInputReceiver*> (object));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::makeLayout (Skin::KeyboardLayout::Mode mode)
{
	keys.removeAll ();

	int flatIndex = 0;
	const char* layout = Skin::KeyboardLayout::getLayout (mode);
	for(int row = 0; row < Skin::KeyboardLayout::kRowCount; row++)
	{
		Point p (keySpacing, keySpacing + row * (keyWidth + rowSpacing));
		if(row == 1) // inset on second row
			p.x += keyWidth/2;
		
		int keyCount = Skin::KeyboardLayout::keysPerRow[row];
		for(int i = 0; i < keyCount; i++, flatIndex++)
		{
			char keyCode = layout[flatIndex];
			float weight = Skin::KeyboardLayout::getKeyWeight (keyCode);
			Coord width = (Coord)(weight * keyWidth + (weight - 1.f) * keySpacing);
			
			Rect keyRect (0, 0, width, keyWidth);
			keyRect.offset (p);
			p.x += width + keySpacing;
			
			Key descriptor;
			Skin::KeyboardLayout::getKeyLabel (descriptor.label, keyCode, mode);
			descriptor.code = keyCode;
			descriptor.rect = keyRect;
			descriptor.background = getBackgroundForKey (keyCode);
			descriptor.icon = getIconForKey (keyCode);
			keys.add (descriptor);
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* TouchKeyboard::getBackgroundForKey (char keyCode) const
{
	switch(keyCode)
	{
	case Skin::KeyboardLayout::kBackspace : return backspaceBack.getBitmap ();
	case Skin::KeyboardLayout::kEnter : return enterBack.getBitmap ();
	case Skin::KeyboardLayout::kShift : return shiftBack.getBitmap ();
	case Skin::KeyboardLayout::kSpace : return spaceBack.getBitmap ();
	}
	return normalBack.getBitmap ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Bitmap* TouchKeyboard::getIconForKey (char keyCode) const
{
	switch(keyCode)
	{
	case Skin::KeyboardLayout::kBackspace : return backspaceIcon.getBitmap ();
	case Skin::KeyboardLayout::kShift : return shiftIcon.getBitmap ();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::drawKey (Graphics& g, const Key& key)
{
	bool special = Skin::KeyboardLayout::isSpecialKey (key.code) && key.code != Skin::KeyboardLayout::kSpace;

	if(key.background)
		BitmapPainter::draw (g, key.rect.getLeftTop (), *key.background, key.pressed ? 1 : 0);
	else
	{
		Color backColor;
		if(special)
			backColor = key.pressed ? pressedKeyColor2 : keyColor2;
		else
			backColor =  key.pressed ? pressedKeyColor : keyColor;

		g.fillRect (key.rect, backColor);
	}

	bool hilite = (key.code == Skin::KeyboardLayout::kShift && shiftEnabled == true) || key.pressed;

	if(key.icon)
		BitmapPainter::drawCentered (g, key.rect, *key.icon, hilite ? 1 : 0);
	else
	{
		const Style& style = getStyle ();
		Color textColor = hilite ? style.getTextColorOn () : style.getTextColor ();
		if(special)
			textColor = style.getHiliteColor ();
			

		if(key.label.index ('\n') == 1) // shift character on top
		{
			char upperString[2] = {key.label[0], '\0'};
			char lowerString[2] = {key.label[2], '\0'};

			Rect textRect (key.rect);
			textRect.top += keySpacing;
			textRect.bottom -= keySpacing;
			g.drawString (textRect, upperString, textColor, style.getFontName (), Alignment::kTop|Alignment::kHCenter);
			g.drawString (textRect, lowerString, textColor, style.getFontName (), Alignment::kBottom|Alignment::kHCenter);			
		}
		else
		{
			CString16 label (key.label);
			if(!special && !shiftEnabled)
				label.toLowercase ();
			
			g.drawString (key.rect, label, textColor, style.getFontName (), Alignment::kCenter);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::draw (const DrawEvent& e)
{
	for(int i = 0; i < keys.count (); i++)
	{
		const Key& key = keys[i];
		if(key.code == Skin::KeyboardLayout::kUnused)
			continue;

		if(key.rect.intersect (e.updateRect))
			drawKey (e.graphics, key);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

TouchKeyboard::Key* TouchKeyboard::findKey (PointRef where) const
{
	for(int i = 0; i < keys.count (); i++)
		if(keys[i].rect.pointInside (where))
			return &keys[i];
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::selectMode (Mode newMode)
{
	if(newMode != mode)
	{
		makeLayout (newMode);
		mode = newMode;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::setCapitalizationMode (Skin::KeyboardCapitalization::Mode mode)
{
	if(mode != capitalizationMode)
	{
		capitalizationMode = mode;
		if(capitalizationMode != Skin::KeyboardCapitalization::kNone)
			setShiftEnabled (true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::clear ()
{
	if(capitalizationMode != Skin::KeyboardCapitalization::kNone && mode == Skin::KeyboardLayout::kLetters)
		setShiftEnabled (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::setShiftEnabled (bool state)
{
	if(shiftEnabled != state)
	{
		shiftEnabled = state;
		invalidate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchKeyboard::onTouchInput (const TouchEvent& e)
{
	Key* key = findKey (e.where);
	if(key == nullptr)
		return true; // swallow touch

	if(e.type == TouchEvent::kDown)
	{
		key->pressed = true;
		invalidate (key->rect);
	}
	else if(e.type == TouchEvent::kUp)
	{
		key->pressed = false;
		invalidate (key->rect);

		if(Skin::KeyboardLayout::isSpecialKey (key->code))
		{
			switch(key->code)
			{
			case Skin::KeyboardLayout::kMode :
				{
					// keeping track of shift state when going between letters and symbols mode
					bool currentShiftState = shiftEnabled;
					setShiftEnabled (previousShiftState);
					
					if(mode == Skin::KeyboardLayout::kLetters)
						selectMode (Skin::KeyboardLayout::kNumbers);
					else
						selectMode (Skin::KeyboardLayout::kLetters);
					
					previousShiftState = currentShiftState;
				}
				break;

			case Skin::KeyboardLayout::kShift :
				if(mode == Skin::KeyboardLayout::kNumbers)
					selectMode (Skin::KeyboardLayout::kSymbols);
				else if(mode == Skin::KeyboardLayout::kSymbols)
					selectMode (Skin::KeyboardLayout::kNumbers);
				
				setShiftEnabled (!shiftEnabled);
				break;

			case Skin::KeyboardLayout::kBackspace :
				if(receiver)
				{
					IKeyboardInputReceiver::Text& text = receiver->getText ();
					if(!text.isEmpty ())
					{
						int newCursorLocation = receiver->getCursorIndex () - 1;
						text.remove (newCursorLocation, 1);
						receiver->setCursorIndex (newCursorLocation);
						
						if(capitalizationMode == Skin::KeyboardCapitalization::kWords)
							setShiftEnabled (text.endsWith (" ") || text.isEmpty ());
						else if(capitalizationMode == Skin::KeyboardCapitalization::kFirst && text.isEmpty ())
							setShiftEnabled (true);
						
						receiver->textChanged ();
					}
				}
				break;

			case Skin::KeyboardLayout::kSpace :
				{
					if(capitalizationMode == Skin::KeyboardCapitalization::kWords)
						setShiftEnabled (true);
					
					insertChar (' ');
				}
				break;

			case Skin::KeyboardLayout::kEnter :
			case Skin::KeyboardLayout::kCancel :
				if(receiver)
					receiver->textInputDone (key->code == Skin::KeyboardLayout::kCancel);
				break;
			}
		}
		else // character input
		{
			char c = Skin::KeyboardLayout::getCharacter (key->code, (mode == Skin::KeyboardLayout::kLetters && shiftEnabled));
			
			if(mode == Skin::KeyboardLayout::kLetters)
				setShiftEnabled (false);

			insertChar (c);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::insertChar (char c)
{
	if(receiver)
	{
		int currentCursorLocation = receiver->getCursorIndex ();
		char str[2] = {c, '\0'};
		
		receiver->getText ().insert (currentCursorLocation, str);
		receiver->setCursorIndex (currentCursorLocation + 1);
		receiver->textChanged ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TouchKeyboard::getHandledGestures (GestureVector& gestures, PointRef where)
{
	gestures.add (kGestureSingleTap|kGesturePriorityNormal);
	gestures.add (kGestureLongPress|kGesturePriorityNormal);
	gestures.add (kGestureSwipe|kGesturePriorityNormal);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TouchKeyboard::onGestureInput (const GestureEvent& e)
{
	if(e.getState () == kGestureBegin || e.getState () == kGesturePossible)
	{
		onTouchInput (TouchEvent (TouchEvent::kDown, e.where));
		if(e.getType () == kGestureSingleTap)
			onTouchInput (TouchEvent (TouchEvent::kUp, e.where));
		return true;
	}
	else if(e.getState () == kGestureEnd)
		return onTouchInput (TouchEvent (TouchEvent::kUp, e.where));

	return false;
}

//************************************************************************************************
// TextInputBox
//************************************************************************************************

TextInputBox::TextInputBox (RectRef size, Parameter* p)
: TextBox (size, p),
  blinkState (false),
  lastBlinkTime (0),
  receiver (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextInputBox::draw (const DrawEvent& e)
{
	Rect r;
	getClientRect (r);
	const Style& style = getStyle ();

	TextValue string;
	getText (string);
	e.graphics.drawString (r, string, style.getTextColor (), style.getFontName (), Alignment::kLeft|Alignment::kVCenter);
	
	if(blinkState)
	{
		int cursorPixelPosition = indexToPixels (receiver ? receiver->getCursorIndex () : string.length (), string);
		e.graphics.drawLine (Point (cursorPixelPosition, 0), Point (cursorPixelPosition, r.getHeight ()), style.getTextColor ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TextInputBox::indexToPixels (int cursorIndex, const TextValue& text) const
{
	if(const BitmapFont* font = FontManager::instance ().getFont (getStyle ().getFontName ()))
	{
		TextValue substr;
		text.subString (substr, 0, cursorIndex);
		return font->getStringWidth (substr, text.length ());
	}
	
	Rect r;
	getClientRect (r);
	return r.getWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int TextInputBox::pixelsToIndex (int pixelXPos, const TextValue& text) const
{
	// does not support multiline text
	
	if(const BitmapFont* font = FontManager::instance ().getFont (getStyle ().getFontName ()))
	{
		int substringWidth = 0;
		
		for(int i = 0; i < text.length (); i++)
		{
			char str[2] = {text.at (i), '\0'};
			int currentCharWidth = font->getStringWidth (str, text.length ());

			if((substringWidth + (currentCharWidth / 2)) >= pixelXPos)
				return i;
			
			substringWidth += currentCharWidth;
		}
	}
	// default case, return location at end of string
	return text.length ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool TextInputBox::onTouchInput (const TouchEvent& e)
{
	if(e.type == TouchEvent::kDown)
	{
		if(receiver)
		{
			TextValue text;
			getText (text);
			receiver->setCursorIndex (pixelsToIndex (e.where.x, text));
			updateCursor (true);
		}
	}
	return TextBox::onTouchInput (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void TextInputBox::onIdle ()
{
	abs_time now = SystemClock::getMilliseconds ();
	if(now - lastBlinkTime >= kBlinkDelay)
		updateCursor (!blinkState);
}
//////////////////////////////////////////////////////////////////////////////////////////////////

void TextInputBox::updateCursor (bool state)
{
	blinkState = state;
	lastBlinkTime = SystemClock::getMilliseconds ();
	invalidate ();
}
