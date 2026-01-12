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
// Filename    : ccl/app/controls/draghandler.h
// Description : Drag Handler
//
//************************************************************************************************

#ifndef _ccl_draghandler_h
#define _ccl_draghandler_h

#include "ccl/public/gui/idatatarget.h"
#include "ccl/public/gui/framework/abstractdraghandler.h"

#include "ccl/base/collections/objectarray.h"

#include "ccl/app/controls/spritebuilder.h"

namespace CCL {

class UserControl;
interface IHelpInfoBuilder;

//********************************************************************************************
// DragHandler
/** Has a SpriteBuilder that can be used to create a sprite for visual feedback.
	Has a list of objects that might be created for feedback in "dragEnter" and reused in "drop". */
//********************************************************************************************

class DragHandler: public Object,
				   public AbstractDragHandler
{
public:
	DECLARE_CLASS (DragHandler, Object)

	DragHandler (IView* view = nullptr);
	DragHandler (UserControl& control);

	// View
	IView* getView () const;

	// Sprite
	SpriteBuilder& getSpriteBuilder ();
	void buildSprite (int options = 0);
	void replaceSpriteItemText (int index, StringRef text);

	// Data items
	const UnknownList& getData () const;
	virtual void addDataItems (const IUnknownList& items, IUnknown* context);
	virtual bool prepare (const IUnknownList& items, IDragSession* session = nullptr); ///< adds items, builds sprite; returns true if objects were added
	virtual void postProcessData () {} ///< called when data get's passed to an IDataTarget; a chance to finalize things that you don't want in the prepare phase (e.g. creating files)

	// Child DragHandler
	PROPERTY_AUTO_POINTER (IDragHandler, childDragHandler, ChildDragHandler)
	PROPERTY_OBJECT (Point, childOffset, ChildOffset) // if child dragHandler has a different view: offset from our view to the child's view

	// provide help info; called in dragEnter
	virtual bool getHelp (IHelpInfoBuilder& helpInfo);

	// IDragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API dragLeave (const DragEvent& event) override;
	tbool CCL_API drop (const DragEvent& event) override;
	tbool CCL_API afterDrop (const DragEvent& event) override;

	CLASS_INTERFACE (IDragHandler, Object)

protected:
	IView* view;
	SpriteBuilder spriteBuilder;
	UnknownList data;
	static bool inChildDragEnter;

	void updateHelp ();

	/// if item is a folder url: tries to prepare files from that folder
	bool prepareFolderContent (IUnknown& item, IUnknown* context, int maxFiles);

	/// called by addDataItems for each item; returned objects are added to the data list
	virtual IUnknown* prepareDataItem (IUnknown& item, IUnknown* context);
	
	/// called after all items have been added (before sprite is created)
	virtual void finishPrepare () {}
};

//************************************************************************************************
// NullDragHandler
//************************************************************************************************

class NullDragHandler: public DragHandler
{
public:
	DECLARE_CLASS (NullDragHandler, DragHandler)

	static void attachToSession (IDragSession* session, IView* view);

	NullDragHandler (IView* view = nullptr);
	NullDragHandler (UserControl& control);

	// DragHandler
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;
	tbool CCL_API dragLeave (const DragEvent& event) override;
	tbool CCL_API drop (const DragEvent& event) override;
	tbool CCL_API isNullHandler () const override;
};

//************************************************************************************************
// DragHandlerDelegate
/** Delegates drop to IDataTarget. */
//************************************************************************************************

template <class BaseHandler>
class DragHandlerDelegate: public BaseHandler
{
public:
	DragHandlerDelegate (IView* view = nullptr, IDataTarget* dataTarget = nullptr)
	: BaseHandler (view),
	  dataTarget (dataTarget)
	{}

	PROPERTY_POINTER (IDataTarget, dataTarget, DataTarget)

	// BaseHandler
	tbool CCL_API afterDrop (const DragEvent& event) override
	{
		BaseHandler::afterDrop (event);
		ASSERT (dataTarget)
		const IUnknownList* data = &BaseHandler::getData ();
		if(data->isEmpty ())
			data = &event.session.getItems (); // BaseHandler might not collect data 
		return dataTarget ? dataTarget->insertData (*data, &event.session) : false;
	}
};

//************************************************************************************************
// DragFeedback
/** Simple DragHandler implementation that only displays a text as feedback.
	The text is provided by an IDragFeedbackProvider. */
//************************************************************************************************

class DragFeedback: public DragHandler
{
public:
	DragFeedback (IDragFeedbackProvider* provider, IView* view = nullptr);
	DragFeedback (IDragFeedbackProvider* provider, UserControl& control);

