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
// Filename    : ccl/text/xml/xmlentities.h
// Description : XML Entities
//
//************************************************************************************************

#ifndef _ccl_xmlentities_h
#define _ccl_xmlentities_h

#include "ccl/text/writer/markupencoder.h"

namespace CCL {

//************************************************************************************************
// XmlEntities
//************************************************************************************************

class XmlEntities: public MarkupEncoder
{
public:
	static MutableCString makeBuiltInDTD (TextLineFormat lineFormat = Text::kSystemLineFormat);

	// MarkupEncoder
	String encode (StringRef text) override;
	MutableCString encodeToASCII (StringRef text) override;
	String decode (StringRef text) override;
};

//************************************************************************************************
// XmlEncodings
//************************************************************************************************

class XmlEncodings
{
public:
	static const char* getEncoding (TextEncoding encoding);
};

} // namespace CCL

#endif // _ccl_xmlentities_h
