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
// Filename    : ccl/app/editing/editmodel.h
// Description : Editing Model
//
//************************************************************************************************

#ifndef _ccl_editmodel_h
#define _ccl_editmodel_h

#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/gui/framework/guievent.h"

namespace CCL {

class Container;
class StringList;
class Selection;
class EditView;
class EditLayer;
class EditHandler;
class EditorComponent;
class EditTaskDescription;
interface IDragHandler;
interface IDragSession;

//************************************************************************************************
// EditModel
//************************************************************************************************

class EditModel: public Object
{
public:
	DECLARE_CLASS (EditModel, Object)
	DECLARE_METHOD_NAMES (EditModel)

	EditModel (EditorComponent* component = nullptr);
	~EditModel ();

	 /** Get associated selection. */
	Selection& getSelection ();
	const Selection& getSelection () const;

	/** Check if model contains items for editing. */
	virtual bool containsAnyItems ();

	/** Find item for editing at given location (must be released by caller). */
	virtual Object* findItem (EditView& editView, PointRef where);

	/** Find subpart of item at given location (must be released by caller). */
	virtual Object* findItemPart (EditView& editView, Object* item, PointRef where);

	/** Find item for editing at given location (must be released by caller). Searches deeply first for item parts. */
	virtual Object* findItemDeep (EditView& editView, PointRef where);

	/** Find an item that will be selected after the current selection is deleted (must be released by caller). */
	virtual Object* findItemAfterSelection ();

	/** Identify item. */
	virtual String getItemType (Object* item);

	/** Get size of item. */
	virtual bool getItemSize (Rect& size, EditView& editView, Object* item);

	/** Get tooltip for item. */
	virtual bool getItemTooltip (String& tooltip, EditView& editView, Object* item);

	/** Get name of edit area at given location. */
	virtual String getEditArea (EditView& editView, PointRef where);

	/** Check if model can select the given item. */
	virtual bool canSelectItem (Object* item);

	/** Select an item. */
	virtual bool selectItem (Object* item);

	/** Unselect an item. */
	virtual bool unselectItem (Object* item);

	/** Select items in given area. */
	virtual bool selectItems (EditView& editView, RectRef rect);

	/** Select all items. */
	virtual bool selectAll ();

	/** Invert selection. */
	virtual bool invertSelection ();

	/** Edit given item. */
	virtual bool editItem (Object* item, EditView& editView);

	/** Edit item part. */
	virtual EditHandler* createEditHandler (Object* itemPart, EditView& editView, const MouseEvent& event);

	/** Zoom to given item. */
	virtual bool zoomItem (Object* item, EditView& editView);

	/** Set focus item. */
	virtual bool setFocusItem (Object* item, EditView* editView);

	/** Set anchor item for selecting a range of items. */
	virtual void setAnchorItem (Object* item, EditView* editView);

	/** Get focus item of a given class. */
	virtual Object* getFocusItem (MetaClassRef itemClass) const;
	template<class T> T* getFocusItem () const;

	/** Get size of selection. */
	virtual bool getSelectionSize (Rect& size, EditView& editView);

	/** Drag selection. */
	virtual bool dragSelection (EditView& editView, const MouseEvent& event);
	virtual bool dragSelection (EditView& editView, PointRef where, int inputDevice = 0);

	/** Create drag handler. */
	virtual IDragHandler* createDragHandler (EditView& editView, const DragEvent& event);

	/** Draw selection. */
	virtual EditHandler* drawSelection (EditView& editView, const MouseEvent& event, StringRef hint = nullptr);

	/** Drag an eraser over the view, deleting items under mouse. */
	virtual EditHandler* dragEraser (EditView& editView, const MouseEvent& event);

	/** Direction and mode for navigation. */
	enum Direction      { kNoDirection, kDirectionLeft, kDirectionRight, kDirectionUp, kDirectionDown,
						  kDirectionPageUp, kDirectionPageDown, kDirectionStart, kDirectionEnd };
	enum NavigationMode { kNavigate, kNavigateExtend, kNavigateExtendAdd, kSkip };

	/** Navigate in given direction. */
	virtual bool navigate (Direction direction, NavigationMode mode);

	/** Delete selected items. */
	virtual bool deleteSelected ();

	/** Check if selected items can be deleted. */
	virtual bool canDeleteSelected ();

	/** Delete given item. */
	virtual bool deleteItem (Object* item);

	/** Copy selected items to data object (must be released by caller). */
	virtual Object* copySelected (bool shared, bool forDuplicate = false);

	/** Check if data can be inserted. */
	virtual bool canInsertData (Object* data);

	/** Insert data. */
	virtual bool insertData (Object* data);

	/** Collect all supported edit task categories. */
	virtual void collectTaskCategories (StringList& taskCategories);

	/** Check if edit task can be performed. */
	virtual bool canPerformTask (EditView& editView, const EditTaskDescription& task);

	/** Collect edit task candidates. */
	virtual bool collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task);

	/** Begin transacation. */
	virtual void beginTransaction (StringRef description);

	/** End transaction. */
	virtual void endTransaction (bool cancel = false);

	/** Set document dirty state. */
	virtual void setDocumentDirty ();

	/** Highlight items depending on mouse position */
	virtual void updateHighlightItem (EditView& editView, PointRef where);
	virtual void hideHighlight ();

	/** Edit layers. */
	void addEditLayer (EditLayer* layer);
	EditLayer* getEditLayer (MetaClassRef type) const;
	EditLayer* getEditLayer (StringID name) const;
	EditLayer* getEditLayerForView (EditView* view) const;
	template<class C> C* getEditLayer () const;
	const ObjectList& getEditLayers () const;

	/** Notification when new edit view gets attached. */
	void onViewAttached (EditView* editView);

	bool getSelectionSize (Rect& size, EditView& editView, MetaClassRef type);
	template <class T> bool getSelectionSize (Rect& size, EditView& editView);

	// Object
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	CCL::tbool CCL_API invokeMethod (CCL::Variant& returnValue, CCL::MessageRef msg) override;

protected:
	EditorComponent* component;
	Selection* selection;
	ObjectList editLayers;

	virtual Selection* createSelection ();
	virtual IDragSession* createDragSession (EditView& editView, PointRef where);

	class FocusSetter;
	class SetFocusAction;
};

//************************************************************************************************
// EditModel::FocusSetter
/** Sets the first added item as focus item. */
//************************************************************************************************

class EditModel::FocusSetter
{
public:
	FocusSetter (EditView& editView);
	~FocusSetter ();

	void operator << (Object* item); ///< set an item, the first one will get the focus

private:
	EditView& editView;
	SharedPtr<Object> item;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline EditModel::FocusSetter::FocusSetter (EditView& editView)
: editView (editView), item (nullptr) {}

inline void EditModel::FocusSetter::operator << (Object* _item)
{ if(!item) item = _item; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class C>
C* EditModel::getEditLayer () const
{ return ccl_cast<C> (getEditLayer (ccl_typeid<C> ())); }

//////////////////////////////////////////////////////////////////////////////////////////////////

inline const ObjectList& EditModel::getEditLayers () const
{ return editLayers; }

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class T>
inline T* EditModel::getFocusItem () const
{ return ccl_cast<T> (getFocusItem (ccl_typeid<T> ())); }

//////////////////////////////////////////////////////////////////////////////////////////////////

template <class T> bool EditModel::getSelectionSize (Rect& size, EditView& editView)
{ return getSelectionSize (size, editView, ccl_typeid<T> ());}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_editmodel_h
