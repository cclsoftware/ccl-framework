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
// Filename    : ccl/gui/help/htmlviewer.h
// Description : HTML Document Viewer
//
//************************************************************************************************

#ifndef _ccl_htmlviewer_h
#define _ccl_htmlviewer_h

#include "ccl/gui/help/documentviewer.h"

namespace CCL {

interface IWindow;
interface INavigator;

//************************************************************************************************
// HtmlDocumentViewer
//************************************************************************************************

class HtmlDocumentViewer: public DocumentViewer
{
public:
	HtmlDocumentViewer ();
	~HtmlDocumentViewer ();

	// DocumentViewer
	tbool CCL_API isInstalled () override;
	tbool CCL_API canOpenDocument (UrlRef document) const override;
	tbool CCL_API openDocument (UrlRef document, StringRef nameDest) override;
	tbool CCL_API closeAllDocuments () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	INavigator* navigator;

	IWindow* getWindow ();
	IWindow* openWindow ();
};

} // namespace CCL

#endif // _ccl_htmlviewer_h
