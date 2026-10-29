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
// Filename    : ccl/platform/android/gui/inputmethod.android.cpp
// Description : Android Input Method
//
//************************************************************************************************

#include "ccl/gui/system/inputmethod.h"

#include "ccl/platform/android/cclandroidjni.h"

#include "ccl/platform/android/gui/androidview.h"
#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/interfaces/jni/androidcontent.h"

#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iwindow.h"
#include "ccl/public/gui/graphics/dpiscale.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.cclgui.InputMethod
//************************************************************************************************

DECLARE_JNI_CLASS (InputMethod, CCLGUI_CLASS_PREFIX "InputMethod")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject, JniIntPtr)
	DECLARE_JNI_METHOD (void, disconnect)
	DECLARE_JNI_METHOD (void, invalidateInput)
	DECLARE_JNI_METHOD (void, updateSelection, int, int)
	DECLARE_JNI_METHOD (void, updateCursor, int, int, int, int)
END_DECLARE_JNI_CLASS (InputMethod)

DEFINE_JNI_CLASS (InputMethod)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Landroid/view/View;J)V")
	DEFINE_JNI_METHOD (disconnect, "()V")
	DEFINE_JNI_METHOD (invalidateInput, "()V")
	DEFINE_JNI_METHOD (updateSelection, "(II)V")
	DEFINE_JNI_METHOD (updateCursor, "(IIII)V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

namespace CCL {

//************************************************************************************************
// AndroidInputMethod
//************************************************************************************************

class AndroidInputMethod: public Object,
						  public IInputMethod
{
public:
	AndroidInputMethod (ITextInputClient* client);
	~AndroidInputMethod ();

	// IInputMethod
	tresult CCL_API enable (tbool state) override;
	void CCL_API textChanged () override;
	void CCL_API selectionChanged () override;
	void CCL_API visibleRectChanged () override;

	// called from Java via JNI
	jstring onGetTextBeforeCursor (int maxChars);
	jstring onGetTextAfterCursor (int maxChars);
	jstring onGetSelectedText ();
	void onGetSelection (int& selStart, int& selEnd);
	void onCommitText (jstring text);
	void onSetComposingText (jstring text, int cursorPos);
	void onDeleteSurroundingText (int beforeLength, int afterLength);
	void onFinishComposing ();
	int getInputType ();
	int getInputFlags ();

	CLASS_INTERFACE (IInputMethod, Object)

private:
	SharedPtr<ITextInputClient> client;
	Android::JniObject inputMethod;
	bool updatingPreeditText;

	jobject getFrameworkViewObject () const;
	jobject getSystemIMM () const;
};

//************************************************************************************************
// AndroidInputMethodManager
//************************************************************************************************

class AndroidInputMethodManager: public InputMethodManager
{
public:
	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;
};

} // namespace CCL

using namespace CCL;
using namespace CCL::Android;

//************************************************************************************************
// AndroidInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, AndroidInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* AndroidInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW AndroidInputMethod (&client);
}

//************************************************************************************************
// AndroidInputMethod
//************************************************************************************************

