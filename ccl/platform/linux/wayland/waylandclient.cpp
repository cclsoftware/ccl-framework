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
// Filename    : ccl/platform/linux/wayland/waylandclient.cpp
// Description : Wayland Client Context
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/wayland/waylandclient.h"
#include "ccl/platform/linux/wayland/monitorhelper.h"
#include "ccl/platform/linux/wayland/dmabufferhelper.h"

#include "ccl/base/storage/configuration.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/system/systemevent.h"

#include "ccl/public/gui/iapplication.h"
#include "ccl/public/text/cstring.h"

using namespace CCL;
using namespace Linux;
    
//************************************************************************************************
// WaylandClient
//************************************************************************************************

DEFINE_SINGLETON (WaylandClient)

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandClient::WaylandClient ()
: compositor (nullptr),
  subCompositor (nullptr),
  registry (nullptr),
  display (nullptr),
  seat (nullptr),
  sharedMemory (nullptr),
  dataDeviceManager (nullptr),
  windowManager (nullptr),
  decorationManager (nullptr),
  outputManager (nullptr),
  textInputManager (nullptr),
  pointerConstraints (nullptr),
  relativePointerManager (nullptr),
  activation (nullptr),
  dialogManager (nullptr),
  importer (nullptr),
  exporter (nullptr),
  importerV1 (nullptr),
  exporterV1 (nullptr),
  dmaBuffer (nullptr),
  seatCapabilities (0),
  serial (0),
  enterSerial (0),
  listener (*this),
  initialized (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClient::startup ()
{
	MonitorHelper::instance ().initialize ();
	
	if(socket.isValid ())
		display = socket->openWaylandConnection ();
	else
		display = wl_display_connect (nullptr);

	if(display == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to connect to Wayland display.")
		return false;
	}

	registry = wl_display_get_registry (display);
	wl_registry_add_listener (registry, &listener, &listener);

	wl_display_roundtrip (display);
	
	if(compositor == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to bind Wayland compositor.")
		return false;
	}
	if(subCompositor == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to bind Wayland subcompositor. No support for layers.")
	}
	if(windowManager == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to bind window manager.")
		return false;		
	}
	if(seat == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to bind seat.")
	}
	
	DmaBufferHelper::instance ().initialize ();

	wl_display_roundtrip (display);

	for(WaylandObject* object : savedObjects)
		object->onCompositorConnected ();
	savedObjects.removeAll ();
	
	initialized = true;
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::shutdown ()
{
	MonitorHelper::instance ().terminate ();
	DmaBufferHelper::instance ().terminate ();
	
	if(dmaBuffer)
		zwp_linux_dmabuf_v1_destroy (dmaBuffer);
	if(pointerConstraints)
		zwp_pointer_constraints_v1_destroy (pointerConstraints);
	if(relativePointerManager)
		zwp_relative_pointer_manager_v1_destroy (relativePointerManager);
	#if WAYLAND_USE_XDG_ACTIVATION
	if(activation)
		xdg_activation_v1_destroy (activation);
	#endif
	#if WAYLAND_USE_XDG_DIALOG
	if(dialogManager)
		xdg_wm_dialog_v1_destroy (dialogManager);
	#endif
	if(dataDeviceManager)
		wl_data_device_manager_destroy (dataDeviceManager);
	if(seat)
		wl_seat_release (seat);
	if(sharedMemory)
		wl_shm_destroy (sharedMemory);
	if(exporter)
		zxdg_exporter_v2_destroy (exporter);
	if(importer)
		zxdg_importer_v2_destroy (importer);
	if(exporterV1)
		zxdg_exporter_v1_destroy (exporterV1);
	if(importerV1)
		zxdg_importer_v1_destroy (importerV1);
	if(decorationManager)
		zxdg_decoration_manager_v1_destroy (decorationManager);
	if(outputManager)
		zxdg_output_manager_v1_destroy (outputManager);
	if(textInputManager)
		zwp_text_input_manager_v3_destroy (textInputManager);
	if(windowManager)
		xdg_wm_base_destroy (windowManager);
	if(subCompositor)
		wl_subcompositor_destroy (subCompositor);
	if(compositor)
		wl_compositor_destroy (compositor);
	if(registry)
		wl_registry_destroy (registry);
	if(display)
	{
		wl_display_flush (display);

		if(socket.isValid ())
			socket->closeWaylandConnection (display);
		else
			wl_display_disconnect (display);
	}
	
	dmaBuffer = nullptr;
	relativePointerManager = nullptr;
	pointerConstraints = nullptr;
	activation = nullptr;
	dialogManager = nullptr;
	dataDeviceManager = nullptr;
	seat = nullptr;
	sharedMemory = nullptr;
	exporter = nullptr;
	importer = nullptr;
	exporterV1 = nullptr;
	importerV1 = nullptr;
	decorationManager = nullptr;
	outputManager = nullptr;
	textInputManager = nullptr;
	windowManager = nullptr;
	subCompositor = nullptr;
	compositor = nullptr;
	registry = nullptr;
	display = nullptr;
	
	initialized = false;
	
	savedObjects = objects;
	for(WaylandObject* object : savedObjects)
		object->onCompositorDisconnected ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClient::hasPointerInput () const
{
	return seat != nullptr && seatCapabilities & WL_SEAT_CAPABILITY_POINTER;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClient::hasKeyboardInput () const
{
	return seat != nullptr && seatCapabilities & WL_SEAT_CAPABILITY_KEYBOARD;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool WaylandClient::hasTouchInput () const
{
	return seat != nullptr && seatCapabilities & WL_SEAT_CAPABILITY_TOUCH;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID WaylandClient::getApplicationId () const
{
	static MutableCString appId;

	if(appId.isEmpty ())
	{
		String appIdString;
		Configuration::Registry::instance ().getValue (appIdString, "CCL.Linux", "AppPackageID");
		appId = MutableCString (appIdString, Text::kASCII);
	}
	
	if(appId.isEmpty ())
	{
		IApplication* application = GUI.getApplication ();
		appId = application ? application->getApplicationPackageID () : CString::kEmpty;
	}

	return appId;
}	

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::registerObject (WaylandObject& object)
{
	if(!objects.contains (&object))
		objects.add (&object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::unregisterObject (WaylandObject& object)
{
	objects.remove (&object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::registerEventHandler (SystemEventHandler& handler)
{
	if(!eventHandlers.contains (&handler))
		eventHandlers.add (&handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::unregisterEventHandler (SystemEventHandler& handler)
{
	eventHandlers.remove (&handler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::signalEvent (SystemEvent& event)
{
	for(SystemEventHandler* handler : eventHandlers)
		handler->handleEvent (event);
}

//************************************************************************************************
// WaylandClient::Listener
//************************************************************************************************

WaylandClient::Listener::Listener (WaylandClient& context)
: context (context)
{
	global = onGlobal;
	global_remove = onGlobalRemoved;
	ping = onPing;
	capabilities = onSeatCapabilities;
	name = onSeatName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::Listener::onGlobal (void* data, wl_registry* registry, uint32_t name, const char* _interfaceName, uint32_t version)
{
	CCL_PRINTF ("Have wayland interface \"%s\", %d\n", _interfaceName, version)
	
	Listener* This = static_cast<Listener*> (data);
	
	CString interfaceName (_interfaceName);
	
	// Compositor
	if(interfaceName == wl_compositor_interface.name && This->context.getCompositor () == nullptr)
		This->context.setCompositor (static_cast<wl_compositor*> (wl_registry_bind (registry, name, &wl_compositor_interface, ccl_min<uint32_t> (WAYLAND_COMPOSITOR_VERSION, version))));
	// Subcompositor
	else if(interfaceName == wl_subcompositor_interface.name && This->context.getSubCompositor () == nullptr)
		This->context.setSubCompositor (static_cast<wl_subcompositor*> (wl_registry_bind (registry, name, &wl_subcompositor_interface, ccl_min<uint32_t> (1, version))));
	// Window Manager
	else if(interfaceName == xdg_wm_base_interface.name && This->context.getWindowManager () == nullptr)
	{
		This->context.setWindowManager (static_cast<xdg_wm_base*> (wl_registry_bind (registry, name, &xdg_wm_base_interface, ccl_min<uint32_t> (7, version))));
		if(This->context.getWindowManager () != nullptr)
			xdg_wm_base_add_listener (This->context.getWindowManager (), This, This);
	}
	// Output
	else if(interfaceName == wl_output_interface.name)
	{
		MonitorHelper::instance ().registerOutput (static_cast<wl_output*> (wl_registry_bind (registry, name, &wl_output_interface, ccl_min<uint32_t> (3, version))), name);
	}
	// Seat
	else if(interfaceName == wl_seat_interface.name && This->context.getSeat () == nullptr)
	{
		This->context.setSeat (static_cast<wl_seat*> (wl_registry_bind (registry, name, &wl_seat_interface, ccl_min<uint32_t> (WAYLAND_SEAT_VERSION, version))));
		if(This->context.getSeat () != nullptr)
			wl_seat_add_listener (This->context.getSeat (), This, This);
	}
	// Shared Memory
	else if(interfaceName == wl_shm_interface.name && This->context.getSharedMemory () == nullptr)
		This->context.setSharedMemory (static_cast<wl_shm*> (wl_registry_bind (registry, name, &wl_shm_interface, ccl_min<uint32_t> (1, version))));
	// Data Device Manager (clipboard, drag&drop)
	else if(interfaceName == wl_data_device_manager_interface.name && This->context.getDataDeviceManager () == nullptr)
		This->context.setDataDeviceManager (static_cast<wl_data_device_manager*> (wl_registry_bind (registry, name, &wl_data_device_manager_interface, ccl_min<uint32_t> (3, version))));
	// Decoration
	else if(interfaceName == zxdg_decoration_manager_v1_interface.name && This->context.getDecorationManager () == nullptr)
		This->context.setDecorationManager (static_cast<zxdg_decoration_manager_v1*> (wl_registry_bind (registry, name, &zxdg_decoration_manager_v1_interface, 1)));
	// Output Manager
	else if(interfaceName == zxdg_output_manager_v1_interface.name && This->context.getOutputManager () == nullptr)
		This->context.setOutputManager (static_cast<zxdg_output_manager_v1*> (wl_registry_bind (registry, name, &zxdg_output_manager_v1_interface, ccl_min<uint32_t> (3, version))));
	// Importer (foreign windows)
	else if(interfaceName == zxdg_importer_v2_interface.name && This->context.getImporter () == nullptr)
		This->context.setImporter (static_cast<zxdg_importer_v2*> (wl_registry_bind (registry, name, &zxdg_importer_v2_interface, 1)));
	// Exporter (foreign windows)
	else if(interfaceName == zxdg_exporter_v2_interface.name && This->context.getExporter () == nullptr)
		This->context.setExporter (static_cast<zxdg_exporter_v2*> (wl_registry_bind (registry, name, &zxdg_exporter_v2_interface, 1)));
	// Importer v1 (foreign windows, legacy interface)
	else if(interfaceName == zxdg_importer_v1_interface.name && This->context.getImporterV1 () == nullptr)
		This->context.setImporterV1 (static_cast<zxdg_importer_v1*> (wl_registry_bind (registry, name, &zxdg_importer_v1_interface, 1)));
	// Exporter v1 (foreign windows, legacy interface)
	else if(interfaceName == zxdg_exporter_v1_interface.name && This->context.getExporterV1 () == nullptr)
		This->context.setExporterV1 (static_cast<zxdg_exporter_v1*> (wl_registry_bind (registry, name, &zxdg_exporter_v1_interface, 1)));
	// Text Input
	else if(interfaceName == zwp_text_input_manager_v3_interface.name && This->context.getTextInputManager () == nullptr)
		This->context.setTextInputManager (static_cast<zwp_text_input_manager_v3*> (wl_registry_bind (registry, name, &zwp_text_input_manager_v3_interface, 1)));
	#if WAYLAND_USE_XDG_ACTIVATION
	// Activation
	else if(interfaceName == xdg_activation_v1_interface.name && This->context.getActivation () == nullptr)
		This->context.setActivation (static_cast<xdg_activation_v1*> (wl_registry_bind (registry, name, &xdg_activation_v1_interface, 1)));
	#endif
	#if WAYLAND_USE_XDG_DIALOG
	// Dialogs
	else if(interfaceName == xdg_wm_dialog_v1_interface.name && This->context.getDialogManager () == nullptr)
		This->context.setDialogManager (static_cast<xdg_wm_dialog_v1*> (wl_registry_bind (registry, name, &xdg_wm_dialog_v1_interface, 1)));
	#endif
	// Pointer Constraints
	else if(interfaceName == zwp_pointer_constraints_v1_interface.name && This->context.getPointerConstraints () == nullptr)
		This->context.setPointerConstraints (static_cast<zwp_pointer_constraints_v1*> (wl_registry_bind (registry, name, &zwp_pointer_constraints_v1_interface, 1)));
	// Relative Pointer
	else if(interfaceName == zwp_relative_pointer_manager_v1_interface.name && This->context.getRelativePointerManager () == nullptr)
		This->context.setRelativePointerManager (static_cast<zwp_relative_pointer_manager_v1*> (wl_registry_bind (registry, name, &zwp_relative_pointer_manager_v1_interface, 1)));
	// DMA Buffer
	else if(interfaceName == zwp_linux_dmabuf_v1_interface.name && This->context.getDmaBuffer () == nullptr)
		This->context.setDmaBuffer (static_cast<zwp_linux_dmabuf_v1*> (wl_registry_bind (registry, name, &zwp_linux_dmabuf_v1_interface, ccl_min<uint32_t> (4, version))));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::Listener::onGlobalRemoved (void* data, wl_registry* registry, uint32_t name)
{
	CCL_PRINTF ("Wayland global %d removed!\n", name)
	MonitorHelper::instance ().unregisterOutput (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::Listener::onPing (void* data, xdg_wm_base* windowManager, uint32_t serial)
{
	xdg_wm_base_pong (windowManager, serial);
	
	Listener* This = static_cast<Listener*> (data);
	This->context.setSerial (serial);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::Listener::onSeatCapabilities (void* data, wl_seat* seat, uint32_t capabilities)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(seat == This->context.getSeat ())
		This->context.setSeatCapabilities (capabilities);

	SystemEvent event (SystemEvent::kSeatCapabilitiesChanged);
	WaylandClient::instance ().signalEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WaylandClient::Listener::onSeatName (void* data, wl_seat* seat, const char* name)
{
	Listener* This = static_cast<Listener*> (data);
	
	if(seat == This->context.getSeat ())
	{
		This->context.setSeatName (name);
		CCL_PRINTF ("Seat name: %s\n", name)
	}
}
