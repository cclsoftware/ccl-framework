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
// Filename    : ccl/platform/linux/gui/clipboard.linux.cpp
// Description : Linux Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"
#include "ccl/gui/gui.h"

#include "ccl/platform/linux/wayland/datadevicehelper.h"

#include <unistd.h>
#include <signal.h>

namespace CCL {

//************************************************************************************************
// LinuxClipboard
//************************************************************************************************

class LinuxClipboard: public Clipboard
{
public:
	LinuxClipboard ();
	~LinuxClipboard ();

	// Clipboard
	bool setNativeText (StringRef text) override;
	bool getNativeText (String& text) const override;
	bool hasNativeContentChanged () override;
	
protected:
	struct ClipboardListener: wl_data_source_listener
	{
		ClipboardListener (LinuxClipboard& clipboard);
		
		// data source
		static void onTarget (void* data, wl_data_source* dataSource, CStringPtr mimeType);
		static void onSendData (void* data, wl_data_source* dataSource, CStringPtr mimeType, int32_t fd);
		static void onCanceled (void* data, wl_data_source* dataSource);
		
		static void onDragDropPerformed (void* data, wl_data_source* dataSource);
		static void onDragDropFinished (void* data, wl_data_source* dataSource);
		static void onSourceAction (void* data, wl_data_source* dataSource, uint32_t sourceAction);
		
	protected:
		LinuxClipboard& clipboard;
	};
	ClipboardListener listener;
	
	mutable String clipboardText;
	MutableCString sourceText;
	wl_data_source* dataSource;
	bool changed;
};

} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxClipboard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, LinuxClipboard)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxClipboard::LinuxClipboard ()
: listener (*this),
  dataSource (nullptr),
  changed (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxClipboard::~LinuxClipboard ()
{
	WaylandClient& client = WaylandClient::instance ();
	if(client.getDataDeviceManager () != nullptr)
	{
		if(dataSource != nullptr && WaylandClient::instance ().isInitialized ())
			wl_data_source_destroy (dataSource);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxClipboard::setNativeText (StringRef text)
{
	WaylandClient& client = WaylandClient::instance ();
	wl_data_device_manager* manager = client.getDataDeviceManager ();
	wl_data_device* dataDevice = DataDeviceHelper::instance ().getDataDevice ();
	if(dataDevice == nullptr || manager == nullptr)
		return false;
	
	if(dataSource != nullptr && WaylandClient::instance ().isInitialized ())
		wl_data_source_destroy (dataSource);
	
	dataSource = wl_data_device_manager_create_data_source (manager);
	if(dataSource)
	{
		wl_data_source_add_listener (dataSource, &listener, &listener);
		sourceText = MutableCString (text, Text::kSystemEncoding);
		wl_data_source_offer (dataSource, DataDeviceHelper::getClipboardMimeType ());
		wl_data_device_set_selection (dataDevice, dataSource, client.getSerial ());
		
		DataDeviceHelper::instance ().getClipboardText (); //< flush changes from system clipboard
		clipboardText = text;
		changed = true;
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxClipboard::getNativeText (String& text) const
{
	if(DataDeviceHelper::instance ().hasClipboardTextChanged ())
		clipboardText = DataDeviceHelper::instance ().getClipboardText ();
	text = clipboardText;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxClipboard::hasNativeContentChanged ()
{
	bool result = changed || DataDeviceHelper::instance ().hasClipboardTextChanged ();
	changed = false;
	return result;
}

//************************************************************************************************
// LinuxClipboard::ClipboardListener
//************************************************************************************************

LinuxClipboard::ClipboardListener::ClipboardListener (LinuxClipboard& clipboard)
: clipboard (clipboard)
{
	wl_data_source_listener::target = onTarget;
	wl_data_source_listener::send = onSendData;
	wl_data_source_listener::cancelled = onCanceled;
	wl_data_source_listener::dnd_drop_performed = onDragDropPerformed;
	wl_data_source_listener::dnd_finished = onDragDropFinished;
	wl_data_source_listener::action = onSourceAction;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onSendData (void* data, wl_data_source* dataSource, CStringPtr mimeType, int32_t fd)
{
	ClipboardListener* This = static_cast<ClipboardListener*> (data);
	if(This->clipboard.dataSource == dataSource && DataDeviceHelper::getClipboardMimeType () == mimeType)
	{
		const char* clipboardData = This->clipboard.sourceText.str ();
		ssize_t length = This->clipboard.sourceText.length ();

		ssize_t bytesWritten = 0;

		sighandler_t handler = ::signal (SIGPIPE, SIG_IGN);
		while(length > 0 && bytesWritten != -1)
		{
			bytesWritten = ::write (fd, clipboardData, length);
			clipboardData += bytesWritten;
			length -= bytesWritten;
		}
		::signal (SIGPIPE, handler);
	}
	::close (fd);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onCanceled (void* data, wl_data_source* dataSource)
{
	ClipboardListener* This = static_cast<ClipboardListener*> (data);
	if(This->clipboard.dataSource == dataSource)
	{
		if(WaylandClient::instance ().isInitialized ())
			wl_data_source_destroy (This->clipboard.dataSource);
		This->clipboard.dataSource = nullptr;
		This->clipboard.changed = true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onTarget (void *data, wl_data_source* dataSource, CStringPtr mimeType)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onDragDropPerformed (void* data, wl_data_source* dataSource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onDragDropFinished (void* data, wl_data_source* dataSource)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxClipboard::ClipboardListener::onSourceAction (void* data, wl_data_source* dataSource, uint32_t sourceAction)
{}
