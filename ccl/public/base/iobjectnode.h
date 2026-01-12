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
// Filename    : ccl/public/base/iobjectnode.h
// Description : Object Node Interface
//
//************************************************************************************************

#ifndef _ccl_iobjectnode_h
#define _ccl_iobjectnode_h

#include "ccl/public/base/debug.h"
#include "ccl/public/text/cclstring.h"

namespace CCL {

interface IMutableArray;

//************************************************************************************************
// IObjectNode
/** Basic interface for nested objects. 
	\ingroup ccl_base */
//************************************************************************************************

interface IObjectNode: IUnknown
{
	/** Get identifier used to build object paths. */
	virtual StringRef CCL_API getObjectID () const = 0;

	/** Get unique identifier of this instance. */
	virtual UIDRef CCL_API getObjectUID () const = 0;

	/** Get unique identifier describing the class (can be empty). */
	virtual UIDRef CCL_API getClassUID () const = 0;
	
	/** Get parent object. */
	virtual IObjectNode* CCL_API getParent () const = 0;
	
	/** Get root object. */
	virtual IObjectNode* CCL_API getRoot () const = 0;

	/** Get number of child objects. */
	virtual int CCL_API countChildren () const = 0;

	/** Get child by index. */
	virtual IObjectNode* CCL_API getChild (int index) const = 0;

	/** Find child by string identifier. */
	virtual IObjectNode* CCL_API findChild (StringRef id) const = 0;
	
	/** Get identifiers of child delegates, i.e. accessible via findChild(), but no direct descendants. */
	virtual tbool CCL_API getChildDelegates (IMutableArray& delegates) const = 0;
	
	/** Get path string describing object location in tree (e.g. "root/parent/object"). */
	virtual tbool CCL_API getChildPath (String& path) const = 0;

	/** Find child by path string, searching the object tree recursively. */
	virtual IObjectNode* CCL_API lookupChild (StringRef path) const = 0;

	DECLARE_IID (IObjectNode)
};

DEFINE_IID (IObjectNode, 0xdd45c0c2, 0x903, 0x4648, 0x9a, 0xa0, 0xcb, 0x89, 0xe5, 0xa6, 0xcd, 0x2)

//************************************************************************************************
// AbstractNode
/** Base class for IObjectNode implementation.
	\ingroup ccl_base */
//************************************************************************************************

class AbstractNode: public IObjectNode
{
public:
	StringRef CCL_API getObjectID () const override
	{
		return String::kEmpty;
	}

	UIDRef CCL_API getObjectUID () const override
	{
		return kNullUID;
	}

	UIDRef CCL_API getClassUID () const override
	{
		return kNullUID;
	}

	IObjectNode* CCL_API getParent () const override
	{
		return nullptr;
	}

	IObjectNode* CCL_API getRoot () const override
	{
		return nullptr;
	}

	int CCL_API countChildren () const override
	{
		return 0;
	}

	IObjectNode* CCL_API getChild  (int index) const override
	{
		return nullptr;
	}

	IObjectNode* CCL_API findChild (StringRef id) const override
	{
		return nullptr;
	}
	
	tbool CCL_API getChildDelegates (IMutableArray& delegates) const override
	{
		return false;
	}

	tbool CCL_API getChildPath (String& path) const override
	{
		path.empty ();
		const IObjectNode* p = this;
		while(p)
		{
			if(!path.isEmpty ())
				path.prepend (CCLSTR ("/"));

			String id (p->getObjectID ());
			if(id.isEmpty ())
				return false;

			path.prepend (id);
			p = p->getParent ();
		}
		return true;
	}

	IObjectNode* CCL_API lookupChild (StringRef path) const override
	{
		ASSERT (path.isEmpty () == false)
		if(path.isEmpty ())
			return nullptr;

		const IObjectNode* current = this;
		ForEachStringToken (path, CCLSTR ("/"), token)
			if(token == "..")
				current = current->getParent ();
			else
				current = current->findChild (token);
			if(!current)
				break;
		EndFor
		return const_cast<IObjectNode*> (current);
	}
};

} // namespace CCL

#endif // _ccl_iobjectnode_h
