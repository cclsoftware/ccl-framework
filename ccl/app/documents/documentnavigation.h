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
// Filename    : ccl/app/documents/documentnavigation.h
// Description : Document Navigation
//
//************************************************************************************************

#ifndef _ccl_documentnavigation_h
#define _ccl_documentnavigation_h

#include "ccl/app/component.h"
#include "ccl/app/documents/idocumentview.h"

#include "ccl/public/app/inavigationserver.h"

namespace CCL {

class Url;

//************************************************************************************************
// DocumentNavigationServer
//************************************************************************************************

class DocumentNavigationServer:	public Component,
								public INavigationServer,
								public ComponentSingleton<DocumentNavigationServer>
{
public:
	DocumentNavigationServer ();

	// INavigationServer
	tresult CCL_API navigateTo (NavigateArgs& args) override;

	CLASS_INTERFACE (INavigationServer, Component)
};

//************************************************************************************************
// DocumentNavigationPage
//************************************************************************************************

class DocumentNavigationPage: public Object,
							  public IDocumentView
{
public:
	DocumentNavigationPage (Document& document);
	~DocumentNavigationPage ();

	/** Document View Factory */
	class Factory: public Object, 
				   public IDocumentViewFactory
	{
	public:
		IDocumentView* createDocumentView (Document& document) override;
		CLASS_INTERFACE (IDocumentViewFactory, Object)
	};

	// IDocumentView
	void activateDocumentView () override;
	void closeDocumentView () override;
	bool isDocumentVisible () const override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IDocumentView, Object)

protected:
	Document& document;

	static void makeUrl (Url& url, UrlRef documentPath);

	void doNavigate ();
};

} // namespace CCL

#endif // _ccl_documentnavigation_h
