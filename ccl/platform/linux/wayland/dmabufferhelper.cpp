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
// Filename    : ccl/platform/linux/wayland/dmabufferhelper.cpp
// Description : Wayland DMA Buffer Handling
//
//************************************************************************************************

#include "ccl/platform/linux/wayland/dmabufferhelper.h"

#include <unistd.h>
#include <sys/mman.h>

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// DmaBufferHelper
//************************************************************************************************

DEFINE_SINGLETON (DmaBufferHelper)

//////////////////////////////////////////////////////////////////////////////////////////////////

DmaBufferHelper::DmaBufferHelper ()
: feedback (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::initialize ()
{
	modifiers.removeAll ();

	if(zwp_linux_dmabuf_v1* dmaBuffer = WaylandClient::instance ().getDmaBuffer ())
	{
		if(zwp_linux_dmabuf_v1_get_version (dmaBuffer) >= ZWP_LINUX_DMABUF_V1_GET_DEFAULT_FEEDBACK_SINCE_VERSION)
		{
			feedback = zwp_linux_dmabuf_v1_get_default_feedback (dmaBuffer);
			if(feedback)
				zwp_linux_dmabuf_feedback_v1_add_listener (feedback, &listener, &listener);
		}
		else
			zwp_linux_dmabuf_v1_add_listener (dmaBuffer, &listener, &listener);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::terminate ()
{
	if(feedback)
		zwp_linux_dmabuf_feedback_v1_destroy (feedback);
	feedback = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int DmaBufferHelper::countModifiers () const
{
	return modifiers.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::clear ()
{
	modifiers.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DmaBufferHelper::getModifier (uint32& format, uint32& modifierHigh, uint32& modifierLow, int index) const
{
	if(index < 0 || index >= modifiers.count ())
		return false;
	format = modifiers[index].format;
	modifierHigh = modifiers[index].modifierHigh;
	modifierLow = modifiers[index].modifierLow;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::addModifier (uint32 format, uint32 modifierHigh, uint32 modifierLow)
{
	modifiers.add ({format, modifierHigh, modifierLow});
}

//************************************************************************************************
// DmaBufferHelper::Listener
//************************************************************************************************

DmaBufferHelper::Listener::Listener ()
{
	zwp_linux_dmabuf_v1_listener::format = onFormat;
	zwp_linux_dmabuf_v1_listener::modifier = onModifier;

	zwp_linux_dmabuf_feedback_v1_listener::done = onDone;
	zwp_linux_dmabuf_feedback_v1_listener::format_table = onFormatTable;
	zwp_linux_dmabuf_feedback_v1_listener::main_device = onMainDevice;
	zwp_linux_dmabuf_feedback_v1_listener::tranche_done = onTrancheDone;
	zwp_linux_dmabuf_feedback_v1_listener::tranche_target_device = onTrancheTargetDevice;
	zwp_linux_dmabuf_feedback_v1_listener::tranche_formats = onTrancheFormats;
	zwp_linux_dmabuf_feedback_v1_listener::tranche_flags = onTrancheFlags;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::Listener::onModifier (void* data, zwp_linux_dmabuf_v1* zwp_linux_dmabuf_v1, uint32_t format, uint32_t modifierHigh, uint32_t modifierLow)
{
	DmaBufferHelper::instance ().addModifier (format, modifierHigh, modifierLow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DmaBufferHelper::Listener::onFormatTable (void* data, zwp_linux_dmabuf_feedback_v1* feedback, int32_t fd, uint32_t size)
{
	DmaBufferModifier* legacyFormatTable = static_cast<DmaBufferModifier*> (::mmap (nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
	::close (fd);

	if(legacyFormatTable != MAP_FAILED && legacyFormatTable != nullptr)
	{
		DmaBufferHelper::instance ().clear ();

		uint32_t index = 0;
		for(int i = 0; i < size / sizeof(DmaBufferModifier); i++)
		{
			DmaBufferModifier& tableEntry = legacyFormatTable[i];
			uint32_t modifierLow = tableEntry.modifier & 0xffffffff;
			uint32_t modifierHigh = tableEntry.modifier >> 32;

			DmaBufferHelper::instance ().addModifier (tableEntry.format, modifierHigh, modifierLow);
		}

		::munmap (legacyFormatTable, size);
	}
}
