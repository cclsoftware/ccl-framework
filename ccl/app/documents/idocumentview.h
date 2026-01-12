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
// Filename    : ccl/app/documents/idocumentview.h
// Description : Document View
//
//************************************************************************************************

#ifndef _ccl_idocumentview_h
#define _ccl_idocumentview_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

class Document;

//************************************************************************************************
// IDocumentView
//************************************************************************************************

interface IDocumentView: IUnknown
{	
	virtual void activateDocumentView () = 0;
	
	virtual void closeDocumentView () = 0;	///< should release object!
	
	virtual bool isDocumentVisible () const = 0;

	DECLARE_IID (IDocumentView)
};

//************************************************************************************************
// IDocumentViewFactory
//************************************************************************************************

interface IDocumentViewFactory: IUnknown
{
	virtual IDocumentView* createDocumentView (Document& document) = 0;

	DECLARE_IID (IDocumentViewFactory)
};

} // namespace CCL

#endif // _ccl_idocumentview_h
