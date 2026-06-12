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
// Filename    : ccl/text/markdown/markdownparser.cpp
// Description : Markdown Parser
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/text/markdown/markdownparser.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/base/istream.h"

#include "cmark/src/cmark.h"

using namespace CCL;

//************************************************************************************************
// MarkdownParser::MarkdownNode
/** Wraps a cmark_node pointer. */
//************************************************************************************************

class MarkdownParser::MarkdownNode: public Unknown,
									public IMarkdownNode
{
public:
	MarkdownNode (cmark_node* node)
	: node (node)
	{}

	// IMarkdownNode
	TextNodeType CCL_API getNodeType () const override			{ return toNodeType (::cmark_node_get_type (node)); }
	tbool CCL_API getText (String& text) const override;
	int CCL_API getStartLine () const override					{ return ::cmark_node_get_start_line (node); }
	int CCL_API getStartColumn () const override				{ return ::cmark_node_get_start_column (node); }
	int CCL_API getEndLine () const override					{ return ::cmark_node_get_end_line (node); }
	int CCL_API getEndColumn () const override					{ return ::cmark_node_get_end_column (node); }
	int CCL_API getHeadingLevel () const override				{ return ::cmark_node_get_heading_level (node); }
	TextListType CCL_API getListType () const override			{ return toListType (::cmark_node_get_list_type (node)); }
	ListDelimiter CCL_API getListDelimiter () const override	{ return toListDelimiter (::cmark_node_get_list_delim (node)); }
	int CCL_API getListStart () const override					{ return ::cmark_node_get_list_start (node); }
	tbool CCL_API isTightList () const override					{ return ::cmark_node_get_list_tight (node) != 0; }
	tbool CCL_API getUrl (String& url) const override;
	tbool CCL_API getTitle (String& title) const override;

	CLASS_INTERFACE (IMarkdownNode, Unknown)

private:
	cmark_node* node;

	static constexpr TextNodeType toNodeType (cmark_node_type type);
	static constexpr TextListType toListType (cmark_list_type type);
	static constexpr ListDelimiter toListDelimiter (cmark_delim_type delim);
};

//************************************************************************************************
// MarkdownParser::MarkdownNode
//************************************************************************************************

