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
// Filename    : core/platform/shared/crt/corecompileall.cpp
// Description : C Runtime Library conditional compilation file
//
//************************************************************************************************

#include "core/public/coreclibrary.h"

#ifdef CORE_STRCASECMP_NOT_PROVIDED
#include "corestrcasecmp.cpp"
#endif

#ifdef CORE_SSCANF_NOT_PROVIDED
#include "coresscanf.cpp"
#endif
