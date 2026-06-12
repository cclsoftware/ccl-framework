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
// Filename    : ccl/public/text/imarkdownparser.h
// Description : Markdown Parser Interface
//
//************************************************************************************************

#ifndef _ccl_imarkdownparser_h
#define _ccl_imarkdownparser_h

#include "ccl/public/text/textformatting.h"

namespace CCL {

interface IStream;
interface IMarkdownContentHandler;

//************************************************************************************************
// IMarkdownParser
/**	Markdown Parser - created via System::CreateMarkdownParser ()
    \ingroup ccl_text */
//************************************************************************************************

interface IMarkdownParser: IUnknown
{
	/** Init content handler. */
	virtual void CCL_API setHandler (IMarkdownContentHandler* handler) = 0;

	/** Parse Markdown data stream. */
	virtual tresult CCL_API parse (IStream& stream) =  0;

	/** Abort parsing, can be called from content handler. */
	virtual void CCL_API abort () = 0;

	DECLARE_IID (IMarkdownParser)
};

DEFINE_IID (IMarkdownParser, 0x874BA85D, 0x2910, 0x4570, 0xAE, 0x92, 0xFA, 0x23, 0xB8, 0x7F, 0x40, 0x2F)

//************************************************************************************************
// IMarkdownNode
/** \ingroup ccl_text */ 
//************************************************************************************************

interface IMarkdownNode: IUnknown
{
	enum ListDelimiter: int
	{
		kListDelimiterNone,
		kListDelimiterPeriod,
		kListDelimiterParen
	};

	// Common nodes properties
	virtual TextNodeType CCL_API getNodeType () const = 0;
	virtual tbool CCL_API getText (String& text) const = 0;

	virtual int CCL_API getStartLine () const = 0;
	virtual int CCL_API getStartColumn () const = 0;
	virtual int CCL_API getEndLine () const = 0;
	virtual int CCL_API getEndColumn () const = 0;

	// Heading node properties
	virtual int CCL_API getHeadingLevel () const = 0;

	// List node properties
	virtual TextListType CCL_API getListType () const = 0;
	virtual ListDelimiter CCL_API getListDelimiter () const = 0;
	virtual int CCL_API getListStart () const = 0;
	virtual tbool CCL_API isTightList () const = 0;

	// Link or image node properties
	virtual tbool CCL_API getUrl (String& url) const = 0;
	virtual tbool CCL_API getTitle (String& title) const = 0;

	DECLARE_IID (IMarkdownNode)
};

DEFINE_IID (IMarkdownNode, 0x694FB199, 0x0C68, 0x46DC, 0xBE, 0x4A, 0xA8, 0x89, 0x55, 0xFB, 0x29, 0xD6)

//************************************************************************************************
// IMarkdownContentHandler
/** \ingroup ccl_text */
//************************************************************************************************

interface IMarkdownContentHandler: IUnknown
{
	/** Notification of the beginning of a Markdown node. A corresponding endNode call will follow. */
	virtual tresult CCL_API startNode (const IMarkdownNode& node) = 0;

	/** Notification of the end of a Markdown node. */
	virtual tresult CCL_API endNode (const IMarkdownNode& node) = 0;

	DECLARE_IID (IMarkdownContentHandler)
};

DEFINE_IID (IMarkdownContentHandler, 0x73A35C2A, 0x97F6, 0x49F9, 0x91, 0x6D, 0xFA, 0x4E, 0x0E, 0x34, 0x63, 0x27)

} // namespace CCL

#endif // _ccl_imarkdownparser_h
