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
// Filename    : ccl/platform/wasm/gui/inputmethod.wasm.cpp
// Description : WebAssembly Input Method
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/systemevent.wasm.h"
#include "ccl/platform/wasm/gui/keyevent.wasm.h"

#include "ccl/gui/system/inputmethod.h"
#include "ccl/gui/views/view.h"

#include <emscripten/html5.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, createInputElement, (int x, int y, int width, int height),
{
	var inputElement = document.createElement ('textarea');
	inputElement.id = 'inputmethod';
	inputElement.setAttribute ("oncontextmenu", "event.preventDefault ()");
	inputElement.style.position = 'fixed';
	inputElement.style.left = x + 'px';
	inputElement.style.top = y + 'px';
	inputElement.style.width = width + 'px';
	inputElement.style.height = height + 'px';
	inputElement.style.pointerEvents = 'none';
	inputElement.style.opacity = 0;
	inputElement.style.border = 'none';
	inputElement.style.outline = 'none';

	document.body.appendChild (inputElement);
	inputElement.focus ();

	inputElement.addEventListener ('input', (e) =>
	{
		if(e.inputType === 'insertText' && !e.isComposing)
			CCL.onJSEvent ('input', 'inputmethod', e.data);
		else if(e.inputType === 'deleteContentBackward')
			CCL.onJSEvent ('deletebefore', 'inputmethod');
		else if(e.inputType === 'deleteContentForward')
			CCL.onJSEvent ('deleteafter', 'inputmethod');
		else if(e.inputType === 'insertFromPaste')
		{
			CCL.onJSEvent ('changed', 'clipboard', e.data);
			CCL.onJSEvent ('paste', 'inputmethod', e.data);
		}
	});
	inputElement.addEventListener ('compositionstart', (e) =>
	{
		CCL.onJSEvent ('compositionstart', 'inputmethod', e.data);
	});
	inputElement.addEventListener ('compositionupdate', (e) =>
	{
		CCL.onJSEvent ('compositionupdate', 'inputmethod', e.data);
	});
	inputElement.addEventListener ('compositionend', (e) =>
	{
		CCL.onJSEvent ('compositionend', 'inputmethod', e.data);
	});
	inputElement.addEventListener ('selectionchange', () =>
	{
		if(document.activeElement === inputElement)
			CCL.onJSEvent ('selectionchange', 'inputmethod', inputElement.selectionStart, inputElement.selectionEnd);
	});
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removeInputElement, (),
{
	var element = document.getElementById ('inputmethod');
	if(element)
		element.remove ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setInputElementValue, (CStringPtr value),
{
	var element = document.getElementById ('inputmethod');
	if(element)
	{
		element.value = UTF8ToString (value);
		element.focus ();
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setInputElementSelectionRange, (int start, int end),
{
	var element = document.getElementById ('inputmethod');
	if(element)
	{
		element.setSelectionRange (start, end);
		element.focus ();
	}
});

} // namespace Wasm

//************************************************************************************************
// WasmInputMethod
//************************************************************************************************

class WasmInputMethod: public Object,
					   public SystemEventHandler,
					   public IInputMethod
{
public:
	WasmInputMethod (ITextInputClient* client);

	PROPERTY_BOOL (compositionActive, CompositionActive)

	// IInputMethod
	tresult CCL_API enable (tbool state) override;
	void CCL_API textChanged () override;
	void CCL_API selectionChanged () override;
	void CCL_API visibleRectChanged () override {}

	// SystemEventHandler
	EventResult handleEvent (SystemEvent& e) override;

	CLASS_INTERFACE (IInputMethod, Object)

protected:
	SharedPtr<ITextInputClient> client;
	
	static bool onKeyDownEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data);
	static bool onKeyUpEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data);
};

//************************************************************************************************
// WasmInputMethodManager
//************************************************************************************************

class WasmInputMethodManager: public InputMethodManager
{
public:
	// InputMethodManager
	IInputMethod* createInputMethod (ITextInputClient& client) override;
};

} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// WasmInputMethodManager
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (InputMethodManager, WasmInputMethodManager)

