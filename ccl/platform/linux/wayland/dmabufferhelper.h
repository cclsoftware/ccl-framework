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
// Filename    : ccl/platform/linux/wayland/dmabufferhelper.h
// Description : Wayland DMA Buffer Handling
//
//************************************************************************************************

#ifndef _ccl_linux_dmabufferhelper_h
#define _ccl_linux_dmabufferhelper_h

#include "ccl/base/singleton.h"
#include "ccl/platform/linux/gui/window.linux.h"

#include "wayland-server-delegate/iwaylandclientcontext.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// DmaBufferHelper
//************************************************************************************************

class DmaBufferHelper: public Object,
					   public Singleton<DmaBufferHelper>
{
public:
	DmaBufferHelper ();
	
	void initialize ();
	void terminate ();

	int countModifiers () const;
	bool getModifier (uint32& format, uint32& modifierHigh, uint32& modifierLow, int index) const;
	void addModifier (uint32 format, uint32 modifierHigh, uint32 modifierLow);
	void clear ();

protected:
	struct Listener: zwp_linux_dmabuf_v1_listener,
					 zwp_linux_dmabuf_feedback_v1_listener
	{
		Listener ();

		// zwp_linux_dmabuf_v1_listener
		static void onFormat (void* data, zwp_linux_dmabuf_v1* zwp_linux_dmabuf_v1, uint32_t format) {}
		static void onModifier (void* data, zwp_linux_dmabuf_v1* zwp_linux_dmabuf_v1, uint32_t format, uint32_t modifierHigh, uint32_t modifierLow);

		// zwp_linux_dmabuf_feedback_v1_listener
		static void onDone (void* data, zwp_linux_dmabuf_feedback_v1* feedback) {}
		static void onFormatTable (void* data, zwp_linux_dmabuf_feedback_v1* feedback, int32_t fd, uint32_t size);
		static void onMainDevice (void* data, zwp_linux_dmabuf_feedback_v1* feedback, wl_array* device) {}
		static void onTrancheDone (void* data, zwp_linux_dmabuf_feedback_v1* feedback) {}
		static void onTrancheTargetDevice (void* data, zwp_linux_dmabuf_feedback_v1* feedback, wl_array* device) {}
		static void onTrancheFormats (void* data, zwp_linux_dmabuf_feedback_v1* feedback, wl_array* indices) {}
		static void onTrancheFlags (void* data, zwp_linux_dmabuf_feedback_v1* feedback, uint32_t flags) {}
	};

	struct ModifierEntry
	{
		uint32 format;
		uint32 modifierHigh;
		uint32 modifierLow;
	};

	struct DmaBufferModifier
	{
		uint32 format;
		uint32 padding;
		uint64 modifier;
	};

	Listener listener;
	zwp_linux_dmabuf_feedback_v1* feedback;
	Vector<ModifierEntry> modifiers;
};

} // namespace Linux
} // namespace CCL

#endif // _ccl_linux_dmabufferhelper_h
