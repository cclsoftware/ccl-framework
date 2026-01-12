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
// Filename    : ccl/platform/linux/gui/dragndrop.linux.h
// Description : Linux-specific Drag-and-Drop
//
//************************************************************************************************

#ifndef _ccl_dragndrop_linux_h
#define _ccl_dragndrop_linux_h

#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/system/mousecursor.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/linux/wayland/imagesurface.h"

namespace CCL {

//************************************************************************************************
// LinuxDragSession
//************************************************************************************************

class LinuxDragSession: public DragSession
{
public:
	DECLARE_CLASS (LinuxDragSession, DragSession)

	LinuxDragSession (IUnknown* source = 0, int inputDevice = kMouseInput);
	LinuxDragSession (wl_data_offer* offer, const Vector<MutableCString>& mimeTypes, int inputDevice = kMouseInput);
	~LinuxDragSession ();

	DECLARE_STRINGID_MEMBER (kUrlListMimeType)
	
	PROPERTY_MUTABLE_CSTRING (preferredMimeType, PreferredMimeType)
	
	// IDragSession
	IAsyncOperation* CCL_API dragAsync () override;
	
protected:
	struct DragListener: wl_data_source_listener
	{
		DragListener (LinuxDragSession& session);
		
		// data source
		static void onTarget (void* data, wl_data_source* dataSource, CStringPtr mimeType);
		static void onSendData (void* data, wl_data_source* dataSource, CStringPtr mimeType, int32_t fd);
		static void onCanceled (void* data, wl_data_source* dataSource);
		
		static void onDragDropPerformed (void* data, wl_data_source* dataSource);
		static void onDragDropFinished (void* data, wl_data_source* dataSource);
		static void onSourceAction (void* data, wl_data_source* dataSource, uint32_t sourceAction);
		
		void updateCursor ();
		
	protected:
		LinuxDragSession& session;
		uint32_t action;
		MutableCString mimeType;
		AutoPtr<MouseCursor> cursor;
		int cursorId;
	};
	DragListener listener;

	Vector<MutableCString> mimeTypes;
	AutoPtr<AsyncOperation> operation;
	Linux::ImageSurface dragImageSurface;
	wl_data_device* dataDevice;
	wl_data_source* dataSource;
	wl_data_offer* dataOffer;
	tbool terminated;
	
	void convertNativeItems ();
	void terminate (bool succeeded);
};

} // namespace CCL

#endif // _ccl_dragndrop_linux_h
