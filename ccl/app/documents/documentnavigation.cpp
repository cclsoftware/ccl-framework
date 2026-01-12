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
// Filename    : ccl/app/documents/documentnavigation.cpp
// Description : Document Navigation
//
//************************************************************************************************

#include "ccl/app/documents/documentnavigation.h"
#include "ccl/app/documents/documentmanager.h"
#include "ccl/app/documents/document.h"

#include "ccl/app/navigation/navigator.h"

#include "ccl/public/text/istringdict.h"
#include "ccl/public/gui/paramlist.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"

namespace CCL {

//************************************************************************************************
// DocumentOpenController
//************************************************************************************************

class DocumentOpenController: public Component
{
public:
	DocumentOpenController (StringRef documentPath);

	// Component
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	enum Tags { kPathTag = 100, kOpenTag };
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// DocumentOpenController
//************************************************************************************************

DocumentOpenController::DocumentOpenController (StringRef documentPath)
{
	paramList.addString (CSTR ("DocumentPath"), kPathTag)->fromString (documentPath);
	paramList.addParam  (CSTR ("OpenDocument"), kOpenTag);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentOpenController::paramChanged (IParameter* param)
{
	if(param->getTag () == kOpenTag)
	{
		String documentPath;
		paramList.byTag (kPathTag)->toString (documentPath);

		if(!documentPath.isEmpty ())
			DocumentManager::instance ().deferOpenDocument (Url (documentPath));
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentOpenController::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isDocumentPathValid")
	{
		String documentPath;
		paramList.byTag (kPathTag)->toString (documentPath);
		var = documentPath.isEmpty () ? 0 : 1;
		return true;
	}
	return Component::getProperty (var, propertyId);
}

//************************************************************************************************
// DocumentNavigationServer
//************************************************************************************************

DEFINE_COMPONENT_SINGLETON (DocumentNavigationServer)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentNavigationServer::DocumentNavigationServer ()
: Component (CCLSTR ("DocumentServer"))
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult DocumentNavigationServer::navigateTo (NavigateArgs& args)
{
	Rect bounds;
	ViewBox (&args.contentFrame).getClientRect (bounds);
	args.contentFrame.getChildren ().removeAll ();

	String pageTitle;
	IView* documentView = nullptr;

	Document* doc = DocumentManager::instance ().getActiveDocument ();

	// check if another document path was requested (e.g. when navigating backwards)...
	StringRef documentPath = args.url.getParameters ().lookupValue (CCLSTR ("DocumentPath"));
	if(doc && !doc->getPath ().isEmpty () && !documentPath.isEmpty ())
	{
		if(!doc->getPath ().isEqualUrl (Url (documentPath)))
			doc = nullptr;
	}

	if(doc)
	{
		documentView = doc->createView ("Document", Variant (), bounds);
		pageTitle = doc->getTitle ();
	}
	else
	{
		AutoPtr<DocumentOpenController> c = NEW DocumentOpenController (documentPath);
		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		documentView = theme ? theme->createView ("CCL/DocumentOpenView", c->asUnknown ()) : nullptr;
		if(documentView)
			pageTitle = ViewBox (documentView).getTitle ();
	}

	if(documentView)
	{
		args.contentFrame.getChildren ().add (documentView);
		ViewBox (&args.contentFrame).setTitle (pageTitle);
	}

	//was: args.errorDocumentName = CCLSTR ("NoActiveDocument");
	return kResultOk;
}

//************************************************************************************************
// DocumentNavigationPage
//************************************************************************************************

IDocumentView* DocumentNavigationPage::Factory::createDocumentView (Document& document)
{
	return NEW DocumentNavigationPage (document);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentNavigationPage::makeUrl (Url& url, UrlRef documentPath)
{
	RootComponent::instance ().makeUrl (url, DocumentNavigationServer::instance ().getName ());

	if(!documentPath.isEmpty ())
	{
		String pathString;
		documentPath.getUrl (pathString);
		url.getParameters ().setEntry (CCLSTR ("DocumentPath"), pathString);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentNavigationPage::DocumentNavigationPage (Document& _document)
: document (_document)
{
	document.setDocumentView (this);
	document.addObserver (this);
	document.retain ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentNavigationPage::~DocumentNavigationPage ()
{
	document.removeObserver (this);
	document.setDocumentView (nullptr);
	document.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentNavigationPage::doNavigate ()
{
	Navigator& navigator = Navigator::instance ();
	navigator.openWindow (); // window must be open!

	if(isDocumentVisible ())
		navigator.refresh ();
	else
	{
		Url url;
		makeUrl (url, document.getPath ());
		navigator.navigateDeferred (url);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentNavigationPage::activateDocumentView ()
{
	bool activated = DocumentManager::instance ().setActiveDocument (&document);
	doNavigate ();
	if(activated)
		DocumentManager::instance ().signalDocumentEvent (document, Document::kViewActivated);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentNavigationPage::closeDocumentView ()
{
	if(isDocumentVisible ())
		Navigator::instance ().goHome ();

	release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentNavigationPage::isDocumentVisible () const
{
	Navigator& navigator = Navigator::instance ();
	if(!navigator.isOpen ())
		return false;

	Url url1, url2;
	makeUrl (url1, document.getPath ());
	makeUrl (url2, Url ()); // without document path

	UrlRef currentUrl = navigator.getCurrentUrl ();
	if(currentUrl.isEqualUrl (url1) || currentUrl.isEqualUrl (url2))
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API DocumentNavigationPage::notify (ISubject* subject, MessageRef msg)
{
	if(subject == &document)
	{
		if(msg == Document::kPathChanged)
		{
			Url* oldPath = unknown_cast<Url> (msg[0]);
			ASSERT (oldPath != nullptr)

			// We have to update the navigator url when the document path changes,
			// otherwise isDocumentVisible() would not work correctly...

			Navigator& navigator = Navigator::instance ();
			Url oldUrl;
			makeUrl (oldUrl, *oldPath);
			if(navigator.getCurrentUrl ().isEqualUrl (oldUrl))
			{
				Url newUrl;
				makeUrl (newUrl, document.getPath ());
				navigator.setCurrentUrl (newUrl);
			}
		}
	}
}
