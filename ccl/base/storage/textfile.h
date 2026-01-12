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
// Filename    : ccl/base/storage/textfile.h
// Description : Text File
//
//************************************************************************************************

#ifndef _ccl_textfile_h
#define _ccl_textfile_h

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/storableobject.h"

#include "ccl/public/text/itextstreamer.h"

namespace CCL {

class StringList;
class TextBlock;

//************************************************************************************************
// TextFile
//************************************************************************************************

class TextFile: public Object
{
public:
	DECLARE_CLASS (TextFile, Object)
	DECLARE_PROPERTY_NAMES (TextFile)
	DECLARE_METHOD_NAMES (TextFile)

	enum Mode { kOpen };

	TextFile ();
	TextFile (UrlRef path, TextEncoding encoding = Text::kUTF8, TextLineFormat lineFormat = Text::kSystemLineFormat, int options = 0);
	TextFile (UrlRef path, Mode mode, TextEncoding encoding = Text::kUnknownEncoding, int options = 0);
	~TextFile ();

	bool isValid () const;
	UrlRef getPath () const;
	void close ();
	
	ITextStreamer* operator -> ();
	operator ITextStreamer* ();

protected:
	ITextStreamer* streamer;
	Url path;

	void create (UrlRef path, TextEncoding encoding = Text::kUTF8, TextLineFormat lineFormat = Text::kSystemLineFormat, int options = 0);
	void open (UrlRef path, TextEncoding encoding = 0, int options = 0);

	// IObject
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// TextResource
//************************************************************************************************

class TextResource: public StorableObject
{
public:
	DECLARE_CLASS (TextResource, StorableObject)

	TextResource (StringRef content = nullptr, TextEncoding encoding = Text::kUTF8);

	PROPERTY_STRING (content, Content)
	PROPERTY_VARIABLE (TextEncoding, encoding, Encoding)
	PROPERTY_BOOL (suppressByteOrderMark, SuppressByteOrderMark)
	PROPERTY_BOOL (suppressFinalLineEnd, SuppressFinalLineEnd)

	// StorableObject
	tbool CCL_API getFormat (FileType& format) const override;
	tbool CCL_API save (IStream& stream) const override;
	tbool CCL_API load (IStream& stream) override;
};

//************************************************************************************************
// ITextPromise
//************************************************************************************************

interface ITextPromise
{
	/** Create formatted text block. */
	virtual void createText (TextBlock& block, StringRef title, VariantRef data) const = 0;
};

//************************************************************************************************
// TextUtils
//************************************************************************************************

namespace TextUtils
{
	/** Determine encoding by name (e.g. "ascii"). */
	TextEncoding getEncodingByName (StringRef name);

	/** Load text file to string without line ending modifications. */
	String loadRawString (UrlRef path);

	/** Load text stream to string without line ending modifications. */
	String loadRawString (IStream& stream);

	/** Load text file to string with normalized line endings. */
	String loadString (UrlRef path, String endline = String::getLineEnd (),
							  TextEncoding encoding = Text::kUnknownEncoding);

	/** Load text stream to string with normalized line endings. */
	String loadString (IStream& stream, String endline = String::getLineEnd (),
							  TextEncoding encoding = Text::kUnknownEncoding);

	/** Load lines from text file to string list. */
	bool loadStringList (StringList& stringList, UrlRef path, bool ignoreEmptyLines = true,
								TextEncoding encoding = Text::kUnknownEncoding);

	/** Load lines from text stream to string list. */
	bool loadStringList (StringList& stringList, IStream& stream, bool ignoreEmptyLines = true,
								TextEncoding encoding = Text::kUnknownEncoding);

	/** Get built-in CSS definitions. */
	StringRef getCSS ();

	/** Save formatted text block to file. */
	bool saveTextBlock (UrlRef path, StringRef title, VariantRef data, const ITextPromise& textPromise);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TextFile inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline bool TextFile::isValid () const
{ return streamer != nullptr; }

inline UrlRef TextFile::getPath () const
{ return path; }

inline ITextStreamer* TextFile::operator -> ()
{ return streamer; }

inline TextFile::operator ITextStreamer* ()
{ return streamer; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_textfile_h
