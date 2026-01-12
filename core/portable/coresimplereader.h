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
// Filename    : core/portable/coresimplereader.h
// Description : Simple Text Stream Reader
//
//************************************************************************************************

#ifndef _coresimplereader_h
#define _coresimplereader_h

#include "core/public/corestringbuffer.h"
#include "core/public/corestream.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// TextReader
/** A simple text stream reader. */
//************************************************************************************************
	
class TextReader
{
public:
	TextReader (IO::Stream& stream, bool rewind = true);

	static const int kMaxLineLength = STRING_STACK_SPACE_MAX;
	typedef CStringBuffer<kMaxLineLength> LineString;
	typedef CString64 ValueString;
	
	/** Given a filename and a key (ie. "KEY=VALUE"), return a value string. */
	static void getValueForKey (ValueString& value, CStringPtr key, CStringPtr fileName);

	bool skipBOM ();
	bool getNextLine (LineString& lineString) const;
	bool advanceToNextWord (CStringPtr word) const;
    bool getNextWord (ValueString& wordString, char delimiter = '\t') const;
	bool advanceToNextChar (char test) const;
	bool getValueForKey (ValueString& value, CStringPtr key) const;

	/** Like getValueForKey() but it'll search the entire file (stream). */
	bool findValueForKey (ValueString& value, CStringPtr key) const;
	
	int64 getPosition () const;
	void setPosition (int64 pos);
	
protected:
	static const int kMaxBytesToAdvance = 4096;

	IO::Stream& stream;
};

} // namespace Portable
} // namespace Core

#endif // _coresimplereader_h
