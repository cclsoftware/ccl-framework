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
// Filename    : core/system/coreinterprocess.h
// Description : Interprocess Communication
//
//************************************************************************************************

#ifndef _coreinterprocess_h
#define _coreinterprocess_h

#include "core/platform/corefeatures.h"

#if CORE_INTERPROCESS_IMPLEMENTATION == CORE_PLATFORM_IMPLEMENTATION
	#include CORE_PLATFORM_IMPLEMENTATION_HEADER (coreinterprocess)
#elif CORE_INTERPROCESS_IMPLEMENTATION == CORE_EXTERNAL_PLATFORM_IMPLEMENTATION
	#include CORE_EXTERNAL_PLATFORM_IMPLEMENTATION_HEADER (coreinterprocess)
#elif CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_IMPLEMENTATION_POSIX
	#include "core/platform/shared/posix/coreinterprocess.posix.h"
#elif CORE_INTERPROCESS_IMPLEMENTATION == CORE_FEATURE_UNIMPLEMENTED
	#include "core/platform/shared/coreplatforminterprocess.h"
#endif

namespace Core {
namespace Threads {

//************************************************************************************************
// Process Functions
//************************************************************************************************

namespace CurrentProcess
{
	/**	Get identifier of current process 
		\ingroup core_thread */
	ProcessID getID ();
}

//************************************************************************************************
// SharedMemory
/** A named piece of memory to share data between processes. 
	\ingroup core_thread */
//************************************************************************************************

class SharedMemory
{
public:
	bool create (CStringPtr name, uint32 size, bool global = false);
	bool open (CStringPtr name, uint32 size, bool global = false);
	void close ();

	void* getMemoryPointer ();
	
protected:
	Platform::SharedMemory platformMemory;
};

//************************************************************************************************
// Semaphore
/** Inter-process synchronization object. Contrary to simple user-mode locks it has an underlying
	kernel object. 
	\ingroup core_thread  */
//************************************************************************************************

class Semaphore
{
public:
	bool create (CStringPtr name);
	bool open (CStringPtr name);
	void close ();

	void lock ();
	void unlock ();

protected:
	Platform::Semaphore platformSemaphore;
};

//************************************************************************************************
// Pipe
/** Duplex named pipe for communication between processes. Implementation incomplete! 
	\ingroup core_thread  */
//************************************************************************************************

class Pipe
{
public:
	bool create (CStringPtr name);
	bool open (CStringPtr name);
	void close ();
	int read (void* buffer, int size);
	int write (const void* buffer, int size);

protected:
	Platform::Pipe platformPipe;
};

//************************************************************************************************
// Process Functions implementation
//************************************************************************************************

inline ProcessID CurrentProcess::getID ()
{ return Platform::CurrentProcess::getID (); }

//************************************************************************************************
// SharedMemory implementation
//************************************************************************************************

inline bool SharedMemory::create (CStringPtr name, uint32 size, bool global)
{ return platformMemory.create (name, size, global); }
	
inline bool SharedMemory::open (CStringPtr name, uint32 size, bool global)
{ return platformMemory.open (name, size, global); }

inline void SharedMemory::close ()
{ platformMemory.close (); }

inline void* SharedMemory::getMemoryPointer ()
{ return platformMemory.getMemoryPointer (); }

//************************************************************************************************
// Semaphore implementation
//************************************************************************************************
	
inline bool Semaphore::create (CStringPtr name)
{ return platformSemaphore.create (name); }
	
inline bool Semaphore::open (CStringPtr name)
{ return platformSemaphore.open (name); }

inline void Semaphore::close ()
{ platformSemaphore.close (); }
	
inline void Semaphore::lock ()
{ platformSemaphore.lock (); }

inline void Semaphore::unlock ()
{ platformSemaphore.unlock (); }
	
//************************************************************************************************
// Pipe implementation
//************************************************************************************************

inline bool Pipe::create (CStringPtr name)
{ return platformPipe.create (name); }

inline bool Pipe::open (CStringPtr name)
{ return platformPipe.open (name); }

inline void Pipe::close ()
{ platformPipe.close (); }

inline int Pipe::read (void* buffer, int size)
{ return platformPipe.read (buffer, size); }

inline int Pipe::write (const void* buffer, int size)
{ return platformPipe.write (buffer, size); }

} // namespace Threads
} // namespace Core

#endif // _coreinterprocess_h
