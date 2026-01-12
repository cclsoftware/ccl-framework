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
// Filename    : ccl/extras/web/webformdata.h
// Description : Web Form Data
//
//************************************************************************************************

#ifndef _ccl_webformdata_h
#define _ccl_webformdata_h

#include "ccl/public/text/cstring.h"

namespace CCL {

class MultiplexStream;

namespace Web {

//************************************************************************************************
// FormData
//************************************************************************************************

class FormData
{
public:
	static StringID getContentType ();
	static IStream* createStream (const IStringDictionary& parameters);
	static IStream* createStream (const ICStringDictionary& parameters);
};

//************************************************************************************************
// MultipartFormData
//************************************************************************************************

class MultipartFormData
{
public:
	MultipartFormData ();
	~MultipartFormData ();

	MutableCString getContentType () const;

	void addField (StringID name, StringID value, bool end = false);
	void addField (StringID name, StringRef value, bool end = false);
	void addFile (StringID name, StringID fileName, IStream* file, int64 fileSize, bool end = false);

	IStream* createStream () const;

protected:
	MultiplexStream& multiplexStream;
	MutableCString boundary;

	void appendText (StringID text);
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webformdata_h
