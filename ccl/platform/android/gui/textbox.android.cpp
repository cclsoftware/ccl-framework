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
// Filename    : ccl/platform/android/gui/textbox.android.cpp
// Description : Platform-specific Text Control implementation
//
//************************************************************************************************

#include "ccl/gui/controls/editbox.h"

#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/public/gui/iparameter.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/base/message.h"

#include "ccl/platform/android/gui/frameworkactivity.h"
#include "ccl/platform/android/gui/frameworkview.h"
#include "ccl/platform/android/gui/window.android.h"

#include "ccl/platform/android/graphics/frameworkgraphics.h"

namespace CCL {
namespace Android {

//************************************************************************************************
// dev.ccl.TextControl
//************************************************************************************************

DECLARE_JNI_CLASS (TextControl, CCLGUI_CLASS_PREFIX "TextControl")
	DECLARE_JNI_CONSTRUCTOR (construct, jobject, JniIntPtr, jobject, int, int)
	DECLARE_JNI_METHOD (void, show)
	DECLARE_JNI_METHOD (void, remove)
	DECLARE_JNI_METHOD (void, updateText, jstring)
	DECLARE_JNI_METHOD (jstring, getControlText)
	DECLARE_JNI_METHOD (void, setSelectionRange, int, int)
	DECLARE_JNI_METHOD (void, setSize, int, int, int, int)
	DECLARE_JNI_METHOD (void, setVisualStyle, jobject, float, int, int, int)
END_DECLARE_JNI_CLASS (TextControl)

DEFINE_JNI_CLASS (TextControl)
	DEFINE_JNI_CONSTRUCTOR (construct, "(Landroid/content/Context;JL" CCLGUI_CLASS_PREFIX "FrameworkView;II)V")
	DEFINE_JNI_METHOD (show, "()V")
	DEFINE_JNI_METHOD (remove, "()V")
	DEFINE_JNI_METHOD (updateText, "(Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (getControlText, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (setSelectionRange, "(II)V")
	DEFINE_JNI_METHOD (setSize, "(IIII)V")
	DEFINE_JNI_METHOD (setVisualStyle, "(Landroid/graphics/Typeface;FIII)V")
END_DEFINE_JNI_CLASS

} // namespace Android
} // namespace CCL

namespace CCL {

//************************************************************************************************
// AndroidTextControl
//************************************************************************************************

class AndroidTextControl: public NativeTextControl
{
public:
	AndroidTextControl (Control& owner, RectRef clientRect, int returnKeyType, int keyboardType);
	~AndroidTextControl ();

	// NativeTextControl
	void updateText () override;
	void getControlText (String& string) override;
	void setSelection (int start, int length) override;
	void setSize (RectRef clientRect) override;
	void updateVisualStyle () override;

private:
	Android::JniObject textControl;
};

} // namespace CCL

using namespace CCL;
using namespace Android;

//************************************************************************************************
// NativeTextControl
//************************************************************************************************

NativeTextControl* NativeTextControl::create (Control& owner, RectRef clientRect, int returnKeyType, int keyboardType)
{
	return NEW AndroidTextControl (owner, clientRect, returnKeyType, keyboardType);
}

//************************************************************************************************
// AndroidTextControl
//************************************************************************************************

AndroidTextControl::AndroidTextControl (Control& owner, RectRef clientRect, int returnKeyType, int keyboardType)
: NativeTextControl (owner, returnKeyType, keyboardType)
{
	// owning control must be attached!
	AndroidWindow* w = AndroidWindow::cast (owner.getWindow ());
	FrameworkView* frameworkView = w ? w->getFrameworkView () : 0;
	ASSERT (frameworkView)
	if(!frameworkView)
		return;

	JniAccessor jni;
	textControl.assign (jni, jni.newObject (TextControl, TextControl.construct, FrameworkActivity::getCurrentActivity ()->getJObject (), (JniIntPtr)this, frameworkView->getJObject (), owner.getStyle ().custom, keyboardType));

	updateVisualStyle ();
	updateText ();
	setSize (clientRect);

	canceled = false;

	TextControl.show (textControl);

	if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
		setSelection (0, -1); // select all

	owner.takeFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTextControl::~AndroidTextControl ()
{
	cancelSignals ();

	TextControl.remove (textControl);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextControl::updateText ()
{
	String text;
	IParameter* p = getTextParameter ();
	if(p)	
		p->toString (text);

	JniCCLString string (text);
	TextControl.updateText (textControl, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextControl::getControlText (String& string)
{
	JniAccessor jni;
	LocalStringRef text (jni, TextControl.getControlText (textControl));
	fromJavaString (string, text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextControl::setSelection (int start, int length)
{
	TextControl.setSelectionRange (textControl, start, length);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextControl::setSize (RectRef clientRect)
{
	if(!textControl.isValid ())
		return;

	Rect rect (clientRect);
	if(!owner.getStyle ().isCustomStyle (Styles::kTextBoxAppearanceMultiLine))
	{
		const IVisualStyle& visualStyle = getVisualStyle ();

		Rect stringSize;
		Font::measureString (stringSize, "Xgjpq", visualStyle.getTextFont ()); // todo: add any taller character you know

		// add margin for system-side decorations like underlines and
		// center the minHeight rect vertically with rounding (up)
		Coord minHeight = ceil (stringSize.bottom * 1.10);

		rect.top += (Coord) ccl_round<0> (rect.getHeight () / 2.f) - (Coord) ccl_round<0> (minHeight / 2.f);
		rect.setHeight (minHeight);
	}

	Point offset;
	owner.clientToWindow (offset);
	rect.offset (offset);

	if(owner.getStyle ().isCommonStyle (Styles::kBorder))
		rect.contract (1);

	AndroidWindow* w = AndroidWindow::cast (owner.getWindow ());
	DpiScale::toPixelRect (rect, w->getContentScaleFactor ());

	TextControl.setSize (textControl, rect.left, rect.top, rect.getWidth (), rect.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextControl::updateVisualStyle ()
{
	if(!textControl.isValid ())
		return;

	const IVisualStyle& visualStyle = getVisualStyle ();
	Font font = visualStyle.getTextFont ();
	FrameworkGraphics::FontHelper f (font);
	AndroidWindow* w = AndroidWindow::cast (owner.getWindow ());

	TextControl.setVisualStyle (textControl, f.typeface, font.getSize () * w->getContentScaleFactor (),
		FrameworkGraphics::toJavaColor (visualStyle.getTextColor ()), FrameworkGraphics::toJavaColor (visualStyle.getBackColor ().setAlphaF (1.0)),
		visualStyle.getTextAlignment ().align);
}

//************************************************************************************************
// dev.ccl.TextControl Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, TextControl, onKillFocusNative, JniIntPtr nativeTextControlPtr)
{
	AndroidTextControl* textControl = JniCast<AndroidTextControl>::fromIntPtr (nativeTextControlPtr);
	if(textControl)
		textControl->getOwner ().killFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CCLGUI (void, TextControl, onTextChangedNative, JniIntPtr nativeTextControlPtr)
{
	AndroidTextControl* textControl = JniCast<AndroidTextControl>::fromIntPtr (nativeTextControlPtr);
	if(textControl)
		if(textControl->isImmediateUpdate ())
			(NEW Message ("checkSubmit"))->post (textControl);
}
