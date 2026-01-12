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
// Filename    : ccl/app/documents/documentversions.h
// Description : Document Version Management
//
//************************************************************************************************

#ifndef _ccl_documentversions_h
#define _ccl_documentversions_h

#include "ccl/app/component.h"

#include "ccl/base/storage/url.h"
#include "ccl/public/base/datetime.h"
#include "ccl/public/gui/graphics/iimage.h"

namespace CCL {

class Document;
class Container;
class DocumentMetaInfo;

//************************************************************************************************
// DocumentDescription
//************************************************************************************************

class DocumentDescription: public Object
{
public:
	DECLARE_CLASS (DocumentDescription, Object)

	PROPERTY_OBJECT (Url, path, Path)
	PROPERTY_OBJECT (DateTime, date, Date)
	PROPERTY_SHARED_AUTO (IImage, icon, Icon)
	PROPERTY_STRING (title, Title)
	PROPERTY_STRING (description, Description)
	PROPERTY_STRING (age, Age)
	PROPERTY_STRING (dateString, DateString)

	void assign (UrlRef documentPath, bool isVersion = false);

	String getSummary () const;
	bool isAutoSave () const;

	// Object
	int compare (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;
};

//************************************************************************************************
// DocumentVersions
//************************************************************************************************

class DocumentVersions
{
public:
	static bool isSupported ();
	static void setSupported (bool);

	DocumentVersions (UrlRef documentPath);

	static StringRef getHistoryFolderName ();
	void getHistoryFolder (Url& path);
	void makeHistoryPath (Url& path, const String* suffix = nullptr, bool withTimeStamp = true);
	void makeHistoryPath (Url& path, StringRef baseName, const String* suffix, bool withTimeStamp);
	void makeVersionPath (Url& path);
	Url makeVersionPathInDocumentFolder (UrlRef sourcePath);

	static constexpr int kBuildTimeout = 5000; // max. 5 seconds
	bool buildHistory (Container& list, int timeout = kBuildTimeout); ///< list of DocumentDescription

	bool copyOldFormatToHistory (const Document& doc);
	bool moveDocumentVersionToHistory ();
	bool moveDocumentToHistory (const IUrl* docFile = nullptr, const String* suffix = nullptr);
	bool purgeOldest (StringRef description, int numFilesToKeep);

	bool restoreDocumentVersion (UrlRef historyFile);

	static String getDisplayDescription (const DocumentMetaInfo& metaInfo, bool forceDescription = true);
	static void appendOriginalSuffix (Url& path);

	static void onActiveVersionChanged (UrlRef oldDocumentPath, UrlRef newDocumentPath);

	static void getSortOrder (MutableCString& columnID, tbool& upwards);
	static void setSortOrder (StringID columnID, tbool upwards);
	static void sortDescriptions (ObjectArray& descriptions);
	static Core::VectorCompareFunction* getCompareFunction (StringID id);
	static int compareAutoSave (DocumentDescription& lhs, DocumentDescription& rhs);

	static const String strDocumentSnapshotSuffix;
	static const String strAutosaveSnapshotSuffix;

protected:
	Url documentPath;

	static bool supported;
	static MutableCString sortColumnID;
	static bool sortUpwards;

	class HistoryChecker;

	static IAttributeList* createMetaAttribs (UrlRef path);
	static String makeVersionFileName (UrlRef path, bool forceDescription = true);
};

//************************************************************************************************
// DocumentVersionSelector
//************************************************************************************************

class DocumentVersionSelector: public Component
{
public:
	DocumentVersionSelector ();
	~DocumentVersionSelector ();

	void runDialog (Document& document);

	// Component
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

private:
	class HistoryList;

	HistoryList* historyList;
	Document* document;
};

} // namespace CCL

#endif // _ccl_documentversions_h
