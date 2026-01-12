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
// Filename    : core/public/coretreeset.h
// Description : Tree Set class
//
//************************************************************************************************

#ifndef _coretreeset_h
#define _coretreeset_h

#include "coremath.h"
#include "corecontainer.h"

namespace Core {

template<class T>
class TreeSetIterator;

//************************************************************************************************
// TreeSet
/**	Set container class based on red-black tree.
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class T>
class TreeSet
{
public:
	/** Compare function type. */
	typedef int (*CompareFunction) (const T&, const T&);

	TreeSet (CompareFunction compareFunction = defaultCompare);
	TreeSet (const TreeSet<T>& other);
	~TreeSet ();

	TreeSet<T>& operator = (const TreeSet<T>& other);

	/** Add element to container. */
	bool add (const T& data);
	
	/** Remove element from container. */
	bool remove (const T& data);
	
	/** Remove all elements. */
	void removeAll ();
	
	/** Check if container is empty. */
	bool isEmpty () const;
	
	/** Get number of elements in container. */
	int count () const;

	/** Find element in container. */
	const T& lookup (const T& data) const;
	
	/** Check if container holds given element. */
	bool contains (const T& data) const;
	
	RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> begin () const;
	RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> end () const;

protected:
	friend TreeSetIterator<T>;

	enum Direction { kLeft, kRight, kNone };
	class ExtendedNode;

	class Node
	{
	public:
		Node (const T& value);
		Node (const Node& other);
		~Node ();

		const T& lookup (const TreeSet<T>& tree, const T& data) const;

	protected:
		friend class TreeSet<T>::ExtendedNode;
		friend class TreeSetIterator<T>;

		enum Type
		{
			kBlack, // Trees and subtrees are always balanced in terms of black nodes (BlackBalanceInvariant)
			kRed	// A red node has no red children (RedRedInvariant)
		};

		T value;
		Type type = Type::kRed;
		Node* children[2];
	};

	// Node class with parent pointer and node pointer reference. Only used temporarily on stack.
	class ExtendedNode
	{
	public:
		ExtendedNode (TreeSet<T>* tree);
		ExtendedNode ();
		ExtendedNode (ExtendedNode* parent, TreeSet<T>::Direction direction, TreeSet<T>* tree);

		bool add (const T& data);
		bool remove (const T& data);

		const ExtendedNode& operator = (const ExtendedNode& other);

	protected:
		friend TreeSetIterator<T>;

		ExtendedNode* parent;
		TreeSet<T>::Direction direction;
		TreeSet<T>* tree;
		TreeSet<T>::Node* node;

		void fixRedRedInvariant ();
	
		void swapRightmostElement (ExtendedNode* swap);
		void fixBlackBalanceInvariant ();

		Node*& getNode (); // Returns a reference to the actual pointer in the tree.
		Node*& getSibling (); // Returns a reference to the actual pointer in the tree.
		Node*& getParent (); // Returns a reference to the actual pointer in the tree.

		void rotateUp (); // This refers to the new child (previous parent node of this).
	};

	Node* root;
	CompareFunction compareFunction;
	int elementCount;

	static const T& error ();
	static int defaultCompare (const T& left, const T& right);
};

//************************************************************************************************
// TreeSetIterator
/**	Tree set iterator. 
    \ingroup core_collect
	\ingroup base_collect */
//************************************************************************************************

template<class T>
class TreeSetIterator
{
public:
	TreeSetIterator (const TreeSet<T>& tree);
	~TreeSetIterator ();

	/** Check if iteration is done. */
	bool done () const;

	/** Seek to first element. */
	void first ();

	/** Seek to last element. */
	void last ();
	
	/** Seek and return next element. */
	const T& next ();
	
	/** Seek and return previous element. */
	const T& previous ();

	/** Peek at next element (but don't seek). */
	const T& peekNext () const;

	bool operator == (const TreeSetIterator<T>& other) const;
	bool operator != (const TreeSetIterator<T>& other) const;

protected:
	friend class TreeSet<T>;

	const TreeSet<T>& tree;
	typename TreeSet<T>::ExtendedNode* nodes;
	int currentDepth;

