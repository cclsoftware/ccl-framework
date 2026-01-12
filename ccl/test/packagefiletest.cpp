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
// Filename    : packagefiletest.cpp
// Description : Package File Unit tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/archivehandler.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/base/streamer.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// PackageFileTest::SaveTask
//************************************************************************************************

class SaveTask: public CCL::ArchiveSaveTask
{
public:
	MutableCString dataString;

	SaveTask (StringID dataString)
	: dataString (dataString)
	{}

	// ArchiveSaveTask
	tresult CCL_API writeData (CCL::IStream& dstStream, CCL::IProgressNotify* progress) override
	{
		Streamer (dstStream).writeCString (dataString);
		return kResultOk;
	}
};

//************************************************************************************************
// PackageFileTest::SaveTask2
//************************************************************************************************

class SaveTask2: public CCL::ArchiveSaveTask
{
public:
	String dataString;

	SaveTask2 (StringRef dataString)
	: dataString (dataString)
	{}

	// ArchiveSaveTask
	tresult CCL_API writeData (CCL::IStream& dstStream, CCL::IProgressNotify* progress) override
	{
		Streamer (dstStream).writeString (dataString);
		return kResultOk;
	}
};

//************************************************************************************************
// PackageFileTest
//************************************************************************************************

CCL_TEST (PackageFileTest, TestXTEAEncryption)
{
	TempFile packageUrl ("test xtea");
	Url dataUrl ("/data.txt");

	String key ("0123456789abcdef0123456789abcdef");
	const char* dataString = "0123456789abcdefghijklmnopqrstuvwxyz";

	// 1) as C String

	Logging::debug ("Writing xtea encrypted package 1");
	{
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (packageUrl.getPath (), CCL::ClassID::PackageFile);
		packageFile->setOption (PackageOption::kFormatVersion, 2);
		packageFile->setOption (PackageOption::kXTEAEncrypted, true);
		packageFile->setOption (PackageOption::kExternalEncryptionKey, key);
		packageFile->create ();

		IPackageFile::Closer packageFileCloser (*packageFile);

		int attributes = IPackageItem::kEncrypted|IPackageItem::kUseExternalKey;
		packageFile->createItem (dataUrl, NEW SaveTask (dataString), &attributes);

		packageFile->flush ();
	}

	Logging::debug ("Reading xtea encrypted package 1:");
	{
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler (). openPackage (packageUrl.getPath () );
		packageFile->setOption (PackageOption::kExternalEncryptionKey, key);
		packageFile->open ();

		IPackageFile::Closer packageFileCloser (*packageFile);

		AutoPtr<IStream> dataFile = packageFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);

		char string[256];
		dataFile->read (string, 256);

		CCL_TEST_ASSERT (String (dataString) == string);
		Logging::debug (dataString);
		Logging::debug (string);
	}

	// 1) as CCL String

	Logging::debug ("Writing xtea encrypted package 2");
	{
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler ().createPackage (packageUrl.getPath (), CCL::ClassID::PackageFile);
		packageFile->setOption (PackageOption::kFormatVersion, 2);
		packageFile->setOption (PackageOption::kXTEAEncrypted, true);
		packageFile->setOption (PackageOption::kExternalEncryptionKey, key);
		packageFile->create ();

		IPackageFile::Closer packageFileCloser (*packageFile);

		int attributes = IPackageItem::kEncrypted|IPackageItem::kUseExternalKey;
		packageFile->createItem (dataUrl, NEW SaveTask2 (dataString), &attributes);

		packageFile->flush ();
	}

	Logging::debug ("Reading xtea encrypted package 2:");
	{
		AutoPtr<IPackageFile> packageFile = System::GetPackageHandler (). openPackage (packageUrl.getPath () );
		packageFile->setOption (PackageOption::kExternalEncryptionKey, key);
		packageFile->open ();

		IPackageFile::Closer packageFileCloser (*packageFile);

		AutoPtr<IStream> dataFile = packageFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);

		String string;
		Streamer (*dataFile).readString (string);

		CCL_TEST_ASSERT (String (dataString) == string);
		Logging::debug (dataString);
		Logging::debug (string);

		Logging::debug ("Seeking:");
		for(int i = 0; i < 36; i++)
		{
			dataFile->seek (2 * i, IStream::kSeekSet);
			Streamer (*dataFile).readString (string);

			CCL_TEST_ASSERT (String (dataString).subString (i) == string);
			Logging::debug (string);
		}
	}
}
