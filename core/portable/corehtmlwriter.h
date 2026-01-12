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
// Filename    : core/portable/corehtmlwriter.h
// Description : HTML Writer
//
//************************************************************************************************

#ifndef _corehtmlwriter_h
#define _corehtmlwriter_h

#include "core/portable/corexmlwriter.h"

namespace Core {
namespace Portable {

//************************************************************************************************
// HtmlWriter
/**	Helper class to programmatically generate HTML output. 
	Implementation borrows entity encoding from XML.
\ingroup core_portable */
//************************************************************************************************

class HtmlWriter: protected XmlWriter
{
public:
	HtmlWriter (IO::Stream& stream)
	: XmlWriter (stream)
	{}

	HtmlWriter& beginDocument (CStringPtr title)
	{
		// TODO: DOCTYPE directive and character set... see CCL!

		*this << "<html>\n"
				 "<head>\n"
				 "\t<title>";
		writeEncoded (title);		
		*this << "</title>\n"
				 "</head>\n"
				 "<body>\n";
		return *this;
	}

	HtmlWriter& write (CStringPtr text)
	{
		writeEncoded (text);
		return *this;
	}

	HtmlWriter& endDocument ()
	{
		*this << "</body>\n"
				 "</html>";
		return *this;
	}
};

} // namespace Portable
} // namespace Core

#endif // _corexmlwriter_h
