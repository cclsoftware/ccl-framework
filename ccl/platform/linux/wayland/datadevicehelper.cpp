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
// Filename    : ccl/platform/linux/wayland/datadevicehelper.cpp
// Description : Wayland Clipboard and Drag-and-Drop Handling
//
//************************************************************************************************
 
#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/datadevicehelper.h"
#include "ccl/platform/linux/wayland/surface.h"

#include "ccl/gui/views/view.h"

#include "ccl/public/storage/filetype.h"

#include <unistd.h>
#include <fcntl.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// DataDeviceHelper
//************************************************************************************************

DEFINE_SINGLETON (DataDeviceHelper)

//////////////////////////////////////////////////////////////////////////////////////////////////

DataDeviceHelper::DataDeviceHelper ()
: listener (*this),
  dataDevice (nullptr),
  clipboardFds {-1, -1},
  clipboardEncoding (Text::kSystemEncoding)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DataDeviceHelper::~DataDeviceHelper ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::initialize ()
{
	WaylandClient& client = WaylandClient::instance ();
	wl_data_device_manager* manager = client.getDataDeviceManager ();
	wl_seat* seat = client.getSeat ();
	if(manager && seat)
		dataDevice = wl_data_device_manager_get_data_device (manager, seat);
	if(dataDevice)
		wl_data_device_add_listener (dataDevice, &listener, &listener);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::terminate ()
{
	surfaces.removeAll ();
	
	if(clipboardFds[0] >= 0)
		::close (clipboardFds[0]);
	clipboardFds[0] = -1;
	if(clipboardFds[1] >= 0)
		::close (clipboardFds[1]);
	clipboardFds[1] = -1;
	
	WaylandClient& client = WaylandClient::instance ();
	if(client.getDataDeviceManager () != nullptr)
	{
		if(dataDevice != nullptr)
			wl_data_device_release (dataDevice);
	}
	dataDevice = nullptr;

	listener.setOffer (nullptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

wl_data_device* DataDeviceHelper::getDataDevice () const
{
	return dataDevice;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef DataDeviceHelper::getClipboardMimeType (bool utf8Encoded)
{
	static MutableCString mimeType (FileTypes::Text ().getMimeType (), Text::kSystemEncoding);
	static MutableCString mimeTypeUTF8 (String (FileTypes::Text ().getMimeType ()).appendASCII (";charset=utf-8"), Text::kSystemEncoding);
	return utf8Encoded ? mimeTypeUTF8 : mimeType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::registerSurface (Surface& surface)
{
	if(!surfaces.contains (&surface))
		surfaces.add (&surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::unregisterSurface (Surface& surface)
{
	surfaces.remove (&surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Surface* DataDeviceHelper::findSurface (wl_surface* waylandSurface) const
{
	for(Surface* surface : surfaces)
	{
		if(surface && surface->getWaylandSurface () == waylandSurface)
			return surface;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef DataDeviceHelper::getClipboardText () const
{
	if(clipboardFds[0] >= 0)
	{
		int flags = ::fcntl (clipboardFds[0], F_GETFL, 0);
		::fcntl (clipboardFds[0], F_SETFL, flags | O_NONBLOCK);
		
		clipboardText.empty ();
		char buffer[STRING_STACK_SPACE_MAX];
		while(true)
		{
			ssize_t bytesRead = ::read (clipboardFds[0], buffer, sizeof(buffer));
			if(bytesRead <= 0)
				break;
			clipboardText.appendCString (clipboardEncoding, buffer, int (bytesRead));
		}
		::close (clipboardFds[0]);
		clipboardFds[0] = -1;
	}
	return clipboardText;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DataDeviceHelper::hasClipboardTextChanged () const
{
	return clipboardFds[0] >= 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::finishInternalDrag ()
{
	if(wl_data_offer* offer = listener.getOffer ())
	{
		CCL_PRINTF ("finishInternalDrag: Destroying offer %p\n", offer)

		wl_data_offer_finish (offer);
		wl_data_offer_destroy (offer);
		listener.setOffer (nullptr);
	}
}

//************************************************************************************************
// DataDeviceHelper::Listener
//************************************************************************************************

DataDeviceHelper::Listener::Listener (DataDeviceHelper& helper)
: helper (helper),
  currentSurface (nullptr),
  offer (nullptr),
  serial (0)
{
	wl_data_device_listener::data_offer = onDataOffer;
	wl_data_device_listener::enter = onEnter;
	wl_data_device_listener::leave = onLeave;
	wl_data_device_listener::motion = onMotion;
	wl_data_device_listener::drop = onDrop;
	wl_data_device_listener::selection = onSelection;
	
	wl_data_offer_listener::offer = onOffer;
	wl_data_offer_listener::source_actions = onOfferSourceActions;
	wl_data_offer_listener::action = onOfferDropAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::updateActions (bool accepted)
{
	if(offer)
	{
		if(accepted && dragSession && !dragSession->getPreferredMimeType ().isEmpty ())
		{
			uint32_t action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;

			if(dragSession->isDropCopyReal () || dragSession->isDropCopyShared ())
				action = WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY & sourceActions;
			if(action == WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE)
				action = WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE & sourceActions;

			wl_data_offer_set_actions (offer, action, action);
			wl_data_offer_accept (offer, serial, dragSession->getPreferredMimeType ());
			CCL_PRINTF ("Accepted mime type %s\n", dragSession->getPreferredMimeType ().str ())
		}
		else
			wl_data_offer_accept (offer, serial, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onDataOffer (void* data, wl_data_device* dataDevice, wl_data_offer* id)
{
	Listener* This = static_cast<Listener*> (data);
	if(dataDevice == This->helper.dataDevice)
	{
		CCL_PRINTF ("onDataOffer: New offer %p, old offer %p\n", id, This->offer)

		if(This->offer)
			wl_data_offer_destroy (This->offer);
		This->offer = id;
		This->mimeTypes.empty ();
		This->sourceActions = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
		This->finalAction = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
		wl_data_offer_add_listener (id, This, This);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onEnter (void* data, wl_data_device* dataDevice, uint32_t serial, wl_surface* waylandSurface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id)
{
	Listener* This = static_cast<Listener*> (data);
	
	This->currentSurface = This->helper.findSurface (waylandSurface);
	This->serial = serial;
	if(This->helper.getDataDevice () == dataDevice && This->currentSurface != nullptr && This->offer == id)
	{
		This->dragSession = nullptr;
		if(DragSession::isInternalDragActive ())
			This->dragSession = return_shared (ccl_cast<LinuxDragSession> (DragSession::getActiveSession ()));
		if(This->dragSession == nullptr)
			This->dragSession = NEW LinuxDragSession (id, This->mimeTypes);
		bool accepted = false;
		View* view = This->currentSurface->getView ();
		if(view)
		{
			This->dragPosition = Point (wl_fixed_to_int (x), wl_fixed_to_int (y));
			DragEvent dragEvent (*This->dragSession, DragEvent::kDragEnter, This->dragPosition);
			InputHandler::instance ().getActiveModifierKeys (dragEvent.keys);
			accepted = view->onDragEnter (dragEvent);
		}
		This->updateActions (accepted);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onLeave (void* data, wl_data_device* dataDevice)
{
	Listener* This = static_cast<Listener*> (data);
	if(This->helper.getDataDevice () == dataDevice && This->dragSession != nullptr && This->currentSurface != nullptr)
	{
		View* view = This->currentSurface->getView ();
		if(view)
		{
			DragEvent dragEvent (*This->dragSession, DragEvent::kDragLeave);
			InputHandler::instance ().getActiveModifierKeys (dragEvent.keys);
			view->onDragLeave (dragEvent);
		}
		This->dragSession.release ();
		This->currentSurface = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onMotion (void* data, wl_data_device* dataDevice, uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	Listener* This = static_cast<Listener*> (data);
	if(This->helper.getDataDevice () == dataDevice && This->dragSession != nullptr && This->currentSurface != nullptr)
	{
		bool accepted = false;
		View* view = This->currentSurface->getView ();
		if(view)
		{
			This->dragPosition = Point (wl_fixed_to_int (x), wl_fixed_to_int (y));
			DragEvent dragEvent (*This->dragSession, DragEvent::kDragOver, This->dragPosition);
			InputHandler::instance ().getActiveModifierKeys (dragEvent.keys);
			accepted = view->onDragOver (dragEvent);
		}
		This->updateActions (accepted);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onDrop (void* data, wl_data_device* dataDevice)
{
	Listener* This = static_cast<Listener*> (data);
	if(This->helper.getDataDevice () == dataDevice && This->dragSession != nullptr && This->offer != nullptr)
	{
		if(This->currentSurface != nullptr)
		{
			View* view = This->currentSurface->getView ();
			if(view && This->finalAction != WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE)
			{
				DragEvent dragEvent (*This->dragSession, DragEvent::kDrop, This->dragPosition);
				InputHandler::instance ().getActiveModifierKeys (dragEvent.keys);
				view->onDrop (dragEvent);
				wl_data_offer_finish (This->offer);
			}
		}
		CCL_PRINTF ("onDrop: Destroying offer %p\n", This->offer)

		wl_data_offer_destroy (This->offer);
		This->offer = nullptr;
	}
	This->dragSession.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onSelection (void* data, wl_data_device* dataDevice, wl_data_offer* id)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(id == nullptr || This->helper.getDataDevice () != dataDevice)
		return;
	
	if(This->helper.clipboardFds[0] >= 0)
		::close (This->helper.clipboardFds[0]);
	if(This->helper.clipboardFds[1] >= 0)
		::close (This->helper.clipboardFds[1]);
	
	::pipe (This->helper.clipboardFds);

	bool supportsUtf8 = This->mimeTypes.contains (This->helper.getClipboardMimeType (true));
	This->helper.clipboardEncoding = supportsUtf8 ? Text::kUTF8 : Text::kSystemEncoding;

	wl_data_offer_receive (id, This->helper.getClipboardMimeType (supportsUtf8), This->helper.clipboardFds[1]);
	::close (This->helper.clipboardFds[1]);
	This->helper.clipboardFds[1] = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onOffer (void* data, wl_data_offer* dataOffer, CStringPtr mimeType)
{
	Listener* This = static_cast<Listener*> (data);
	if(dataOffer == This->offer)
	{
		MutableCString type (mimeType, Text::kSystemEncoding);
		This->mimeTypes.add (type);
		CCL_PRINTF ("Data offer supports mime type: %s\n", type.str ())
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onOfferSourceActions (void* data, wl_data_offer* dataOffer, uint32_t sourceActions)
{
	Listener* This = static_cast<Listener*> (data);
	if(dataOffer == This->offer)
		This->sourceActions = sourceActions;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DataDeviceHelper::Listener::onOfferDropAction (void* data, wl_data_offer* dataOffer, uint32_t dropAction)
{
	Listener* This = static_cast<Listener*> (data);
	if(dataOffer == This->offer)
		This->finalAction = dropAction;
}

