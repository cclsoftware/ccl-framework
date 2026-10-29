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
// Filename    : ccl/platform/wasm/gui/dragndrop.wasm.cpp
// Description : WebAssembly Drag-and-Drop
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/dragndrop.wasm.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/views/view.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/storage/filetype.h"

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, addDragData, (CStringPtr mimeType, CStringPtr data),
{
	if(CCL.currentDataTransfer)
		CCL.currentDataTransfer.setData (UTF8ToString (mimeType), UTF8ToString (data));
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setDefaultDragActions, (),
{
	if(CCL.currentDataTransfer)
		CCL.currentDataTransfer.effectAllowed = 'copyMove';
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, createTextPreviewElement, (int x, int y, CStringPtr text),
{
	let dragPreview = document.getElementById ('dragPreview');
	if(dragPreview)
		dragPreview.remove ();
	
	dragPreview = document.createElement ('div');
	dragPreview.id = 'dragPreview';
	dragPreview.style.position = 'fixed';
	document.body.appendChild (dragPreview);

	dragPreview.innerText = UTF8ToString (text);
	dragPreview.style.left = x + 'px';
	dragPreview.style.top = y + 'px';
	dragPreview.style.display = 'block';

	if(CCL.currentDataTransfer)
		CCL.currentDataTransfer.setDragImage (dragPreview, dragPreview.outerWidth, dragPreview.outerHeight);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, createImagePreviewElement, (int x, int y, const uint8* imageData, int width, int height, int offsetX, int offsetY, float scaleFactor),
{
	try
	{
		let dragPreview = document.getElementById ('dragPreview');
		if(dragPreview)
			dragPreview.remove ();

		let canvas = CCL.createImageCanvas (imageData, width, height, 'dragPreview', scaleFactor);
		canvas.style.position = 'fixed';
		canvas.style.left = x + 'px';
		canvas.style.top = y + 'px';

		document.body.appendChild (canvas);

		if(CCL.currentDataTransfer)
			CCL.currentDataTransfer.setDragImage (canvas, offsetX, offsetY);
	}
	catch(e)
	{}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removePreviewElement, (),
{
	var dragPreview = document.getElementById ('dragPreview');
	if(dragPreview)
		dragPreview.remove ();
});

} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::create (IUnknown* source, int inputDevice)
{
	return NEW WasmDragSession (source, inputDevice);
}

//************************************************************************************************
// WasmDragSession
//************************************************************************************************

DEFINE_CLASS (WasmDragSession, DragSession)
DEFINE_CLASS_UID (WasmDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

DEFINE_STRINGID_MEMBER_ (WasmDragSession, kUrlListMimeType, "text/uri-list")

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmDragSession::WasmDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  dragGuard (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmDragSession::~WasmDragSession ()
{
	delete dragGuard;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API WasmDragSession::dragAsync ()
{
	Point where;
	GUI.getMousePosition (where);

	// find window
	IWindow* parentWindow = nullptr;
	UnknownPtr<IView> view = source;
	if(view)
		parentWindow = view->getIWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getDialogParentWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getApplicationWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.findWindow (where);

	Window* window = unknown_cast<Window> (parentWindow);
	if(window)
	{
		GUI.hideTooltip ();

		ASSERT (!dragGuard)
		dragGuard = NEW DragGuard (*this);

		window->screenToClient (where);

		KeyState keyState;
		GUI.getKeyState (keyState);
		DragEvent dragEvent (*this, DragEvent::kDragEnter, where, keyState);
		dragEvent.keys.keys |= KeyState::kLButton;
		window->onDragEnter (dragEvent);
	}

	retain (); // released in onDragFinished

	MutableCString dragText;
	ForEachUnknown (getItems (), unk)
		String string;
		if(Url* url = unknown_cast<Url> (unk))
			addDragData (kUrlListMimeType, MutableCString (UrlUtils::toEncodedString (*url), Text::kUTF8));
		else if(Clipboard::instance ().toText (string, unk))
		{
			dragText = MutableCString (string, Text::kUTF8);
			addDragData (MutableCString (FileTypes::Text ().getMimeType (), Text::kUTF8), dragText);
		}
	EndFor
	setDefaultDragActions ();

	Image* dragImage = unknown_cast<Image> (getDragImage ());
	if(dragImage != nullptr)
	{
		float scaleFactor = 1.0f;
		if(window)
			scaleFactor = window->getContentScaleFactor ();

		const Point size (dragImage->getSize ());
		AutoPtr<Bitmap> bitmap = NEW Bitmap (size.x, size.y, Bitmap::kRGBAlpha, scaleFactor);
		BitmapGraphicsDevice device (bitmap);

		if(!device.isNullDevice ())
		{
			device.drawImage (dragImage, Point (0, 0));

			BitmapDataLocker locker (bitmap, IBitmap::kRGBAlpha, IBitmap::kLockRead);
			if(locker.result == kResultOk && locker.data.scan0 != nullptr)
			{
				const Point& offset = getOffset ();
				createImagePreviewElement (kMinCoord, kMinCoord,
					static_cast<const uint8*> (locker.data.scan0),
					locker.data.width, locker.data.height,
					offset.x, offset.y,
					scaleFactor
				);
			}
		}
	}
	else
		createTextPreviewElement (kMinCoord, kMinCoord, dragText);

	operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);
	return operation;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmDragSession::onDragFinished (const DragEvent& event)
{
	if(operation)
	{
		removePreviewElement ();
		operation->setResult (getResult ());
		operation->setState (AsyncOperation::kCompleted);
		operation.release ();
		release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmDragSession::addData (CStringRef mimeType, StringRef data)
{
	if(mimeType == kUrlListMimeType)
	{
		String listData (data);
		while(!listData.isEmpty ())
		{
			int pos = listData.index ("\n");
			String urlItem = listData.subString (0, pos);
			if(urlItem.endsWith ("\r"))
				urlItem.truncate (urlItem.length () - 1);

			if(!urlItem.isEmpty () && !urlItem.startsWith ("#"))
			{
				Url* url = NEW Url;
				UrlUtils::fromEncodedString (*url, urlItem);
				items.add (url->asUnknown (), false);
			}
			listData = pos >= 0 ? listData.subString (pos + 1) : "";
		}

	}
	else if(mimeType == MutableCString (FileTypes::Text ().getMimeType (), Text::kUTF8))
	{
		Boxed::String* string = NEW Boxed::String (data);
		items.add (string->asUnknown (), false);
	}
}