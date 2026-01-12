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
// Filename    : core/public/coredefpush.h
// Description : Push core definitions
//
//************************************************************************************************

// This header file does not contain the usual include guard, as it might be needed several times per translation unit.
// Balance with coredefpop.h when using in a header file

#pragma push_macro ("malloc")
#undef malloc
#pragma push_macro ("free")
#undef free
#pragma push_macro ("realloc")
#undef realloc
