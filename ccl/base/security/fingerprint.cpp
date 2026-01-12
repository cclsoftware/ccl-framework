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
// Filename    : ccl/base/security/fingerprint.cpp
// Description : Fingerprint
//
//************************************************************************************************

#include "ccl/base/security/fingerprint.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/security/cryptobox.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/base/iprogress.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/system/userthread.h"
#include "ccl/public/systemservices.h"

namespace CCL {
namespace Security {

//************************************************************************************************
// FileFingerprint::FingerprintCalculator
//************************************************************************************************

class FileFingerprint::FingerprintCalculator: public CCL::Unknown,
											  public Threading::UserThread,
											  public CCL::AbstractProgressNotify
{
public:
	FingerprintCalculator (FileFingerprint& fingerprint);

	PROPERTY_AUTO_POINTER (AsyncOperation, operation, Operation)

	// AbstractProgressNotify
	tbool CCL_API isCanceled () override;

	// UserThread
	int threadEntry () override;

	CLASS_INTERFACE (IProgressNotify, Unknown)

private:
	FileFingerprint& fingerprint;
};

} // namespace Security
} // namespace CCL

using namespace CCL;
using namespace Security;

//************************************************************************************************
// Fingerprint
//************************************************************************************************

DEFINE_CLASS_HIDDEN (FileFingerprint, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

FileFingerprint::FileFingerprint (UrlRef filePath)
: filePath (filePath),
  calculator (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

FileFingerprint::~FileFingerprint ()
{
	if(calculator)
		calculator->stopThread (5000);
	safe_release (calculator);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAsyncOperation* FileFingerprint::calculate ()
{
	if(calculator == nullptr)
	{
		calculator = NEW FingerprintCalculator (*this);
		calculator->startThread (Threading::kPriorityBelowNormal);
	}
	return return_shared (calculator->getOperation ());
}

//************************************************************************************************
// AppFingerprint
//************************************************************************************************

DEFINE_CLASS_HIDDEN (AppFingerprint, FileFingerprint)

//////////////////////////////////////////////////////////////////////////////////////////////////

AppFingerprint::AppFingerprint ()
: FileFingerprint (Url ())
{
	const IExecutableImage& mainImage = System::GetExecutableLoader ().getMainImage ();
	bool succeeded = mainImage.getBinaryPath (filePath);
	ASSERT (succeeded)
}

//************************************************************************************************
// FileFingerprint::FingerprintCalculator
//************************************************************************************************

FileFingerprint::FingerprintCalculator::FingerprintCalculator (FileFingerprint& fingerprint)
: fingerprint (fingerprint),
  operation (NEW AsyncOperation)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int FileFingerprint::FingerprintCalculator::threadEntry ()
{
	operation->setStateDeferred (IAsyncOperation::kStarted);

	File file (fingerprint.filePath);

	AutoPtr<IStream> stream = file.open (IStream::kOpenMode);
	if(!stream.isValid ())
	{
		operation->setStateDeferred (IAsyncOperation::kFailed);
		return 1;
	}

	Security::Crypto::Material digest (Security::Crypto::MD5::kDigestSize);
	bool succeeded = Security::Crypto::MD5::calculate (digest, *stream, this);
	if(!succeeded)
	{
		operation->setStateDeferred (IAsyncOperation::kFailed);
		return 1;
	}

	operation->setStateDeferred (IAsyncOperation::kCompleted);
	operation->setResult (Variant ().fromString (digest.toHex ()));

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API FileFingerprint::FingerprintCalculator::isCanceled ()
{
	return shouldTerminate () || operation->getState () == IAsyncOperation::kCanceled;
}