	void decend (typename TreeSet<T>::Direction direction);
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// TreeSet<T>::Node
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::Node::Node (const T& value)
: value (value),
  children (),
  type (kRed)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::Node::Node (const Node& other)
: value (other.value),
  children (),
  type (other.type)
{
	for(int i = 0; i < 2; i++)
		if(other.children[i])
			children[i] = NEW Node (*other.children[i]);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::Node::~Node ()
{
	for(int i = 0; i < 2; i++)
		if(children[i])
			delete children[i];
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSet<T>::Node::lookup (const TreeSet<T>& tree, const T& data) const
{
	int comparison = tree.compareFunction (value, data);
	if(comparison == 0)
		return value;
	else if(comparison > 0 && children[kLeft])
		return children[kLeft]->lookup (tree, data);
	else if(comparison < 0 && children[kRight])
		return children[kRight]->lookup(tree, data);
	else
		return tree.error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TreeSet<T>::ExtendedNode
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::ExtendedNode::ExtendedNode (TreeSet<T>* tree)
: node (tree ? tree->root : nullptr),
  parent (nullptr),
  direction (TreeSet<T>::Direction::kNone),
  tree (tree)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::ExtendedNode::ExtendedNode ()
: ExtendedNode (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::ExtendedNode::ExtendedNode (ExtendedNode * parent, TreeSet<T>::Direction direction, TreeSet<T>* tree)
: node (parent->node->children[direction]),
  parent (parent),
  direction (direction),
  tree (tree)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::ExtendedNode::add (const T& data)
{
	Node*& node = getNode ();
	if(!node)
	{
		node = NEW Node (data);
		this->node = node;
		fixRedRedInvariant ();
		return true;
	}

	int comparison = tree->compareFunction (node->value, data);
	if(comparison > 0)
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kLeft, tree);
		return child.add (data);
	}
	if(comparison < 0)
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kRight, tree);
		return child.add (data);
	}
	node->value = data;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::ExtendedNode::remove (const T& data)
{
	Node* node = getNode ();
	if(!node)
		return false;

	int comparison = tree->compareFunction (node->value, data);

	if(comparison > 0)
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kLeft, tree);
		return child.remove (data);
	}
	if(comparison < 0)
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kRight, tree);
		return child.remove (data);
	}

	if(node->children[kLeft])
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kLeft, tree);
		child.swapRightmostElement (this);
		return true;
	}

	// This has at most one leaf as a child
	ASSERT (!node->children[kRight] || (node->type == Node::kBlack && node->children[kRight]->type == Node::kRed))
	ASSERT (!node->children[kRight] || (!node->children[kRight]->children[kRight] && !node->children[kRight]->children[kLeft]))
	swapRightmostElement (this);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const typename TreeSet<T>::ExtendedNode& TreeSet<T>::ExtendedNode::operator = (const TreeSet<T>::ExtendedNode& other)
{
	parent = other.parent;
	tree = other.tree;
	node = other.node;
	direction = other.direction;
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSet<T>::ExtendedNode::fixRedRedInvariant ()
{
	Node* node = getNode ();
	ASSERT (node->type == Node::kRed)
	if(parent == nullptr)
	{
		node->type = Node::kBlack;
		return;
	}

	Node*& parentNode = getParent ();
	if(parentNode->type == Node::kBlack)
		return;

	ASSERT (parent->parent && parent->getParent ()->type == Node::kBlack)
	
	ExtendedNode* grandParent = parent->parent;
	Node* parentSibling = parent->getSibling ();
	if(parentSibling && parentSibling->type == Node::kRed)
	{
		parentSibling->type = Node::kBlack;
		parent->node->type = Node::kBlack;
		grandParent->node->type = Node::kRed;
		grandParent->fixRedRedInvariant ();
		return;
	}

	if(parent->direction != direction) // Inner grand child
		rotateUp (); // Make this an outer grand child

	// Outer grand child
	ASSERT (parent->direction == direction)
	parentNode->type = Node::kBlack;
	grandParent->node->type = Node::kRed;
	parent->rotateUp ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSet<T>::ExtendedNode::swapRightmostElement (TreeSet<T>::ExtendedNode* swapNode)
{
	Node*& node = getNode ();
	if(node->children[kRight])
	{
		ExtendedNode child (this, TreeSet<T>::Direction::kRight, tree);
		child.swapRightmostElement (swapNode);
		return;
	}

	swapNode->getNode ()->value = node->value;
	
	// Remove this node
	if(Node* leftChild = node->children[kLeft])
	{
		ASSERT (node->type == Node::kBlack && leftChild->type == Node::kRed)
		leftChild->type = Node::kBlack;
		node->children[kLeft] = nullptr; // Don't delete child in destructor
		delete node;
		node = leftChild;
		return;
	}

	// Single black leaf, this is the complex case
	if(node->type == Node::kBlack)
		fixBlackBalanceInvariant ();
	ASSERT (!node->children[kLeft] && !node->children[kRight])
	delete node;
	node = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSet<T>::ExtendedNode::fixBlackBalanceInvariant ()
{
	ASSERT (node->type == Node::kBlack)
	if(!parent)
		return;
	Node* parentNode = getParent ();
	Node* siblingNode = getSibling ();
	ASSERT (siblingNode)
	ExtendedNode sibling (parent, Direction (1 - direction), tree);

	if(siblingNode->type == Node::kRed)
	{
		siblingNode->type = Node::kBlack;
		parentNode->type = Node::kRed;

		sibling.rotateUp (); // Breaks 'unrelated' extendedNodes like 'this'
		parent = &sibling; // Fix 'this' after sibling rotation
		fixBlackBalanceInvariant ();
		return;
	}

	Node* distantSiblingChild = siblingNode->children[1 - direction];
	if(distantSiblingChild && distantSiblingChild->type == Node::kRed)
	{
		siblingNode->type = parentNode->type;
		parentNode->type = Node::kBlack;
		distantSiblingChild->type = Node::kBlack;
		sibling.rotateUp ();
		return;
	}

	Node* closeSiblingChild = siblingNode->children[direction];
	if(closeSiblingChild && closeSiblingChild->type == Node::kRed)
	{
		siblingNode->type = Node::kRed;
		closeSiblingChild->type = Node::kBlack;
		ExtendedNode siblingChild (&sibling, direction, tree);
		siblingChild.rotateUp ();
		fixBlackBalanceInvariant ();
		return;
	}
	if(parentNode->type == Node::kRed)
	{
		parentNode->type = Node::kBlack;
		siblingNode->type = Node::kRed;
		return;
	}

	// All relevant nodes are black or not existent (in case of siblingChildren)
	siblingNode->type = Node::kRed;
	parent->fixBlackBalanceInvariant ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
typename TreeSet<T>::Node*& TreeSet<T>::ExtendedNode::getNode ()
{
	if(!parent)
		return tree->root;
	return parent->node->children[direction];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
typename TreeSet<T>::Node*& TreeSet<T>::ExtendedNode::getSibling ()
{
	ASSERT (parent)
	return parent->node->children[1 - direction];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
typename TreeSet<T>::Node*& TreeSet<T>::ExtendedNode::getParent ()
{
	ASSERT (parent)
	return parent->getNode ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSet<T>::ExtendedNode::rotateUp ()
{
	ASSERT (parent)
	Node*& self = getNode ();
	Node*& parentNode = getParent ();
	
	Node* temp = self->children[1 - direction];
	self->children[1 - direction] = parentNode;
	parentNode = self;
	self = temp;

	// Swap references of this and parent
	direction = Direction (1 - direction);
	parent->node = parentNode;
	node = parentNode->children[direction];
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TreeSetIterator implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSetIterator<T>::TreeSetIterator (const TreeSet<T>& tree)
: tree (tree),
  nodes (nullptr),
  currentDepth (0)
{
	int count = tree.count ();
	int maxDepth = ceil (2 * log2 (count + 2) - 2);
	nodes = NEW typename TreeSet<T>::ExtendedNode[maxDepth];
	first ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSetIterator<T>::~TreeSetIterator ()
{
	delete [] nodes;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSetIterator<T>::done () const
{
	return currentDepth < 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSetIterator<T>::first ()
{
	if(tree.root)
	{
		nodes[0].node = tree.root;
		currentDepth = 0;
		decend (TreeSet<T>::kLeft);
	}
	else
		currentDepth = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSetIterator<T>::last ()
{
	if(tree.root)
	{
		nodes[0].node = tree.root;
		currentDepth = 0;
		decend (TreeSet<T>::kRight);
	}
	else
		currentDepth = -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSetIterator<T>::next ()
{
	if(done ())
		return TreeSet<T>::error ();

	typename TreeSet<T>::ExtendedNode& current = nodes[currentDepth];
	T& result = current.node->value;
	if(current.node->children[TreeSet<T>::kRight])
	{
		nodes[++currentDepth] = typename TreeSet<T>::ExtendedNode (&current, TreeSet<T>::kRight, nullptr);
		decend (TreeSet<T>::kLeft);
	}
	else
	{
		while(nodes[currentDepth].direction == TreeSet<T>::kRight)
			currentDepth--;
		currentDepth--;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSetIterator<T>::previous ()
{
	if(done ())
		return TreeSet<T>::error ();

	typename TreeSet<T>::ExtendedNode& current = nodes[currentDepth];
	T& result = current.node->value;
	if(current.node->children[TreeSet<T>::kLeft])
	{
		nodes[++currentDepth] = TreeSet<T>::ExtendedNode (&current, TreeSet<T>::kLeft, nullptr);
		decend (TreeSet<T>::kRight);
	}
	else
	{
		while(nodes[currentDepth].direction == TreeSet<T>::kLeft)
			currentDepth--;

		currentDepth--;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSetIterator<T>::peekNext () const
{
	return nodes[currentDepth].node->value;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSetIterator<T>::operator == (const TreeSetIterator<T>& other) const
{
	if(other.done ())
		return done ();
	else if(done ())
		return false;
	else
		return nodes[currentDepth].node == other.nodes[other.currentDepth].node;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSetIterator<T>::operator != (const TreeSetIterator<T>& other) const
{
	return !operator == (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSetIterator<T>::decend (typename TreeSet<T>::Direction direction)
{
	typename TreeSet<T>::ExtendedNode& current = nodes[currentDepth];
	if(!current.node->children[direction])
		return;

	nodes[++currentDepth] = typename TreeSet<T>::ExtendedNode (&current, direction, nullptr);
	ASSERT (currentDepth < ceil (2 * log2 (tree.count () + 2) - 2)) // Array bounds
	decend (direction);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// TreeSet implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::TreeSet (CompareFunction compareFunction)
: compareFunction (compareFunction),
  root (nullptr),
  elementCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::TreeSet (const TreeSet<T>& other)
: TreeSet<T> ()
{
	operator = (other);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>::~TreeSet ()
{
	if(root)
		delete root;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
TreeSet<T>& TreeSet<T>::operator = (const TreeSet<T>& other)
{
	removeAll ();
	compareFunction = other.compareFunction;
	elementCount = other.elementCount;
	if(other.root)
		root = NEW Node (*other.root);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::add (const T& data)
{
	ExtendedNode root (this);
	bool success = root.add (data);
	if(success)
		elementCount++;
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::remove (const T& data)
{
	ExtendedNode root (this);
	bool success = root.remove (data);
	if(success)
		elementCount--;
	ASSERT (elementCount >= 0)
	return success;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
void TreeSet<T>::removeAll ()
{
	elementCount = 0;
	if(root)
		delete root;
	root = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::isEmpty () const
{
	return elementCount == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
int TreeSet<T>::count () const
{
	return elementCount;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSet<T>::lookup (const T& data) const
{
	if(!root)
		return error ();
	return root->lookup (*this, data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
bool TreeSet<T>::contains (const T& data) const
{
	return lookup (data) != error ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> TreeSet<T>::begin () const
{
	return RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> TreeSet<T>::end () const
{
	static TreeSet<T> dummy;
	return RangeIterator<TreeSet<T>, TreeSetIterator<T>, const T> (dummy);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
const T& TreeSet<T>::error ()
{ 
	static T kError = T ();
	return kError;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
int TreeSet<T>::defaultCompare (const T& left, const T& right)
{
	if(left > right)
		return 1;
	if(left == right)
		return 0;
	return -1;
}

} // namespace Core

#endif // _coretreeset_h
