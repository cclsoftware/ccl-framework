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
// Filename    : core/platform/win/coremalloc.win.h
// Description : Memory Allocator Debug Info
//
//************************************************************************************************

#ifndef _coremalloc_win_h
#define _coremalloc_win_h

#include "core/system/corethread.h"
#include "core/public/corestringtraits.h"

static void* core_private_alloc (size_t size);

namespace Core {

//************************************************************************************************
// DebugInfo / DebugInfoList / DebugInfoTable
//************************************************************************************************

struct DebugInfo
{
	char* filename;
	DebugInfo* next;

	DebugInfo (CStringPtr _filename)
	: next (nullptr)
	{
		size_t size = ::strlen (_filename) + 1;
		filename = (char*)core_private_alloc (size);
		::memcpy (filename, _filename, size);
	}

	void* operator new (size_t size)
	{
		return core_private_alloc (size);
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

struct DebugInfoList
{
	DebugInfo* head;

	DebugInfoList ()
	: head (nullptr)
	{}

	DebugInfo* lookup (CStringPtr filename)
	{
		for(DebugInfo* i = head; i != nullptr; i = i->next)
			if(::strcmp (filename, i->filename) == 0)
				return i;
		return nullptr;
	}

	DebugInfo* prepend (CStringPtr filename)
	{
		DebugInfo* i = new DebugInfo (filename);
		i->next = head;
		head = i;
		return i;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<int size>
struct DebugInfoTable
{
	DebugInfoList table[size];

	CStringPtr getFilename (CStringPtr filename)
	{
		int index = CStringFunctions::hashCFSIndex (filename) % size;
		DebugInfo* i = table[index].lookup (filename);
		if(i == nullptr)
			i = table[index].prepend (filename);
		return i->filename;
	}
};

} // namespace Core

//////////////////////////////////////////////////////////////////////////////////////////////////

static const char* core_get_debug_filename (const char* filename)
{
	if(filename == nullptr || *filename == 0)
		return nullptr;

	static Core::DebugInfoTable<10000> theTable;
	static Core::Threads::Lock theLock;

	Core::Threads::ScopedLock scopedLock (theLock);
	const char* result = theTable.getFilename (filename);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#include <crtdbg.h>

static void* core_private_alloc (size_t size)
{
	// Note: private allocations are never freed!
	return ::_malloc_dbg (size, _IGNORE_BLOCK, nullptr, 0);
}

#endif // _coremalloc_win_h
