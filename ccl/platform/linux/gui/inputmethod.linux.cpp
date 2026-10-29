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
// Filename    : ccl/platform/linux/gui/inputmethod.linux.cpp
// Description : Linux Input Method
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/system/inputmethod.h"
#include "ccl/gui/views/view.h"

#include "ccl/platform/linux/wayland/waylandclient.h"

#include "ccl/public/text/cclstring.h"

#include "core/text/coreutfcodec.h"

namespace CCL {

//************************************************************************************************
// LinuxInputMethod
//************************************************************************************************

class LinuxInputMethod: public Object,
						public Linux::WaylandObject,
						public IInputMethod
{
public:
	LinuxInputMethod (ITextInputClient* client);
	~LinuxInputMethod ();

	void commit (CStringRef text, int deleteBefore, int deleteAfter);
	void setPreeditText (CStringRef text, int selection, int caret);

	// IInputMethod
	tresult CCL_API enable (tbool state) override;
	void CCL_API textChanged () override;
	void CCL_API selectionChanged () override;
	void CCL_API visibleRectChanged () override;

	// WaylandObject
	void onCompositorDisconnected () override;
	void onCompositorConnected () override;

	CLASS_INTERFACE (IInputMethod, Object)

protected:
	static const int kMaxTextLength = 4000; // according to interface documentation
	static const int kMaxCodePointLength = 4;

	struct Listener: zwp_text_input_v3_listener
	{
		Listener (LinuxInputMethod& inputMethod);

		static void onEnter (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface);
		static void onLeave (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface);
		static void onPreeditString (void* data, zwp_text_input_v3* textInput, CStringPtr text, int32_t cursorBegin, int32_t cursorEnd);
		static void onCommitString (void* data, zwp_text_input_v3* textInput, CStringPtr text);
		static void onDeleteSurroundingText (void* data, zwp_text_input_v3* textInput, uint32_t beforeLength, uint32_t afterLength);
		static void onDone (void* data, zwp_text_input_v3* textInput, uint32_t serial);

	private:
		LinuxInputMethod& inputMethod;

		MutableCString commitString;
		MutableCString preeditString;
		int cursorBegin;
		int cursorEnd;
		int deleteBefore;
		int deleteAfter;
	};
	Listener listener;

	SharedPtr<ITextInputClient> client;
	zwp_text_input_v3* textInput;

	Vector<int> utf8Positions;
	int offset;
	int caretUtf8Index;
	int selectionUtf8Index;
	int savedCaretIndex;
	int savedSelectionIndex;

	void initialize ();
	void terminate ();
	void configureInputMethod ();
	void updateSurroundingText ();
	void updateCaretRect ();
	void updateInputType ();
	int getUtf16Position (int utf8Position) const;
};

//************************************************************************************************
// LinuxInputMethodManager
//************************************************************************************************

class LinuxInputMethodManager: public InputMethodManager
{
public:
	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, LinuxInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* LinuxInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW LinuxInputMethod (&client);
}

//************************************************************************************************
// LinuxInputMethod
//************************************************************************************************

LinuxInputMethod::LinuxInputMethod (ITextInputClient* client)
: listener (*this),
  client (client),
  textInput (nullptr),
  offset (0),
  caretUtf8Index (-1),
  selectionUtf8Index (-1),
  savedCaretIndex (-1),
  savedSelectionIndex (-1)
{
	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxInputMethod::~LinuxInputMethod ()
{
	terminate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxInputMethod::enable (tbool state)
{
	if(textInput)
	{
		if(state)
			zwp_text_input_v3_enable (textInput);
		else
			zwp_text_input_v3_disable (textInput);
		return kResultOk;
	}
	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxInputMethod::textChanged ()
{
	if(textInput)
		zwp_text_input_v3_set_text_change_cause (textInput, ZWP_TEXT_INPUT_V3_CHANGE_CAUSE_OTHER);
	configureInputMethod ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxInputMethod::selectionChanged ()
{
	int caretIndex = 0;
	int selectionIndex = 0;
	client->getSelection (caretIndex, selectionIndex, ITextInputClient::kSelectionOriginal);
	if(caretIndex != savedCaretIndex || selectionIndex != savedSelectionIndex)
		configureInputMethod ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxInputMethod::visibleRectChanged ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::configureInputMethod ()
{
	updateSurroundingText ();
	updateCaretRect ();
	updateInputType ();
	if(textInput)
		zwp_text_input_v3_commit (textInput);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::updateSurroundingText ()
{
	if(textInput == nullptr)
		return;

	int caretIndex = 0;
	int selectionIndex = 0;
	client->getSelection (caretIndex, selectionIndex, ITextInputClient::kSelectionOriginal);
	if(caretIndex < 0)
		caretIndex = 0;
	if(selectionIndex < 0)
		selectionIndex = caretIndex;

	CCL_PRINTF ("selection: %d, %d\n", caretIndex, selectionIndex)

	int totalLength = client->getTextLength ();
	int start = 0;
	int length = totalLength;
	if(totalLength > kMaxTextLength)
	{
		int center = caretIndex != selectionIndex ? selectionIndex + (caretIndex - selectionIndex) / 2 : caretIndex;
		start = ccl_max (center - kMaxTextLength / 2, 0);
		length = ccl_min (kMaxTextLength, totalLength - start);
		caretIndex = ccl_max (0, caretIndex - start);
		selectionIndex = ccl_max (0, selectionIndex - start);
	}

	String text;
	if(client->getText (text, start, length) != kResultOk)
		return;
	offset = start;

	utf8Positions.empty ();
	utf8Positions.resize (text.length () + 1);
	StringChars characters (text);
	Core::Text::UTF16Reader reader (characters, text.length ());
	unsigned char uCharBuffer[kMaxCodePointLength + 1] = {0};

	int currentLength = 0;
	uchar32 codePoint = 0;
	int bytesUsed = 0;
	while(utf8Positions.count () <= ccl_max (caretIndex, selectionIndex) && (codePoint = reader.getNext (&bytesUsed)))
	{
		utf8Positions.add (currentLength);
		currentLength += Core::Text::UTFCodec::encodeUTF8 (codePoint, uCharBuffer, kMaxCodePointLength);
		for(int i = 2; i < bytesUsed; i += 2)
			utf8Positions.add (currentLength);
	}
	utf8Positions.add (currentLength);

	caretUtf8Index = utf8Positions.at (caretIndex);
	selectionUtf8Index = utf8Positions.at (selectionIndex);

	savedCaretIndex = caretIndex;
	savedSelectionIndex = selectionIndex;

	MutableCString utf8Text (text, Text::kUTF8);

	CCL_PRINTF ("utf8 text length: %d\n", utf8Text.length ())

	if(utf8Text.length () > kMaxTextLength)
	{
		utf8Text.truncate (kMaxTextLength);
		while(caretUtf8Index > kMaxTextLength)
			caretUtf8Index = utf8Positions.at (--caretIndex);
		while(selectionUtf8Index > kMaxTextLength)
			selectionUtf8Index = utf8Positions.at (--selectionIndex);
	}

	CCL_PRINTF ("truncated utf8 text length: %d\n", utf8Text.length ())
	CCL_PRINTF ("utf8 selection: %d, %d\n", caretUtf8Index, selectionUtf8Index)

	zwp_text_input_v3_set_surrounding_text (textInput, utf8Text, caretUtf8Index, selectionUtf8Index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int LinuxInputMethod::getUtf16Position (int utf8Position) const
{
	int position = 0;
	for(; position < utf8Positions.count (); position++)
	{
		if(utf8Positions.at (position) >= utf8Position)
			return position + offset;
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::updateCaretRect ()
{
	if(textInput == nullptr)
		return;

	Rect caretRect;
	if(client->getCaretRect (caretRect) == kResultOk)
	{
		Point offset = caretRect.getLeftTop ();
		if(const IView* view = client->getInputView ())
			view->clientToWindow (offset);
		zwp_text_input_v3_set_cursor_rectangle (textInput, offset.x, offset.y, caretRect.getWidth (), caretRect.getHeight ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::updateInputType ()
{
	if(textInput == nullptr)
		return;

	int inputType = client->getInputType ();
	int inputFlags = client->getInputFlags ();

	int contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NORMAL;
	switch(inputType)
	{
	case Styles::kKeyboardTypeDecimal : CCL_FALLTHROUGH
	case Styles::kKeyboardTypeDecimalSigned : CCL_FALLTHROUGH
	case Styles::kKeyboardTypeNumericSigned :
		contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_NUMBER;
		break;
	case Styles::kKeyboardTypeNumeric :
		contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_DIGITS;
		break;
	case Styles::kKeyboardTypeEmail :
		contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_EMAIL;
		break;
	case Styles::kKeyboardTypeUrl :
		contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_URL;
		break;
	case Styles::kKeyboardTypePhoneNumber :
		contentPurpose = ZWP_TEXT_INPUT_V3_CONTENT_PURPOSE_PHONE;
		break;
	}

	int contentHint = 0;
	if(get_flag<int> (inputFlags, ITextInputClient::kSensitiveData))
		set_flag<int> (contentHint, ZWP_TEXT_INPUT_V3_CONTENT_HINT_SENSITIVE_DATA);
	if(get_flag<int> (inputFlags, ITextInputClient::kMultiline))
		set_flag<int> (contentHint, ZWP_TEXT_INPUT_V3_CONTENT_HINT_MULTILINE);

	zwp_text_input_v3_set_content_type (textInput, contentHint, contentPurpose);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::commit (CStringRef text, int deleteBefore, int deleteAfter)
{
	client->setPreeditText (String::kEmpty);
	int caret = getUtf16Position (caretUtf8Index);
	int deleteUtf16Before = 0;
	for(int position = caret; position >= 0; position--)
	{
		if(utf8Positions.at (position) <= caretUtf8Index - deleteBefore)
			break;
		deleteUtf16Before++;
	}
	int deleteUtf16After = 0;
	for(int position = caret; position >= 0 && position < utf8Positions.count (); position++)
	{
		if(utf8Positions.at (position) >= caretUtf8Index + deleteAfter)
			break;
		deleteUtf16After++;
	}
	client->deleteText (deleteUtf16Before, deleteUtf16After);
	client->applyText (String (Text::kUTF8, text));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::setPreeditText (CStringRef text, int selection, int cursor)
{
	client->setPreeditText (String (Text::kUTF8, text));

	if(selection > -1 && cursor > -1)
	{
		String selectionString (Text::kUTF8, text.subString (0, selection));
		String cursorString (Text::kUTF8, text.subString (0, cursor));

		int caretIndex = 0;
		int selectionIndex = 0;
		client->getSelection (caretIndex, selectionIndex, ITextInputClient::kSelectionOriginal);

		CCL_PRINTF ("preedit selection: %d->%d, %d->%d\n", caretIndex, caretIndex + cursorString.length (), selectionIndex, selectionIndex + selectionString.length ())
		client->setSelection (caretIndex + cursorString.length (), selectionIndex + selectionString.length ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::onCompositorConnected ()
{
	initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::onCompositorDisconnected ()
{
	textInput = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::initialize ()
{
	ASSERT (textInput == nullptr)
	WaylandClient& waylandClient = WaylandClient::instance ();
	zwp_text_input_manager_v3* inputManager = waylandClient.getTextInputManager ();
	wl_seat* seat = waylandClient.getSeat ();
	if(inputManager && seat)
		textInput = zwp_text_input_manager_v3_get_text_input (inputManager, seat);
	if(textInput)
		zwp_text_input_v3_add_listener (textInput, &listener, &listener);
	textChanged ();
	selectionChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::terminate ()
{
	if(textInput)
		zwp_text_input_v3_destroy (textInput);
	textInput = nullptr;
}

//************************************************************************************************
// LinuxInputMethod::Listener
//************************************************************************************************

LinuxInputMethod::Listener::Listener (LinuxInputMethod& inputMethod)
: inputMethod (inputMethod),
  cursorBegin (-1),
  cursorEnd (-1),
  deleteBefore (0),
  deleteAfter (0)
{
	enter = onEnter;
	leave = onLeave;
	preedit_string = onPreeditString;
	commit_string = onCommitString;
	delete_surrounding_text = onDeleteSurroundingText;
	done = onDone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onEnter (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onLeave (void* data, zwp_text_input_v3* textInput, struct wl_surface *surface)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onPreeditString (void* data, zwp_text_input_v3* textInput, CStringPtr text, int32_t cursorBegin, int32_t cursorEnd)
{
	Listener* This = static_cast<Listener*> (data);
	This->preeditString = text;
	This->cursorBegin = cursorBegin;
	This->cursorEnd = cursorEnd;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onCommitString (void* data, zwp_text_input_v3* textInput, CStringPtr text)
{
	Listener* This = static_cast<Listener*> (data);
	This->commitString = text;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onDeleteSurroundingText (void* data, zwp_text_input_v3* textInput, uint32_t beforeLength, uint32_t afterLength)
{
	Listener* This = static_cast<Listener*> (data);
	This->deleteBefore = int(beforeLength);
	This->deleteAfter = int(afterLength);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxInputMethod::Listener::onDone (void* data, zwp_text_input_v3* textInput, uint32_t serial)
{
	Listener* This = static_cast<Listener*> (data);
	if(!This->commitString.isEmpty ())
		This->inputMethod.commit (This->commitString, This->deleteBefore, This->deleteAfter);
	if(!This->preeditString.isEmpty ())
		This->inputMethod.setPreeditText (This->preeditString, This->cursorBegin, This->cursorEnd);

	This->commitString.empty ();
	This->preeditString.empty ();
	This->deleteBefore = 0;
	This->deleteAfter = 0;
	This->cursorBegin = -1;
	This->cursorEnd = -1;
}
