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
// Filename    : ccl/base/storage/xmlpihandler.h
// Description : XML Processing Instruction Handler
//
//************************************************************************************************

#ifndef _ccl_xmlpihandler_h
#define _ccl_xmlpihandler_h

#include "ccl/public/base/platform.h"

namespace CCL {

//************************************************************************************************
// XmlProcessingInstructionHandler
/** Mix-in class for conditional XML decoding via processing instruction. */
//************************************************************************************************

class XmlProcessingInstructionHandler
{
public:
	enum ProcessingOptions
	{
		kForceReleaseConfiguration = 1<<0
	};

	XmlProcessingInstructionHandler (int processingOptions = 0);

	static const char* getPlatform ();
	static const char* getPlatformArchitecture ();
	static const char* getConfiguration (int processingOptions = 0);

	void handleInstruction (StringRef target, StringRef data);

protected:
	int processingOptions;
	bool skipping;
};

} // namespace CCL

#endif // _ccl_xmlpihandler_h
