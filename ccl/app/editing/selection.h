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
// Filename    : ccl/app/editing/selection.h
// Description : Selection
//
//************************************************************************************************

#ifndef _ccl_selection_h
#define _ccl_selection_h

#include "ccl/base/collections/objectlist.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/app/iselection.h"
#include "ccl/public/base/irecognizer.h"

namespace CCL {

//************************************************************************************************
// ISelectionViewer
/** Selection viewer interface. */
//************************************************************************************************

interface ISelectionViewer
{
	/** Show selection. */
	virtual void showSelection (bool redraw = true) = 0;
	
	/** Hide selection. */
	virtual void hideSelection (bool redraw = true) = 0;

	/** Try to make the selected items visible (e.g. by scrolling). */
	virtual void makeSelectedItemsVisible (bool relaxed) = 0;
};

//************************************************************************************************
// SelectionContainer
/** Selection container base class. */
//************************************************************************************************

class SelectionContainer: public Object,
                          public IObjectFilter
{
public:
	DECLARE_CLASS_ABSTRACT (SelectionContainer, Object)

	virtual bool isEmpty () const = 0;
	virtual bool isMultiple () const = 0;
	virtual bool isSelected (Object* object) const = 0;
	virtual bool select (Object* object) = 0;
	virtual bool unselect (Object* object) = 0;
	virtual bool unselectAll () = 0;
	virtual bool unselectType (MetaClassRef type) = 0;
	virtual MetaClassRef getType () const = 0;
	virtual Iterator* newIterator (MetaClassRef type) const = 0;
	virtual Object* getFirst () const = 0;

	// IObjectFilter
	tbool CCL_API matches (IUnknown* iObject) const override
	{
		if(Object* object = unknown_cast<Object> (iObject))
			return isSelected (object);
		return false;
	}
	
	CLASS_INTERFACE (IObjectFilter, Object)
};

//************************************************************************************************
// SelectionList
/** Selection list implementation. */
//************************************************************************************************

class SelectionList: public SelectionContainer
{
public:
	DECLARE_CLASS (SelectionList, SelectionContainer)

	SelectionList (bool sharedItems = false);

	// SelectionContainer
	bool isEmpty () const override;
	bool isMultiple () const override;
	bool isSelected (Object* object) const override;
	bool select (Object* object) override;
	bool unselect (Object* object) override;
	bool unselectAll () override;
	bool unselectType (MetaClassRef type) override;
	MetaClassRef getType () const override;
	Iterator* newIterator (MetaClassRef type) const override;
	Object* getFirst () const override;

protected:
	ObjectList items;
	bool sharedItems;
};

//************************************************************************************************
// TypedSelectionList
//************************************************************************************************

template<class Type>
class TypedSelectionList: public SelectionList
{
public:
	using SelectionList::SelectionList;

	// SelectionList
	CCL::MetaClassRef getType () const override { return CCL::ccl_typeid<Type> (); }
};

//************************************************************************************************
// Selection
/** Selection base class. */
//************************************************************************************************

class Selection: public SelectionContainer,
				 public ISelection
{
public:
	DECLARE_CLASS_ABSTRACT (Selection, SelectionContainer)
	DECLARE_METHOD_NAMES (Selection)

	Selection ();
	~Selection ();

	int32 CCL_API getEditTag () const override; ///< [ISelection]
	void ignoreChanges (); ///< suppress kChanged signal for recent changes

	void addViewer (ISelectionViewer* viewer);
	void removeViewer (ISelectionViewer* viewer);

	/// delegated to selection viewers
	void hide (bool redraw = true);
	void show (bool redraw = true);
	void makeItemsVisible (bool relaxed);

	PROPERTY_BOOL (showHideSuspended, ShowHideSuspended)

	bool canSelect (Object* object) const;

	virtual int countTypes () const;
	virtual bool canSelectType (MetaClassRef type) const;
	virtual Iterator* newIterator (int index) const;
	template <class T> Iterator* newIterator () const;
	using SelectionContainer::newIterator;

	// SelectionContainer
	Object* getFirst () const override;

	CLASS_INTERFACE (ISelection, SelectionContainer)

	/// a guard that hides the selection and suspends showing it during it's scope
	struct Hideout
	{
		Hideout (Selection& selection, bool redraw = true);
		~Hideout ();

		Selection& selection;
		bool redraw;
		bool wasSuspended;
	};

protected:
	LinkedList<ISelectionViewer*> viewers;
	int32 editTag;
	int32 lastEditTag;

	void contentChanged ();
	void flushChanged ();

	// ISelection
	IUnknownIterator* CCL_API newIterator (StringID typeName) const override;
	tbool CCL_API isObjectSelected (IUnknown* object) const override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

//************************************************************************************************
// SimpleSelection
/** Selection for single data type. */
//************************************************************************************************

class SimpleSelection: public Selection
{
public:
	DECLARE_CLASS (SimpleSelection, Selection)

	// Selection
	bool isEmpty () const override;
	bool isMultiple () const override;
	bool isSelected (Object* object) const override;
	bool select (Object* object) override;
	bool unselect (Object* object) override;
	bool unselectAll () override;
	bool unselectType (MetaClassRef type) override;
	MetaClassRef getType () const override;
	Iterator* newIterator (MetaClassRef type) const override;
	using Selection::newIterator;

protected:
	SelectionList list;
};

//************************************************************************************************
// MixedSelection
/** Selection for mixed data types. */
//************************************************************************************************

class MixedSelection: public Selection
{
public:
	DECLARE_CLASS (MixedSelection, Selection)

	MixedSelection ();

	void addType (SelectionContainer* c, bool isDefault = false);
	SelectionContainer* getTypeAt (int index);

	// Selection
	bool isEmpty () const override;
	bool isMultiple () const override;
	bool isSelected (Object* object) const override;
	bool select (Object* object) override;
	bool unselect (Object* object) override;
	bool unselectAll () override;
	bool unselectType (MetaClassRef type) override;
	bool canSelectType (MetaClassRef type) const override;
	MetaClassRef getType () const override;
	int countTypes () const override;
	Iterator* newIterator (int index) const override;
	Iterator* newIterator (MetaClassRef type) const override;
	using Selection::newIterator;
	Object* getFirst () const override;

protected:
	ObjectArray containers;
	SelectionContainer* defaultContainer;

	virtual SelectionContainer* getContainer (Object* object) const;
	SelectionContainer* getContainer (MetaClassRef type) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Selection inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> CCL::Iterator* Selection::newIterator () const
{ return newIterator (ccl_typeid<T> ()); }

//////////////////////////////////////////////////////////////////////////////////////////////////
// Selection::Hideout inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline Selection::Hideout::Hideout (Selection& selection, bool redraw)
: selection (selection),
  redraw (redraw),
  wasSuspended (selection.isShowHideSuspended ())
{
	selection.hide (false);
	selection.setShowHideSuspended (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline Selection::Hideout::~Hideout ()
{
	selection.setShowHideSuspended (wasSuspended);
	selection.show (redraw);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_selection_h
