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
// Filename    : ccl/platform/linux/wayland/datadevicehelper.h
// Description : Wayland Clipboard and Drag&Drop Handling
//
//************************************************************************************************

#ifndef _ccl_waylanddatadevicehelper_h
#define _ccl_waylanddatadevicehelper_h

#include "ccl/platform/linux/wayland/waylandclient.h"
#include "ccl/platform/linux/gui/dragndrop.linux.h"

namespace CCL {
namespace Linux {
class Surface;

//************************************************************************************************
// DataDeviceHelper
//************************************************************************************************

class DataDeviceHelper: public Object,
						public Singleton<DataDeviceHelper>
{
public:
	DataDeviceHelper ();
	~DataDeviceHelper ();
	
	void initialize ();
	void terminate ();
	
	static CStringRef getClipboardMimeType (bool utf8Encoded = false);
	
	wl_data_device* getDataDevice () const;
	
	void registerSurface (Linux::Surface& surface);
	void unregisterSurface (Linux::Surface& surface);
	
	StringRef getClipboardText () const;
	bool hasClipboardTextChanged () const;
	void setClipboardText (StringRef text);
	
	void finishInternalDrag ();
	
protected:	
    struct Listener: wl_data_device_listener,
					 wl_data_offer_listener
	{
		Listener (DataDeviceHelper& helper);
		
		PROPERTY_POINTER (wl_data_offer, offer, Offer)
		
		// data device
		static void onDataOffer (void* data, wl_data_device* dataDevice, wl_data_offer* id);
		static void onEnter (void* data, wl_data_device* dataDevice, uint32_t serial, wl_surface* surface, wl_fixed_t x, wl_fixed_t y, wl_data_offer* id);
		static void onLeave (void* data, wl_data_device* dataDevice);
		static void onMotion (void* data, wl_data_device* dataDevice, uint32_t time, wl_fixed_t x, wl_fixed_t y);
		static void onDrop (void* data, wl_data_device* dataDevice);
		static void onSelection (void* data, wl_data_device* dataDevice, wl_data_offer* id);
		
		// data offer
		static void onOffer (void* data, wl_data_offer* dataOffer, CStringPtr mimeType);
		static void onOfferSourceActions (void* data, wl_data_offer* dataOffer, uint32_t sourceActions);
		static void onOfferDropAction (void* data, wl_data_offer* dataOffer, uint32_t dropAction);
		
	protected:
		DataDeviceHelper& helper;
		AutoPtr<LinuxDragSession> dragSession;
		uint32_t serial;
		Vector<MutableCString> mimeTypes;
		uint32_t sourceActions;
		uint32_t finalAction;
		Surface* currentSurface;
		Point dragPosition;
		
		void updateActions (bool accepted);
	};
	Listener listener;
    
	wl_data_device* dataDevice;
	
	mutable String clipboardText;
	mutable int clipboardFds[2];
	TextEncoding clipboardEncoding;
	
	Vector<Linux::Surface*> surfaces;
	
	Surface* findSurface (wl_surface* waylandSurface) const;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_waylanddatadevicehelper_h
