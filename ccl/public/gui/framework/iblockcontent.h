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
// Filename    : ccl/public/gui/framework/iblockcontent.h
// Description : Block Content Interfaces
//
//************************************************************************************************

#ifndef _ccl_iblockcontent_h
#define _ccl_iblockcontent_h

#include "ccl/public/collections/iunknownlist.h"

namespace CCL {

interface IBlockContentChildren;
interface IBlockContentBuilder;

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	/** Block content root [IBlockContentRoot] */
	DEFINE_CID (BlockContentRoot, 0xdc637d05, 0x6999, 0x4aa9, 0xa9, 0x3f, 0x34, 0x92, 0x20, 0x60, 0x90, 0x33)
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Block Content Types
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Plain text content type. */
DEFINE_STRINGID (kPlainText, "text/plain")

/** CCL Markup content type. */
extern const CString kCCLMarkup;

/** Markdown content type (RFC 7763). */
DEFINE_STRINGID (kMarkdown, "text/markdown")

//************************************************************************************************
// IBlockContentNode
//************************************************************************************************

interface IBlockContentNode: IUnknown
{
	/** Get interface to manage child nodes. */
	virtual IBlockContentChildren& CCL_API getChildren () = 0;

	/** Get parent node. */
	virtual IBlockContentNode* CCL_API getParentNode () = 0;

	/** Set custom style identifier. */
	virtual void CCL_API setCustomStyleID (StringID id) = 0;

	/** Get custom style identifier. */
	virtual StringID CCL_API getCustomStyleID () const = 0;

	DECLARE_IID (IBlockContentNode)
};

DEFINE_IID (IBlockContentNode, 0x6f7bb906, 0xf441, 0x4e0e, 0xb0, 0x94, 0x7d, 0x71, 0xee, 0x8d, 0xf7, 0xdd)

//************************************************************************************************
// IBlockContentChildren
//************************************************************************************************

interface IBlockContentChildren: IContainer
{
	/** Insert child node, parent takes ownerhip. */
	virtual tresult CCL_API insertNode (IBlockContentNode* node, int index = -1) = 0;

	/** Remove child node, ownership is transferred to caller. */
	virtual tresult CCL_API removeNode (IBlockContentNode* node) = 0;

	/** Remove (and release) all child nodes. */
	virtual void CCL_API removeNodes () = 0;

	DECLARE_IID (IBlockContentChildren)
};

DEFINE_IID (IBlockContentChildren, 0x872cb6df, 0x215d, 0x4e34, 0x91, 0x7c, 0x83, 0x50, 0x1a, 0xe4, 0x86, 0xff)

//************************************************************************************************
// IBlockContentRoot
/** Interface for root node in block-based structured content (text, images, lists, etc.). */
//************************************************************************************************

interface IBlockContentRoot: IBlockContentNode
{
	/**	Create new builder instance to load content.
		Builder can be short-lived or kept for longer. */
	virtual IBlockContentBuilder* CCL_API createBuilder () = 0;

	/** Scroll to make given node visible in a BlockView. */
	virtual void CCL_API makeNodeVisible (IBlockContentNode* node) = 0;

	DECLARE_IID (IBlockContentRoot)
};

DEFINE_IID (IBlockContentRoot, 0xa5a141e3, 0x7beb, 0x47ce, 0xa4, 0x3a, 0x9, 0xf6, 0x35, 0x67, 0xb5, 0x11)

//************************************************************************************************
// IBlockContentBuilder
//************************************************************************************************

interface IBlockContentBuilder: IUnknown
{
	/**	Load content from string, data stream (IStream), or local file (IUrl).
		Supported formats include plain text, CCL Markup, Markdown, and images (PNG, SVG, etc.).
		Images are also supported directly by passing IImage.
		Newly created node is owned by caller and not inserted automatically. */
	virtual tresult CCL_API loadContent (IBlockContentNode*& newNode, VariantRef data, StringID contentType) = 0;

	DECLARE_IID (IBlockContentBuilder)
};

DEFINE_IID (IBlockContentBuilder, 0x3fecd5cb, 0x5bf2, 0x4d72, 0xb1, 0x51, 0xa, 0xe6, 0xc6, 0xd8, 0x5d, 0x35)

//************************************************************************************************
// IBlockContentResource
/**	Block content resource interface.
	Defined in Skin XML, can be accessed by name via ITheme::getResource().
	\ingroup gui_graphics3d */
//************************************************************************************************

interface IBlockContentResource: IUnknown
{
	/** Create new content instance as defined in skin package. */
	virtual IBlockContentRoot* CCL_API createContent () = 0;

	DECLARE_IID (IBlockContentResource)
};

DEFINE_IID (IBlockContentResource, 0x82f8a41e, 0x7293, 0x47c5, 0x88, 0xfe, 0xb0, 0xd7, 0x4b, 0x2a, 0x27, 0xb1)

} // namespace CCL

#endif // _ccl_iblockcontent_h