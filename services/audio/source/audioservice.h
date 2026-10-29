//************************************************************************************************
//
// Audio Service
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
//
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : audioservice.h
// Description : Audio Service
//
//************************************************************************************************

#ifndef _audioservice_h
#define _audioservice_h

#include "ccl/public/plugins/serviceplugin.h"

namespace CCL {

//************************************************************************************************
// AudioService
//************************************************************************************************

class AudioService: public ServicePlugin
{
public:
	// ServicePlugin
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
};

} // namespace CCL

#endif // _audioservice_h
