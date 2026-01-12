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
// Filename    : ccl/public/app/ibrowser.h
// Description : Browser Interfaces
//
//************************************************************************************************

#ifndef _ccl_ibrowser_h
#define _ccl_ibrowser_h

#include "ccl/public/text/cstring.h"

namespace CCL {

interface IImage;
interface IContextMenu;
interface IUnknownList;
interface IClassDescription;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Class category for browser extensions
//////////////////////////////////////////////////////////////////////////////////////////////////

#define PLUG_CATEGORY_BROWSEREXTENSION_ "BrowserExtension"
#define PLUG_CATEGORY_BROWSEREXTENSION CCLSTR (PLUG_CATEGORY_BROWSEREXTENSION_)

#define MAKE_BROWSEREXTENSION_CATEGORY(subCategory) PLUG_CATEGORY_BROWSEREXTENSION_ ":" subCategory

//************************************************************************************************
// IBrowserNode
/** Basic interface of all nodes in browser. 
	\ingroup app_inter */
//************************************************************************************************

interface IBrowserNode: IUnknown
{
	/** Get type of this node (returned by value). */
	virtual CString CCL_API getNodeType () const = 0;

	/** Check if this node is related to given type. */
	virtual tbool CCL_API isNodeType (StringID type) const = 0;

	/** Get title of this node. */
	virtual StringRef CCL_API getNodeTitle () const = 0;

	// Browser Node Properties (IObject)
	DECLARE_STRINGID_MEMBER (kTitleProperty)
	DECLARE_STRINGID_MEMBER (kIconProperty)

	DECLARE_IID (IBrowserNode)
};

DEFINE_IID (IBrowserNode, 0x460d9b4, 0xa1bb, 0x412e, 0xa1, 0xbc, 0x3a, 0x97, 0x7a, 0x29, 0x37, 0x70)
DEFINE_STRINGID_MEMBER (IBrowserNode, kTitleProperty, "title")
DEFINE_STRINGID_MEMBER (IBrowserNode, kIconProperty, "icon")

//************************************************************************************************
// IBrowserExtension
/** Browser extension interface. 
	\ingroup app_inter */
//************************************************************************************************

interface IBrowserExtension: IUnknown
{
	/** Extend context menu for given node. */
	virtual tresult CCL_API extendBrowserNodeMenu (IBrowserNode* node, IContextMenu& menu, IUnknownList* selectedNodes) = 0;

	DECLARE_IID (IBrowserExtension)
};

DEFINE_IID (IBrowserExtension, 0x388b0bd0, 0x81c4, 0x4389, 0x95, 0xc8, 0x8b, 0x14, 0x36, 0xb0, 0x82, 0x54)

namespace Browsable {

//************************************************************************************************
// Browsable::IFileNode
/** Interface for nodes representing file system objects. 
	\ingroup app_inter */
//************************************************************************************************

interface IFileNode: IBrowserNode
{
	/** Get associated file URL. */
	virtual UrlRef CCL_API getFilePath () const = 0;

	DECLARE_IID (IFileNode)
};

DEFINE_IID (IFileNode, 0x5545a16e, 0x4fee, 0x4b8d, 0xa7, 0xdb, 0x95, 0x80, 0xda, 0x8, 0xe7, 0x6e)

//************************************************************************************************
// Browsable::IClassNode
/** Interface for nodes representing classes. 
	\ingroup app_inter */
//************************************************************************************************

interface IClassNode: IBrowserNode
{
	/** Get associated class description. */
	virtual const IClassDescription& CCL_API getClassDescription () const = 0;
	
	DECLARE_IID (IClassNode)
};

DEFINE_IID (IClassNode, 0x7fdaddeb, 0xfcec, 0x486c, 0xa2, 0xf1, 0xdc, 0xe9, 0x88, 0x8e, 0xaa, 0xdb)

} // namespace Browsable

} // namespace CCL

#endif // _ccl_ibrowser_h