constexpr TextNodeType MarkdownParser::MarkdownNode::toNodeType (cmark_node_type type)
{
	switch(type)
	{
	default:
	case CMARK_NODE_NONE : return Text::kNodeTypeUnknown;
	case CMARK_NODE_DOCUMENT : return Text::kDocument;
	case CMARK_NODE_BLOCK_QUOTE : return Text::kBlockQuote;
	case CMARK_NODE_LIST : return Text::kList;
	case CMARK_NODE_ITEM : return Text::kListItem;
	case CMARK_NODE_CODE_BLOCK : return Text::kCodeBlock;
	case CMARK_NODE_HTML_BLOCK : return Text::kHtmlBlock;
	case CMARK_NODE_CUSTOM_BLOCK : return Text::kCustomBlock;
	case CMARK_NODE_PARAGRAPH : return Text::kParagraph;
	case CMARK_NODE_HEADING : return Text::kHeading;
	case CMARK_NODE_THEMATIC_BREAK : return Text::kThematicBreak;
	case CMARK_NODE_TEXT : return Text::kPlainText;
	case CMARK_NODE_SOFTBREAK : return Text::kSoftBreak;
	case CMARK_NODE_LINEBREAK : return Text::kLineBreak;
	case CMARK_NODE_CODE : return Text::kCodeInline;
	case CMARK_NODE_HTML_INLINE : return Text::kHtmlInline;
	case CMARK_NODE_CUSTOM_INLINE : return Text::kCustomInline;
	case CMARK_NODE_EMPH : return Text::kEmphasis;
	case CMARK_NODE_STRONG : return Text::kStrong;
	case CMARK_NODE_LINK : return Text::kLink;
	case CMARK_NODE_IMAGE : return Text::kImage;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr TextListType MarkdownParser::MarkdownNode::toListType (cmark_list_type type)
{
	switch(type)
	{
	default:
	case CMARK_NO_LIST : return Text::kListTypeUnknown;
	case CMARK_BULLET_LIST : return Text::kUnorderedList;
	case CMARK_ORDERED_LIST : return Text::kOrderedList;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

constexpr IMarkdownNode::ListDelimiter MarkdownParser::MarkdownNode::toListDelimiter (cmark_delim_type delim)
{
	switch(delim)
	{
	default:
	case CMARK_NO_DELIM : return kListDelimiterNone;
	case CMARK_PERIOD_DELIM : return kListDelimiterPeriod;
	case CMARK_PAREN_DELIM : return kListDelimiterParen;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MarkdownParser::MarkdownNode::getText (String& text) const
{
	if(CStringPtr cString = ::cmark_node_get_literal (node))
	{
		text = String (Text::kUTF8, cString);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MarkdownParser::MarkdownNode::getUrl (String& url) const
{
	if(CStringPtr cString = ::cmark_node_get_url (node))
	{
		url = String (Text::kUTF8, cString);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API MarkdownParser::MarkdownNode::getTitle (String& title) const
{
	if(CStringPtr cString = ::cmark_node_get_title (node))
	{
		title = String (Text::kUTF8, cString);
		return true;
	}
	return false;
}

//************************************************************************************************
// MarkdownParser::Traverser
//************************************************************************************************

class MarkdownParser::Traverser
{
public:
	Traverser (IMarkdownContentHandler& handler)
	: handler (handler)
	{}

	void traverseNodes (cmark_node* startNode)
	{
		cmark_iter* iter = ::cmark_iter_new (startNode);
		if(iter)
		{
			cmark_event_type eventType = CMARK_EVENT_NONE;
			while((eventType = ::cmark_iter_next (iter)) != CMARK_EVENT_DONE)
			{
				cmark_node* node = ::cmark_iter_get_node (iter);
				if(node)
				{
					MarkdownNode nodeInfo (node);

					switch(eventType)
					{
					case CMARK_EVENT_ENTER :
						handler.startNode (nodeInfo);

						#if DEBUG_LOG
						{
							String t; nodeInfo.getText (t);
							CCL_PRINTF ("%sEnter Node [%d %s] \t%s\n", CCL_INDENT, nodeInfo.getNodeType (), cmark_node_get_type_string (node), MutableCString (t).str ())
							if(!::cmark_node_is_leaf (node)) // there is no exit event for leafs
								Debugger::indent (4);
						}
						#endif
						break;

					case CMARK_EVENT_EXIT :
						#if DEBUG_LOG
						{
							String t; nodeInfo.getText (t);
							Debugger::unindent (4);
							CCL_PRINTF ("%sExit Node  [%d %s] \t%s\n", CCL_INDENT, nodeInfo.getNodeType (), cmark_node_get_type_string (node), MutableCString (t).str ())
						}
						#endif

						handler.endNode (nodeInfo);
						break;
					}
				}
			}
			cmark_iter_free (iter);
		}
	}

private:
	IMarkdownContentHandler& handler;
};

//************************************************************************************************
// MarkdownParser
//************************************************************************************************

MarkdownParser::MarkdownParser ()
: handler (nullptr),
  aborted (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MarkdownParser::~MarkdownParser ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool MarkdownParser::isAborted () const
{
	return aborted;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMarkdownContentHandler* MarkdownParser::getHandler () const
{
	return handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkdownParser::setHandler (IMarkdownContentHandler* _handler)
{
	handler = _handler;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API MarkdownParser::parse (IStream& stream)
{
	aborted = false;

	ASSERT (handler)
	if(!handler)
		return kResultFailed;

	cmark_parser* parser = ::cmark_parser_new (CMARK_OPT_DEFAULT);
	ASSERT (parser)
	if(!parser)
		return kResultOutOfMemory;

	constexpr int kBufferSize = 8192;
	char buffer[kBufferSize];

	// feed data from stream into parser
	while(!isAborted ())
	{
		int numRead = stream.read (buffer, kBufferSize);
		if(numRead <= 0)
			break;

		// if the stream ends with a trailing 0 byte (e.g. from FileUtilities::createStringStream)
		// cmark would replace it with the "replacement character" (UTF8: 0xEF, 0xBF, 0xBF) that would be part of the last text node
		if(buffer[numRead - 1] == 0)
		{
			numRead--;
			if(numRead <= 0)
				continue;
		}
		::cmark_parser_feed (parser, buffer, numRead);
	}

	// get document (root node of AST)
	cmark_node* document = nullptr;
	if(!isAborted ())
		document = ::cmark_parser_finish (parser);

	::cmark_parser_free (reinterpret_cast<cmark_parser*> (parser));

	if(document)
	{
		Traverser traverser (*handler);
		traverser.traverseNodes (document);

		::cmark_node_free (document);
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API MarkdownParser::abort ()
{
	aborted = true;
}
