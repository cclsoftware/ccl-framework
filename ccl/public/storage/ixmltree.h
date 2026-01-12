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
// Filename    : ccl/public/storage/ixmltree.h
// Description : XML Tree Interface
//
//************************************************************************************************

#ifndef _ccl_ixmltree_h
#define _ccl_ixmltree_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

//************************************************************************************************
// IXmlNode
/** Node in an XML Tree. The nested nodes typically also support IObjectNode.
	\ingroup base_io  */
//************************************************************************************************

interface IXmlNode: IUnknown
{
	/** Get attribute inside XML tag. */
	virtual StringRef CCL_API getAttribute (StringRef key) const = 0;
	
	/** Set attribute inside XML tag. */
	virtual void CCL_API setAttribute (StringRef key, StringRef value) = 0;

	/** Get text inside XML tag. */
	virtual StringRef CCL_API getText () const = 0;

	/** Set text inside XML tag. */	
	virtual void CCL_API setText (StringRef text) = 0;

	/** Get text of child node with given name. */
	virtual String CCL_API getElementString (StringRef name) const = 0;

	/** Add new child node with given name. */
	virtual IXmlNode& CCL_API newChildNode (StringRef name) = 0;

	DECLARE_IID (IXmlNode)
};

DEFINE_IID (IXmlNode, 0x36d41645, 0x93fd, 0x4f4e, 0x9e, 0x73, 0xac, 0xa9, 0xd9, 0xa3, 0x2e, 0x44)

} // namespace CCL

#endif // _ccl_ixmltree_h
