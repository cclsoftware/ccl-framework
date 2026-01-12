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
// Filename    : ccl/platform/linux/gui/exceptionhandler.cpp
// Description : Linux Exception Handler
//
//************************************************************************************************

#include "ccl/platform/linux/gui/exceptionhandler.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxExceptionHandler
//************************************************************************************************

DEFINE_UNMANAGED_SINGLETON (LinuxExceptionHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxExceptionHandler* LinuxExceptionHandler::theInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxExceptionHandler::handleSignal (int sig, siginfo_t* info, void* context)
{
	switch(sig)
	{
	case SIGSEGV:
	case SIGABRT:
	case SIGILL:
	case SIGBUS:
	case SIGTRAP:
		System::GetSafetyManager ().reportException (context, nullptr);
		break;
	default:
		return;
	}
	
	// Uninstall our handler and re-raise the signal.
	// This allows the system to generate a coredump.
	theInstance->uninstall ();
	raise (sig);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxExceptionHandler::LinuxExceptionHandler ()
{
	ASSERT (theInstance == nullptr)
	theInstance = this;

	signalHandler = handleSignal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxExceptionHandler::~LinuxExceptionHandler ()
{
	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxExceptionHandler::install ()
{
	PosixExceptionHandler::install ();
	
	const IExecutableImage& mainImage = System::GetExecutableLoader ().getMainImage ();
	Url executablePath;
	mainImage.getPath (executablePath);
	executablePath.getName (applicationFileName, false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxExceptionHandler::isValidDumpFile (UrlRef file) const
{
	String fileName;
	file.getName (fileName);
	return fileName.contains (applicationFileName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxExceptionHandler::scanDumps ()
{
	if(savedDumpsValid)
		return;

	savedDumps.removeAll ();

	for(CStringPtr dumpPath : { "/var/crash/", "/var/lib/systemd/coredump" })
	{
		Url dumpFolder;
		dumpFolder.fromPOSIXPath (dumpPath);
		scanDumpFolder (dumpFolder);
	}

	savedDumpsValid = true;
}
