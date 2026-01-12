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
// Filename    : ccl/public/app/documentlistener.h
// Description : Document Listener
//
//************************************************************************************************

#ifndef _ccl_documentlistener_h
#define _ccl_documentlistener_h

#include "ccl/public/app/idocument.h"

#include "ccl/public/base/cclmacros.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/storage/istorage.h"

namespace CCL {

//************************************************************************************************
// DocumentListener
//************************************************************************************************

class DocumentListener: public IStorageHandler
{
public:
	DocumentListener (IDocument* appDocument);

	PROPERTY_POINTER (IDocument, appDocument, AppDocument)

	/** Called after ctor (new/load/import). */
	virtual void initialize () {}

	/** Called before dtor. */
	virtual void terminate () {}

	/** Handle document event. */
	virtual void onEvent (int eventCode) {}

	// IStorageHandler
	tbool CCL_API saveContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress = nullptr) const override;
	tbool CCL_API loadContent (IFileSystem& fileSystem, VariantRef data, IProgressNotify* progress = nullptr) override;
};

//************************************************************************************************
// DocumentListenerFactory
//************************************************************************************************

class DocumentListenerFactory: public AbstractDocumentEventHandler
{
public:
	DocumentListenerFactory ();
	~DocumentListenerFactory ();

	/** Get document manager of host application. */
	static IDocumentManager* getDocumentManager ();

	/** Register for document events. */
	bool beginListen ();

	/** Unregister from document events. */
	void endListen ();

	/** Check if any listeners are active. */
	bool hasListeners () const;

	/** Find listener for given document. */
	DocumentListener* findListener (IDocument& document) const;

	// IDocumentEventHandler
	void CCL_API onDocumentEvent (IDocument& document, int eventCode) override;

protected:
	IDocumentManager* documentManager;
	Vector<DocumentListener*> listeners;

	/** A document becomes available or goes away. */
	virtual void onDocumentAvailable (IDocument& document, bool state);

	/** Overwrite to create listener for given document. */
	virtual DocumentListener* createListener (IDocument& document) { return nullptr; }

	/** A listener becomes available or goes away. */
	virtual void onListenerAvailable (DocumentListener& listener, bool state);
};

} // namespace CCL

#endif // _ccl_documentlistener_h
