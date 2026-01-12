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
// Filename    : zipfiletest.cpp
// Description : Package File Unit tests
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/archivehandler.h"

#include "ccl/public/base/streamer.h"
#include "ccl/public/system/cryptotypes.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Security::Crypto;

//************************************************************************************************
// ZipFileTest
//************************************************************************************************

class ZipFileTest: public Test
{
public:
	void setUp () override
	{
		uint32 size = 128 * 1024 * 1024 - 1;

		Logging::debugf ("Generating %.0d MB of test data...", size / 1024 / 1024);

		original.length = size;
		original.data = (uint8*)malloc (size);
		
		retrieved.length = size;
		retrieved.data = (uint8*)malloc (size);
		
		for(size_t i = 0 ; i < size ; i++)
			original.data[i] = (uint8) i;
	}

	void tearDown () override
	{
		if(original.data)
		{
			free (original.data);
			original.data = nullptr;
			original.length = 0;
		}

		
		if(retrieved.data)
		{
			free (retrieved.data);
			retrieved.data = nullptr;
			retrieved.length = 0;
		}
	}

protected:
	class SaveTask;
	
	Security::Crypto::Block original;
	Security::Crypto::Block retrieved;

	bool compare ()
	{
		if(original.length != retrieved.length)
			return false;
		
		return memcmp (original.data, retrieved.data, original.length) == 0;
	}
};

//************************************************************************************************
// SaveTask
//************************************************************************************************

class ZipFileTest::SaveTask: public CCL::ArchiveSaveTask
{
public:
	BlockRef buffer;

	SaveTask (BlockRef _buffer)
	: buffer (_buffer)
	{}

	// ArchiveSaveTask
	tresult CCL_API writeData (CCL::IStream& dstStream, CCL::IProgressNotify* progress) override
	{
		Streamer (dstStream).write (buffer.data, buffer.length);
		return kResultOk;
	}
};

CCL_TEST_F (ZipFileTest, TestCompression)
{
	TempFile packageUrl ("compressed.zip");
	Url dataUrl ("/counting.bin");

	Logging::debug ("Writing compressed zip file:");
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().createPackage (packageUrl.getPath (), CCL::ClassID::ZipFile);
		zipFile->create ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		int attributes = IPackageItem::kCompressed;
		int64 startTime = System::GetSystemTicks () - 1;
		zipFile->createItem (dataUrl, NEW SaveTask (original), &attributes);
		zipFile->flush ();
		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
	}	

	Logging::debug ("Reading compressed zip file:");
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().openPackage (packageUrl.getPath () );
		zipFile->open ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		AutoPtr<IStream> dataFile = zipFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);
		int64 startTime = System::GetSystemTicks () - 1;
		retrieved.length = dataFile->read (retrieved.data, retrieved.length);

		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
		CCL_TEST_ASSERT (compare ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
	
CCL_TEST_F (ZipFileTest, TestAESEncryption)
{
	TempFile packageUrl ("encrypted.zip");
	Url dataUrl ("/counting.bin");

	String key ("00112233445566778899AABBCCDDEEFF");
	
	Logging::debug ("Writing encrypted zip file:");
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().createPackage (packageUrl.getPath (), CCL::ClassID::ZipFile);
		zipFile->setOption (PackageOption::kAESEncrypted, true);
		zipFile->setOption (PackageOption::kExternalEncryptionKey, key);
		zipFile->create ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		int attributes = IPackageItem::kEncrypted|IPackageItem::kUseExternalKey;
		int64 startTime = System::GetSystemTicks () - 1;
		zipFile->createItem (dataUrl, NEW SaveTask (original), &attributes);
		zipFile->flush ();
		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
	}
	
	Logging::debug ("Reading encrypted zip file (1/2):");
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().openPackage (packageUrl.getPath () );
		zipFile->setOption (PackageOption::kExternalEncryptionKey, key);
		zipFile->open ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		AutoPtr<IStream> dataFile = zipFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);
		int64 startTime = System::GetSystemTicks () - 1;
		retrieved.length = dataFile->read (retrieved.data, retrieved.length);
		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
		CCL_TEST_ASSERT (compare ());
	}
	
	Logging::debug ("Reading encrypted zip file (2/2):");
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().openPackage (packageUrl.getPath () );
		zipFile->setOption (PackageOption::kExternalEncryptionKey, key);
		zipFile->open ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		AutoPtr<IStream> dataFile = zipFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);
		int64 startTime = System::GetSystemTicks () - 1;
		retrieved.length = dataFile->read (retrieved.data, retrieved.length);
		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
		CCL_TEST_ASSERT (compare ());
	}
	
	const unsigned int blocks = 100;
	Logging::debugf ("Reading encrypted zip file with seek, %d blocks:", blocks);
	{
		AutoPtr<IPackageFile> zipFile = System::GetPackageHandler ().openPackage (packageUrl.getPath () );
		zipFile->setOption (PackageOption::kExternalEncryptionKey, key);
		zipFile->open ();
		IPackageFile::Closer zipFileCloser (*zipFile);
		AutoPtr<IStream> dataFile = zipFile->getFileSystem ()->openStream (dataUrl, IStream::kReadMode);
		int64 startTime = System::GetSystemTicks () - 1;
		memset (retrieved.data, 0x00, retrieved.length);
		const unsigned int blockSize = original.length / blocks;

		// odd parts, forward
		for(int i = 0; i < blocks; i += 2)
		{
			dataFile->seek (i * blockSize, IStream::kSeekSet);
			dataFile->read (retrieved.data + i * blockSize, blockSize);
		}

		// even parts, backward
		for(int i = blocks - 1; i > 0 ; i -= 2)
		{	
			dataFile->seek (i * blockSize, IStream::kSeekSet);
			dataFile->read (retrieved.data + i * blockSize, blockSize);
		}

		// the rest
		const int remainder = original.length - blockSize * blocks;
		dataFile->seek (-remainder, IStream::kSeekEnd);
		dataFile->read (retrieved.data + blocks * blockSize, remainder);
		
		retrieved.length = original.length;
		Logging::debugf ("%.1f MB/s\n", original.length / (System::GetSystemTicks () - startTime) / 1000.f);
		CCL_TEST_ASSERT (compare ());
	}
}
