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
// Filename    : ccl/platform/linux/gui/dragndrop.linux.cpp
// Description : Linux-specific Drag-and-Drop 
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/gui/dragndrop.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"
#include "ccl/platform/linux/wayland/datadevicehelper.h"

#include "ccl/base/boxedtypes.h"
#include "ccl/base/storage/url.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/system/clipboard.h"

#include <unistd.h>
#include <signal.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// DragSession
//************************************************************************************************

DragSession* DragSession::create (IUnknown* source, int inputDevice)
{
	return NEW LinuxDragSession (source, inputDevice);
}

//************************************************************************************************
// LinuxDragSession
//************************************************************************************************

DEFINE_CLASS (LinuxDragSession, DragSession)
DEFINE_CLASS_UID (LinuxDragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f) // ClassID::DragSession

DEFINE_STRINGID_MEMBER_ (LinuxDragSession, kUrlListMimeType, "text/uri-list")

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxDragSession::LinuxDragSession (IUnknown* source, int inputDevice)
: DragSession (source, inputDevice),
  listener (*this),
  dataDevice (nullptr),
  dataSource (nullptr),
  dataOffer (nullptr),
  terminated (true)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxDragSession::LinuxDragSession (wl_data_offer* offer, const Vector<MutableCString>& offeredMimeTypes, int inputDevice)
