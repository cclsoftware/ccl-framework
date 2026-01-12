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
// Filename    : ccl/public/gui/framework/idragndrop.h
// Description : Drag-and-Drop Interfaces
//
//************************************************************************************************

#ifndef _ccl_idragndrop_h
#define _ccl_idragndrop_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/gui/framework/guievent.h"
#include "ccl/public/gui/graphics/rect.h"
#include "ccl/public/gui/graphics/color.h"

namespace CCL {

interface IUnknownList;
interface IAttributeList;
interface IAsyncOperation;
interface IDragHandler;
interface IImage;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Built-in classes
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace ClassID
{
	DEFINE_CID (DragSession, 0x5447ed24, 0x42cf, 0x43ed, 0x8a, 0x5b, 0xa9, 0x56, 0x4b, 0x93, 0xea, 0x5f);
}

//************************************************************************************************
// Drag and Drop definitions
//************************************************************************************************

static const int kCopySharedModifer = KeyState::kCommand;
static const int kCopyRealModifer = KeyState::kOption;

DEFINE_STRINGID (kTrashBinTargetID, "TrashBin")

//************************************************************************************************
// IDragSession
/** Represents a "Drag and Drop" session.
	\ingroup gui_data */
//************************************************************************************************

interface IDragSession: IUnknown
{
	/** Drop results. */
	enum DropResults
	{
		kDropNone = 0,			///< no effect
		kDropCopyShared = 1<<0,	///< drop causes a copy that share the data
		kDropCopyReal = 1<<1,	///< drop causes a complete copy
		kDropMove = 1<<2		///< drop causes move
	};

	/** Input device used for dragging. */
	enum InputDevice
	{
		kMouseInput = 0,
		kTouchInput
	};

	/** Perform drag session. */
	virtual int CCL_API drag () = 0;

	/** Perform drag session asynchronously. */
	virtual IAsyncOperation* CCL_API dragAsync () = 0;

	/** Set source of drag session. */
	virtual void CCL_API setSource (IUnknown* source) = 0;

	/** Get source of drag session. */
	virtual IUnknown* CCL_API getSource () const = 0;

	/** Set target identifier. */
	virtual void CCL_API setTargetID (StringID targetID) = 0;

	/** Get target identifier. */
	virtual StringID CCL_API getTargetID () const = 0;

	/** Set size of dragged items. */
	virtual void CCL_API setSize (const Rect& size) = 0;

	/** Get size of dragged items. */
	virtual const Rect& CCL_API getSize () const = 0;

	/** Set offset into drag area. */
	virtual void CCL_API setOffset (const Point& offset) = 0;

	/** Get offset into drag area. */
	virtual const Point& CCL_API getOffset () const = 0;

	/** Check if dragging was canceled (e.g. by pressing Escape). */
	virtual tbool CCL_API wasCanceled () const = 0;

	/** Assign image to drag operation (if supported by OS). */
	virtual void CCL_API setDragImage (IImage* image, const Color& backColor) = 0;

	/** Get drop result code. */
	virtual int CCL_API getResult () const = 0;

	/** Set drop result code. */
	virtual void CCL_API setResult (int result) = 0;

	/** Dragged text (instead of items). */
	virtual tbool CCL_API getText (String& text) = 0;

	/** List of dragged items. */
	virtual IUnknownList& CCL_API getItems () = 0;

	/** Get attributes for transferring context information from source to target. */
	virtual IAttributeList& CCL_API getAttributes () = 0;

	/** Get current drag handler. */
	virtual IDragHandler* CCL_API getDragHandler () const = 0;

	/** Assign drag handler, shared by session. */
	virtual void CCL_API setDragHandler (IDragHandler* handler) = 0;
	
	/** Get input device used for dragging. */
	virtual int CCL_API getInputDevice () const = 0;

	/** Set input device used for dragging. */
	virtual void CCL_API setInputDevice (int inputDevice) = 0;

	DECLARE_IID (IDragSession)
};

DEFINE_IID (IDragSession, 0x243933ee, 0xb13a, 0x4053, 0x98, 0x64, 0x33, 0x2f, 0x58, 0x61, 0xb0, 0x82)

//************************************************************************************************
// IDragHandler
/** Interface for handling a drag session over a view.
	\ingroup gui_data */
//************************************************************************************************

interface IDragHandler: IUnknown
{
	virtual tbool CCL_API dragEnter (const DragEvent& event) = 0;
	
	virtual tbool CCL_API dragOver (const DragEvent& event) = 0;
	
	virtual tbool CCL_API dragLeave (const DragEvent& event) = 0;
	
	virtual tbool CCL_API drop (const DragEvent& event) = 0;
	
	virtual tbool CCL_API afterDrop (const DragEvent& event) = 0;

	/** Check if the handler provides any visual feedback. */
	virtual tbool CCL_API hasVisualFeedback () const = 0;

	/** Check if the handler was only a created to prevent parent views from receiving drag events over this view. */
	virtual tbool CCL_API isNullHandler () const = 0;

	virtual tbool CCL_API wantsAutoScroll () const = 0;

	DECLARE_IID (IDragHandler)
};

DEFINE_IID (IDragHandler, 0xDB4F071B, 0xFA35, 0x4226, 0xB5, 0xD4, 0x2C, 0xF0, 0xED, 0xC4, 0xC5, 0x7D)

//************************************************************************************************
// IDragFeedbackProvider
/** Simplified interface for providing a text as drag feedback.
	getFeedbackString is called on every mouse move and keypress. 
	\ingroup gui_data */
//************************************************************************************************

interface IDragFeedbackProvider: IUnknown
{
	virtual String getFeedbackString (const DragEvent& event) = 0;

	DECLARE_IID (IDragFeedbackProvider)
};

DEFINE_IID (IDragFeedbackProvider, 0xd6d2d6ee, 0xd6da, 0x434c, 0xa7, 0x2, 0x82, 0xd0, 0x78, 0x0, 0x23, 0xf4)

//************************************************************************************************
// ISourceDragBlocker
/** A drag handler implementing this interface "blocks" a competing drag handler from the source side. */
//************************************************************************************************

interface ISourceDragBlocker: IUnknown
{
	DECLARE_IID (ISourceDragBlocker)
};

DEFINE_IID (ISourceDragBlocker, 0x33199110, 0x2d59, 0x4de8, 0x88, 0xb, 0xba, 0xc0, 0x73, 0x75, 0xdc, 0x3b)

} // namespace CCL

#endif // _ccl_idragndrop_h
