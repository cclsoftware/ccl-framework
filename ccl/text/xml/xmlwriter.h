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
// Filename    : ccl/text/xml/xmlwriter.h
// Description : XML Writer
//
//************************************************************************************************

#ifndef _ccl_xmlwriter_h
#define _ccl_xmlwriter_h

#include "ccl/text/writer/textwriter.h"

#include "ccl/public/text/ixmlwriter.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// XML Writer Macros
//////////////////////////////////////////////////////////////////////////////////////////////////

#define DEFINE_XMLWRITER_METHODS(Parent) \
tresult CCL_API characterData (IStream& charData, TextEncoding encoding = Text::kUnknownEncoding) override \
{ return Parent::characterData (charData, encoding); }

//************************************************************************************************
// XmlWriter
//************************************************************************************************

class XmlWriter: public SgmlWriter,
				 public IXmlWriter
{
public:
	XmlWriter ();

	typedef SgmlWriter SuperClass;

	// IXmlWriter
	tresult CCL_API beginDocument (IStream& stream, TextEncoding encoding = Text::kUnknownEncoding) override;
	DEFINE_TEXTWRITER_METHODS (SuperClass)
	DEFINE_MARKUPWRITER_METHODS (SuperClass)
	DEFINE_SGMLWRITER_METHODS (SuperClass)
	tresult CCL_API characterData (IStream& charData, TextEncoding encoding = Text::kUnknownEncoding) override;

	CLASS_INTERFACE (IXmlWriter, SuperClass)
};

} // namespace CCL

#endif // _ccl_xmlwriter_h
