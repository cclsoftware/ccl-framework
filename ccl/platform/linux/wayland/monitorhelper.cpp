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
// Filename    : ccl/platform/linux/wayland/monitorhelper.cpp
// Description : Wayland Output Handling
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/monitorhelper.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// MonitorHelper
//************************************************************************************************

DEFINE_SINGLETON (MonitorHelper)

//////////////////////////////////////////////////////////////////////////////////////////////////

MonitorHelper::MonitorHelper ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::initialize ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::terminate ()
{
	for(WaylandOutput& output : outputs)
	{
		if(output.xdgOutput)
			zxdg_output_v1_destroy (output.xdgOutput);
		wl_output_destroy (output.handle);
	}
	outputs.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::registerOutput (wl_output* output, uint32_t id)
{
	if(!outputs.contains ({output}))
	{
		WaylandOutput waylandOutput ({output});
		waylandOutput.id = id;
		zxdg_output_manager_v1* outputManager = WaylandClient::instance ().getOutputManager ();
		if(outputManager != nullptr)
			waylandOutput.xdgOutput = zxdg_output_manager_v1_get_xdg_output (outputManager, output);
		outputs.add (waylandOutput);
		wl_output_add_listener (output, &listener, &listener);
		if(waylandOutput.xdgOutput)
			zxdg_output_v1_add_listener (waylandOutput.xdgOutput, &listener, &listener);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::unregisterOutput (uint32_t id)
{
	bool removed = false;
	for(WaylandOutput& output : outputs)
	{
		if(output.id == id)
		{
			if(output.xdgOutput)
				zxdg_output_v1_destroy (output.xdgOutput);
			wl_output_destroy (output.handle);
			removed = outputs.remove (output);
			break;
		}
	}

	if(removed)
	{
		SystemEvent event (SystemEvent::kOutputsChanged);
		WaylandClient::instance ().signalEvent (event);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MonitorHelper::countOutputs () const
{
	return outputs.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WaylandOutput& MonitorHelper::getOutput (int index) const
{
	return outputs.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int MonitorHelper::getScaleFactor (wl_output* handle)
{
	for(WaylandOutput& output : outputs)
	{
		if(output.handle == handle)
			return output.scaleFactor;
	}
	return 1;
}

//************************************************************************************************
// MonitorHelper::Listener
//************************************************************************************************

MonitorHelper::Listener::Listener ()
{
	wl_output_listener::geometry = onGeometry;
	wl_output_listener::mode = onMode;
	wl_output_listener::done = onDone;
	wl_output_listener::scale = onScale;
	zxdg_output_v1_listener::logical_position = onLogicalPosition;
	zxdg_output_v1_listener::logical_size = onLogicalSize;
	zxdg_output_v1_listener::done = onDone;
	zxdg_output_v1_listener::name = onName;
	zxdg_output_v1_listener::description = onDescription;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onGeometry (void* data, wl_output* handle, int32_t x, int32_t y, int32_t physicalWidth, int32_t physicalHeight, int32_t subPixel, const char* make, const char* model, int32_t transform)
{
	for(WaylandOutput& output : MonitorHelper::instance ().outputs)
	{
		if(output.handle == handle)
		{
			output.x = x;
			output.y = y;
			output.physicalWidth = physicalWidth;
			output.physicalHeight = physicalHeight;
			output.subPixelOrientation = subPixel;
			CString (make).copyTo (output.manufacturer, ARRAY_COUNT (output.manufacturer));
			CString (model).copyTo (output.model, ARRAY_COUNT (output.model));
			output.transformType = transform;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onMode (void* data, wl_output* handle, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
	if(flags & WL_OUTPUT_MODE_CURRENT)
	{
		for(WaylandOutput& output : MonitorHelper::instance ().outputs)
		{
			if(output.handle == handle)
			{
				output.width = width;
				output.height = height;
				output.refreshRate = refresh;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onDone (void* data, wl_output* output)
{
	SystemEvent event (SystemEvent::kOutputsChanged);
	WaylandClient::instance ().signalEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onScale (void* data, wl_output* handle, int32_t factor)
{
	for(WaylandOutput& output : MonitorHelper::instance ().outputs)
	{
		if(output.handle == handle)
			output.scaleFactor = factor;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onLogicalPosition (void* data, zxdg_output_v1* handle, int32_t x, int32_t y)
{
	for(WaylandOutput& output : MonitorHelper::instance ().outputs)
	{
		if(output.xdgOutput == handle)
			output.logicalSize.moveTo (Point (x, y));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onLogicalSize (void* data, zxdg_output_v1* handle, int32_t width, int32_t height)
{
	for(WaylandOutput& output : MonitorHelper::instance ().outputs)
	{
		if(output.xdgOutput == handle)
			output.logicalSize.setSize (Point (width, height));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onDone (void* data, zxdg_output_v1* handle)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onName (void* data, zxdg_output_v1* handle, const char *name)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MonitorHelper::Listener::onDescription (void* data, zxdg_output_v1* handle, const char* description)
{}
