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
// Filename    : ccl/platform/linux/wayland/monitorhelper.h
// Description : Wayland Output Handling
//
//************************************************************************************************

#ifndef _ccl_linux_monitorhelper_h
#define _ccl_linux_monitorhelper_h

#include "ccl/base/singleton.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "wayland-server-delegate/iwaylandclientcontext.h"

namespace CCL {
namespace Linux {

struct WaylandOutput: WaylandServerDelegate::WaylandOutput
{
	uint32_t id = 0;
	Rect logicalSize = Rect ();
	zxdg_output_v1* xdgOutput = nullptr;
};

//************************************************************************************************
// MonitorHelper
//************************************************************************************************

class MonitorHelper: public Object,
					 public Singleton<MonitorHelper>
{
public:
	MonitorHelper ();
	
	PROPERTY_OBJECT (Point, workAreaSize, WorkAreaSize)

	void initialize ();
	void terminate ();
	
	void registerOutput (wl_output* output, uint32_t id);
	void unregisterOutput (uint32_t id);
	
	int countOutputs () const;
	WaylandOutput& getOutput (int index) const;
	int getScaleFactor (wl_output* output);

protected:
	struct Listener: wl_output_listener,
					 zxdg_output_v1_listener
	{
		Listener ();
		
		// wl_output_listener
		static void onGeometry (void* data, wl_output* output, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subPixel, const char* make, const char* model, int32_t transform);
		static void onMode (void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
		static void onDone (void* data, wl_output* output);
		static void onScale (void* data, wl_output* output, int32_t factor);

		// zxdg_output_v1_listener
		static void onLogicalPosition (void* data, zxdg_output_v1* output, int32_t x, int32_t y);
		static void onLogicalSize (void* data, zxdg_output_v1* output, int32_t width, int32_t height);
		static void onDone (void* data, zxdg_output_v1* output);
		static void onName (void* data, zxdg_output_v1* output, const char *name);
		static void onDescription (void* data, zxdg_output_v1* output, const char* description);
	};

	Listener listener;
	Vector<WaylandOutput> outputs;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_monitorhelper_h
