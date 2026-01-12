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
// Filename    : ccl/app/controls/draghandler.cpp
// Description : Drag Handler
//
//************************************************************************************************

#include "ccl/app/controls/draghandler.h"
#include "ccl/app/controls/usercontrol.h"

#include "ccl/public/storage/iurl.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/ihelpmanager.h"
#include "ccl/public/gui/framework/ipresentable.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// DragHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DragHandler, Object)

bool DragHandler::inChildDragEnter = false;

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandler::DragHandler (IView* view)
: view (view),
  spriteBuilder (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandler::DragHandler (UserControl& control)
: view (control),
  spriteBuilder (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* DragHandler::getView () const
{
	return view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SpriteBuilder& DragHandler::getSpriteBuilder ()
{
	return spriteBuilder;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandler::buildSprite (int options)
{
	sprite = spriteBuilder.createSprite (options);
	setSpriteOffset (spriteBuilder.getDefaultOffset ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandler::replaceSpriteItemText (int index, StringRef text)
{
	if(sprite)
		spriteBuilder.replaceItemText (*sprite, index, text);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const UnknownList& DragHandler::getData () const
{
	return data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandler::addDataItems (const IUnknownList& items, IUnknown* context)
{
	ForEachUnknown (items, item)
		if(IUnknown* preparedItem = prepareDataItem (*item, context))
			data.add (preparedItem, false);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* DragHandler::prepareDataItem (IUnknown& item, IUnknown* context)
{
	item.retain ();
	return &item;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragHandler::prepare (const IUnknownList& items, IDragSession* session)
{
	addDataItems (items, session ? session->getSource () : nullptr);
	finishPrepare ();
	buildSprite ();

	return !data.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragHandler::prepareFolderContent (IUnknown& item, IUnknown* context, int maxFiles)
{
	UnknownPtr<IUrl> folder (&item);
	if(folder && folder->isFolder ()) 
	{
		// if a folder is dragged, try to prepare items from files in the folder
		int numFiles = 0;
		ForEachFile (System::GetFileSystem ().newIterator (*folder), path)
			if(path->isFile ())
			{
				
				AutoPtr<IUrl> releaser;
				IUrl* newPath = nullptr;
				path->clone (newPath);
				releaser = newPath;
				if(IUnknown* fileItem = prepareDataItem (*newPath, context))
				{
					data.add (fileItem, false);
					numFiles++;
					if(numFiles >= maxFiles)
						break;
				}
			}
		EndFor
		return numFiles > 0;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragHandler::getHelp (IHelpInfoBuilder& helpInfo)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandler::updateHelp ()
{
	if(System::GetHelpManager ().hasInfoViewers ())
	{
		AutoPtr<IHelpInfoBuilder> builder (ccl_new<IHelpInfoBuilder> (ClassID::HelpInfoBuilder));
		if(builder)
			if(getHelp (*builder))
				System::GetHelpManager ().showInfo (UnknownPtr<IPresentable> (builder));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandler::dragEnter (const DragEvent& event)
{
	// try to get help and show it
	if(!inChildDragEnter)
		updateHelp ();

	if(childDragHandler)
	{
		ScopedVar<bool> guard (inChildDragEnter, true);
		DragEvent e2 (event);
		e2.where += childOffset;
		childDragHandler->dragEnter (event);
	}

	if(event.session.getInputDevice () == IDragSession::kTouchInput)
		setSpriteOffset (spriteBuilder.getTouchOffset ());

	return AbstractDragHandler::dragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandler::dragOver (const DragEvent& event)
{
	if(childDragHandler)
	{
		DragEvent e2 (event);
		e2.where += childOffset;
		childDragHandler->dragOver (e2);
	}
	return AbstractDragHandler::dragOver (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandler::dragLeave (const DragEvent& event)
{
	System::GetHelpManager ().showInfo (nullptr);

	if(childDragHandler)
	{
		DragEvent e2 (event);
		e2.where += childOffset;
		childDragHandler->dragLeave (event);
	}
	return AbstractDragHandler::dragLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandler::drop (const DragEvent& event)
{
	System::GetHelpManager ().showInfo (nullptr);

	if(childDragHandler)
	{
		DragEvent e2 (event);
		e2.where += childOffset;
		childDragHandler->drop (event);
	}
	return AbstractDragHandler::drop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandler::afterDrop (const DragEvent& event)
{
	if(childDragHandler)
	{
		DragEvent e2 (event);
		e2.where += childOffset;
		childDragHandler->afterDrop (event);
	}
	return AbstractDragHandler::afterDrop (event);
}

//************************************************************************************************
// NullDragHandler
//************************************************************************************************

DEFINE_CLASS_HIDDEN (NullDragHandler, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

void NullDragHandler::attachToSession (IDragSession* session, IView* view)
{
	if(session)
	{
		AutoPtr<DragHandler> nullHandler = NEW NullDragHandler (view);
		session->setDragHandler (nullHandler);
		session->setResult (IDragSession::kDropNone);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

NullDragHandler::NullDragHandler (IView* view)
: DragHandler (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

NullDragHandler::NullDragHandler (UserControl& control)
: DragHandler (control)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NullDragHandler::dragEnter (const DragEvent& event)
{
	return dragOver (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NullDragHandler::dragOver (const DragEvent& event)
{ 
	event.session.setResult (IDragSession::kDropNone);
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NullDragHandler::dragLeave (const DragEvent& event)
{
	return true; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API NullDragHandler::drop (const DragEvent& event)
{ 
	return true; 
}

/////////////////////////////////////////////////////////////////////////////////////////////////

inline tbool CCL_API NullDragHandler::isNullHandler () const
{
	return true;
}

//************************************************************************************************
// DragFeedback
//************************************************************************************************

DragFeedback::DragFeedback (IDragFeedbackProvider* provider, IView* view)
: DragHandler (view),
  feedbackProvider (provider)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragFeedback::DragFeedback (IDragFeedbackProvider* provider, UserControl& control)
: DragHandler (control),
  feedbackProvider (provider)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragFeedback::dragOver (const DragEvent& event)
{
	String text (feedbackProvider->getFeedbackString (event));

	if(sprite)
		spriteBuilder.replaceItemText (*sprite, 0, text);
	else
	{
		spriteBuilder.addHeader (text);
		buildSprite ();
	}
	return DragHandler::dragOver (event);
}

//************************************************************************************************
// DragHandlerVariant
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DragHandlerVariant, DragHandler)

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandlerVariant::DragHandlerVariant (UserControl& control)
: DragHandler (control),
  blockSourceDragHandler (false),
  hasFeedback (false)
{
	handlerItems.objectCleanup ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API DragHandlerVariant::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<ISourceDragBlocker> ())
	{
		// 1.) explicitely blocked
		if(isBlockSourceDragHandler ())
			QUERY_INTERFACE (ISourceDragBlocker)

		// 2.) currently selected handler can block
		if(childDragHandler)
			return childDragHandler->queryInterface (iid, ptr);
	}

	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandlerVariant::addDragHandler (DragHandler* handler, int modifiers, StringRef helpText)
{
	handlerItems.add (NEW HandlerItem (handler, modifiers, helpText));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DragHandler* DragHandlerVariant::selectDragHandler (const DragEvent& event)
{
	HandlerItem* defaultItem = nullptr;

	ArrayForEachFast (handlerItems, HandlerItem, item)
		int itemModifiers = item->getModifiers ();
		if(itemModifiers != 0)
		{
			if((event.keys.getModifiers () & itemModifiers) == itemModifiers) // all specified modifiers must be pressed
				return item->getHandler ();
		}
		else if(!defaultItem)
			defaultItem = item;
	EndFor

	return defaultItem ? defaultItem->getHandler () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DragHandlerVariant::updateHandler (const DragEvent& event)
{
	IDragHandler* selectedhandler = selectDragHandler (event);
	if(selectedhandler != childDragHandler)
	{
		if(childDragHandler)
			SuperClass::dragLeave (event);

		hasFeedback = false;
		setChildDragHandler (return_shared (selectedhandler));

		if(childDragHandler)
		{
			SuperClass::dragEnter (event);
			hasFeedback = childDragHandler && childDragHandler->hasVisualFeedback ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DragHandlerVariant::getHelp (IHelpInfoBuilder& helpInfo)
{
	SuperClass::getHelp (helpInfo);

	HandlerItem* defaultItem = nullptr;
	ArrayForEachFast (handlerItems, HandlerItem, item)
		if(item->getModifiers () == 0)
		{
			if(defaultItem)
				continue;
			else
				defaultItem = item;
		}
		if(!item->getHelpText ().isEmpty ())
			helpInfo.addOption (item->getModifiers (), nullptr, item->getHelpText ());
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandlerVariant::hasVisualFeedback () const
{
	return hasFeedback;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandlerVariant::dragEnter (const DragEvent& event)
{
	updateHandler (event);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DragHandlerVariant::dragOver (const DragEvent& event)
{
	updateHandler (event);
	return SuperClass::dragOver (event);
}
