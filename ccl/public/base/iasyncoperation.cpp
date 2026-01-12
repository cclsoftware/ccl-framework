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
// Filename    : ccl/public/base/iasyncoperation.cpp
// Description : Asynchronous operation interfaces
//
//************************************************************************************************

#include "ccl/public/base/iasyncoperation.h"

using namespace CCL;

DEFINE_IID_ (IAsyncCompletionHandler, 0x2F561B7F, 0x36D0, 0x4C10, 0x8B, 0x01, 0xB0, 0xF3, 0x9D, 0x51, 0x9E, 0xE4)
DEFINE_IID_ (IAsyncInfo, 0x26e783fe, 0x62bf, 0x427a, 0xa5, 0x44, 0x6b, 0xd2, 0x71, 0xe8, 0xdc, 0xf0)
DEFINE_IID_ (IAsyncOperation, 0x5395A579, 0xE320, 0x48DB, 0x8B, 0x40, 0x0F, 0xAE, 0x65, 0xB7, 0x2D, 0xFD)
DEFINE_IID_ (IAsyncCall, 0xc8896a45, 0x4646, 0x46e8, 0xb3, 0xea, 0x46, 0x13, 0xab, 0x33, 0x43, 0x36)
