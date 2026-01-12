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
// Filename    : ccl/app/editing/editmodel.cpp
// Description : Editing Model
//
//************************************************************************************************

#include "ccl/app/editing/editmodel.h"
#include "ccl/app/editing/editor.h"
#include "ccl/app/editing/editlayer.h"
#include "ccl/app/editing/editview.h"
#include "ccl/app/editing/editdraghandler.h"
#include "ccl/app/editing/edithandler.h"
#include "ccl/app/editing/editextension.h"
#include "ccl/app/editing/tasks/edittaskhandler.h"
#include "ccl/app/actions/action.h"
#include "ccl/app/utilities/boxedguitypes.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/app/signals.h"
#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/collections/unknownlist.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// EditTaskDragHandler
//************************************************************************************************

class EditTaskDragHandler: public EditDragHandler
{
public:
	EditTaskDragHandler (EditView& editView)
	: EditDragHandler (editView)
	{}

	// EditDragHandler
	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		AutoPtr<EditTaskHandler> handler = EditTaskHandler::createTask (event.session.getItems ().getFirst ());
		if(handler)
		{
			AutoPtr<Object> mouseItem (getModel ().findItem (getEditView (), event.where));
			if(mouseItem)
			{
				// select mouse item exclusively, if not already selected
				if(!getModel ().getSelection ().isSelected (mouseItem))
				{
					Selection::Hideout hideout (getModel ().getSelection ());
					getModel ().getSelection ().unselectAll ();
					getModel ().selectItem (mouseItem);
				}
			}

			ObjectList candidates;
			candidates.objectCleanup (true);
			if(getModel ().collectTaskCandidates (candidates, getEditView (), handler->getDescription ()))
			{
				handler->runTask (candidates, &getEditView ());
				return true;
			}
		}
		return false;
	}
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// EditModel::FocusSetter
//************************************************************************************************

EditModel::FocusSetter::~FocusSetter ()
{
	if(item)
		editView.getModel ().setFocusItem (item, &editView);
}

//************************************************************************************************
// EditModel
//************************************************************************************************

