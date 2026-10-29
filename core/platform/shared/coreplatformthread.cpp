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
// Filename    : core/platform/shared/coreplatformthread.cpp
// Description : Multithreading platform implementation base
//
//************************************************************************************************

#include "core/platform/shared/coreplatformthread.h"

using namespace Core;
using namespace Platform;

//************************************************************************************************
// ThreadPriorityHandler
//************************************************************************************************

ThreadPriorityHandler* ThreadPriorityHandler::customHandler = nullptr;
