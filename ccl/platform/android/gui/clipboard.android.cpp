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
// Filename    : ccl/platform/android/gui/clipboard.android.cpp
// Description : System Clipboard
//
//************************************************************************************************

#include "ccl/gui/system/clipboard.h"

namespace CCL {

//************************************************************************************************
// AndroidClipboard
//************************************************************************************************

class AndroidClipboard: public Clipboard
{
public:
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// AndroidClipboard
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (Clipboard, AndroidClipboard)

// todo ...
