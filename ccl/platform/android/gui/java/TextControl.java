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
// Filename    : ccl/platform/android/gui/java/TextControl.java
// Description : TextControl
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.core.CoreGuiConstants;

import android.widget.EditText;
import android.widget.TextView;
import android.text.*;
import android.view.*;
import android.view.inputmethod.*;
import android.graphics.*;
import android.content.Context;
import android.os.*;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

//************************************************************************************************
// TextControl
//************************************************************************************************

@Keep
public class TextControl extends EditText
						 implements TextWatcher,
									TextView.OnEditorActionListener 

{
	private final FrameworkView parentView;
	private long nativeTextControlPtr;
	private int width = 0;
	private int height = 0;
	private int alignment = CoreGuiConstants.kAlignmentLeft;
	private int hScrollRange = 0;
	private int vScrollRange = 0;

	private InputMethodManager getImm ()
	{
		return (InputMethodManager) getContext ().getSystemService (Context.INPUT_METHOD_SERVICE);
	}
	
	public TextControl (Context context, long nativeTextControlPtr, FrameworkView parentView, int options, int keyboardType)
	{
		super (context);
		this.parentView = parentView;
		this.nativeTextControlPtr = nativeTextControlPtr;
		
		int inputType = InputType.TYPE_CLASS_TEXT;
		switch(keyboardType)
		{
		case ControlStyles.kKeyboardTypeEmail :
			inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS;
			break;
		case ControlStyles.kKeyboardTypeUrl :
			inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI;
			break;
		case ControlStyles.kKeyboardTypePhoneNumber :
			inputType = InputType.TYPE_CLASS_PHONE;
			break;
		case ControlStyles.kKeyboardTypeNumeric :
			inputType = InputType.TYPE_CLASS_NUMBER;
			break;
		case ControlStyles.kKeyboardTypeNumericSigned :
			inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_SIGNED;
			break;
		case ControlStyles.kKeyboardTypeDecimal :
			inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL;
			break;
		case ControlStyles.kKeyboardTypeDecimalSigned :
			inputType = InputType.TYPE_CLASS_NUMBER | InputType.TYPE_NUMBER_FLAG_DECIMAL | InputType.TYPE_NUMBER_FLAG_SIGNED;
			break;
		}
		
		if((options & ControlStyles.kTextBoxAppearanceMultiLine) != 0)
			inputType |= InputType.TYPE_TEXT_FLAG_MULTI_LINE;
		if((options & ControlStyles.kTextBoxBehaviorPasswordEdit) != 0)
			inputType |= InputType.TYPE_TEXT_VARIATION_PASSWORD;
		if((options & ControlStyles.kEditBoxBehaviorNoSuggestions) != 0)
			inputType |= InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS;
		
		setInputType (inputType);

		if(parentView instanceof DialogView)
		{
			Window window = ((DialogView)parentView).getDialogWindow ();
			if(window != null)
				window.setSoftInputMode (WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
		}
		
		setHorizontallyScrolling (true);

		addTextChangedListener (this);
		setOnEditorActionListener (this);
    }

	public void show ()
	{
		parentView.addView (this);
	}

	public void remove ()
	{
		nativeTextControlPtr = 0; // called from ~AndroidTextControl (C++ destructor): prevent further native calls (e.g. in case the keyboard does not disappear despite the following code)

		hideKeyboard ();

		parentView.removeView (this);

		FrameworkActivity activity = (FrameworkActivity)getContext ();
		activity.refreshSystemUiVisibility ();
	}

	public void showKeyboard ()
	{
		getImm ().showSoftInput (this, 0,
			new ResultReceiver (null)
			{
				@Override
				protected void onReceiveResult(int resultCode, Bundle resultData)
				{
					// show navigation buttons while keyboard is visible
					if(resultCode == InputMethodManager.RESULT_SHOWN)
						parentView.setSystemUiVisibility (parentView.getSystemUiVisibility () & ~View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
				}
			});
	}

	public void hideKeyboard ()
	{
		getImm ().hideSoftInputFromWindow (getWindowToken (), 0);

		parentView.refreshSystemUiVisibility ();
	}

	public void updateText (String text)
	{
		setText (text, TextView.BufferType.EDITABLE);
	}

	public String getControlText ()
	{
		Editable editable = getText ();
		return editable.toString ();
	}

	public void setSelectionRange (int start, int length)
	{
		if(start == -1)
			setSelected (false);
		else
		{
			int end = Math.min (start + length, length ());
			if(length == -1)
				end = length ();

			setSelection (start, end);
		}
	}

	public void setSize (int left, int top, int width, int height)
	{
		this.width = width;
		this.height = height;

		setPadding (0, 0, 0, 0);
		layout (left, top, left + width, top + height);

		hScrollRange = 0;
		vScrollRange = 0;
	}

	public void setVisualStyle (Typeface typeface, float fontSize, int textColor, int backColor, int alignment)
	{
		FrameworkActivity activity = (FrameworkActivity)getContext ();

		setTypeface (typeface);

		// TextView expects font size in dp, so convert here
		setTextSize (fontSize * 160.f / activity.getDensity ());

		setTextColor (textColor);
		setBackgroundColor (backColor);

		switch(alignment & CoreGuiConstants.kAlignmentHMask)
		{
			case CoreGuiConstants.kAlignmentHCenter :
				setGravity (Gravity.CENTER_HORIZONTAL);
				break;
			case CoreGuiConstants.kAlignmentRight :
				setGravity (Gravity.RIGHT);
				break;
			default :
				setGravity (Gravity.LEFT);
				break;
		}

		this.alignment = alignment;
    }

	@Override
    protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec)
	{
		setMeasuredDimension (width, height);
    }

	@Override
	protected void onDetachedFromWindow ()
	{
		hideKeyboard ();

		super.onDetachedFromWindow ();
	}

	@Override
    protected void onVisibilityChanged (View view, int visibility)
	{
		if(visibility == View.VISIBLE)
			requestFocus ();

		// hide keyboard when view is hidden
		if(visibility == View.INVISIBLE || visibility == View.GONE)
			hideKeyboard ();
    }

	@Override
	public boolean onPreDraw ()
	{
		int newHScrollRange = computeHorizontalScrollRange ();
		int newVScrollRange = computeVerticalScrollRange ();

		if(newHScrollRange == hScrollRange && newVScrollRange == vScrollRange)
			return true;
			
		int hScrollOffset = computeHorizontalScrollOffset ();
		int vScrollOffset = computeVerticalScrollOffset ();

		if(newHScrollRange != hScrollRange)
		{
			hScrollRange = computeHorizontalScrollRange ();

			switch(alignment & CoreGuiConstants.kAlignmentHMask)
			{
				default :
				case CoreGuiConstants.kAlignmentLeft :
					hScrollOffset = 0;
					break;
				case CoreGuiConstants.kAlignmentHCenter :
					hScrollOffset = (hScrollRange - computeHorizontalScrollExtent ()) / 2;
					break;
				case CoreGuiConstants.kAlignmentRight :
					hScrollOffset = hScrollRange - computeHorizontalScrollExtent ();
					break;
			}
		}

		if(newVScrollRange != vScrollRange)
		{
			vScrollRange = computeVerticalScrollRange ();

			switch(alignment & CoreGuiConstants.kAlignmentVMask)
			{
				default :
				case CoreGuiConstants.kAlignmentTop :
					vScrollOffset = 0;
					break;
				case CoreGuiConstants.kAlignmentVCenter :
					vScrollOffset = (vScrollRange - computeVerticalScrollExtent ()) / 2;
					break;
				case CoreGuiConstants.kAlignmentBottom :
					vScrollOffset = vScrollRange - computeVerticalScrollExtent ();
					break;
			}
		}

		scrollTo (hScrollOffset, vScrollOffset);
		return true;
	}


	@Override
    public void onDrawForeground (@NonNull Canvas canvas)
	{
		super.onDrawForeground (canvas);

		// show keyboard
		if(isFocused ())
			showKeyboard ();
	}

	@Override
    public boolean onKeyPreIme (int keyCode, KeyEvent event)
	{
		// the "Back" key hides the keyboard: give up focus
        if(event.getKeyCode () == KeyEvent.KEYCODE_BACK && nativeTextControlPtr != 0)
		{
			onKillFocusNative (nativeTextControlPtr);
			return true;
		}

        return super.onKeyPreIme (keyCode, event);
    }

	// TextWatcher
	@Override
	public void	afterTextChanged(Editable s)
	{
			onTextChangedNative (nativeTextControlPtr);
	}

	@Override
	public void	beforeTextChanged(CharSequence s, int start, int count, int after)
	{}

	@Override
	public void	onTextChanged(CharSequence s, int start, int before, int count)
	{}

	// TextView.OnEditorActionListener
	@Override
	public boolean onEditorAction (TextView v, int actionId, KeyEvent event)
	{
		//CCL.log ("TextControl.onEditorAction: " + actionId);

		// give up focus on when "done" key is pressed
		if(actionId == EditorInfo.IME_ACTION_DONE
			|| actionId == EditorInfo.IME_ACTION_GO
			|| actionId == EditorInfo.IME_ACTION_NEXT
			|| actionId == EditorInfo.IME_ACTION_SEARCH
			|| actionId == EditorInfo.IME_ACTION_SEND)
			onKillFocusNative (nativeTextControlPtr);
		return false;
	}

	public native void onKillFocusNative (long nativeTextControlPtr);
	public native void onTextChangedNative (long nativeTextControlPtr);
}
