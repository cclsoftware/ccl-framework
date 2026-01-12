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
// Filename    : ccl/base/objectnode.h
// Description : Object Node
//
//************************************************************************************************

#ifndef _ccl_objectnode_h
#define _ccl_objectnode_h

#include "ccl/base/object.h"

#include "ccl/public/base/iobjectnode.h"

namespace CCL {

class Container;
class Iterator;
class ObjectArray;

//************************************************************************************************
// ObjectNode
//************************************************************************************************

class ObjectNode: public Object,
				  public AbstractNode
{
public:
	DECLARE_CLASS (ObjectNode, Object)
	DECLARE_METHOD_NAMES (ObjectNode)

	ObjectNode (StringRef objectID = nullptr);
	ObjectNode (const ObjectNode&);
	~ObjectNode ();

	void setName (StringRef name);
	bool setUniqueName (StringRef name); // accept name only when unique
	StringRef getName () const;

	virtual Iterator* newIterator () const;

	virtual bool addChild (ObjectNode* child);
	virtual bool insertChild (int index, ObjectNode* child);
	virtual bool addChildSorted (ObjectNode* child);
	virtual bool removeChild (ObjectNode* child);
	virtual void removeAll ();

	virtual ObjectNode* findChildNode (StringRef id) const;
	virtual ObjectNode* findChildNode (MetaClassRef typeID) const;

	template <class T> T* findChildNode (StringRef id) const;
	template <class T> T* findChildNode () const;
	template <class T> T* getChildNode (int index) const;
	template <class I> I* findChildByInterface (bool deep = false) const;

	ObjectNode* getChildNode (int index) const;
	int getChildIndex (const ObjectNode* child) const;
	virtual StringRef getChildID (const ObjectNode* child) const; // allow to simplify paths

	ObjectNode* getParentNode () const;
	template <class T> T* getParentNode () const;

	bool getRelativePath (String& path, ObjectNode* subNode) const;

	void signalDeep (MessageRef msg, bool recursive = false); // signal and notify children
	virtual void notifyChildren (ISubject* subject, MessageRef msg, bool recursive = false); // notify children
	tresult queryChildInterface (UIDRef iid, void** ptr, bool recursive = false);

	template<class Lambda>
	bool visitChildren (const Lambda& visitLambda, bool recursive) const;

	void dump (int level = 0);

	// IObjectNode
	StringRef CCL_API getObjectID () const override;
	UIDRef CCL_API getObjectUID () const override;
	UIDRef CCL_API getClassUID () const override;
	IObjectNode* CCL_API getParent () const override;
	IObjectNode* CCL_API getRoot () const override;
	int CCL_API countChildren () const override;
	IObjectNode* CCL_API getChild  (int index) const override;
	IObjectNode* CCL_API findChild (StringRef id) const override;

	CLASS_INTERFACE (IObjectNode, Object)

protected:
	ObjectArray& getChildArray () const;
	Container& getChildren () const;
	void setObjectUID (UIDRef uid);
	void resetObjectUID ();
	bool isValidObjectUID () const;
	void setParent (ObjectNode* parent);
	void unlinkAll ();
	void assignUniqueName (ObjectNode& child, StringRef base) const;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;

private:
	String objectID;
	mutable UID objectUID;
	ObjectNode* parent;
	mutable ObjectArray* children;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// ObjectNode inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline void ObjectNode::setName (StringRef name)
{ objectID = name; }

inline StringRef ObjectNode::getName () const
{ return objectID; }

inline void ObjectNode::setObjectUID (UIDRef uid)
{ ASSERT (objectUID.isValid () == false) objectUID = uid; }

inline void ObjectNode::resetObjectUID ()
{ objectUID = kNullUID; }

inline bool ObjectNode::isValidObjectUID () const
{ return objectUID.isValid (); }

inline void ObjectNode::setParent (ObjectNode* _parent)
{ parent = _parent; }

inline ObjectNode* ObjectNode::getParentNode () const
{ return parent ? parent : unknown_cast<ObjectNode> (getParent ()); }

template <class T> T* ObjectNode::getParentNode () const
{ return ccl_cast<T> (getParentNode ()); }

template <class T> T* ObjectNode::getChildNode (int index) const
{ return ccl_cast<T> (getChildNode (index)); }

template <class T> T* ObjectNode::findChildNode (StringRef id) const
{ return ccl_cast<T> (findChildNode (id)); }

template <class T> T* ObjectNode::findChildNode () const
{ return (T*)findChildNode (ccl_typeid<T> ()); }

template<class Lambda>
bool ObjectNode::visitChildren (const Lambda& visitLambda, bool recursive) const
{
	for(int i = 0, count = countChildren (); i < count; i++)
	{
		ObjectNode* node = getChildNode (i);
		if(visitLambda (node) == false)
			return false;

		if(recursive)
			if(node->visitChildren (visitLambda, recursive) == false)
				return false;
	}
	return true;
}

template <class I> I* ObjectNode::findChildByInterface (bool deep) const
{
	I* result = nullptr;
	visitChildren ([&] (ObjectNode* child) 
	{
		if(result = UnknownPtr<I> (child->asUnknown ()))		
			return false; // break iteration
		return true;
	}, deep);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_objectnode_h