AndroidInputMethod::AndroidInputMethod (ITextInputClient* client)
: client (client),
  updatingPreeditText (false)
{
	if(jobject view = getFrameworkViewObject ())
	{
		JniAccessor jni;
		inputMethod.assign (jni, jni.newObject (InputMethod, InputMethod.construct, view, JniIntPtr(this)));
		FrameworkViewClass.setActiveInputMethod (view, inputMethod.getJObject ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidInputMethod::~AndroidInputMethod ()
{
	enable (false);

	// unregister from the view
	if(jobject view = getFrameworkViewObject ())
	{
		FrameworkViewClass.setActiveInputMethod (view, nullptr);
		InputMethod.disconnect (inputMethod);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidInputMethod::getFrameworkViewObject () const
{
	const IView* view = client->getInputView ();
	if(!view)
		return nullptr;

	const IWindow* window = view->getIWindow ();
	if(!window)
		return nullptr;

	AndroidWindow* androidWindow = AndroidWindow::cast (unknown_cast<Window> (window));
	if(!androidWindow)
		return nullptr;

	Android::FrameworkView* frameworkView = androidWindow->getFrameworkView ();
	if(!frameworkView)
		return nullptr;

	return frameworkView->getJObject ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jobject AndroidInputMethod::getSystemIMM () const
{
	jobject activity = FrameworkActivity::getCurrentActivity ()->getJObject ();
	if(!activity)
		return nullptr;

	// Look up the INPUT_METHOD_SERVICE constant from android.content.Context
	JniAccessor jni;
	jstring serviceName = jobject_cast<jstring> (jni.getStaticField (Context, "INPUT_METHOD_SERVICE", "Ljava/lang/String;"));

	return Context.getSystemService (activity, serviceName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API AndroidInputMethod::enable (tbool state)
{
	if(!inputMethod.isValid ())
		return kResultFailed;

	jobject view = getFrameworkViewObject ();
	if(!view)
		return kResultFailed;

	JniAccessor jni;
	LocalRef imm (jni, getSystemIMM ());
	if(!imm)
		return kResultFailed;

	if(state)
	{
		Android::InputMethodManager.showSoftInput (imm, view, 0);
		InputMethod.invalidateInput (inputMethod);
	}
	else
	{
		LocalRef token (jni, AndroidView.getWindowToken (view));
		if(token)
			Android::InputMethodManager.hideSoftInputFromWindow (imm, token.getJObject (), 0);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidInputMethod::textChanged ()
{
	if(!inputMethod.isValid ())
		return;

	if(updatingPreeditText)
		return;

	// invalidate input so the IME re-reads the text state
	InputMethod.invalidateInput (inputMethod);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidInputMethod::selectionChanged ()
{
	if(!inputMethod.isValid ())
		return;

	int caret = 0, selection = 0;
	client->getSelection (caret, selection);

	InputMethod.updateSelection (inputMethod, selection, caret);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API AndroidInputMethod::visibleRectChanged ()
{
	if(!inputMethod.isValid ())
		return;

	Rect caretRect;
	if(client->getCaretRect (caretRect) != kResultOk)
		return;

	Point leftTop = caretRect.getLeftTop ();
	Point rightBottom = caretRect.getRightBottom ();

	if(const IView* view = client->getInputView ())
	{
		view->clientToWindow (leftTop);
		view->clientToWindow (rightBottom);
	}

	float scaleFactor = 1.f;
	if(const IView* view = client->getInputView ())
		if(const IWindow* window = view->getIWindow ())
			scaleFactor = window->getContentScaleFactor ();

	DpiScale::toPixelPoint (leftTop, scaleFactor);
	DpiScale::toPixelPoint (rightBottom, scaleFactor);

	InputMethod.updateCursor (inputMethod, leftTop.x, leftTop.y, rightBottom.x, rightBottom.y);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jstring AndroidInputMethod::onGetTextBeforeCursor (int maxChars)
{
	int caret = 0, selection = 0;
	client->getSelection (caret, selection);

	int end = ccl_min (caret, selection);
	int start = ccl_max (0, end - maxChars);

	String text;
	client->getText (text, start, end - start);

	JniAccessor jni;
	JniCCLString jstr (text);
	return jstring(jni->NewLocalRef (jstr.getString ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jstring AndroidInputMethod::onGetTextAfterCursor (int maxChars)
{
	int caret = 0, selection = 0;
	client->getSelection (caret, selection);

	int start = ccl_max (caret, selection);
	int totalLength = client->getTextLength ();
	int count = ccl_min (maxChars, totalLength - start);

	String text;
	if(count > 0)
		client->getText (text, start, count);

	JniAccessor jni;
	JniCCLString jstr (text);
	return jstring(jni->NewLocalRef (jstr.getString ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

jstring AndroidInputMethod::onGetSelectedText ()
{
	int caret = 0, selection = 0;
	client->getSelection (caret, selection);

	int start = ccl_min (caret, selection);
	int end = ccl_max (caret, selection);

	if(end <= start)
		return nullptr;

	String text;
	client->getText (text, start, end - start);

	JniAccessor jni;
	JniCCLString jstr (text);
	return jstring(jni->NewLocalRef (jstr.getString ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidInputMethod::onGetSelection (int& selStart, int& selEnd)
{
	client->getSelection (selEnd, selStart, ITextInputClient::kSelectionMerged);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidInputMethod::onCommitText (jstring jtext)
{
	JniAccessor jni;
	String text;
	fromJavaString (text, jni, jtext);

	client->setPreeditText (String::kEmpty);
	client->applyText (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidInputMethod::onSetComposingText (jstring jtext, int cursorPos)
{
	ScopedVar<bool> guard (updatingPreeditText, true);

	JniAccessor jni;
	String text;
	fromJavaString (text, jni, jtext);

	int caret = 0, selection = 0;
	client->getSelection (caret, selection);
	client->replaceText (String::kEmpty, ccl_min (caret, selection), ccl_abs (caret - selection));
	client->setPreeditText (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidInputMethod::onDeleteSurroundingText (int beforeLength, int afterLength)
{
	client->deleteText (beforeLength, afterLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidInputMethod::onFinishComposing ()
{
	client->setPreeditText (String::kEmpty);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidInputMethod::getInputType ()
{
	return client->getInputType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidInputMethod::getInputFlags ()
{
	return client->getInputFlags ();
}

//************************************************************************************************
// InputMethod Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (jstring, InputMethod, getTextBeforeCursorNative, JniIntPtr nativePtr, jint maxChars)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		return inputMethod->onGetTextBeforeCursor ((int)maxChars);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jstring, InputMethod, getTextAfterCursorNative, JniIntPtr nativePtr, jint maxChars)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		return inputMethod->onGetTextAfterCursor ((int)maxChars);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jstring, InputMethod, getSelectedTextNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		return inputMethod->onGetSelectedText ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jint, InputMethod, getSelectionStartNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
	{
		int selStart = 0, selEnd = 0;
		inputMethod->onGetSelection (selStart, selEnd);
		return selStart;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jint, InputMethod, getSelectionEndNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
	{
		int selStart = 0, selEnd = 0;
		inputMethod->onGetSelection (selStart, selEnd);
		return selEnd;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, InputMethod, commitTextNative, JniIntPtr nativePtr, jstring text)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		inputMethod->onCommitText (text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, InputMethod, setComposingTextNative, JniIntPtr nativePtr, jstring text, jint cursorPos)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		inputMethod->onSetComposingText (text, (int)cursorPos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, InputMethod, deleteSurroundingTextNative, JniIntPtr nativePtr, jint beforeLength, jint afterLength)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		inputMethod->onDeleteSurroundingText ((int)beforeLength, (int)afterLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, InputMethod, finishComposingNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		inputMethod->onFinishComposing ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jint, InputMethod, getInputTypeNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		return inputMethod->getInputType ();
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (jint, InputMethod, getInputFlagsNative, JniIntPtr nativePtr)
{
	if(AndroidInputMethod* inputMethod = JniCast<AndroidInputMethod>::fromIntPtr (nativePtr))
		return inputMethod->getInputFlags ();
	return 0;
}
