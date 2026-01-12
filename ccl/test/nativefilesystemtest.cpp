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
// Filename    : nativefilesystemtest.cpp
// Description : Filesystem Unit Test class
//
//************************************************************************************************

#include "ccl/base/unittest.h"

#include "ccl/base/storage/file.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/collections/linkedlist.h"
#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/base/buffer.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/logging.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

#include <limits.h>

#if CCL_PLATFORM_WINDOWS
	#define random rand
#endif

using namespace CCL;

//************************************************************************************************
// NativeFileSystemTest
//************************************************************************************************

class NativeFileSystemTest: public Test
{
public:
	void setUp () override
	{
		fs = &System::GetFileSystem ();
	}
	
protected:
	INativeFileSystem* fs;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, FileExists)
{
	Url tempFolder;
	System::GetSystem ().getLocation (tempFolder, System::kTempFolder);
	Logging::debug ("File exists: %(1)", tempFolder.getPath ());
	CCL_TEST_ASSERT (fs->fileExists (tempFolder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, CreateDirectory)
{
	Url tempFolder;
	System::GetSystem ().getLocation (tempFolder, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFolder.descend (folderName, IUrl::kFolder);
	Logging::debug ("Create directory: %(1)", tempFolder.getPath ());
	CCL_TEST_ASSERT (fs->createFolder (tempFolder));
	fs->removeFolder (tempFolder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, RemoveDirectory)
{
	Url tempFolder;
	System::GetSystem ().getLocation (tempFolder, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFolder.descend (folderName, IUrl::kFolder);
	Logging::debug ("Remove directory: %(1)", tempFolder.getPath ());
	fs->createFolder (tempFolder);
	CCL_TEST_ASSERT (fs->removeFolder (tempFolder));
	CCL_TEST_ASSERT (!fs->fileExists (tempFolder));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, CreateNewFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Create file: %(1)", tempFile.getPath ());
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	CCL_TEST_ASSERT (stream != nullptr);
	CCL_TEST_ASSERT (fs->fileExists (tempFile));
	stream->release ();
	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, WriteReadFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Writing to file: %(1)", tempFile.getPath ());
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	const unsigned int blockSize = 1024 * 120;
	Logging::debugf ("%d bytes ", blockSize);
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	stream->write (buffer, blockSize);
	stream->release ();

	Logging::debug ("Reading from file: %(1)", tempFile.getPath ());
	Logging::debugf ("%d bytes ", blockSize);
	stream = fs->openStream (tempFile, IStream::kReadMode);
	stream->read (buffer, blockSize);
	for(int i = 0; i < blockSize; i++)
		CCL_TEST_ASSERT (buffer[i] == (i % CHAR_MAX));

	stream->release ();
	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, SeekFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Writing to file: %(1)", tempFile.getPath ());
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	const unsigned int blockSize = 1024 * 120;
	int blocks = 100;
	Logging::debugf ("%d blocks a %d bytes ", blocks, blockSize);
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	for(int i = 0; i < blocks; i++)
		stream->write (buffer, blockSize);
	stream->release ();

	Logging::debug ("Seeking in file: %(1)", tempFile.getPath ());
	stream = fs->openStream (tempFile, IStream::kReadMode);

	for(int i = 0; i < 100; i++)
	{
		int64 position = random () % (blockSize * blocks);
		Logging::debugf ("offset %lld", position);
		CCL_TEST_ASSERT (stream->seek (position, IStream::kSeekSet) != -1);
		stream->read (buffer, 1);
		CCL_TEST_ASSERT (buffer[0] == ((position % blockSize) % CHAR_MAX));
	}

	stream->release ();
	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, RemoveFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Remove file: %(1)", tempFile.getPath ());
	AutoPtr<IStream> stream = fs->openStream (tempFile, IStream::kCreateMode);
	CCL_TEST_ASSERT (fs->fileExists (tempFile));
	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, RenameFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	stream->release ();
	fs->renameFile (tempFile, "fresh.temp");
	Logging::debug ("Rename file: %(1)", tempFile.getPath ());
	CCL_TEST_ASSERT (!fs->fileExists (tempFile));
	tempFile.ascend ();
	tempFile.descend ("fresh.temp", IUrl::kFile);
	CCL_TEST_ASSERT (fs->fileExists (tempFile));
	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, CopyFile)
{
	Url tempFile1;
	System::GetSystem ().getLocation (tempFile1, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile1.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile1);
	tempFile1.descend ("dummy.temp", IUrl::kFile);
	IStream* stream = fs->openStream (tempFile1, IStream::kCreateMode);
	stream->release ();

	Url tempFile2;
	System::GetSystem ().getLocation (tempFile2, System::kTempFolder);
	folderName = UIDString::generate ();
	tempFile2.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile2);
	tempFile2.descend ("dummy.temp", IUrl::kFile);

	Logging::debug ("Copy file: %(1)", tempFile1.getPath ());
	CCL_TEST_ASSERT (fs->copyFile (tempFile2, tempFile1, 0, nullptr));
	CCL_TEST_ASSERT (fs->fileExists (tempFile1));
	CCL_TEST_ASSERT (fs->fileExists (tempFile2));

	tempFile1.ascend ();
	fs->removeFolder (tempFile1);
	tempFile2.ascend ();
	fs->removeFolder (tempFile2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, MoveFile)
{
	Url tempFile1;
	System::GetSystem ().getLocation (tempFile1, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile1.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile1);
	tempFile1.descend ("dummy.temp", IUrl::kFile);
	IStream* stream = fs->openStream (tempFile1, IStream::kCreateMode);
	stream->release ();

	Url tempFile2;
	System::GetSystem ().getLocation (tempFile2, System::kTempFolder);
	folderName = UIDString::generate ();
	tempFile2.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile2);
	tempFile2.descend ("dummy2.temp", IUrl::kFile);

	Logging::debug ("Move file: %(1)", tempFile1.getPath ());
	CCL_TEST_ASSERT (fs->moveFile (tempFile2, tempFile1, 0, nullptr));
	CCL_TEST_ASSERT (!fs->fileExists (tempFile1));
	CCL_TEST_ASSERT (fs->fileExists (tempFile2));

	fs->copyFile (tempFile1, tempFile2, 0);
	Logging::debug ("Move file: %(1)", tempFile1.getPath ());
	CCL_TEST_ASSERT (!fs->moveFile (tempFile2, tempFile1, INativeFileSystem::kDoNotOverwrite));

	tempFile1.ascend ();
	fs->removeFolder (tempFile1);
	tempFile2.ascend ();
	fs->removeFolder (tempFile2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, MoveFileAcrossVolumes)
{
	TempFile source (UIDString::generate ());
	TempFile destination (UIDString::generate ());

	// Fill source file with data
	auto stream = source.open (IStream::kCreateMode);
	ASSERT (stream != nullptr);
	if(stream == nullptr)
		return;

	const unsigned int blockSize = 1024 * 120;
	int blocks = uint64(2) * 1024 * 1024 * 1024 / blockSize;
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	for(int i = 0; i < blocks; i++)
		stream->write (buffer, blockSize);
	stream->release ();

	CCL_TEST_ASSERT (fs->moveFile (destination.getPath (), source.getPath (), 0, nullptr));
	CCL_TEST_ASSERT (!fs->fileExists (source.getPath ()));
	CCL_TEST_ASSERT (fs->fileExists (destination.getPath ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, GetFileInfo)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	const unsigned int blockSize = 1024 * 120;
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	stream->write (buffer, blockSize);
	stream->release ();
	Logging::debug ("File info of file: %(1)", tempFile.getPath ());
	FileInfo fileInfo;
	CCL_TEST_ASSERT (fs->getFileInfo (fileInfo, tempFile));
	CCL_TEST_ASSERT (fileInfo.fileSize == blockSize);

	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, WriteReadLargeFile)
{
	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Writing to file: %(1)", tempFile.getPath ());
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode);
	const unsigned int blockSize = 1024 * 120;
	int blocks = uint64(2) * 1024 * 1024 * 1024 / blockSize;
	Logging::debugf ("Writing %d MB", blocks * blockSize / 1024 / 1024);
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	for(int i = 0; i < blocks; i++)
		stream->write (buffer, blockSize);
	stream->release ();

	Logging::debug ("Reading from file: %(1)", tempFile.getPath ());
	stream = fs->openStream (tempFile, IStream::kReadMode);
	for(int i = 0; i < blocks; i++)
		stream->read (buffer, blockSize);
	stream->release ();

	tempFile.ascend ();
	fs->removeFolder (tempFile);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_TEST_F (NativeFileSystemTest, WriteReadLargeFileUncached)
{
	Logging::debug ("This test does not work...");
	return;

	Url tempFile;
	System::GetSystem ().getLocation (tempFile, System::kTempFolder);
	String folderName = UIDString::generate ();
	tempFile.descend (folderName, IUrl::kFolder);
	fs->createFolder (tempFile);
	tempFile.descend ("dummy.temp", IUrl::kFile);
	Logging::debug ("Writing to file: %(1)", tempFile.getPath ());
	IStream* stream = fs->openStream (tempFile, IStream::kCreateMode | INativeFileStream::kWriteThru);
	const unsigned int blockSize = 1024 * 120;
	int blocks = uint64(2) * 1024 * 1024 * 1024 / blockSize;
	Logging::debugf ("Writing %d MB", blocks * blockSize / 1024 / 1024);
	char buffer[blockSize];
	for(int i = 0; i < blockSize; i++)
		buffer[i] = (i % CHAR_MAX);
	for(int i = 0; i < blocks; i++)
		stream->write (buffer, blockSize);
	stream->release ();

	Logging::debug ("Reading from file: %(1)", tempFile.getPath ());
	stream = fs->openStream (tempFile, IStream::kReadMode | INativeFileStream::kReadNonBuffered);
	for(int i = 0; i < blocks; i++)
		stream->read (buffer, blockSize);
	stream->release ();

	tempFile.ascend ();
	fs->removeFolder (tempFile);
}
