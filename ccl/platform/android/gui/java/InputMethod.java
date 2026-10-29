//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/android/gui/java/InputMethod.java
// Description : Android Input Method
//
//************************************************************************************************

package dev.ccl.cclgui;

import android.content.Context;
import android.text.InputType;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.*;

import androidx.annotation.Keep;

//************************************************************************************************
// InputMethod
//************************************************************************************************

@Keep
public class InputMethod extends BaseInputConnection
{
	private long nativePtr;
	private View view;

	public InputMethod (View view, long nativePtr)
	{
		super (view, true);
		this.view = view;
		this.nativePtr = nativePtr;
	}

	public void disconnect ()
	{
		nativePtr = 0;
	}

	/** Called by C++ when text changed from app side - forces the IME to re-read our state. */
	public void invalidateInput ()
	{
		InputMethodManager imm = (InputMethodManager)view.getContext ().getSystemService (Context.INPUT_METHOD_SERVICE);
		if(imm != null)
			imm.restartInput (view);
	}

	/** Called by C++ when the selection/caret moved from app side. */
	public void updateSelection (int selStart, int selEnd)
	{
		InputMethodManager imm = (InputMethodManager)view.getContext ().getSystemService (Context.INPUT_METHOD_SERVICE);
		if(imm != null)
			imm.updateSelection (view, selStart, selEnd, -1, -1);
	}

	/** Called by C++ to update the IME's cursor anchor info (for IME popup positioning). */
	public void updateCursor (int left, int top, int right, int bottom)
	{
		InputMethodManager imm = (InputMethodManager)view.getContext ().getSystemService (Context.INPUT_METHOD_SERVICE);
		if(imm != null)
			imm.updateCursor (view, left, top, right, bottom);
	}

	public InputConnection onCreateInputConnection (EditorInfo outAttrs)
	{
		// Fill in the EditorInfo so the IME shows the right keyboard type.
		outAttrs.inputType = InputType.TYPE_CLASS_TEXT;
		outAttrs.imeOptions = EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI;

		if(nativePtr != 0)
		{
			int inputType = getInputTypeNative (nativePtr);
			int inputFlags = getInputFlagsNative (nativePtr);

			switch(inputType)
			{
			case ControlStyles.kKeyboardTypeNumeric :
				outAttrs.inputType = InputType.TYPE_CLASS_NUMBER;
				break;
			case ControlStyles.kKeyboardTypeNumericSigned :
				outAttrs.inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;
				break;
			case ControlStyles.kKeyboardTypeDecimal :
				outAttrs.inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL;
				break;
			case ControlStyles.kKeyboardTypeDecimalSigned :
				outAttrs.inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
				break;
			case ControlStyles.kKeyboardTypeEmail :
				outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
				break;
			case ControlStyles.kKeyboardTypeUrl :
				outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI;
				break;
			case ControlStyles.kKeyboardTypePhoneNumber :
				outAttrs.inputType = InputType.TYPE_CLASS_PHONE;
				break;
			}

			if((inputFlags & GuiConstants.kTextInputFlagSensitiveData) != 0)
			{
				if((outAttrs.inputType & InputType.TYPE_MASK_CLASS) == InputType.TYPE_CLASS_TEXT)
					outAttrs.inputType = (outAttrs.inputType & ~InputType.TYPE_MASK_VARIATION) | InputType.TYPE_TEXT_VARIATION_PASSWORD;
				else if((outAttrs.inputType & InputType.TYPE_MASK_CLASS) == InputType.TYPE_CLASS_NUMBER)
					outAttrs.inputType = (outAttrs.inputType & ~InputType.TYPE_MASK_VARIATION) | InputType.TYPE_NUMBER_VARIATION_PASSWORD;
			}

			if((inputFlags & GuiConstants.kTextInputFlagMultiline) != 0)
			{
				outAttrs.inputType |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
				outAttrs.imeOptions = (outAttrs.imeOptions & ~EditorInfo.IME_MASK_ACTION) | EditorInfo.IME_ACTION_NONE;
			}

			outAttrs.initialSelStart = getSelectionStartNative (nativePtr);
			outAttrs.initialSelEnd = getSelectionEndNative (nativePtr);
		}

		return this;
	}

	// -----------------------------------------------------------------------------------------
	// InputConnection - called by the Android IME on the UI thread
	// -----------------------------------------------------------------------------------------

	@Override
	public CharSequence getTextBeforeCursor (int n, int flags)
	{
		if(nativePtr == 0)
			return "";
		String text = getTextBeforeCursorNative (nativePtr, n);
		return text != null ? text : "";
	}

	@Override
	public CharSequence getTextAfterCursor (int n, int flags)
	{
		if(nativePtr == 0)
			return "";
		String text = getTextAfterCursorNative (nativePtr, n);
		return text != null ? text : "";
	}

	@Override
	public CharSequence getSelectedText (int flags)
	{
		if(nativePtr == 0)
			return null;
		return getSelectedTextNative (nativePtr);
	}

	@Override
	public int getCursorCapsMode (int reqModes)
	{
		return 0;
	}

	@Override
	public boolean commitText (CharSequence text, int newCursorPosition)
	{
		if(nativePtr != 0)
			commitTextNative (nativePtr, text.toString ());
		return true;
	}

	@Override
	public boolean setComposingText (CharSequence text, int newCursorPosition)
	{
		if(nativePtr != 0)
			setComposingTextNative (nativePtr, text.toString (), newCursorPosition);
		return true;
	}

	@Override
	public boolean finishComposingText ()
	{
		if(nativePtr != 0)
			finishComposingNative (nativePtr);
		return true;
	}

	@Override
	public boolean deleteSurroundingText (int beforeLength, int afterLength)
	{
		if(nativePtr != 0)
			deleteSurroundingTextNative (nativePtr, beforeLength, afterLength);
		return true;
	}

	@Override
	public boolean performEditorAction (int editorAction)
	{
		// Translate "Done" / "Go" / "Search" / "Next" actions to Enter
		// key events, matching the behavior on other platforms.
		view.dispatchKeyEvent (new KeyEvent (KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
		view.dispatchKeyEvent (new KeyEvent (KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER));
		return true;
	}

	// -----------------------------------------------------------------------------------------
	// Native methods - implemented in inputmethod.android.cpp
	// -----------------------------------------------------------------------------------------

	private native String getTextBeforeCursorNative (long nativePtr, int maxChars);
	private native String getTextAfterCursorNative (long nativePtr, int maxChars);
	private native String getSelectedTextNative (long nativePtr);
	private native int getSelectionStartNative (long nativePtr);
	private native int getSelectionEndNative (long nativePtr);
	private native void commitTextNative (long nativePtr, String text);
	private native void setComposingTextNative (long nativePtr, String text, int cursorPos);
	private native void deleteSurroundingTextNative (long nativePtr, int beforeLength, int afterLength);
	private native void finishComposingNative (long nativePtr);
	private native int getInputTypeNative (long nativePtr);
	private native int getInputFlagsNative (long nativePtr);
}
