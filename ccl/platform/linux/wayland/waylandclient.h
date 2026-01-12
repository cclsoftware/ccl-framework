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
// Filename    : ccl/platform/linux/wayland/waylandclient.h
// Description : Wayland Client Context
//
//************************************************************************************************

#ifndef _ccl_linux_waylandclient_h
#define _ccl_linux_waylandclient_h

#include "ccl/platform/linux/wayland/waylandobject.h"
#include "ccl/platform/linux/wayland/cclwaylandclient.h"

#include "ccl/base/singleton.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/ilinuxspecifics.h"

namespace CCL {
class SystemEventHandler;
struct SystemEvent;

namespace Linux {
	
//////////////////////////////////////////////////////////////////////////////////////////////////
// Wayland Arrays
//////////////////////////////////////////////////////////////////////////////////////////////////

#define WaylandArrayForEach(type, item, array) \
	for(type item = (type)(array)->data; \
		(const char*)item < ((const char*)(array)->data + (array)->size); \
		(item)++)

//************************************************************************************************
// WaylandClient
//************************************************************************************************

class WaylandClient: public Object,
					 public Singleton<WaylandClient>
{
public:
	WaylandClient ();

	PROPERTY_SHARED_POINTER (Linux::IWaylandSocket, socket, Socket)
	
	PROPERTY_POINTER (wl_compositor, compositor, Compositor)
	PROPERTY_POINTER (wl_subcompositor, subCompositor, SubCompositor)
	PROPERTY_POINTER (wl_registry, registry, Registry)
	PROPERTY_POINTER (wl_display, display, Display)
	PROPERTY_POINTER (wl_seat, seat, Seat)
	PROPERTY_POINTER (wl_shm, sharedMemory, SharedMemory)
	PROPERTY_POINTER (wl_data_device_manager, dataDeviceManager, DataDeviceManager)
	PROPERTY_POINTER (xdg_wm_base, windowManager, WindowManager)
	PROPERTY_POINTER (xdg_activation_v1, activation, Activation)
	PROPERTY_POINTER (xdg_wm_dialog_v1, dialogManager, DialogManager)
	PROPERTY_POINTER (zxdg_decoration_manager_v1, decorationManager, DecorationManager)
	PROPERTY_POINTER (zxdg_output_manager_v1, outputManager, OutputManager)
	PROPERTY_POINTER (zxdg_importer_v1, importerV1, ImporterV1)
	PROPERTY_POINTER (zxdg_exporter_v1, exporterV1, ExporterV1)
	PROPERTY_POINTER (zxdg_importer_v2, importer, Importer)
	PROPERTY_POINTER (zxdg_exporter_v2, exporter, Exporter)
	PROPERTY_POINTER (zwp_text_input_manager_v3, textInputManager, TextInputManager)
	PROPERTY_POINTER (zwp_pointer_constraints_v1, pointerConstraints, PointerConstraints)
	PROPERTY_POINTER (zwp_relative_pointer_manager_v1, relativePointerManager, RelativePointerManager)
	PROPERTY_POINTER (zwp_linux_dmabuf_v1, dmaBuffer, DmaBuffer)
	
	PROPERTY_VARIABLE (uint32_t, seatCapabilities, SeatCapabilities)
	PROPERTY_MUTABLE_CSTRING (seatName, SeatName)
	PROPERTY_VARIABLE (uint32_t, serial, Serial)
	PROPERTY_VARIABLE (uint32_t, enterSerial, EnterSerial)
	
	PROPERTY_BOOL (initialized, Initialized)
	
	bool startup ();
	void shutdown ();
	
	bool hasPointerInput () const;
	bool hasKeyboardInput () const;
	bool hasTouchInput () const;
	
	StringID getApplicationId () const;
	
	void registerObject (WaylandObject& object);
	void unregisterObject (WaylandObject& object);
	
	void registerEventHandler (SystemEventHandler& handler);
	void unregisterEventHandler (SystemEventHandler& handler);
	void signalEvent (SystemEvent& event);

protected:
	struct Listener: wl_registry_listener,
					 xdg_wm_base_listener,
					 wl_seat_listener
	{
		Listener (WaylandClient& context);
		
		WaylandClient& context;
		
		// registry
		static void onGlobal (void* data, wl_registry* registry, uint32_t name, const char* interfaceName, uint32_t version);
		static void onGlobalRemoved (void* data, wl_registry* registry, uint32_t name);
		
		// wm base
		static void onPing (void* data, xdg_wm_base* windowManager, uint32_t serial);
		
		// seat
		static void onSeatCapabilities (void* data, wl_seat* seat, uint32_t capabilities);
		static void onSeatName (void* data, wl_seat* seat, const char* name);
	};
	Listener listener;

	MutableCString applicationId;
	Vector<WaylandObject*> objects;
	Vector<WaylandObject*> savedObjects;
	Vector<SystemEventHandler*> eventHandlers;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_waylandclient_h