DEFINE_CLASS_HIDDEN (EditModel, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel::EditModel (EditorComponent* component)
: component (component),
  selection (nullptr)
{
	ASSERT (component != nullptr)
	editLayers.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditModel::~EditModel ()
{
	signal (Message (kDestroyed));

	if(selection)
	{
		ASSERT (component != nullptr)
		if(component)
			selection->removeViewer (component);

		selection->release ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::onViewAttached (EditView* editView)
{
	// layers decide if they want to be assigned to the view
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->handlesView (editView))
			editView->addEditLayer (editLayer);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection* EditModel::createSelection ()
{
	return NEW SimpleSelection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Selection& EditModel::getSelection ()
{
	if(selection == nullptr)
	{
		selection = createSelection ();

		ForEach (editLayers, EditLayer, editLayer)
			editLayer->onSelectionCreated (* selection);
		EndFor

		ASSERT (component != nullptr)
		if(component)
			selection->addViewer (component);
	}
	return *selection;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Selection& EditModel::getSelection () const
{
	return ccl_const_cast (this)->getSelection ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::addEditLayer (EditLayer* layer)
{
	editLayers.add (layer);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditLayer* EditModel::getEditLayer (MetaClassRef type) const
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->canCast (type))
			return editLayer;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditLayer* EditModel::getEditLayer (StringID name) const
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->getName () == name)
			return editLayer;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditLayer* EditModel::getEditLayerForView (EditView* view) const
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->handlesView (view))
			return editLayer;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::containsAnyItems ()
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->containsAnyItems ())
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::findItem (EditView& editView, PointRef where)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(Object* item = editLayer->findItem (editView, where))
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::findItemPart (EditView& editView, Object* item, PointRef where)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::findItemDeep (EditView& editView, PointRef where)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(Object* item = editLayer->findItemDeep (editView, where))
			return item;
	EndFor

	if(Object* item = findItem (editView, where))
	{
		if(Object* part = findItemPart (editView, item, where))
		{
			item->release ();
			return part;
		}
		return item;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::findItemAfterSelection ()
{
	ForEach (editLayers, EditLayer, editLayer)
		if(Object* item = editLayer->findItemAfterSelection ())
			return item;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditModel::getItemType (Object* item)
{
	ForEach (editLayers, EditLayer, editLayer)
		String str = editLayer->getItemType (item);
		if(!str.isEmpty ())
			return str;
	EndFor
	return String ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String EditModel::getEditArea (EditView& editView, PointRef where)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		String str = editLayer->getEditArea (editView, where);
		if(!str.isEmpty ())
			return str;
	EndFor
	return String::kEmpty;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::getItemSize (Rect& size, EditView& editView, Object* item)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(editLayer->getItemSize (size, editView, item))
			return true;
	EndFor

	CCL_PRINT ("EditModel::getItemSize: object not handled.")
	size.setEmpty ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::getItemTooltip (String& tooltip, EditView& editView, Object* item)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(editLayer->getItemTooltip (tooltip, editView, item))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::canSelectItem (Object* item)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->canSelectItem (item))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::selectItem (Object* item)
{
	Selection& selection = getSelection ();

	selection.hide (false);
	bool result = selection.select (item);
	selection.show (true);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::unselectItem (Object* item)
{
	Selection& selection = getSelection ();

	selection.hide (false);
	bool result = selection.unselect (item);
	selection.show (true);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::selectItems (EditView& editView, RectRef rect)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(editLayer->selectItems (editView, rect))
			return true;
	EndFor
	return false;
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::selectAll ()
{
	bool result = false;
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->selectAll ())
			result = true;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::invertSelection ()
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::editItem (Object* item, EditView& editView)
{
	ForEach (editView.getEditLayers (), EditLayer, editLayer)
		if(editLayer->editItem (item, editView))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditModel::createEditHandler (Object* itemPart, EditView& editView, const MouseEvent& event)
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::zoomItem (Object* item, EditView& editView)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::getFocusItem (MetaClassRef itemClass) const
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::setFocusItem (Object* item, EditView* editView)
{
	if(editView)
		ForEach (editView->getEditLayers (), EditLayer, editLayer)
			if(editLayer->setFocusItem (item, editView))
				return true;
		EndFor
	else
		ForEach (editLayers, EditLayer, editLayer)
			if(editLayer->setFocusItem (item, nullptr))
				return true;
		EndFor

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::setAnchorItem (Object* item, EditView* editView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::getSelectionSize (Rect& size, EditView& editView)
{
	size.setReallyEmpty ();

	Selection& selection = getSelection ();
	for(int i = 0; i < selection.countTypes (); i++)
	{
		IterForEach (selection.newIterator (i), Object, item)
			Rect r;
			if(getItemSize (r, editView, item))
				size.join (r);
		EndFor
	}
	return !size.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::getSelectionSize (Rect& size, EditView& editView, MetaClassRef type)
{
	Selection& selection = getSelection ();
	IterForEach (selection.newIterator (type), Object, item)
		Rect r;
		if(getItemSize (r, editView, item))
			size.join (r);
	EndFor

	return !size.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragSession* EditModel::createDragSession (EditView& editView, PointRef where)
{
	IDragSession* session = ccl_new<IDragSession> (ClassID::DragSession);
	session->setSource (editView.asUnknown ());
	ASSERT (session != nullptr)
	session->getItems ().add (getSelection ().asUnknown (), true);

	Rect rect;
	getSelectionSize (rect, editView);

	if(rect.isEmpty () && rect.left == kMaxCoord && rect.top == kMaxCoord) // "really empty"
		rect.setEmpty ();

	// hmm...should this be done here???
	session->setSize (rect);

	Point offset;
	offset.x = where.x - rect.left;
	offset.y = where.y - rect.top;
	session->setOffset (offset);
	return session;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::dragSelection (EditView& editView, const MouseEvent& event)
{
	int inputDevice = event.wasTouchEvent () ? IDragSession::kTouchInput : IDragSession::kMouseInput;
	return dragSelection (editView, event.where, inputDevice);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::dragSelection (EditView& editView, PointRef where, int inputDevice)
{	
	AutoPtr<IDragSession> session = createDragSession (editView, where);
	if(session)
	{
		session->setInputDevice (inputDevice);
		session->drag ();
	}
	return session != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* EditModel::createDragHandler (EditView& editView, const DragEvent& event)
{
	// allow drop of edit tasks...
	EditTaskDescription description;
	if(EditTaskHandler::canCreateTask (description, event.session.getItems ().getFirst ()))
	{
		if(canPerformTask (editView, description))
		{
			event.session.setResult (IDragSession::kDropMove); // hmm???
			return NEW EditTaskDragHandler (editView);
		}
	}

	ForEach (editLayers, EditLayer, editLayer)
		if(IDragHandler* handler = editLayer->createDragHandler (editView, event))
			return handler;
	EndFor

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditModel::drawSelection (EditView& editView, const MouseEvent& event, StringRef hint)
{
	return NEW DrawSelectionHandler (&editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EditHandler* EditModel::dragEraser (EditView& editView, const MouseEvent& event)
{
	return NEW DeleteEditHandler (&editView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::navigate (Direction direction, NavigationMode mode)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->navigate (direction, mode))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::deleteSelected ()
{
	bool result = false;
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->deleteSelected ())
			result = true;
	EndFor
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::canDeleteSelected ()
{
	return !getSelection ().isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::deleteItem (Object* item)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->deleteItem (item))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Object* EditModel::copySelected (bool shared, bool forDuplicate)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(Object* object = editLayer->copySelected (shared))
			return object;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::canInsertData (Object* data)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->canInsertData (data))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::insertData (Object* data)
{
	ForEach (editLayers, EditLayer, editLayer)
		if(editLayer->insertData (data))
			return true;
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::collectTaskCategories (StringList& taskCategories)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::canPerformTask (EditView& editView, const EditTaskDescription& task)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool EditModel::collectTaskCandidates (Container& resultList, EditView& editView, const EditTaskDescription& task)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::beginTransaction (StringRef description)
{
	CCL_NOT_IMPL ("EditModel::beginTransaction")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::endTransaction (bool cancel)
{
	CCL_NOT_IMPL ("EditModel::endTransaction")
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::setDocumentDirty ()
{
	SignalSource (Signals::kDocumentManager).signal (Message (Signals::kDocumentDirty));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::updateHighlightItem (EditView& editView, PointRef where)
{
	for(EditLayer* editLayer : iterate_as<EditLayer> (editLayers))
		editLayer->updateHighlightItem (editView, where);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EditModel::hideHighlight ()
{
	for(EditLayer* editLayer : iterate_as<EditLayer> (editLayers))
		editLayer->hideHighlight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditModel::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "extensions")
	{
		var = ccl_as_unknown (EditExtensionRegistry::instance ());
		return true;
	}
/*	if(propertyId == "layers")
	{
		// todo: container class or accessor that has elements of container as properties (via toString)
		var = const_cast<ObjectList&> (editLayers);
		return true;
	}
	// instead:
	*/

	if(EditLayer* editLayer = getEditLayer (propertyId))
	{
		var = ccl_as_unknown (editLayer);
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (EditModel)
	DEFINE_METHOD_NAME ("updateHighlightItem")
	DEFINE_METHOD_NAME ("hideHighlight")
	DEFINE_METHOD_NAME ("setDocumentDirty")
END_METHOD_NAMES (EditModel)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API EditModel::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "updateHighlightItem")
	{
		EditView* editView = msg.getArgCount () > 0 ? unknown_cast<EditView> (msg[0]) : nullptr;
		Boxed::Point* where = msg.getArgCount () > 1 ? unknown_cast<Boxed::Point> (msg[1]) : nullptr;

		ASSERT (editView && where)
		if(editView && where)
			updateHighlightItem (*editView, *where);

		return true;
	}
	else if(msg == "hideHighlight")
	{
		hideHighlight ();
		return true;
	}
	else if(msg == "setDocumentDirty")
	{
		setDocumentDirty ();
		return true;
	}
	return SuperClass::invokeMethod (returnValue, msg);
}
