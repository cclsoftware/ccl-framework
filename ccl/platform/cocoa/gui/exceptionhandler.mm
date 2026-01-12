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
// Filename    : ccl/platform/cocoa/gui/exceptionhandler.mm
// Description : Cocoa Exception Handler
//
//************************************************************************************************

#include "ccl/platform/cocoa/gui/exceptionhandler.h"
#include "ccl/platform/cocoa/macutils.h"

#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isafetymanager.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// CocoaExceptionHandler
//************************************************************************************************

DEFINE_UNMANAGED_SINGLETON (CocoaExceptionHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaExceptionHandler* CocoaExceptionHandler::theInstance = nullptr;

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaExceptionHandler::handleSignal (int sig, siginfo_t* info, void* context)
{
	switch(sig)
	{
	case SIGSEGV:
	case SIGABRT:
	case SIGILL:
	case SIGBUS:
	case SIGTRAP:
		System::GetSafetyManager ().reportException (context, theInstance->nativeDumpPath);
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

CocoaExceptionHandler::CocoaExceptionHandler ()
{
	ASSERT (theInstance == nullptr)
	theInstance = this;

	signalHandler = handleSignal;
	
	Url path;
	MacUtils::urlFromNSUrl (path, [[[NSFileManager defaultManager] URLForDirectory:NSLibraryDirectory inDomain:NSUserDomainMask appropriateForURL:nullptr create:NO error:nil]  URLByAppendingPathComponent:@"Logs/DiagnosticReports/*" isDirectory:NO], IUrl::kFile);
	path.toNativePath (nativeDumpPath, nativeDumpPath.size ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaExceptionHandler::~CocoaExceptionHandler ()
{
	ASSERT (theInstance == this)
	theInstance = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaExceptionHandler::install ()
{
	PosixExceptionHandler::install ();
	
	const IExecutableImage& mainImage = System::GetExecutableLoader ().getMainImage ();
	Url executablePath;
	mainImage.getPath (executablePath);
	executablePath.descend ("Contents");
	executablePath.descend ("MacOS");
	ForEachFile (System::GetFileSystem ().newIterator (executablePath, IFileIterator::kFiles), file)
		String extension;
		file->getExtension (extension);
		if(extension.isEmpty ())
		{
			file->getName (dumpFilePrefix);
			break;
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool CocoaExceptionHandler::isValidDumpFile (UrlRef file) const
{
	String fileName;
	file.getName (fileName);
	return fileName.startsWith (dumpFilePrefix);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaExceptionHandler::scanDumps ()
{
	if(savedDumpsValid)
		return;

	savedDumps.removeAll ();

	Url dumpFolder;
	dumpFolder.fromNativePath (nativeDumpPath);
	dumpFolder.ascend ();
	
	scanDumpFolder (dumpFolder);

	savedDumpsValid = true;
}
