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
// Filename    : ccl/app/documents/documentperspective.h
// Description : Document Perspective
//
//************************************************************************************************

#ifndef _ccl_documentperspective_h
#define _ccl_documentperspective_h

#include "ccl/base/object.h"

#include "ccl/app/documents/idocumentview.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/gui/framework/iworkspace.h"

namespace CCL {

class ArchiveHandler;

//************************************************************************************************
// DocumentPerspective
//************************************************************************************************

class DocumentPerspective: public Object,
						   public IDocumentView,
						   public IPerspectiveActivator
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentPerspective, Object)
	
	DocumentPerspective (Document& document, IPerspective* perspective, StringID viewID, StringID altPerspectiveID);
	~DocumentPerspective ();

	static IWorkspace* getWorkspace ();
	static IPerspective* createPerspective (StringID perspectiveID);

	// IDocumentView
	void activateDocumentView () override;
	void closeDocumentView () override;
	bool isDocumentVisible () const override;

	// IPerspectiveActivator
	String CCL_API getPerspectiveTitle () override;
	String CCL_API getPerspectiveDescription () override;
	IImage* CCL_API getPerspectiveIcon () override;
	void CCL_API activatePerspective () override;
	void CCL_API notifyPerspectiveSelected () override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE2 (IDocumentView, IPerspectiveActivator, Object)

protected:
	Document& document;
	IWorkspace* workspace;
	IPerspective* perspective;
	MutableCString viewID;
	MutableCString altPerspectiveID;
	IImage* icon;
	bool closing;

	void setPerspective (IPerspective* perspective, StringID viewID = nullptr);
};

//************************************************************************************************
// PerspectiveStorageHelper
//************************************************************************************************

class PerspectiveStorageHelper
{
public:
	PerspectiveStorageHelper (IPerspective* perspective);

	IPerspective* getPerspective ();

protected:
	bool loadPerspective (ArchiveHandler& archiveHandler, StringRef progressText);
	bool savePerspective (ArchiveHandler& archiveHandler, StringRef progressText, StringID debugName = nullptr);

private:
	SharedPtr<IPerspective> perspective;
};

} // namespace CCL

#endif // _ccl_documentperspective_h
