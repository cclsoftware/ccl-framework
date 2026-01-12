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
// Filename    : ccl/gui/help/documentviewer.cpp
// Description : Document Viewer
//
//************************************************************************************************

#include "ccl/gui/help/documentviewer.h"

#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/guiservices.h"

namespace CCL {

//************************************************************************************************
// SystemDocumentViewer
//************************************************************************************************

class SystemDocumentViewer: public DocumentViewer
{
public:
	// DocumentViewer
	tbool CCL_API isInstalled () override;
	tbool CCL_API canOpenDocument (UrlRef document) const override;
	tbool CCL_API openDocument (UrlRef document, StringRef nameDest) override;
	tbool CCL_API closeAllDocuments () override;
};

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// IDocumentViewer
//************************************************************************************************

DEFINE_IID_ (IDocumentViewer, 0xbd99f94b, 0x6bed, 0x4d62, 0x8f, 0xd2, 0x18, 0xaa, 0x5f, 0xe0, 0x7b, 0xb0)

//************************************************************************************************
// DocumentViewer
//************************************************************************************************

DocumentViewerFactory* DocumentViewer::factory = nullptr;
void DocumentViewer::setFactory (DocumentViewerFactory* _factory)
{
	factory = _factory;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentViewer* DocumentViewer::createPDFViewer ()
{
	return factory ? factory->createPDFViewer () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDocumentViewer* DocumentViewer::createSystemViewer ()
{
	return NEW SystemDocumentViewer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentViewer, Object)

//************************************************************************************************
// SystemDocumentViewer
//************************************************************************************************

tbool CCL_API SystemDocumentViewer::isInstalled ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemDocumentViewer::canOpenDocument (UrlRef document) const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemDocumentViewer::openDocument (UrlRef document, StringRef nameDest)
{
	return System::GetSystemShell ().openUrl (document) == kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SystemDocumentViewer::closeAllDocuments ()
{
	return false;
}
