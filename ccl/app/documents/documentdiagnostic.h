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
// Filename    : ccl/app/documents/documentdiagnostic.h
// Description : Document Diagnostic Dialogs
//
//************************************************************************************************

#ifndef _ccl_documentdiagnostic_h
#define _ccl_documentdiagnostic_h

#include "ccl/app/component.h"

#include "ccl/base/singleton.h"
#include "ccl/base/collections/objectarray.h"

#include "ccl/public/gui/inavigator.h"
#include "ccl/public/system/idiagnosticstore.h"
#include "ccl/public/system/idiagnosticdataprovider.h"

namespace CCL {

class DialogBox;
class TextBlock;
interface IHtmlWriter;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Diagnostic Context IDs
//////////////////////////////////////////////////////////////////////////////////////////////////

namespace DiagnosticID
{
	const CStringPtr kDocumentPrefix = "document/";
}

//************************************************************************************************
// DocumentDiagnosticData
//************************************************************************************************

class DocumentDiagnosticData: public Object
{
public:
	DECLARE_CLASS (DocumentDiagnosticData, Object)

	DocumentDiagnosticData ();

	enum Type { kLoadData, kSaveData };
	PROPERTY_VARIABLE (Type, type, Type)

	PROPERTY_SHARED_AUTO (IDiagnosticResultSet, loadDurationData, LoadDurationData)
	PROPERTY_SHARED_AUTO (IDiagnosticResult, loadDuration, LoadDuration)

	PROPERTY_SHARED_AUTO (IDiagnosticResultSet, saveDurationData, SaveDurationData)
	PROPERTY_SHARED_AUTO (IDiagnosticResultSet, saveSizeData, SaveSizeData)
	PROPERTY_SHARED_AUTO (IDiagnosticResult, saveDuration, SaveDuration)

	void captureLoadData (StringID documentContext);
	void captureSaveData (StringID documentContext);
};
	
//************************************************************************************************
// DocumentDiagnosticDataProvider
//************************************************************************************************

class DocumentDiagnosticDataProvider: public Object,
									  public IDiagnosticDataProvider,
									  public Singleton<DocumentDiagnosticDataProvider>
{
public:
	DocumentDiagnosticDataProvider ();

	static const String kSubFolder;
	static const String kLoadFileName;
	static const String kSaveFileName;
	static const String kLoadDurationName;
	static const String kSaveDurationName;
	static const String kSaveSizeName;

	void getBasePath (IUrl& path) const;
	void getReportFilePath (IUrl& path, StringRef documentName, DocumentDiagnosticData::Type type) const;
	void getDataFilePath (IUrl& path, StringRef documentName, StringID key) const;
	void getReportFileName (String& fileName, StringRef documentName, DocumentDiagnosticData::Type type) const;
	void getDataFileName (String& fileName, StringRef documentName, StringID key) const;

	void scanReports ();

	// IDiagnosticDataProvider
	int CCL_API countDiagnosticData () const override;
	tbool CCL_API getDiagnosticDescription (DiagnosticDescription& description, int index) const override;
	IStream* CCL_API createDiagnosticData (int index) override;
	
	CLASS_INTERFACE (IDiagnosticDataProvider, Object)

protected:
	Vector<Url> savedReports;
	bool reportsValid;
};

//************************************************************************************************
// DocumentDiagnosticDialog
//************************************************************************************************

class DocumentDiagnosticDialog: public Component
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentDiagnosticDialog, Component)

	DocumentDiagnosticDialog (DocumentDiagnosticData::Type type, StringID formName = nullptr, StringRef documentName = nullptr);
	~DocumentDiagnosticDialog ();

	void runDialog ();
	
	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	class DiagnosticList;

	MutableCString formName;
	SharedPtr<INavigator> navigator;
	ObjectArray listModels;
	Variant documentTotal;
	double pluginsTotal;
	double dataTotal;
	String documentName;
	Vector<String> topItems;
	Vector<String> criticalItems;
	DocumentDiagnosticData::Type type;
	IParameter* exportParam;
	IParameter* overviewParam;
	IParameter* usePlainTextParam;

	void addData (StringID key, StringRef title, const IDiagnosticResultSet& data, double criticalItemThreshold = 0.5);
	void updateTopItems (IDiagnosticResultSet& data, int count = 5);
	void updateCriticalItems (IDiagnosticResultSet& data, double threshold = 0.5);
	float getTotal (const IDiagnosticResultSet& data, StringID prefix) const;

	void updateOverview ();
	IHtmlWriter* beginDocument (UrlRef path) const;
	void endDocument (IHtmlWriter& writer) const;

	virtual bool writeOverview (UrlRef path) const;
	virtual void writeOverviewContent (TextBlock& block) const;
	virtual void writeCriticalItems (TextBlock& block, StringRef description) const;
	virtual void writeTopItems (TextBlock& block) const;
	virtual void writeData (TextBlock& block);
	
	virtual bool exportCsv (DiagnosticList& list, UrlRef filePath);
	virtual bool exportHtml (UrlRef filePath);
	virtual void exportCsvWithFileSelector (DiagnosticList& list);
	virtual void exportHtmlWithFileSelector ();

	virtual void exportData ();
};

//************************************************************************************************
// DocumentDiagnosticLoadDialog
//************************************************************************************************

class DocumentDiagnosticLoadDialog: public DocumentDiagnosticDialog
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentDiagnosticLoadDialog, DocumentDiagnosticDialog)

	DocumentDiagnosticLoadDialog (const DocumentDiagnosticData& documentData, StringRef documentName);

	// DocumentDiagnosticDialog
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	IParameter* exportLoadDurationParam;

	// DocumentDiagnosticDialog
	void writeOverviewContent (TextBlock& block) const override;
};

//************************************************************************************************
// DocumentDiagnosticSaveDialog
//************************************************************************************************

class DocumentDiagnosticSaveDialog: public DocumentDiagnosticDialog
{
public:
	DECLARE_CLASS_ABSTRACT (DocumentDiagnosticSaveDialog, DocumentDiagnosticDialog)
	
	DocumentDiagnosticSaveDialog (const DocumentDiagnosticData& documentData, StringRef documentName);
	
	// DocumentDiagnosticDialog
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	static const int kCriticalSizeThreshold = 50000;

	IParameter* exportSaveDurationParam;
	IParameter* exportSaveSizeParam;

	// DocumentDiagnosticDialog
	void writeOverviewContent (TextBlock& block) const override;
};

}; // namespace CCL

#endif // _ccl_documentdiagnostic_h
