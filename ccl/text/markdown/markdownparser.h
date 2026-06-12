//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/text/markdown/markdownparser.h
// Description : Markdown Parser
//
//************************************************************************************************

#ifndef _ccltext_markdownparser_h
#define _ccltext_markdownparser_h

#include "ccl/public/base/unknown.h"

#include "ccl/public/text/imarkdownparser.h"

namespace CCL {

//************************************************************************************************
// MarkdownParser
//************************************************************************************************

class MarkdownParser: public Unknown,
					  public IMarkdownParser
{
public:
	MarkdownParser ();
	~MarkdownParser ();

	bool isAborted () const;
	IMarkdownContentHandler* getHandler () const;

	// IMarkdownParser
	void CCL_API setHandler (IMarkdownContentHandler* handler) override;
	tresult CCL_API parse (IStream& stream) override;
	void CCL_API abort () override;

	CLASS_INTERFACE (IMarkdownParser, Unknown)

protected:
	IMarkdownContentHandler* handler;
	bool aborted;

	class MarkdownNode;
	class Traverser;
};

} // namespace CCL

#endif // _ccltext_markdownparser_h