: DragSession (inputDevice),
  listener (*this),
  dataDevice (nullptr),
  dataSource (nullptr),
  dataOffer (offer),
  terminated (true)
{
	for(CStringRef mimeType : offeredMimeTypes)
	{
		if(mimeType == kUrlListMimeType || mimeType == DataDeviceHelper::getClipboardMimeType ())
			mimeTypes.add (mimeType);
	}
	if(offer)
		convertNativeItems ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxDragSession::~LinuxDragSession ()
{
	ASSERT (terminated == true)
	ASSERT (dataSource == nullptr)
	ASSERT (dragImageSurface.getWaylandSurface () == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* CCL_API LinuxDragSession::dragAsync ()
{
	IWindow* parentWindow = nullptr;
	WindowContext* parentContext = nullptr;
	
	UnknownPtr<IView> view (source);
	if(view)
		parentWindow = view->getIWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getDialogParentWindow ();
	if(parentWindow == nullptr)
		parentWindow = Desktop.getApplicationWindow ();
	if(parentWindow)
		parentContext = static_cast<WindowContext*> (parentWindow->getSystemWindow ());
	LinuxWindow* parent = LinuxWindow::cast (unknown_cast<Window> (parentWindow));
	
	if(parentContext == nullptr)
		return AsyncOperation::createCompleted (IDragSession::kDropNone);
	
	WaylandClient& client = WaylandClient::instance ();
	wl_data_device_manager* manager = client.getDataDeviceManager ();
	dataDevice = DataDeviceHelper::instance ().getDataDevice ();
	
	if(dataDevice == nullptr || manager == nullptr)
		return AsyncOperation::createCompleted (IDragSession::kDropNone);
	
	if(dataSource != nullptr && client.isInitialized ())
		wl_data_source_destroy (dataSource);
	
	dataSource = wl_data_device_manager_create_data_source (manager);
	if(dataSource)
	{
		bool containsText = false;
		bool containsUrl = false;
		bool containsBinary = false;
		ForEachUnknown (getItems (), unk)
			UnknownPtr<IUrl> url (unk);
			String string;

			if(url)
				containsUrl = true;
			else if(Clipboard::instance ().toText (string, unk))
				containsText = true;
			else
				containsBinary = true;
		EndFor
		
		wl_data_source_add_listener (dataSource, &listener, &listener);
		
		if(containsText)
			wl_data_source_offer (dataSource, DataDeviceHelper::getClipboardMimeType ());
		if(containsUrl)
			wl_data_source_offer (dataSource, kUrlListMimeType);
		
		MutableCString binaryMimeType (FileTypes::Binary ().getMimeType (), Text::kSystemEncoding);
		if(containsBinary)
			wl_data_source_offer (dataSource, binaryMimeType);
		setPreferredMimeType (binaryMimeType);
		
		wl_data_source_set_actions (dataSource, WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE | WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
		
		dragImageSurface.destroySurface ();
		if(dragImage != nullptr)
		{
			if(parent)
				dragImageSurface.setScaleFactor (parent->getContentScaleFactor ());
			dragImageSurface.setImage (dragImage);
			dragImageSurface.createSurface ();
		}
		
		SharedPtr<MouseCursor> oldCursor = GUI.getCursor ();
		
		{
			SOFT_ASSERT (DragSession::activeSession == nullptr, "Starting a new drag session while old session is still active")
			if(LinuxDragSession* session = ccl_cast<LinuxDragSession> (DragSession::activeSession))
				session->terminate (false);
			
			DragGuard guard (*this);
			
			uint32_t serial = InputHandler::instance ().getSerial ();
			ASSERT (serial != 0)
			wl_data_device_start_drag (dataDevice, dataSource, parentContext->waylandSurface, dragImageSurface.getWaylandSurface (), serial);
			
			listener.updateCursor ();
			
			operation = NEW AsyncOperation;
			operation->setState (IAsyncOperation::kStarted);
			
			terminated = false;
			while(!terminated && !wasCanceled () && !isDropped ())
				GUI.flushUpdates ();
			if(!terminated)
			{
				DataDeviceHelper::instance ().finishInternalDrag ();
				GUI.runModalLoop (parentWindow, terminated);
			}
		}
		
		GUI.setCursor (oldCursor);
		
		if(parent)
		{
			DragEvent dragEvent (*this, DragEvent::kDragLeave);
			parent->onDragLeave (dragEvent);
		}

		return operation.detach ();
	}
	return AsyncOperation::createCompleted (IDragSession::kDropNone);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::convertNativeItems ()
{
	bool acceptedText = false;
	bool acceptedUrlList = false;
	for(CStringRef mimeType : mimeTypes)
	{
		int fds[2] = {-1, -1};
		::pipe (fds);
		
		wl_data_offer_receive (dataOffer, mimeType, fds[1]);
		::close (fds[1]);
		
		if(wl_display* display = WaylandClient::instance ().getDisplay ())
			wl_display_roundtrip (display);
		
		String data;
		char buffer[STRING_STACK_SPACE_MAX];
		while(true)
		{
			ssize_t bytesRead = ::read (fds[0], buffer, sizeof(buffer));
			if(bytesRead <= 0)
				break;
			data.appendCString (Text::kSystemEncoding, buffer, int (bytesRead));
		}
		::close (fds[0]);
		
		CCL_PRINTF ("Received data: %s \"%s\"\n", mimeType.str (), MutableCString (data, Text::kSystemEncoding).str ());
		if(mimeType == kUrlListMimeType)
		{
			while(!data.isEmpty ())
			{
				int pos = data.index ("\n");
				String urlItem = data.subString (0, pos);
				if(urlItem.endsWith ("\r"))
					urlItem.truncate (urlItem.length () - 1);

				if(!urlItem.isEmpty () && !urlItem.startsWith ("#"))
				{
					Url* url = NEW Url;
					UrlUtils::fromEncodedString (*url, urlItem);
					items.add (url->asUnknown (), false);
					acceptedUrlList = true;
					CCL_PRINTF ("Added Url item %s\n", MutableCString (UrlDisplayString (*url), Text::kSystemEncoding).str ());
				}
				data = pos >= 0 ? data.subString (pos + 1) : "";
			}
		}
		else if(mimeType == DataDeviceHelper::getClipboardMimeType ())
		{
			Boxed::String* string = NEW Boxed::String (data);
			items.add (string->asUnknown (), false);
			acceptedText = true;
		}
	}
	
	if(acceptedUrlList)
		preferredMimeType = kUrlListMimeType;
	else if(acceptedText)
		preferredMimeType = DataDeviceHelper::getClipboardMimeType ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::terminate (bool succeeded)
{
	if(operation)
	{
		if(!succeeded)
			operation->setResult (IDragSession::kDropNone);
		operation->setState (succeeded ? IAsyncOperation::kCompleted : IAsyncOperation::kCanceled);
	}
	
	if(dataSource && WaylandClient::instance ().isInitialized ())
		wl_data_source_destroy (dataSource);
	dataSource = nullptr;
	
	dragImageSurface.setImage (nullptr);
	dragImageSurface.destroySurface ();
	
	terminated = true;
}

//************************************************************************************************
// LinuxDragSession::DragListener
//************************************************************************************************

LinuxDragSession::DragListener::DragListener (LinuxDragSession& session)
: session (session),
  action (WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE),
  cursorId (-1)
{
	wl_data_source_listener::target = onTarget;
	wl_data_source_listener::send = onSendData;
	wl_data_source_listener::cancelled = onCanceled;
	wl_data_source_listener::dnd_drop_performed = onDragDropPerformed;
	wl_data_source_listener::dnd_finished = onDragDropFinished;
	wl_data_source_listener::action = onSourceAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onSendData (void* data, wl_data_source* dataSource, CStringPtr mimeType, int32_t fd)
{
	DragListener* This = static_cast<DragListener*> (data);
	if(This->session.dataSource == dataSource)
	{
		ForEachUnknown (This->session.getItems (), unk)
			UnknownPtr<IUrl> url (unk);
			String string;
		
			MutableCString dataString;
			const char* data = nullptr;
			ssize_t length = 0;
			if(url && LinuxDragSession::kUrlListMimeType == mimeType)
			{
				dataString = MutableCString (UrlUtils::toEncodedString (*url), Text::kSystemEncoding);
				dataString.append ("\n");
				data = dataString.str ();
				length = dataString.length ();

			}
			else if(Clipboard::instance ().toText (string, unk) && DataDeviceHelper::getClipboardMimeType () == mimeType)
			{
				dataString = MutableCString (string, Text::kSystemEncoding);
				data = dataString.str ();
				length = dataString.length ();
			}

			if(data == nullptr)
				continue;

			ssize_t bytesWritten = 0;

			sighandler_t handler = ::signal (SIGPIPE, SIG_IGN);
			while(length > 0 && bytesWritten != -1)
			{
				bytesWritten = ::write (fd, data, length);
				data += bytesWritten;
				length -= bytesWritten;
			}
			::signal (SIGPIPE, handler);

			break;

		EndFor
	}
	::close (fd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onCanceled (void* data, wl_data_source* dataSource)
{
	DragListener* This = static_cast<DragListener*> (data);
	if(This->session.dataSource == dataSource)
		This->session.terminate (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onTarget (void *data, wl_data_source* dataSource, CStringPtr mimeType)
{
	DragListener* This = static_cast<DragListener*> (data);
	This->mimeType = mimeType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onDragDropPerformed (void* data, wl_data_source* dataSource)
{
	DragListener* This = static_cast<DragListener*> (data);
	if(This->session.operation == nullptr || This->session.dataSource != dataSource)
		return;
	
	switch(This->action)
	{
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE :
		This->session.operation->setResult (IDragSession::kDropMove);
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY :
		This->session.operation->setResult (IDragSession::kDropCopyReal);
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE :
		This->session.operation->setResult (IDragSession::kDropNone);
		break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onDragDropFinished (void* data, wl_data_source* dataSource)
{
	DragListener* This = static_cast<DragListener*> (data);
	if(This->session.operation != nullptr && dataSource == This->session.dataSource)
		This->session.terminate (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::onSourceAction (void* data, wl_data_source* dataSource, uint32_t sourceAction)
{
	DragListener* This = static_cast<DragListener*> (data);
	if(dataSource == This->session.dataSource)
	{
		This->action = sourceAction;
		This->updateCursor ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxDragSession::DragListener::updateCursor ()
{
	ThemeCursorID newCursorId = ThemeElements::kNoDropCursor;
	switch(action)
	{
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_ASK :
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY :
		newCursorId = ThemeElements::kCopyCursor;
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE :
		newCursorId = ThemeElements::kGrabbingCursor;
		break;
	case WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE :
		break;
	}

	if(cursorId != newCursorId)
	{
		cursorId = newCursorId;
		cursor = MouseCursor::createCursor (cursorId);
		if(cursor)
			cursor->makeCurrent ();
	}
}
