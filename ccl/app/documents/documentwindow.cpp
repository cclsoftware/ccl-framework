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
// Filename    : ccl/app/documents/documentwindow.cpp
// Description : Document Window
//
//************************************************************************************************

#include "ccl/app/documents/documentwindow.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"

#include "ccl/public/base/variant.h"
#include "ccl/public/gui/framework/viewbox.h"

using namespace CCL;

//************************************************************************************************
// DocumentWindowFactory
//************************************************************************************************

IDocumentView* DocumentWindowFactory::createDocumentView (Document& document)
{
	Rect bounds (0, 0, DocumentWindow::kDefaultWidth, DocumentWindow::kDefaultHeight);
	ViewBox view = document.createView ("Document", Variant (), bounds);
	ASSERT (view != nullptr)
	if(!view)
		return nullptr;

	IWindow* window = nullptr;
	if(FormBox::isForm (view))
	{
		view.setTitle (document.getTitle ());
		window = FormBox (view).openWindow ();
	}
	else
	{
		FormBox form (view.getSize (), StyleFlags (0, Styles::kWindowCombinedStyleSizable), document.getTitle ());
		view.setPosition (Point ());
		view.setSizeMode (IView::kAttachAll);
		form.getChildren ().add (view);
		window = form.openWindow ();
	}

	ASSERT (window != nullptr)
	return NEW DocumentWindow (document, window);
}

//************************************************************************************************
// DocumentWindow
//************************************************************************************************

DocumentWindow::DocumentWindow (Document& _document, IWindow* _window)
: document (_document),
  window (_window),
  isClosing (false)
{
	window->addHandler (this);
	document.setDocumentView (this);
	document.retain ();
}
					
//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentWindow::~DocumentWindow ()
{
	ASSERT (window == nullptr)

	document.setDocumentView (nullptr);
	document.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentWindow::activateDocumentView ()
{
	ASSERT (window != nullptr)
	if(!window->isVisible ())
		window->show ();
	else
		window->activate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentWindow::closeDocumentView ()
{
	if(isClosing)
		return;

	isClosing = true;
	if(window)
	{
		window->removeHandler (this);
		window->close ();
		window = nullptr; // window is dead after close!
	}
	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentWindow::isDocumentVisible () const
{
	return window->isVisible () ? true : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentWindow::onWindowEvent (WindowEvent& windowEvent)
{
	if(windowEvent.eventType == WindowEvent::kActivate)
	{
		if(DocumentManager::instance ().setActiveDocument (&document))
			DocumentManager::instance ().signalDocumentEvent (document, Document::kViewActivated);
	}
	else
	if(windowEvent.eventType == WindowEvent::kClose)
	{
		// to be tested!!!!
		if(!isClosing)
		{
			isClosing = true;
			tbool result = DocumentManager::instance ().closeDocument (&document);
			isClosing = false;
			if(!result)
				return false;
		}
	}
	return true;
}
