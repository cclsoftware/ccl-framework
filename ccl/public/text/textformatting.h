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
// Filename    : ccl/public/text/textformatting.h
// Description : Common Text Formatting Definitions (HTML, Markdown, etc.)
//
//************************************************************************************************

#ifndef _ccl_textformatting_h
#define _ccl_textformatting_h

#include "ccl/public/base/platform.h"

namespace CCL {

//////////////////////////////////////////////////////////////////////////////////////////////////
// Text Formatting Definitions
//////////////////////////////////////////////////////////////////////////////////////////////////

/**	\addtogroup ccl_text */
namespace Text 
{
	/** Text node type. */
	DEFINE_ENUM (NodeType)
	{
		kNodeTypeUnknown = 0,
		
		// Block-level nodes
		kDocument = 'docu',			///< top-level document
		kHeading = 'head',			///< heading, can have level
		kParagraph = 'para',		///< paragraph of text
		kBlockQuote = 'bkqt',		///< block quote (a long quotation block)
		kListBegin = 'lstb',		///< list begin 
		kListEnd = 'lste',			///< list end
		kListItem = 'litm',			///< list item
		kList = 'list',				///< list (alternative to kListBegin/kListEnd)
		kTable = 'tabl',			///< table
		kCodeBlock = 'cdbk',		///< code block
		kHtmlBlock = 'htmb',		///< HTML block
		kCustomBlock = 'cstb',		///< custom block (format-dependent)
		kThematicBreak = 'thbr',	///< thematic break (semantic)
		kHorizontalLine = 'hrln',	///< horizontal line (visual)
		
		// Inline nodes
		kPlainText = 'plnt',		///< plain text
		kSoftBreak = 'sftb',		///< soft break
		kLineBreak = 'lnbr',		///< hard line break
		kAnchor = 'anch',			///< local anchor definition in document (invisible)		
		kFragmentLink = 'frag',		///< link to anchor in same document
		kLink = 'link',				///< external link
		kImage = 'imag',			///< image
		kEmphasis = 'emph',			///< emphasis (semantic, usually italic visually)
		kStrong = 'strg',			///< strong (semantic, usually bold visually)
		kBold = 'bold',				///< visually bold text
		kItalic = 'ital',			///< visually italic text
		kUnderline = 'udrl',		///< visually underlined text
		kSuperscript = 'sups',		///< superscript
		kSubscript = 'subs',		///< subscript
		kFontColor = 'fclr',		///< font color
		kFontSize = 'fsiz',			///< font size
		kStyleSpan = 'stsp',		///< style span
		kCodeInline = 'code',		///< code (inline)
		kHtmlInline = 'html',		///< HTML (inline)
		kCustomInline = 'cstm'		///< custom (inline, format-dependent)
	};

	/** Text list type. */
	DEFINE_ENUM (ListType)
	{
		kListTypeUnknown = 0,

		kOrderedList = 'ordr',		///< ordered list (1., 2., 3., etc.)
		kUnorderedList = 'unod'		///< unordered (bulleted) list
	};

	/** Text heading levels. */
	enum HeadingLevel
	{
		kH1 = 1,
		kH2,
		kH3,
		kH4,
		kH5,
		kH6,

		kMaxHeadingLevel = kH6
	};
}

/** Text Node Type. */
typedef Text::NodeType TextNodeType;

/** Text List Type. */
typedef Text::ListType TextListType;

} // namespace CCL

#endif // _ccl_textformatting_h
