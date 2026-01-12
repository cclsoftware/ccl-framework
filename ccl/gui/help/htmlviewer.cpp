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
// Filename    : ccl/gui/help/htmlviewer.cpp
// Description : HTML Document Viewer
//
//************************************************************************************************

#include "ccl/gui/help/htmlviewer.h"

#include "ccl/base/storage/url.h"
#include "ccl/base/storage/urlencoder.h"

#include "ccl/gui/skin/form.h"
#include "ccl/gui/theme/theme.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/system/webbrowserview.h"

using namespace CCL;

//************************************************************************************************
// HtmlDocumentViewer
//************************************************************************************************

HtmlDocumentViewer::HtmlDocumentViewer ()
: navigator (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

HtmlDocumentViewer::~HtmlDocumentViewer ()
{
	ASSERT (navigator == nullptr)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* HtmlDocumentViewer::getWindow ()
{
	return Desktop.getWindowByOwner (this->asUnknown ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* HtmlDocumentViewer::openWindow ()
{
	IWindow* window = getWindow ();
	if(window == nullptr)
	{
		Theme& theme = FrameworkTheme::instance ();
		Form* form = unknown_cast<Form> (theme.createView ("CCL/HelpViewer", this->asUnknown ()));
		ASSERT (form != nullptr)
		if(form != nullptr)
			window = form->openWindow ();
	}
	else
	{
		// restore window if currently minimized
		if(window->isMinimized ())
			window->maximize (false);
	}
	return window;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API HtmlDocumentViewer::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged)
		if(WebBrowserView* webView = unknown_cast<WebBrowserView> (subject))
			navigator = webView->getNavigator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HtmlDocumentViewer::isInstalled ()
{
	return NativeWebControl::isAvailable ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HtmlDocumentViewer::canOpenDocument (UrlRef document) const
{
	return NativeWebControl::isAvailable () && document.getFileType ().getExtension ().startsWith (CCLSTR ("htm"), false); // .htm + .html
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HtmlDocumentViewer::openDocument (UrlRef document, StringRef nameDest)
{
	openWindow ();
	if(navigator == nullptr)
		return false;

	Url destination (document);
	if(nameDest.isEmpty () == false)
	{
		String path (destination.getPath ());
		path << "#" << UrlEncoder ().encode (nameDest);
		destination.setPath (path);
	}

	navigator->navigate (destination);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API HtmlDocumentViewer::closeAllDocuments ()
{
	if(IWindow* window = getWindow ())
		window->close ();
	return true;
}
