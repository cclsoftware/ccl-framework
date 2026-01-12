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
// Filename    : ccl/gui/help/documentviewer.h
// Description : Document Viewer
//
//************************************************************************************************

#ifndef _ccl_documentviewer_h
#define _ccl_documentviewer_h

#include "ccl/base/object.h"

namespace CCL {

class DocumentViewerFactory;

//************************************************************************************************
// IDocumentViewer (could be made public later)
//************************************************************************************************

interface IDocumentViewer: IUnknown
{
	/** Check if viewer is installed. */
	virtual tbool CCL_API isInstalled () = 0;

	/** Check if viewer can open given document. */
	virtual tbool CCL_API canOpenDocument (UrlRef document) const = 0;

	/** Open document in viewer. */
	virtual tbool CCL_API openDocument (UrlRef document, StringRef nameDest) = 0;

	/** Close all open documents. */
	virtual tbool CCL_API closeAllDocuments () = 0;

	DECLARE_IID (IDocumentViewer)
};

//************************************************************************************************
// DocumentViewer
//************************************************************************************************

class DocumentViewer: public Object,
					  public IDocumentViewer
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentViewer, Object)

	static void setFactory (DocumentViewerFactory* factory);

	/** Create viewer for PDF documents. */
	static IDocumentViewer* createPDFViewer ();

	/** Create default system viewer. */
	static IDocumentViewer* createSystemViewer ();

	CLASS_INTERFACE (IDocumentViewer, Object)

private:
	static DocumentViewerFactory* factory;
};

//************************************************************************************************
// DocumentViewerFactory
//************************************************************************************************

class DocumentViewerFactory
{
public:
	virtual IDocumentViewer* createPDFViewer () = 0;
};

} // namespace CCL

#endif // _ccl_documentviewer_h
