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
// Filename    : ccl/base/objectnode.cpp
// Description : Object Node
//
//************************************************************************************************

#include "ccl/base/objectnode.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/public/base/variant.h"

using namespace CCL;

//************************************************************************************************
// ObjectNode
//************************************************************************************************

DEFINE_CLASS (ObjectNode, Object)
DEFINE_CLASS_NAMESPACE (ObjectNode, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode::ObjectNode (StringRef objectID)
: objectID (objectID),
  parent (nullptr),
  children (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode::ObjectNode (const ObjectNode& g)
: objectID (g.objectID),
  objectUID (kNullUID), // do not copy unique id!
  parent (nullptr),
  children (nullptr)
{
	// clone children
	IterForEach (g.newIterator (), ObjectNode, child)
		addChild ((ObjectNode*)child->clone ());
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode::~ObjectNode ()
{
	if(children)
		children->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::unlinkAll ()
{
	if(children)
		ArrayForEach (*children, ObjectNode, child)
			child->setParent (nullptr);
		EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* ObjectNode::newIterator () const
{
	return children ? children->newIterator () : NEW NullIterator; // do not fail for scripts!
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectArray& ObjectNode::getChildArray () const
{
	if(!children)
	{
		children = NEW ObjectArray;
		children->objectCleanup (true);
	}
	return *children;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Container& ObjectNode::getChildren () const
{
	return getChildArray ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::addChild (ObjectNode* child)
{
	ASSERT (child != nullptr && child->parent == nullptr)
	if(!child || (child && child->parent != nullptr))
		return false;

	child->parent = this;
	return getChildArray ().add (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::insertChild (int index, ObjectNode* child)
{
	ASSERT (child != nullptr && child->parent == nullptr)
	if(!child || (child && child->parent != nullptr))
		return false;

	child->parent = this;
	bool result = getChildArray ().insertAt (index, child);

	if(result == false)
		child->parent = nullptr;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::addChildSorted (ObjectNode* child)
{
	ASSERT (child != nullptr && child->parent == nullptr)
	if(!child || (child && child->parent != nullptr))
		return false;

	child->parent = this;
	bool result = getChildArray ().addSorted (child);

	if(result == false)
		child->parent = nullptr;

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::removeChild (ObjectNode* child)
{
	ASSERT (child != nullptr && child->parent == this)
	if(children && children->remove (child))
	{
		child->parent = nullptr;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::removeAll ()
{
	if(children)
		children->removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef CCL_API ObjectNode::getObjectID () const
{
	return objectID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ObjectNode::getObjectUID () const
{
	if(!objectUID.isValid ())
		objectUID.generate ();
	return objectUID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UIDRef CCL_API ObjectNode::getClassUID () const
{
	return myClass ().getClassID ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ObjectNode::getParent () const
{
	return parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ObjectNode::getRoot () const
{
	const IObjectNode* g = this;
	while(true)
	{
		if(g->getParent () == nullptr)
			return const_cast<IObjectNode*> (g);
		g = g->getParent ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ObjectNode::countChildren () const
{
	return children ? children->count () : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ObjectNode::getChild (int index) const
{
	ObjectNode* child = getChildNode (index);
	return child;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObjectNode* CCL_API ObjectNode::findChild (StringRef id) const
{
	return findChildNode (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode* ObjectNode::findChildNode (StringRef id) const
{
	if(children)
		ArrayForEach (*children, ObjectNode, child)
			if(child->getObjectID () == id)
				return child;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode* ObjectNode::findChildNode (MetaClassRef typeID) const
{
	if(children)
		ArrayForEach (*children, ObjectNode, child)
			if(child->canCast (typeID))
				return child;
		EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ObjectNode* ObjectNode::getChildNode (int index) const
{
	return children ? (ObjectNode*)children->at (index) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int ObjectNode::getChildIndex (const ObjectNode* child) const
{
	return getChildArray ().index (child);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef ObjectNode::getChildID (const ObjectNode* child) const
{
	return child ? child->getObjectID () : String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::getRelativePath (String& path, ObjectNode* subNode) const
{
	path.empty ();
	ObjectNode* p = subNode;
	while(p)
	{
		if(!path.isEmpty ())
			path.prepend (CCLSTR ("/"));

		ObjectNode* parentNode = p->getParentNode ();

		String id;
		if(parentNode)
			id = parentNode->getChildID (p);
		else
			id = p->getObjectID ();

		if(id.isEmpty ())
			return false;

		path.prepend (id);
		p = parentNode;
		if(p == this)
			return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::signalDeep (MessageRef msg, bool recursive)
{
	signal (msg);
	notifyChildren (this, msg, recursive);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::notifyChildren (ISubject* subject, MessageRef msg, bool recursive)
{
	// note: visitChildren not used to allow overwrite of notifyChildren
	for(int i = 0, count = countChildren (); i < count; i++)
	{
		ObjectNode* child = getChildNode (i);
		child->notify (subject, msg);
		
		if(recursive)
			child->notifyChildren (subject, msg, true);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult ObjectNode::queryChildInterface (UIDRef iid, void** ptr, bool recursive)
{
	tresult result = kResultNoInterface;
	auto queryChild = [&] (ObjectNode* child)
	{
		if(child->queryInterface (iid, ptr) == kResultOk)
		{
			result = kResultOk;
			return false; // break iteration
		}
		return true;
	};
	visitChildren (queryChild, recursive);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ObjectNode::setUniqueName (StringRef name)
{
	ObjectNode* myParent = getParentNode ();
	if(myParent)
	{
		ObjectNode* node = myParent->findChildNode (name);
		if(node == this)
			return true;
		else if(node != nullptr)
			return false;
	}
	setName (name);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::assignUniqueName (ObjectNode& child, StringRef baseName) const
{
	int index = 1;
	while(1)
	{
		String name = baseName;
		name.appendIntValue (index++, 2); // %02d
		if(!findChild (name))
		{
			child.setName (name);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectNode::dump (int level)
{
	for(int i = 0; i < level; i++)
		Debugger::print ("+");

	Debugger::print (objectID);
	Debugger::print (" (");
	Debugger::print (myClass ().getPersistentName ());
	Debugger::println (")");

	IterForEach (newIterator (), ObjectNode, child)
		child->dump (level + 1);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ObjectNode)
	DEFINE_METHOD_ARGR ("find", "childName: string", "ObjectNode")
END_METHOD_NAMES (ObjectNode)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ObjectNode::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "find")
	{
		String path (msg[0].asString ());
		returnValue = lookupChild (path);
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