//////////////////////////////////////////////////////////////////////////////////////////////////

IInputMethod* WasmInputMethodManager::createInputMethod (ITextInputClient& client)
{
	return NEW WasmInputMethod (&client);
}

//************************************************************************************************
// WasmInputMethod
//************************************************************************************************

WasmInputMethod::WasmInputMethod (ITextInputClient* client)
: client (client),
  compositionActive (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API WasmInputMethod::enable (tbool state)
{
	const View* view = unknown_cast<View> (client->getInputView ());
	Rect screenRect;
	if(view)
	{
		view->getClientRect (screenRect);
		Point pos;
		view->clientToScreen (pos);
		screenRect.offset (pos);
	}
	if(state)
	{
		SystemEventDelegate::instance ().registerEventHandler ("inputmethod", *this);
		createInputElement (screenRect.left, screenRect.top, screenRect.getWidth (), screenRect.getHeight ());
		emscripten_set_keydown_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onKeyDownEvent);
		emscripten_set_keyup_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onKeyUpEvent);
		textChanged ();
		selectionChanged ();
	}
	else
	{
		emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_KEYDOWN, (void*)onKeyDownEvent);
		emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_KEYUP, (void*)onKeyUpEvent);
		removeInputElement ();
		SystemEventDelegate::instance ().unregisterEventHandler (*this);
	}
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmInputMethod::textChanged ()
{
	if(isCompositionActive ())
		return;
	
	String text;
	if(client->getText (text) != kResultOk)
		return;
	MutableCString utf8Text (text, Text::kUTF8);
	setInputElementValue (utf8Text);
	selectionChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmInputMethod::selectionChanged ()
{
	int caret = 0;
	int selection = 0;
	client->getSelection (caret, selection, ITextInputClient::kSelectionMerged);
	setInputElementSelectionRange (ccl_min (caret, selection), ccl_max (caret, selection));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmInputMethod::handleEvent (SystemEvent& e)
{
	if(e.eventId == "input")
	{
		client->applyText (e.arg1);
	}
	else if(e.eventId == "compositionstart")
	{
		setCompositionActive (true);
	}
	else if(e.eventId == "compositionupdate")
	{
		client->setPreeditText (e.arg1);
	}
	else if(e.eventId == "compositionend" || e.eventId == "paste")
	{
		setCompositionActive (false);
		client->setPreeditText (String::kEmpty);
		client->applyText (e.arg1);
	}
	else if(e.eventId == "deletebefore")
	{
		if(isCompositionActive ())
			client->deleteText (1, 0);
	}
	else if(e.eventId == "deleteafter")
	{
		if(isCompositionActive ())
			client->deleteText (0, 1);
	}
	else if(e.eventId == "selectionchange")
	{
		if(isCompositionActive ())
			client->setSelection (e.arg1.asInt (), e.arg2.asInt ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmInputMethod::onKeyDownEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data)
{
	WasmInputMethod* This = static_cast<WasmInputMethod*> (data);
	if(This->isCompositionActive ())
		return false;

	KeyEvent event;
	VKey::fromSystemEvent (event, *keyEvent, eventType);

	if(event.isCharValid () && event.character == 'v' && (event.state.getModifiers () & KeyState::kCommand) != 0)
		return false; // we handle pasting internally while the input method is active

	View* view = const_cast<View*> (unknown_cast<View> (This->client->getInputView ()));
	if(view)
		view->onKeyDown (event);
	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmInputMethod::onKeyUpEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data)
{
	WasmInputMethod* This = static_cast<WasmInputMethod*> (data);
	if(This->isCompositionActive ())
		return false;

	KeyEvent event;
	VKey::fromSystemEvent (event, *keyEvent, eventType);
	View* view = const_cast<View*> (unknown_cast<View> (This->client->getInputView ()));
	if(view)
		view->onKeyUp (event);
	return false; // don't swallow
}
