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
// Filename    : ccl/app/components/istartupprogress.h
// Description : Startup Progress Interface
//
//************************************************************************************************

#ifndef _ccl_istartupprogress_h
#define _ccl_istartupprogress_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IComponent;

//************************************************************************************************
// IStartupProgress
//************************************************************************************************

interface IStartupProgress: IUnknown
{
	/** Register startup component. */
	virtual void CCL_API declareStartupComponent (IComponent* component) = 0;
	
	/** Report startup begin. */
	virtual void CCL_API reportStartup (IComponent* component, StringRef title) = 0;
	
	/** Report startup end. */
	virtual void CCL_API reportStartupDone (IComponent* component) = 0;
	
	// Signals
	DECLARE_STRINGID_MEMBER (kCollectStartupComponents)	///< args[0]: IStartupProgress
	
	DECLARE_IID (IStartupProgress)
	
	//////////////////////////////////////////////////////////////////////////////////////////////

	static IStartupProgress* getInstance ();
	static void declareStartup (MessageRef msg, IComponent* component);
};

//************************************************************************************************
// StartupProgressScope
//************************************************************************************************

struct StartupProgressScope
{
	IStartupProgress* startupProgress;
	IComponent* component;
	
	StartupProgressScope (IComponent* component, StringRef title)
	: startupProgress (nullptr),
	  component(component)
	{
		startupProgress = IStartupProgress::getInstance ();
		if(startupProgress && component)
			startupProgress->reportStartup (component, title);
	}
	
	~StartupProgressScope ()
	{
		if(startupProgress && component)
			startupProgress->reportStartupDone (component);
	}
};

} // namespace CCL

#endif // _ccl_istartupprogress_h
