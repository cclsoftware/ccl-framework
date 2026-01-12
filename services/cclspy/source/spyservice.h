//************************************************************************************************
//
// CCL Spy
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
// Filename    : spyservice.h
// Description : Spy Service plugin
//
//************************************************************************************************

#ifndef _spyservice_h
#define _spyservice_h

#include "ccl/public/plugins/serviceplugin.h"

namespace Spy {
class SpyManager; }

//************************************************************************************************
// SpyService
//************************************************************************************************

class SpyService: public CCL::ServicePlugin
{
public:
	SpyService ();
	~SpyService ();

	static IUnknown* createInstance (CCL::UIDRef, void*);

	// ServicePlugin
	CCL::tresult CCL_API initialize (IUnknown* context = nullptr) override;
	CCL::tresult CCL_API terminate () override;

private:
	Spy::SpyManager* manager;
};

#endif // _spyservice_h
