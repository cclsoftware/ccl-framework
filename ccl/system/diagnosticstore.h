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
// Filename    : ccl/system/diagnosticstore.h
// Description : Diagnostic Store
//
//************************************************************************************************

#ifndef _ccl_diagnosticstore_h
#define _ccl_diagnosticstore_h

#include "ccl/base/singleton.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/url.h"

#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/system/idiagnosticdataprovider.h"

namespace CCL {

class DiagnosticResultSet;

//************************************************************************************************
// DiagnosticStore
//************************************************************************************************

class DiagnosticStore: public Object,
					   public IDiagnosticStore,
					   public IDiagnosticDataProvider,
					   public Singleton<DiagnosticStore>
{
public:
	DiagnosticStore ();
	~DiagnosticStore ();

	// IDiagnosticStore
	tresult CCL_API submitValue (StringID context, StringID key, VariantRef value = Variant (), StringRef label = nullptr) override;
	tresult CCL_API clearData (StringID context, StringID key) override;
	IDiagnosticResultSet* CCL_API queryResults (StringID context, StringID key) const override;
	IDiagnosticResult* CCL_API queryResult (StringID context, StringID key) const override;
	IDiagnosticResultSet* CCL_API queryMultipleResults (StringID context, CString keys[], int keyCount) const override;
	DiagnosticMode CCL_API setMode (DiagnosticMode mode) override;

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;

	CLASS_INTERFACE2 (IDiagnosticStore, IDiagnosticDataProvider, Object)

protected:
	static const String kPersistentName;
	
	ObjectArray data;
	ObjectArray shortTermData;
	DiagnosticMode mode;

	void store ();
	void restore ();

	Attributes* findContextData (const ObjectArray& contextContainer, StringID context) const;
	Attributes& getData (StringID context, StringID key, DiagnosticMode mode = kLongTerm);
	void cleanup ();
	void queryResults (DiagnosticResultSet& results, StringID context, StringID key, int count = -1) const;
	void queryMultipleResults (DiagnosticResultSet& results, StringID context, CString keys[], int keyCount) const;
};

} // namespace CCL

#endif // _ccl_diagnosticstore_h
