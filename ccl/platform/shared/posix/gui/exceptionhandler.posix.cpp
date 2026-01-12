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
// Filename    : ccl/platform/shared/posix/gui/exceptionhandler.posix.cpp
// Description : POSIX Exception Handler
//
//************************************************************************************************

#include "ccl/platform/linux/gui/exceptionhandler.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PosixExceptionHandler
//************************************************************************************************

PosixExceptionHandler::PosixExceptionHandler ()
: previousSigSegVHandler {},
  previousSigAbrtHandler {},
  previousSigIllHandler {},
  previousSigBusHandler {},
  previousSigTrapHandler {},
  savedDumpsValid (false),
  signalHandler (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixExceptionHandler::install ()
{
	struct sigaction handler {};
	handler.sa_sigaction = signalHandler;
	handler.sa_flags = SA_RESTART | SA_SIGINFO | SA_RESETHAND;
	
	sigaction (SIGSEGV, &handler, &previousSigSegVHandler);
	sigaction (SIGABRT, &handler, &previousSigAbrtHandler);
	sigaction (SIGBUS, &handler, &previousSigBusHandler);
	sigaction (SIGILL, &handler, &previousSigIllHandler);
	sigaction (SIGTRAP, &handler, &previousSigTrapHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PosixExceptionHandler::uninstall ()
{
	sigaction (SIGSEGV, &previousSigSegVHandler, (struct sigaction*)nullptr);
	sigaction (SIGABRT, &previousSigAbrtHandler, (struct sigaction*)nullptr);
	sigaction (SIGBUS, &previousSigBusHandler, (struct sigaction*)nullptr);
	sigaction (SIGILL, &previousSigIllHandler, (struct sigaction*)nullptr);
	sigaction (SIGTRAP, &previousSigTrapHandler, (struct sigaction*)nullptr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void PosixExceptionHandler::scanDumpFolder (UrlRef path)
{
	FileInfo info;
	int64 now = UnixTime::getTime ();
	ForEachFile (System::GetFileSystem ().newIterator (path, IFileIterator::kFiles), file)
		System::GetFileSystem ().getFileInfo (info, *file);
		if(!isValidDumpFile (*file))
			continue;
		if(UnixTime::fromLocal (info.modifiedTime) > now - 7 * DateTime::kSecondsInDay)
			savedDumps.add (*file);
		if(savedDumps.count () >= 10)
			break;
	EndFor
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API PosixExceptionHandler::countDiagnosticData () const
{
	PosixExceptionHandler* This = const_cast<PosixExceptionHandler*> (this);
	This->scanDumps ();

	return savedDumps.count ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PosixExceptionHandler::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index >= 0 && index < savedDumps.count ())
	{
		description.categoryFlags = DiagnosticDescription::kErrorInformation;
		savedDumps[index].getName (description.fileName);
		description.subFolder = "Dumps";
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API PosixExceptionHandler::createDiagnosticData (int index)
{
	if(index >= 0 && index < savedDumps.count ())
		return System::GetFileSystem ().openStream (savedDumps[index]);
	return nullptr;
}
