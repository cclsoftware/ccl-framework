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
// Filename    : ccl/app/documents/documentwindow.h
// Description : Document Window
//
//************************************************************************************************

#ifndef _ccl_documentwindow_h
#define _ccl_documentwindow_h

#include "ccl/base/object.h"
#include "ccl/app/documents/idocumentview.h"
#include "ccl/public/gui/framework/iwindow.h"

namespace CCL {

class Document;

//************************************************************************************************
// DocumentWindow
//************************************************************************************************

class DocumentWindow: public Object,
					  public IDocumentView,
					  public IWindowEventHandler
{
public:
	enum Constants
	{
		kDefaultWidth = 600,
		kDefaultHeight = 400
	};

	DocumentWindow (Document& document, IWindow* window);
	~DocumentWindow ();

	// IDocumentView
	void activateDocumentView () override;
	void closeDocumentView () override;
	bool isDocumentVisible () const override;

	// IWindowEventHandler
	tbool CCL_API onWindowEvent (WindowEvent& windowEvent) override;

	CLASS_INTERFACE2 (IDocumentView, IWindowEventHandler, Object)

protected:
	bool isClosing;
	Document& document;
	IWindow* window;
};

//************************************************************************************************
// DocumentWindowFactory
//************************************************************************************************

class DocumentWindowFactory: public Object,
							 public IDocumentViewFactory
{
public:
	// IDocumentViewFactory
	IDocumentView* createDocumentView (Document& document) override;

	CLASS_INTERFACE (IDocumentViewFactory, Object)
};

} // namespace CCL

#endif // _ccl_documentwindow_h
