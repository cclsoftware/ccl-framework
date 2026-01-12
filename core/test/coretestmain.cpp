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
// Filename    : core/test/coretestmain.cpp
// Description : Simple unit test application
//
//************************************************************************************************

#include "coretestrunner.h"

#ifdef CORETEST_NO_ARGS
int main (void)
{
	char* arguments[] = {"coretestmain", "run"};
	Core::coreTest (2, arguments);
	return 0;
}
#else
int main (int argc, char* argv[])
{
	Core::coreTest (argc, argv);
	return 0;
}
#endif
