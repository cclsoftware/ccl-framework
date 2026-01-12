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
// Filename    : ccl/public/text/ihtmlwriter.h
// Description : HTML Writer Interface
//
//************************************************************************************************

#ifndef _ccl_ihtmlwriter_h
#define _ccl_ihtmlwriter_h

#include "ccl/public/text/itextwriter.h"

namespace CCL {

interface ITextBuilder;

//************************************************************************************************
// HTML Tags
//************************************************************************************************

namespace HtmlTags
{
	DEFINE_STRINGID (kHtml,  "html")
	DEFINE_STRINGID (kHead,  "head")
	DEFINE_STRINGID (kBody,  "body")

	DEFINE_STRINGID (kTitle, "title")
	DEFINE_STRINGID (kMeta,  "meta")
	DEFINE_STRINGID (kStyle, "style")
}

//************************************************************************************************
// IHtmlWriter
/**	\ingroup ccl_text */
//************************************************************************************************

interface IHtmlWriter: ISgmlWriter
{
	/** Create text block builder for HTML. */
	virtual ITextBuilder* CCL_API createHtmlBuilder () = 0;

	/** Push <meta> element to be written inside HTML <head>. */
	virtual tresult CCL_API pushMetaElement (StringRef name, StringRef content, tbool isHttpEquiv) = 0;

	/** Push <style> element to be written inside HTML <head>. */
	virtual tresult CCL_API pushStyleElement (StringRef cssContent) = 0;

	/** Write <head></head> including <title> and all previously pushed meta elements. */
	virtual tresult CCL_API writeHead (StringRef title) = 0;

	DECLARE_IID (IHtmlWriter)
};

DEFINE_IID (IHtmlWriter, 0x42dd4092, 0xac3b, 0x4ff5, 0x9c, 0x8b, 0x20, 0x5c, 0x6e, 0xbc, 0xf4, 0x18)

} // namespace CCL

#endif // _ccl_ihtmlwriter_h
