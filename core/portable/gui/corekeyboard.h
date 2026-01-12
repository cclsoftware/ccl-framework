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
// Filename    : core/portable/gui/corekeyboard.h
// Description : Touch Screen Keyboard
//
//************************************************************************************************

#ifndef _corekeyboard_h
#define _corekeyboard_h

#include "core/portable/gui/corecontrols.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// ViewClasses
//************************************************************************************************

namespace ViewClasses
{
	const CStringPtr kTouchKeyboard = "TouchKeyboard";
	const CStringPtr kTextInputBox = "TextInputBox";
}

//************************************************************************************************
// IKeyboardInputReceiver
/** \ingroup core_gui */
//************************************************************************************************

struct IKeyboardInputReceiver
{
	typedef CString256 Text;

	virtual Text& getText () = 0;

	virtual void textChanged () = 0;

	virtual void textInputDone (bool canceled) = 0;
	
	virtual int getCursorIndex () const = 0;
	virtual void setCursorIndex (int index) = 0;
};

static const CStringPtr kKeyboardInputType = "KeyInput";

//************************************************************************************************
// TouchKeyboard
/** \ingroup core_gui */
//************************************************************************************************

class TouchKeyboard: public View
{
public:
	DECLARE_CORE_CLASS ('TKbd', TouchKeyboard, View)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kTouchKeyboard)

	TouchKeyboard (RectRef size = Rect (), IKeyboardInputReceiver* receiver = nullptr);

	PROPERTY_POINTER (IKeyboardInputReceiver, receiver, Receiver)

	PROPERTY_VARIABLE (Coord, keySpacing, KeySpacing)
	PROPERTY_VARIABLE (Coord, rowSpacing, RowSpacing)
	PROPERTY_VARIABLE (Coord, keyWidth, KeyWidth)

	PROPERTY_VARIABLE (Color, keyColor, KeyColor)
	PROPERTY_VARIABLE (Color, pressedKeyColor, PressedKeyColor)
	PROPERTY_VARIABLE (Color, keyColor2, KeyColor2)
	PROPERTY_VARIABLE (Color, pressedKeyColor2, PressedKeyColor2)
	
	PROPERTY_VARIABLE (BitmapReference, normalBack, NormalBack)
	PROPERTY_VARIABLE (BitmapReference, backspaceBack, BackspaceBack)
	PROPERTY_VARIABLE (BitmapReference, enterBack, EnterBack)
	PROPERTY_VARIABLE (BitmapReference, shiftBack, ShiftBack)
	PROPERTY_VARIABLE (BitmapReference, spaceBack, SpaceBack)
	
	PROPERTY_VARIABLE (BitmapReference, backspaceIcon, BackspaceIcon)
	PROPERTY_VARIABLE (BitmapReference, shiftIcon, ShiftIcon)
	
	typedef Skin::KeyboardLayout::Mode Mode;
	void selectMode (Mode mode);
	void setCapitalizationMode (Skin::KeyboardCapitalization::Mode mode);
	
	void clear ();

	// View
	void setAttributes (const Attributes& a) override;
	CStringPtr getConnectionType () const override;
	void connect (void* object) override;
	void draw (const DrawEvent& e) override;
	bool onTouchInput (const TouchEvent& e) override;
	bool onGestureInput (const GestureEvent& e) override;
	void getHandledGestures (GestureVector& gestures, PointRef where) override;

protected:
	Mode mode;
	Skin::KeyboardCapitalization::Mode capitalizationMode;
	
	bool shiftEnabled;
	bool previousShiftState;
	void setShiftEnabled (bool state);
	
	struct Key
	{
		char code;
		CString32 label;
		Bitmap* background;
		Bitmap* icon;
		Rect rect;
		bool pressed;
		
		Key ()
		: code (0),
		  background (nullptr),
		  icon (nullptr),
		  pressed (false)
		{}
	};
	
	static const int kMaxKeyCount = 36;
	FixedSizeVector<Key, kMaxKeyCount> keys;
	
	void makeLayout (Mode mode);
	Bitmap* getBackgroundForKey (char keyCode) const;
	Bitmap* getIconForKey (char keyCode) const;
	void drawKey (Graphics& g, const Key& key);	
	Key* findKey (PointRef where) const;
	void insertChar (char c);
};

//************************************************************************************************
// TextInputBox
/** \ingroup core_gui */
//************************************************************************************************

class TextInputBox: public TextBox
{
public:
	DECLARE_CORE_CLASS ('TxIB', TextInputBox, TextBox)
	DECLARE_CORE_VIEWCLASS (ViewClasses::kTextInputBox)

	TextInputBox (RectRef size = Rect (), Parameter* p = nullptr);
	
	PROPERTY_POINTER (IKeyboardInputReceiver, receiver, Receiver)

	// TextBox
	void draw (const DrawEvent& e) override;
	void onIdle () override;
	bool onTouchInput (const TouchEvent& e) override;

	void updateCursor (bool state);
	
protected:
	static const int kBlinkDelay = 500;

	bool blinkState;
	abs_time lastBlinkTime;
	
	int indexToPixels (int cursorIndex, const TextValue& text) const;
	int pixelsToIndex (int pixelXPos, const TextValue& text) const;
};

} // namespace Portable
} // namespace Core

#endif // _corekeyboard_h
