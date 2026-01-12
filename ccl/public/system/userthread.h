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
// Filename    : ccl/public/system/userthread.h
// Description : User thread base class
//
//************************************************************************************************

#ifndef _ccl_userthread_h
#define _ccl_userthread_h

#include "ccl/public/system/ithreading.h"

namespace CCL {
namespace Threading {

//************************************************************************************************
// UserThread
/** Helper for implementing a class that runs a member function in a thread.
	Derive from UserThread and implement threadEntry to do the thread work. 
	\ingroup ccl_system */
//************************************************************************************************

class UserThread
{
public:
	UserThread (const char* threadName = "User Thread");
	~UserThread ();
	
	bool isThreadStarted () const;	///< true if native thread exists, even if already finished with execution
	bool isThreadAlive () const;	///< true if thread has been started and not finished execution yet 

	void startThread (ThreadPriority priority, int cpuAffinity = -1);
	bool stopThread (unsigned int milliseconds);
	void requestTerminate ();

protected:
	bool shouldTerminate () const;	///< to be checked periodically...

	virtual int threadEntry () = 0; ///< implement to do the thread work

private:
	static int CCL_API threadFunc (void* arg);

	const char* threadName;
	IThread* thread;
	bool threadAlive;
	bool terminateRequested;
};

} // namespace Threading
} // namespace CCL

#endif // _ccl_userthread_h
