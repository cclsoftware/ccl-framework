//************************************************************************************************
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
// Filename    : ccl/gui/system/inputmethod.cpp
// Description : Input Method Support
//
//************************************************************************************************

#include "ccl/gui/system/inputmethod.h"

using namespace CCL;

//************************************************************************************************
// IInputMethod / ITextInputClient
//************************************************************************************************

DEFINE_IID_ (IInputMethod, 0x9e924a01, 0xbb9e, 0x4d17, 0x9c, 0x3e, 0xbb, 0x25, 0x2e, 0x0d, 0x29, 0x1e)
DEFINE_IID_ (ITextInputClient, 0xb123bf3f, 0xfdcc, 0x406a, 0xbd, 0x6b, 0x74, 0x26, 0xfb, 0xc5, 0x07, 0xb6)

//************************************************************************************************
// InputMethodManager
//************************************************************************************************

IInputMethod* InputMethodManager::createInputMethod (ITextInputClient& client)
{
	return nullptr;
}
