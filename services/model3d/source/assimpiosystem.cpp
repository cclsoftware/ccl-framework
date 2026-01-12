//************************************************************************************************
//
// 3D Model Importer Library
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
// Filename    : assimpiosystem.cpp
// Description : Assimp I/O System
//
//************************************************************************************************

#include "assimpiosystem.h"

#include "ccl/base/storage/url.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "assimp/IOStream.hpp"

namespace CCL {

//************************************************************************************************
// AssimpStream
//************************************************************************************************

class AssimpStream: public Assimp::IOStream
{
public:
	AssimpStream (IStream* baseStream);
	~AssimpStream ();

	// IOStream
	size_t Read (void* buffer, size_t size, size_t count) override;
	size_t Write (const void* buffer, size_t size, size_t count) override;
	aiReturn Seek (size_t offset, aiOrigin origin) override;
	size_t Tell () const override;
	size_t FileSize () const override;
	void Flush () override;

private:
	AutoPtr<IStream> baseStream;
};

} // namespace CCL

using namespace Assimp;
using namespace CCL;

//************************************************************************************************
// AssimpStream
//************************************************************************************************

AssimpStream::AssimpStream (IStream* baseStream)
: baseStream (baseStream)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AssimpStream::~AssimpStream ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t AssimpStream::Read (void* buffer, size_t size, size_t count)
{
	ASSERT (baseStream != nullptr)
	return baseStream->read (buffer, int (size * count));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t AssimpStream::Write (const void* buffer, size_t size, size_t count)
{
	ASSERT (baseStream != nullptr)
	return baseStream->write (buffer, int (size * count));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

aiReturn AssimpStream::Seek (size_t offset, aiOrigin origin)
{
	const IStream::SeekMode kMode[3] = { IStream::kSeekSet, IStream::kSeekCur, IStream::kSeekEnd };

	ASSERT (baseStream != nullptr)
	ASSERT (origin < ARRAY_COUNT (kMode))	

	int64 position = baseStream->seek (offset, kMode[origin]);
	return position >= 0 ? aiReturn_SUCCESS : aiReturn_FAILURE;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t AssimpStream::Tell () const
{
	ASSERT (baseStream != nullptr)
	return baseStream->seek (0, IStream::kSeekCur);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

size_t AssimpStream::FileSize () const
{
	ASSERT (baseStream != nullptr)

	int64 position = baseStream->seek (0, IStream::kSeekCur);
	int64 fileSize = baseStream->seek (0, IStream::kSeekEnd);
	baseStream->seek (position, IStream::kSeekSet);

	return fileSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AssimpStream::Flush ()
{
	ASSERT (baseStream != nullptr)
	ASSERT (false) // not implemented
}

//************************************************************************************************
// AssimpIOSystem
//************************************************************************************************

AssimpIOSystem::AssimpIOSystem ()
: fileSystem (System::GetFileSystem ())
{}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool AssimpIOSystem::Exists (const char* path) const
{
	return fileSystem.fileExists (Url (String (Text::kUTF8, path)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

char AssimpIOSystem::getOsSeparator () const
{
	return '/';
}

///////////////////////////////////////////////////////////////////////////////////////////////////

IOStream* AssimpIOSystem::Open (const char* path, const char* mode)
{
	AutoPtr<IStream> stream = fileSystem.openStream (Url (String (Text::kUTF8, path)), parseOpenMode (mode));
	if(stream == nullptr)
		return nullptr;

	return NEW AssimpStream (stream.detach ());
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void AssimpIOSystem::Close (IOStream* file)
{
	AssimpStream* stream = static_cast<AssimpStream*> (file);
	delete stream;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int AssimpIOSystem::parseOpenMode (const char* mode)
{
	ASSERT (mode != nullptr)

	int result = 0;

	while(*mode != '\0')
	{
		char c = *mode++;

		switch(c)
		{
		case 'r':
			result |= IStream::kReadMode;
			break;

		case 'w':
			result |= IStream::kWriteMode;
			break;

		case 'b':
			break; // ignore

		default:
			ASSERT (false) // not implemented
			break;
		}
	}

	return result;
}
