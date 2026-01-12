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
// Filename    : ccl/platform/linux/gui/printservice.linux.cpp
// Description : Linux Print Service
//
//************************************************************************************************

#include "ccl/platform/linux/gui/printservice.linux.h"

#if CCLGUI_XDG_PRINTING_ENABLED
#include "ccl/platform/linux/gui/xdgprintservice.h"
#endif

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// PrintService
//************************************************************************************************

#if CCLGUI_XDG_PRINTING_ENABLED
DEFINE_EXTERNAL_SINGLETON (PrintService, XdgPrintService)
#else
DEFINE_EXTERNAL_SINGLETON (PrintService, PrintServiceStub)
#endif
