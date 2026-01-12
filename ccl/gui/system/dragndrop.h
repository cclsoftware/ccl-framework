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
// Filename    : ccl/gui/system/dragndrop.h
// Description : Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_dragndrop_h
#define _ccl_dragndrop_h

#include "ccl/base/storage/attributes.h"

#include "ccl/public/gui/framework/idragndrop.h"
#include "ccl/public/gui/graphics/types.h"
#include "ccl/public/collections/unknownlist.h"

namespace CCL {

class Image;
class View;
class AutoScroller;
class Container;

//************************************************************************************************
// DragSession
//************************************************************************************************

class DragSession: public Object,
				   public IDragSession
{
public:
	DECLARE_CLASS (DragSession, Object)

	static DragSession* create (IUnknown* source = nullptr, int inputDevice = DragSession::kMouseInput);

	~DragSession ();

	static bool isInternalDragActive (); ///< check for active drag session inside our application (not started by another process)
	static DragSession* getActiveSession (bool target = false);

	void copyFrom (const DragSession&);	///< used when dragged inside our application

	PROPERTY_FLAG (dropResult, kDropCopyShared, isDropCopyShared)
	PROPERTY_FLAG (dropResult, kDropCopyReal, isDropCopyReal)
	PROPERTY_FLAG (dropResult, kDropMove, isDropMove)

	void setCanceled (bool state = true);
	PROPERTY_BOOL (dropped, Dropped) ///< used internally to indicate that a drop is happening

	virtual void onDragFinished (const DragEvent& event);

	void setAutoScrollTarget (View* view);
	void triggerAutoScroll ();
	AutoScroller* getAutoScroller () { return autoScroller; }
	
	// Drag handler
	PROPERTY_SHARED_AUTO (IDragHandler, dragHandler, Handler)
	void leaveDragHandler (const DragEvent& event);
	bool hasVisualFeedback () const;

	// Drag handler provided by source, has own result code
	IDragHandler* getSourceDragHandler () const;
	void setSourceDragHandler (IDragHandler* handler);
	PROPERTY_BOOL (sourceHandlerActive, SourceHandlerActive)
	int getSourceResult () const;
	void setSourceResult (int result);
	int getTotalResult () const;

	// IDragSession
	int CCL_API drag () override;
	IAsyncOperation* CCL_API dragAsync () override;
	void CCL_API setSource (IUnknown* source) override;
	IUnknown* CCL_API getSource () const override;
	void CCL_API setTargetID (StringID targetID) override;
	StringID CCL_API getTargetID () const override;
	void CCL_API setSize (const Rect& size) override;
	const Rect& CCL_API getSize () const override;
	void CCL_API setOffset (const Point& offset) override;
	const Point& CCL_API getOffset () const override;
	tbool CCL_API wasCanceled () const override;
	void CCL_API setDragImage (IImage* image, const Color& backColor) override;
	int CCL_API getResult () const override;
	void CCL_API setResult (int result) override;
	tbool CCL_API getText (String& text) override;
	IUnknownList& CCL_API getItems () override;
	IAttributeList& CCL_API getAttributes () override;
	IDragHandler* CCL_API getDragHandler () const override;
	void CCL_API setDragHandler (IDragHandler* handler) override;
	int CCL_API getInputDevice () const override;
	void CCL_API setInputDevice (int inputDevice) override;

	CLASS_INTERFACE (IDragSession, Object)

	virtual void showNativeDragImage (bool state);
	IImage* getDragImage () const;
	PROPERTY_OBJECT (Point, dragImagePosition, DragImagePosition)

	bool containsNativePaths ();
	bool getNativePaths (Container& urls);

	void setTargetSession (DragSession* session);

protected:
	IUnknown* source;
	DragSession* sourceSession;
	DragSession* targetSession;
	SharedPtr<IDragHandler> sourceDragHandler;
	int sourceResult;
	MutableCString targetID;
	Image* dragImage;
	Color backColor;
	int dropResult;
	bool canceled;
	Rect size;
	Point offset;
	UnknownList items;
	Attributes attributes;
	AutoScroller* autoScroller;
	static DragSession* activeSession;
	int flags;
	int inputDevice;	

	PROPERTY_FLAG (flags, (1<<0), pathsChecked)
	PROPERTY_FLAG (flags, (1<<1), hasNativePaths)
	PROPERTY_FLAG (flags, (1<<2), dragImageVisible)

	struct DragGuard
	{
		DragGuard (DragSession& session);
		~DragGuard ();
	private:
		DragSession* oldSession;
	};

	struct DropGuard
	{
		DropGuard (DragSession& session);
		~DropGuard ();
	private:
		DragSession* oldSession;
	};

	friend class UserInterface;

	DragSession (IUnknown* source = nullptr, int inputDevice = kMouseInput);
	DragSession (int inputDevice);

	void deferDrop (IDragHandler* handler, const DragEvent& dragEvent, View* dragView);
	class DeferredDrop;
};

} // namespace CCL

#endif // _ccl_dragndrop_h