	PROPERTY_POINTER (IDragFeedbackProvider, feedbackProvider, FeedbackProvider)

	// DragHandler
	tbool CCL_API dragOver (const DragEvent& event) override;
};

//************************************************************************************************
// DragHandlerVariant
//************************************************************************************************

class DragHandlerVariant: public DragHandler,
						  public ISourceDragBlocker
{
public:
	DECLARE_CLASS_ABSTRACT (DragHandlerVariant, DragHandler)

	DragHandlerVariant (UserControl& control);

	void addDragHandler (DragHandler* handler, int modifiers = 0, StringRef helpText = nullptr); ///< takes ownership of handler

	PROPERTY_BOOL (blockSourceDragHandler, BlockSourceDragHandler)

	// DragHandler
	bool getHelp (CCL::IHelpInfoBuilder& helpInfo) override;
	tbool CCL_API hasVisualFeedback () const override;
	tbool CCL_API dragEnter (const DragEvent& event) override;
	tbool CCL_API dragOver (const DragEvent& event) override;

	CLASS_INTERFACES (DragHandler)

protected:
	ObjectArray handlerItems;
	tbool hasFeedback;

	struct HandlerItem: public Object
	{
		PROPERTY_AUTO_POINTER (DragHandler, handler, Handler)
		PROPERTY_VARIABLE (int, modifiers, Modifiers)
		PROPERTY_STRING (helpText, HelpText)

		HandlerItem (DragHandler* handler, int modifiers, StringRef helpText)
		: handler (handler), modifiers (modifiers), helpText (helpText)
		{}
	};

	void updateHandler (const DragEvent& event);
	DragHandler* getDragHandler (int index) const;
	virtual DragHandler* selectDragHandler (const DragEvent& event); ///< implement to return one of the dragHandlers
};

//************************************************************************************************
// DragDataExtractor
/** Helps extracting data items from a drag handler. */
//************************************************************************************************

class DragDataExtractor
{
public:
	/// use data from drag session's handler or create a new TDragHandler instance
    template<class TDragHandler> void construct (const IUnknownList& data, IDragSession* session);
	template<class TDragHandler, class Arg> void construct (const IUnknownList& data, IDragSession* session, Arg arg);

	// access data
	tbool isEmpty ();
	IUnknown* getFirstItem ();
	template<class IFace> IFace* getFirstItem ();
	const IUnknownList* getData ();
    IUnknownIterator* createIterator ();

private:
	AutoPtr<DragHandler> dragHandler;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// DragDataExtractor inline
//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TDragHandler>
inline void DragDataExtractor::construct (const IUnknownList& data, IDragSession* session)
{
	IUnknown* context = nullptr;

	// try drag handler from session
	if(session)
	{
		context = session->getSource ();

		if((dragHandler = unknown_cast<TDragHandler> (session->getDragHandler ())))
			dragHandler->retain ();
	}

	if(!dragHandler)
	{
		// create new handler and feed data
		dragHandler = NEW TDragHandler;
		dragHandler->addDataItems (data, context);
	}
	dragHandler->postProcessData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<class TDragHandler, class Arg>
inline void DragDataExtractor::construct (const IUnknownList& data, IDragSession* session, Arg arg)
{
	IUnknown* context = nullptr;

	// try drag handler from session
	if(session)
	{
		context = session->getSource ();

		if((dragHandler = unknown_cast<TDragHandler> (session->getDragHandler ())))
			dragHandler->retain ();
	}

	if(!dragHandler)
	{
		// create new handler and feed data
		dragHandler = NEW TDragHandler (0, arg);
		dragHandler->addDataItems (data, context);
	}
	dragHandler->postProcessData ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

inline DragHandler* DragHandlerVariant::getDragHandler (int index) const
{ HandlerItem* item = (HandlerItem*)handlerItems.at (index); return item ? item->getHandler () : nullptr; }

inline tbool DragDataExtractor::isEmpty ()
{ return dragHandler ? dragHandler->getData ().isEmpty () : true; }

inline IUnknown* DragDataExtractor::getFirstItem ()
{ return dragHandler ? dragHandler->getData ().getFirst () : nullptr; }

template<class IFace> inline IFace* DragDataExtractor::getFirstItem ()
{ UnknownPtr<IFace> iface (getFirstItem ()); return iface; }

inline const IUnknownList* DragDataExtractor::getData ()
{ return dragHandler ? &dragHandler->getData () : nullptr; }

inline IUnknownIterator* DragDataExtractor::createIterator ()
{ return dragHandler ? dragHandler->getData ().createIterator () : nullptr; }

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_draghandler_h
