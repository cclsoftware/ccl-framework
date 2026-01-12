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
// Filename    : ccl/app/editing/editlayer.h
// Description : Edit Layer
//
//************************************************************************************************

#ifndef _ccl_editlayer_h
#define _ccl_editlayer_h

#include "ccl/app/editing/editmodel.h"

#include "ccl/public/text/cstring.h"

namespace CCL {

class EditorComponent;
class EditModel;
class EditView;
class Selection;
interface IDragHandler;

//************************************************************************************************
// EditLayer
//** An edit layer can be plugged into an edit model as an additional editing aspect. */
//************************************************************************************************

class EditLayer: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (EditLayer, Object)

	EditLayer (EditorComponent* editor, StringID name);

	PROPERTY_MUTABLE_CSTRING (name, Name)

	EditModel& getModel () const;
	Selection& getSelection () const;
	EditorComponent* getEditor () const;

	virtual bool handlesView (EditView* editView);
	virtual void onSelectionCreated (Selection& selection);

	/// delegated from EditModel:
	virtual bool containsAnyItems ();
	virtual Object* findItem (EditView& editView, PointRef where);
	virtual Object* findItemPart (EditView& editView, Object* item, PointRef where);
	virtual Object* findItemDeep (EditView& editView, PointRef where);
	virtual Object* findItemAfterSelection ();
	virtual String getItemType (Object* item);
	virtual bool getItemSize (Rect& size, EditView& editView, Object* item);
	virtual bool getItemTooltip (String& tooltip, EditView& editView, Object* item);
	virtual String getEditArea (EditView& editView, PointRef where);
	virtual bool canSelectItem (Object* item);
	virtual bool selectItems (EditView& editView, RectRef rect); ///< return true to ignore other layers
	virtual bool selectAll ();
	virtual bool setFocusItem (Object* item, EditView* editView);
	virtual bool editItem (Object* item, EditView& editView);
	virtual bool navigate (EditModel::Direction direction, EditModel::NavigationMode mode);
	virtual bool deleteSelected ();
	virtual bool deleteItem (Object* item);
	virtual Object* copySelected (bool shared);
	virtual bool canInsertData (Object* data);
	virtual bool insertData (Object* data);
	virtual IDragHandler* createDragHandler (EditView& editView, const DragEvent& event);
	virtual void updateHighlightItem (EditView& editView, PointRef where);
	virtual void hideHighlight ();

protected:
	EditorComponent* component;
};

} // namespace CCL

#endif // _ccl_editlayer_h
