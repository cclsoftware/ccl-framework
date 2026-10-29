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
// Filename    : ccl/gui/system/inputmethod.h
// Description : Input Method Support
//
//************************************************************************************************

#ifndef _ccl_inputmethod_h
#define _ccl_inputmethod_h

#include "ccl/base/singleton.h"

#include "ccl/public/gui/graphics/rect.h"

#include "ccl/meta/generated/cpp/gui-constants-generated.h"

namespace CCL {

interface IView;
interface IMutableRegion;

//************************************************************************************************
// IInputMethod
//************************************************************************************************

interface IInputMethod: IUnknown
{
	/** Enable or disable the input method. */
	virtual tresult CCL_API enable (tbool state) = 0;

	/** Notify that text changed. */
	virtual void CCL_API textChanged () = 0;

	/** Notify that caret position or selection changed. */
	virtual void CCL_API selectionChanged () = 0;

	/** Notify that character screen coordinates changed, e.g. by scrolling. */
	virtual void CCL_API visibleRectChanged () = 0;

	DECLARE_IID (IInputMethod)
};

//************************************************************************************************
// ITextInputClient
//************************************************************************************************

interface ITextInputClient: IUnknown
{
	enum InputFlags
	{
		kSensitiveData = kTextInputFlagSensitiveData,
		kMultiline = kTextInputFlagMultiline
	};

	DEFINE_ENUM (SelectionDomain)
	{
		kSelectionOriginal = 0,	///< selection within committed text, ignoring preedit
		kSelectionPreedit,		///< selection within preedit text
		kSelectionMerged		///< selection within full text (committed merged with preedit) 
	};

	/** Get the view owning this input client. */
	virtual const IView* CCL_API getInputView () const = 0;

	/** Get the full length of editable text. */
	virtual int CCL_API getTextLength () const = 0;

	/** Get the text at given range. */
	virtual tresult CCL_API getText (String& result, int start = 0, int length = -1) const = 0;

	/** Get caret and selection positions. */
	virtual void CCL_API getSelection (int& caret, int& selection, SelectionDomain selectionDomain = kSelectionOriginal) const = 0;

	/** Set caret and selection positions. */
	virtual void CCL_API setSelection (int caret, int selection) = 0;

	/** Get caret rect relative to owning view. */
	virtual tresult CCL_API getCaretRect (Rect& caretRect) const = 0;

	/** Get input type. \return One of \a Styles::KeyboardTypes. */
	virtual int CCL_API getInputType () const = 0;

	/** Get input flags. */
	virtual int CCL_API getInputFlags () const = 0;

	/** Get preedit text. */
	virtual void CCL_API getPreeditText (String& text) const = 0;

	/** Set preedit text. */
	virtual void CCL_API setPreeditText (StringRef text) = 0;

	/** Apply text. */
	virtual void CCL_API applyText (StringRef text) = 0;

	/** Delete text before and after the caret. */
	virtual void CCL_API deleteText (int beforeLength, int afterLength) = 0;

	/** Replace text in a given range. */
	virtual void CCL_API replaceText (StringRef text, int start, int length) = 0;

	/** Get bounds of a text range relative to owning view. */
	virtual void CCL_API getTextBounds (IMutableRegion& bounds, int start, int length) = 0;

	DECLARE_IID (IInputMethod)
};

//************************************************************************************************
// InputMethodManager
//************************************************************************************************

class InputMethodManager: public Object,
						  public ExternalSingleton<InputMethodManager>
{
public:
	/** Create an input method. */
	virtual IInputMethod* createInputMethod (ITextInputClient& client);
};

} // namespace CCL

#endif // _ccl_inputmethod_h
