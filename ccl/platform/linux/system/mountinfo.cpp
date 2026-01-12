//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/linux/system/mountinfo.cpp
// Description : Class providing information about mount points
//
//************************************************************************************************

#include "ccl/base/storage/url.h"
#include "ccl/public/storage/iurl.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "mountinfo.h"

#include <sys/sysmacros.h>
#include <limits.h>

using namespace Core;
using namespace CCL;

//************************************************************************************************
// MountInfo
//************************************************************************************************

bool MountInfo::load ()
{
	Url url;
	url.fromPOSIXPath ("/proc/self/mountinfo");
	AutoPtr<IStream> stream (System::GetFileSystem ().openStream (url, IStream::kOpenMode));
	if(stream == nullptr)
		return false;

	AutoPtr<ITextStreamer> reader (System::CreateTextStreamer (*stream, {Text::kUTF8}));
	if(reader == nullptr)
		return false;

	String line;
	while(reader->readLine (line))
		parseLine (line);

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const MountInfo::Entry* MountInfo::find (UrlRef path) const
{
	String pathString = UrlDisplayString (path);
	char realPath[PATH_MAX] = "";
	::realpath (MutableCString (pathString, Text::kUTF8), realPath);
	pathString = String (Text::kUTF8, realPath);

	const Entry* bestMatch = nullptr;
	uint32 bestMatchLength = 0;

	for(const Entry& entry : entries)
	{
		uint32 mountPointLength = entry.mountPoint.length ();

		if(mountPointLength <= bestMatchLength)
			continue;

		if(pathString.startsWith (entry.mountPoint))
		{
			bestMatch = &entry;
			bestMatchLength = mountPointLength;
		}
	}

	return bestMatch;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void MountInfo::parseLine (StringRef line)
{
	Entry entry {};

	AutoPtr<IStringTokenizer> tokenizer = line.tokenize (" ");
	if(tokenizer == nullptr)
		return;

	uchar delimiter = '\0';

	if(!tokenizer->done ())
	{
		StringRef field = tokenizer->nextToken (delimiter);
		int64 value = 0;

		if(field.getIntValue (value))
			entry.mountID = static_cast<uint32> (value);
	}

	if(!tokenizer->done ())
	{
		StringRef field = tokenizer->nextToken (delimiter);
		int64 value = 0;

		if(field.getIntValue (value))
			entry.parentID = static_cast<uint32> (value);
	}

	if(!tokenizer->done ())
	{
		StringRef field = tokenizer->nextToken (delimiter);
		int index = field.index (":");

		if(index != -1)
		{
			int64 majorNumber = 0;
			int64 minorNumber = 0;

			if(field.subString (0, index).getIntValue (majorNumber) &&
			   field.subString (index + 1).getIntValue (majorNumber))
				entry.deviceID = makedev (static_cast<int> (majorNumber), static_cast<int> (minorNumber));
		}
	}


	if(!tokenizer->done ())
		entry.root = tokenizer->nextToken (delimiter);

	if(!tokenizer->done ())
		entry.mountPoint = tokenizer->nextToken (delimiter);

	if(!tokenizer->done ())
		entry.mountOptions = tokenizer->nextToken (delimiter);

	while(!tokenizer->done ())
	{
		StringRef field = tokenizer->nextToken (delimiter);
		if(field == "-")
			break;

        // Record the first optional field and ignore the remaining ones
		if(entry.optionalFields.isEmpty ())
			entry.optionalFields = field;
	}

	if(!tokenizer->done ())
		entry.filesystemType = tokenizer->nextToken (delimiter);

	if(!tokenizer->done ())
		entry.mountSource = tokenizer->nextToken (delimiter);

	if(!tokenizer->done ())
		entry.superOptions = tokenizer->nextToken (delimiter);

	// Since  fields  in  the mtab and fstab files are separated by whitespace, octal escapes are
	// used to represent the four characters space (\040), tab (\011), newline (\012)  and  back-
	// slash  (\134) in those files when they occur in one of the four strings in a mntent struc-
	// ture.
	entry.mountSource.replace ("\\040", " ");
	entry.mountSource.replace ("\\011", "\t");
	entry.mountSource.replace ("\\134", "\\");
	entry.mountPoint.replace ("\\040", " ");
	entry.mountPoint.replace ("\\011", "\t");
	entry.mountPoint.replace ("\\134", "\\");

	entries.add (entry);
}
