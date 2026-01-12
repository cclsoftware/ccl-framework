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
// Filename    : ccl/system/logger.cpp
// Description : Logger
//
//************************************************************************************************

#include "ccl/system/logger.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// System Services API
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT void CCL_API System::CCL_ISOLATED (DebugReportWarning) (ModuleRef module, StringRef message)
{
	Alert::Event e (message, Alert::kWarning);

	Url path;
	AutoPtr<IExecutableImage> executable = System::GetExecutableLoader ().createImage (module);
	ASSERT (executable != nullptr)
	executable->getPath (path);
	path.getName (e.moduleName, false);

	System::GetLogger ().reportEvent (e);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT System::ILogger& CCL_API System::CCL_ISOLATED (GetLogger) ()
{
	static Logger theLogger;
	return theLogger;
}

//************************************************************************************************
// Logger
//************************************************************************************************

Logger::~Logger ()
{
	ASSERT (outputs.isEmpty () == true)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Logger::addOutput (Alert::IReporter* output)
{
	Threading::ScopedLock scopedLock (lock);
	outputs.append (output);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Logger::removeOutput (Alert::IReporter* output)
{
	Threading::ScopedLock scopedLock (lock);
	outputs.remove (output);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Logger::reportEvent (const Alert::Event& _e)
{
	Alert::Event e (_e);
	if(e.time == DateTime ())
		System::GetSystem ().getLocalTime (e.time);

	Threading::ScopedLock scopedLock (lock);
	ListForEach (outputs, Alert::IReporter*, output)
		output->reportEvent (e);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Logger::setReportOptions (Severity minSeverity, int eventFormat)
{
	Threading::ScopedLock scopedLock (lock);
	ListForEach (outputs, Alert::IReporter*, output)
		output->setReportOptions (minSeverity, eventFormat);
	EndFor
}
