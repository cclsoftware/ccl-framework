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
// Filename    : ccl/base/storage/urlencoder.h
// Description : Url Encoder
//
//************************************************************************************************

#ifndef _ccl_urlencoder_h
#define _ccl_urlencoder_h

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/text/istringdict.h"

#include "core/public/coreurlencoding.h"

namespace CCL {

//************************************************************************************************
// UrlEncoder
//************************************************************************************************

class UrlEncoder
{
public:
	/** URL encoding scheme. */
	enum Scheme
	{
		kRFC3986 = Core::URLEncoding::kRFC3986,
		kWebForm = Core::URLEncoding::kWebForm,

		kDefault = kRFC3986
	};

	UrlEncoder (Scheme scheme = kDefault, TextEncoding textEncoding = Text::kUTF8);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Encoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Encode C-String. */
	MutableCString encode (CStringRef string);

	/** Encode Unicode. */
	String encode (StringRef string);

	/** Encode path components respecting "/" delimiter (C-String). */
	MutableCString encodePathComponents (CStringRef string);

	/** Encode path components respecting "/" delimiter (Unicode). */
	String encodePathComponents (StringRef string);

	/** Encode C-String parameters. */
	MutableCString encode (const ICStringDictionary& parameters);

	/** Encode Unicode parameters. */
	String encode (const IStringDictionary& parameters);

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Decoding
	//////////////////////////////////////////////////////////////////////////////////////////////

	/** Decode to C-String. */
	MutableCString decode (CStringRef string);

	/** Decode to Unicode. */
	String decode (StringRef string);

	/** Decode path components respecting "/" delimiter (C-String). */
	MutableCString decodePathComponents (CStringRef string);

	/** Decode path components respecting "/" delimiter (Unicode). */
	String decodePathComponents (StringRef string);

	/** Decode Unicode parameters. */
	IStringDictionary& decode (IStringDictionary& parameters, StringRef string);

	/** Decode C-String parameters. */
	ICStringDictionary& decode (ICStringDictionary& parameters, CStringRef string);

protected:
	static const int kBufferSize = 4096;

	Scheme scheme;
	TextEncoding textEncoding;

	template <class Text, class Dictionary>
	Text encode (const Dictionary& parameters);

	template <class Dictionary, class TextRef, class Text>
	Dictionary& decode (Dictionary& parameters, TextRef string);

	typedef String (UrlEncoder::*ConvertMethod) (StringRef string);
	template<ConvertMethod convert>
	String convertPathComponents (StringRef string);
};

} // namespace CCL

#endif // _ccl_urlencoder_h
