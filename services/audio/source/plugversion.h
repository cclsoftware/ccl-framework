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
// Filename    : plugversion.h
// Description : Audio Service Plug-in Version
//
//************************************************************************************************

#ifndef _plugversion_h
#define _plugversion_h

#include "ccl/public/cclversion.h"

#include "ccl/public/audio/iaudioservice.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_ID				"audioservice"
#define PLUG_NAME			CCL_AUDIOSERVICE_NAME_
#define PLUG_CLASS_UID		UID (0x8b8df67c, 0x2b78, 0x48e7, 0x9c, 0xd2, 0xae, 0x42, 0xa1, 0x7b, 0x92, 0xb8)
#define PLUG_PACKAGE_ID		CCL_PACKAGE_DOMAIN "." PLUG_ID

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _plugversion_h
