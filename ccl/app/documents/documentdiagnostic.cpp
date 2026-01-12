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
// Filename    : ccl/app/documents/documentdiagnostic.cpp
// Description : Document Diagnostic Dialogs
//
//************************************************************************************************

#include "ccl/app/documents/documentdiagnostic.h"

#include "ccl/app/controls/listviewmodel.h"
#include "ccl/app/safety/appsafetymanager.h"
#include "ccl/app/utilities/appdiagnostic.h"

#include "ccl/base/trigger.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/textfile.h"

#include "ccl/public/gui/framework/ifileselector.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/dialogbox.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/guiservices.h"

#include "ccl/public/system/ifileutilities.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/text/ihtmlwriter.h"
#include "ccl/public/text/itextbuilder.h"
#include "ccl/public/text/itextstreamer.h"
#include "ccl/public/base/memorystream.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// DocumentDiagnosticDialog::DiagnosticList
//************************************************************************************************
	
class DocumentDiagnosticDialog::DiagnosticList: public ListViewModel
{
public:
	DECLARE_CLASS_ABSTRACT (DiagnosticList, ListViewModel)

	DiagnosticList (StringID key, StringRef title, const IDiagnosticResultSet& data, double criticalItemThreshold);
	
	PROPERTY_VARIABLE (CString, key, Key)
	PROPERTY_VARIABLE (String, title, Title)
		
	DECLARE_STRINGID_MEMBER (kKeyID)
	DECLARE_STRINGID_MEMBER (kAverageID)
	DECLARE_STRINGID_MEMBER (kCountID)
	DECLARE_STRINGID_MEMBER (kTotalID)
	
	static void format (String& string, double value, StringID key);

	void writeHtml (TextBlock& block);
	void writeCsv (ITextStreamer& streamer);

protected:
	class DiagnosticListViewItem: public ListViewItem
	{
	public:
		DECLARE_CLASS (DiagnosticListViewItem, ListViewItem)

		DiagnosticListViewItem ()
		: critical (false)
		{}

		PROPERTY_SHARED_AUTO (IDiagnosticResult, diagnosticResult, DiagnosticResult)
		PROPERTY_MUTABLE_CSTRING (type, Type)
		PROPERTY_BOOL (critical, Critical)

		// ListViewItem
		StringID getCustomBackground () const override
		{
			return isCritical () ? CSTR ("CriticalItem") : CString::kEmpty;
		}
	};

	double criticalItemThreshold;
	
	static DEFINE_ARRAY_COMPARE (sortByType, DiagnosticListViewItem, item1, item2)
		int result = item1->getType ().compare (item2->getType ());
		if(result == 0)
			result = int (item1->getIcon () - item2->getIcon ());
		return result;
	}

	static DEFINE_ARRAY_COMPARE (sortByName, DiagnosticListViewItem, item1, item2)
		return item1->getTitle ().compareWithOptions (item2->getTitle (), Text::kIgnoreCase|Text::kIgnoreDiacritic);
	}

	static DEFINE_ARRAY_COMPARE (sortByAverage, DiagnosticListViewItem, item1, item2)
		return ccl_compare (item1->getDiagnosticResult ()->getAverage (), item2->getDiagnosticResult ()->getAverage ());
	}

	static DEFINE_ARRAY_COMPARE (sortByCount, DiagnosticListViewItem, item1, item2)
		return ccl_compare (item1->getDiagnosticResult ()->getCount (), item2->getDiagnosticResult ()->getCount ());
	}

	static DEFINE_ARRAY_COMPARE (sortByTotal, DiagnosticListViewItem, item1, item2)
		return ccl_compare (item1->getDiagnosticResult ()->getSum (), item2->getDiagnosticResult ()->getSum ());
	}
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("DocumentDiagnostics")
	XSTRING (Name, "Name")
	XSTRING (Average, "Average")
	XSTRING (Count, "Count")
	XSTRING (Total, "Total")

	XSTRING (SaveDurationTitle, "Save duration of plug-ins and document data")
	XSTRING (SaveSizeTitle, "Size of plug-in presets and other document data")
	XSTRING (LoadDurationTitle, "Load duration of plug-ins and document data")

	XSTRING (DocumentDiagnostics, "Document Diagnostics")
	XSTRING (DocumentLoadDuration, "Loading the document '%(1)' took %(2).")
	XSTRING (DocumentSaveDuration, "Saving the document '%(1)' took %(2).")
	XSTRING (SaveDurationPlugins, "%(1) of the time was spent saving plug-ins.")
	XSTRING (SaveDurationData, "%(1) of the time was spent saving other document data.")
	XSTRING (LoadDurationPlugins, "%(1) of the time was spent loading plug-ins.")
	XSTRING (LoadDurationData, "%(1) of the time was spent loading other document data.")
	XSTRING (CriticalLoadItems, "These items have a high average load time:")
	XSTRING (CriticalSaveItems, "These items have a high average save time:")
	XSTRING (TopItems, "In total, the most time-consuming items are:")
END_XSTRINGS

//************************************************************************************************
// DocumentDiagnosticData
//************************************************************************************************

DEFINE_CLASS_HIDDEN (DocumentDiagnosticData, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticData::DocumentDiagnosticData ()
: type (kLoadData)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticData::captureLoadData (StringID documentContext)
{
	setType (kLoadData);

	AutoPtr<IDiagnosticResultSet> durationData = System::GetDiagnosticStore ().queryResults ("*", DiagnosticID::kLoadDuration);
	setLoadDurationData (durationData);
	
	AutoPtr<IDiagnosticResult> documentDuration = System::GetDiagnosticStore ().queryResult (documentContext, DiagnosticID::kLoadDuration);
	setLoadDuration (documentDuration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticData::captureSaveData (StringID documentContext)
{
	setType (kSaveData);

	AutoPtr<IDiagnosticResultSet> durationData = System::GetDiagnosticStore ().queryResults ("*", DiagnosticID::kSaveDuration);
	setSaveDurationData (durationData);
	
	AutoPtr<IDiagnosticResultSet> sizeData = System::GetDiagnosticStore ().queryResults ("*", DiagnosticID::kSaveSize);
	setSaveSizeData (sizeData);

	AutoPtr<IDiagnosticResult> documentDuration = System::GetDiagnosticStore ().queryResult (documentContext, DiagnosticID::kSaveDuration);
	setSaveDuration (documentDuration);
}

//************************************************************************************************
// DocumentDiagnosticsDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentDiagnosticDialog, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticDialog::DocumentDiagnosticDialog (DocumentDiagnosticData::Type type, StringID formName, StringRef documentName)
: Component ("DocumentDiagnosticDialog"),
  formName (formName),
  documentName (documentName),
  exportParam (nullptr),
  overviewParam (nullptr),
  usePlainTextParam (nullptr),
  type (type),
  pluginsTotal (0),
  dataTotal (0)
{
	listModels.objectCleanup ();

	exportParam = paramList.addParam ("export");
	overviewParam = paramList.addString ("overview");
	usePlainTextParam = paramList.addParam ("usePlainText");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticDialog::~DocumentDiagnosticDialog ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::addData (StringID key, StringRef title, const IDiagnosticResultSet& data, double criticalItemThreshold)
{
	DiagnosticList* list = NEW DiagnosticList (key, title, data, criticalItemThreshold);
	if(key == DiagnosticID::kSaveDuration)
		addObject ("saveDurationList", list);
	else if(key == DiagnosticID::kSaveSize)
		addObject ("saveSizeList", list);
	else if(key == DiagnosticID::kLoadDuration)
		addObject ("loadDurationList", list);
	listModels.add (list);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::runDialog ()
{
	exportData ();

	AutoPtr<IView> view = getTheme ()->createView (formName, this->asUnknown ());
	if(view)
	{
		DialogBox dialog;
		dialog->runDialog (view.detach (), Styles::kWindowCombinedStyleDialog, Styles::kOkayButton);
	}
}

///////////////////////////////////////////////////////////////

void CCL_API DocumentDiagnosticDialog::notify (ISubject* subject, MessageRef msg)
{
	if(msg == kPropertyChanged)
	{
		// from WebBrowserView: aquire navigator
		UnknownPtr<INavigator> navigator = Property (UnknownPtr<IObject> (subject), kWebBrowserViewNavigator).get ().asUnknown ();
		if(navigator)
			this->navigator = navigator;

		usePlainTextParam->setValue (this->navigator == nullptr);

		updateOverview ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDiagnosticDialog::paramChanged (IParameter* param)
{
	if(param == exportParam)
	{
		exportHtmlWithFileSelector ();
		return true;
	}

	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDiagnosticDialog::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "saveDurationTitle")
	{
		var.fromString (XSTR (SaveDurationTitle));
		return true;
	}
	else if(propertyId == "loadDurationTitle")
	{
		var.fromString (XSTR (LoadDurationTitle));
		return true;
	}
	else if(propertyId == "saveSizeTitle")
	{
		var.fromString (XSTR (SaveSizeTitle));
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::updateTopItems (IDiagnosticResultSet& data, int count)
{
	topItems.removeAll ();

	data.sortBySum ();

	for(int i = 0; i < count && i < data.getCount (); i++)
	{
		String label = DiagnosticPresentation::getLabel (data.at (i));
		if(label.isEmpty ())
		{
			count++;
			continue;
		}

		topItems.add (String ().appendFormat ("%(1) (%(2))", label,
					  DiagnosticPresentation::printDuration (data.at (i)->getSum ())));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::updateCriticalItems (IDiagnosticResultSet& data, double threshold)
{
	criticalItems.removeAll ();

	data.sortByAverage ();

	for(int i = 0; i < data.getCount (); i++)
	{
		if(data.at (i)->getAverage () < threshold)
			return;

		String label = DiagnosticPresentation::getLabel (data.at (i));
		if(label.isEmpty ())
			continue;
		
		criticalItems.add (label);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float DocumentDiagnosticDialog::getTotal (const IDiagnosticResultSet& data, StringID prefix) const
{
	float sum = 0;
	for(int i = 0; i < data.getCount (); i++)
	{
		if(prefix.isEmpty () || data.at (i)->getContext ().startsWith (prefix))
			sum += data.at (i)->getSum ();
	}
	return sum;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::updateOverview ()
{
	Url path;
	System::GetFileUtilities ().makeUniqueTempFile (path, getName ());
	path.setFileType (FileTypes::Html ());

	if(!writeOverview (path))
		return;

	if(navigator)
		navigator->navigate (path);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IHtmlWriter* DocumentDiagnosticDialog::beginDocument (UrlRef path) const
{
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, IStream::kCreateMode);
	ASSERT(stream != nullptr)
	if(stream == nullptr)
		return nullptr;
	
	AutoPtr<IHtmlWriter> writer = System::CreateTextWriter<IHtmlWriter> ();
	writer->setShouldIndent (true);
	if(writer->beginDocument (*stream, Text::kUTF8) != kResultOk)
		return nullptr;

	writer->pushStyleElement (TextUtils::getCSS ());
	
	writer->startElement (String (HtmlTags::kHtml));
	writer->writeHead (XSTR (DocumentDiagnostics));
	writer->startElement (String (HtmlTags::kBody));

	return writer.detach ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::endDocument (IHtmlWriter& writer) const
{
	writer.endElement (String (HtmlTags::kBody));
	writer.endElement (String (HtmlTags::kHtml));
	writer.endDocument ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentDiagnosticDialog::writeOverview (UrlRef path) const
{
	if(navigator)
	{
		// write HTML file to be displayed in WebView
		AutoPtr<IHtmlWriter> writer = beginDocument (path);
		if(writer == nullptr)
			return false;

		AutoPtr<ITextBuilder> htmlBuilder = writer->createHtmlBuilder ();
		TextBlock block (htmlBuilder);
	
		writeOverviewContent (block);
		writer->writeMarkup (block);

		endDocument (*writer);
	}
	else
	{
		// write plain text into string param
		AutoPtr<MemoryStream> stream (NEW MemoryStream);
		{
			AutoPtr<IPlainTextWriter> writer = System::CreateTextWriter<IPlainTextWriter> ();
			if(writer->beginDocument (*stream, Text::kUTF16) != kResultOk)
				return false;

			AutoPtr<ITextBuilder> builder (writer->createPlainTextBuilder ());
			TextBlock block (builder);
	
			writeOverviewContent (block);
			writer->writeLine (block);
			writer->endDocument ();
		}

		int charCount = int(stream->getBytesWritten () / sizeof(uchar));
		String overviewString;
		overviewString.append (reinterpret_cast<const uchar*> (stream->getMemoryAddress ()), charCount);
		overviewParam->fromString (overviewString);
		return true;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::writeOverviewContent (TextBlock& block) const
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::writeCriticalItems (TextBlock& block, StringRef description) const
{
	if(criticalItems.count () == 0)
		return;

	block << Text::Paragraph (description);

	block << Text::ListBegin (Text::kUnordered);
	for(StringRef item : criticalItems)
		block << Text::ListItem (Text::kUnordered, item);
	block << Text::ListEnd (Text::kUnordered);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::writeTopItems (TextBlock& block) const
{
	if(topItems.count () == 0)
		return;

	block << Text::Paragraph (XSTR (TopItems));

	block << Text::ListBegin (Text::kUnordered);
	for(StringRef item : topItems)
		block << Text::ListItem (Text::kUnordered, item);
	block << Text::ListEnd (Text::kUnordered);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::writeData (TextBlock& block)
{
	for(DiagnosticList* list : iterate_as<DiagnosticList> (listModels))
	{
		block << Text::Heading (Text::kH2, list->getTitle ());
		list->writeHtml (block);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentDiagnosticDialog::exportCsv (DiagnosticList& list, UrlRef filePath)
{
	int mode = IStream::kWriteMode | IStream::kOpenMode;
	if(!System::GetFileSystem ().fileExists (filePath))
		mode |= IStream::kCreateMode;

	AutoPtr<IStream> fileStream = System::GetFileSystem ().openStream (filePath, mode);
	if(fileStream)
	{
		AutoPtr<ITextStreamer> textStreamer = System::CreateTextStreamer (*fileStream, {Text::kUTF8, Text::kSystemLineFormat});
		if(textStreamer)
		{
			list.writeCsv (*textStreamer);
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DocumentDiagnosticDialog::exportHtml (UrlRef filePath)
{
	AutoPtr<IHtmlWriter> writer = beginDocument (filePath);
	if(writer == nullptr)
		return false;

	AutoPtr<ITextBuilder> htmlBuilder = writer->createHtmlBuilder ();
	TextBlock block (htmlBuilder);

	block << Text::Heading (Text::kH1, XSTR (DocumentDiagnostics));
	
	writeOverviewContent (block);
	writeData (block);
	
	writer->writeMarkup (block);
	endDocument (*writer);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::exportData ()
{
	Url path;
	DocumentDiagnosticDataProvider::instance ().getReportFilePath (path, documentName, type);
	exportHtml (path);

	for(DiagnosticList* list : iterate_as<DiagnosticList> (listModels))
	{	
		DocumentDiagnosticDataProvider::instance ().getDataFilePath (path, documentName, list->getKey ());
		exportCsv (*list, path);
	}

	DocumentDiagnosticDataProvider::instance ().scanReports ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::exportCsvWithFileSelector (DiagnosticList& list)
{
	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	String fileName;
	DocumentDiagnosticDataProvider::instance ().getDataFileName (fileName, documentName, list.getKey ());
	fs->addFilter (FileTypes::Csv ());
	fs->setFileName (fileName);
	if(!fs->run (IFileSelector::kSaveFile))
		return;
	Url filePath (*fs->getPath (0));
	if(exportCsv (list, filePath))
		System::GetSystemShell ().openUrl (filePath);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::exportHtmlWithFileSelector ()
{
	AutoPtr<IFileSelector> fs = ccl_new<IFileSelector> (ClassID::FileSelector);
	String fileName;
	DocumentDiagnosticDataProvider::instance ().getReportFileName (fileName, documentName, type);
	fs->addFilter (FileTypes::Html ());
	fs->setFileName (fileName);
	if(!fs->run (IFileSelector::kSaveFile))
		return;
	Url filePath (*fs->getPath (0));
	if(exportHtml (filePath))
		System::GetSystemShell ().openUrl (filePath);
}

//************************************************************************************************
// DocumentDiagnosticsDialog::DiagnosticsList
//************************************************************************************************
	
DEFINE_CLASS_HIDDEN (DocumentDiagnosticDialog::DiagnosticList, ListViewModel)
DEFINE_CLASS_HIDDEN (DocumentDiagnosticDialog::DiagnosticList::DiagnosticListViewItem, ListViewItem)

DEFINE_STRINGID_MEMBER_ (DocumentDiagnosticDialog::DiagnosticList, kKeyID, "key")
DEFINE_STRINGID_MEMBER_ (DocumentDiagnosticDialog::DiagnosticList, kAverageID, "average")
DEFINE_STRINGID_MEMBER_ (DocumentDiagnosticDialog::DiagnosticList, kCountID, "count")
DEFINE_STRINGID_MEMBER_ (DocumentDiagnosticDialog::DiagnosticList, kTotalID, "total")

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticDialog::DiagnosticList::DiagnosticList (StringID key, StringRef title, const IDiagnosticResultSet& data, double criticalItemThreshold)
: key (key),
  title (title),
  criticalItemThreshold (criticalItemThreshold)
{
	getColumns ().addColumn (20, "", ListViewModel::kIconID, 0, IColumnHeaderList::kSortable);
	getColumns ().addColumn (200, XSTR (Name), ListViewModel::kTitleID, 0, IColumnHeaderList::kSortable);
	getColumns ().addColumn (100, XSTR (Average), kAverageID, 0, IColumnHeaderList::kSortable);
	getColumns ().addColumn (100, XSTR (Count), kCountID, 0, IColumnHeaderList::kSortable);
	getColumns ().addColumn (100, XSTR (Total), kTotalID, 0, IColumnHeaderList::kSortable);
	
	addSorter (NEW ListViewSorter (ListViewModel::kIconID, "", &sortByType));
	addSorter (NEW ListViewSorter (ListViewModel::kTitleID, XSTR (Name), &sortByName));
	addSorter (NEW ListViewSorter (kAverageID, XSTR (Average), &sortByAverage));
	addSorter (NEW ListViewSorter (kCountID, XSTR (Count), &sortByCount));
	ListViewSorter* totalSorter = NEW ListViewSorter (kTotalID, XSTR (Total), &sortByTotal);
	totalSorter->setReversed (true);
	addSorter (totalSorter);
	sortBy (totalSorter);

	ForEachUnknown (data, unk)
		UnknownPtr<IDiagnosticResult> result (unk);
		if(result)
		{
			if(result->getContext ().startsWith (DiagnosticID::kDocumentPrefix))
				continue;

			String label = DiagnosticPresentation::getLabel (result);
			if(label.isEmpty ())
			{
				// Don't display items without labels
				continue;
			}

			AutoPtr<DiagnosticListViewItem> item = NEW DiagnosticListViewItem;
			item->setTitle (label);
			item->setIcon (AutoPtr<IImage> (DiagnosticPresentation::createIcon (result)));
			item->setDiagnosticResult (result);
			item->getDetails ().set (kKeyID, key);

			String string;
			format (string, result->getAverage (), key);
			item->getDetails ().set (kAverageID, string);
			string.empty ();
			string.appendIntValue (result->getCount ());
			item->getDetails ().set (kCountID, string);
			string.empty ();
			format (string, result->getSum (), key);
			item->getDetails ().set (kTotalID, string);
			
			item->setType (result->getContext ().subString (0, result->getContext ().index ('/')));

			if(result->getAverage () > criticalItemThreshold)
				item->setCritical (true);

			addSorted (item.detach ());
		}
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::DiagnosticList::format (String& string, double value, StringID key)
{
	if(key == DiagnosticID::kSaveDuration || key == DiagnosticID::kLoadDuration)
		string.append (DiagnosticPresentation::printDuration (value));
	else if(key == DiagnosticID::kSaveSize)
		string.append (DiagnosticPresentation::printSize (value));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::DiagnosticList::writeHtml (TextBlock& block)
{
	int rowCount = countFlatItems ();
	int columnCount = 4;

	AutoPtr<ITextTable> table = block->createTable ();

	table->construct (rowCount + 1, columnCount);

	(*table)[0][0].setContent (Text::Decoration (Text::kBold, XSTR (Name)));
	(*table)[0][1].setContent (Text::Decoration (Text::kBold, XSTR (Average)));
	(*table)[0][2].setContent (Text::Decoration (Text::kBold, XSTR (Count)));
	(*table)[0][3].setContent (Text::Decoration (Text::kBold, XSTR (Total)));

	for(int row = 0; row < rowCount; row++)
	{
		DiagnosticListViewItem* item = static_cast<DiagnosticListViewItem*> (getItem (row));
		(*table)[row + 1][0].setContent (Text::Plain (item->getTitle ()));
		for(int column = 1; column < columnCount; column++)
		{
			Variant value;
			item->getDetail (value, getColumnID (column + 1));
			(*table)[row + 1][column].setContent (Text::Plain (value));
		}
	}

	block << Text::Table (table);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDialog::DiagnosticList::writeCsv (ITextStreamer& streamer)
{
	streamer.writeString (XSTR (Name));
	streamer.writeChar (',');
	streamer.writeString (XSTR (Average));
	streamer.writeChar (',');
	streamer.writeString (XSTR (Count));
	streamer.writeChar (',');
	streamer.writeLine (XSTR (Total));
	
	int rowCount = countFlatItems ();
	int columnCount = 4;

	for(int row = 0; row < rowCount; row++)
	{
		DiagnosticListViewItem* item = static_cast<DiagnosticListViewItem*> (getItem (row));
		streamer.writeString (item->getTitle ());
		for(int column = 1; column < columnCount; column++)
		{
			Variant value;
			item->getDetail (value, getColumnID (column + 1));
			
			streamer.writeChar (',');
			streamer.writeString (value);
		}
		streamer.writeLine ("");
	}
}

//************************************************************************************************
// DocumentDiagnosticLoadDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentDiagnosticLoadDialog, DocumentDiagnosticDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticLoadDialog::DocumentDiagnosticLoadDialog (const DocumentDiagnosticData& documentData,
															StringRef documentName)
: DocumentDiagnosticDialog (documentData.getType (), "CCL/DocumentDiagnosticLoadDialog", documentName)
{
	if(auto durationData = documentData.getLoadDurationData ())
	{
		addData (DiagnosticID::kLoadDuration, XSTR (LoadDurationTitle), *durationData);
		updateTopItems (*durationData);
		updateCriticalItems (*durationData);
		
		pluginsTotal = getTotal (*durationData, DiagnosticID::kClassIDPrefix);
		dataTotal = getTotal (*durationData, DiagnosticID::kFileTypePrefix);
	}
	
	if(auto documentDuration = documentData.getLoadDuration ())
		documentDuration->getValue (documentTotal, 0);

	exportLoadDurationParam = paramList.addParam ("exportLoadDuration");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDiagnosticLoadDialog::paramChanged (IParameter* param)
{
	if(param == exportLoadDurationParam)
	{
		exportCsvWithFileSelector (*static_cast<DiagnosticList*> (listModels.at (0)));
		return true;
	}

	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticLoadDialog::writeOverviewContent (TextBlock& block) const
{
	block << Text::Paragraph (String ().appendFormat (XSTR (DocumentLoadDuration),
							  documentName, DiagnosticPresentation::printDuration (documentTotal)));

	String loadDurationDistribution;
	double pluginPercentage = pluginsTotal / documentTotal.asDouble ();
	if(pluginPercentage >= 0.01)
	{
		loadDurationDistribution.appendFormat (XSTR (LoadDurationPlugins), 
									Format::Percent::print (pluginPercentage));
	}

	double dataPercentage = dataTotal / documentTotal.asDouble ();
	if(dataPercentage >= 0.01)
	{
		if(!loadDurationDistribution.isEmpty ())
			loadDurationDistribution << " ";
		loadDurationDistribution.appendFormat (XSTR (LoadDurationData), 
									Format::Percent::print (dataPercentage));
	}

	if(!loadDurationDistribution.isEmpty ())
		block << Text::Paragraph (loadDurationDistribution);

	writeCriticalItems (block, XSTR (CriticalLoadItems));
	writeTopItems (block);
}

//************************************************************************************************
// DocumentDiagnosticSaveDialog
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (DocumentDiagnosticSaveDialog, DocumentDiagnosticDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticSaveDialog::DocumentDiagnosticSaveDialog (const DocumentDiagnosticData& documentData,
															StringRef documentName)
: DocumentDiagnosticDialog (documentData.getType (), "CCL/DocumentDiagnosticSaveDialog", documentName)
{
	if(auto durationData = documentData.getSaveDurationData ())
	{
		addData (DiagnosticID::kSaveDuration, XSTR (SaveDurationTitle), *durationData);
		updateTopItems (*durationData);
		updateCriticalItems (*durationData);
		
		pluginsTotal = getTotal (*durationData, DiagnosticID::kClassIDPrefix);
		dataTotal = getTotal (*durationData, DiagnosticID::kFileTypePrefix);
	}
	
	if(auto sizeData = documentData.getSaveSizeData ())
		addData (DiagnosticID::kSaveSize, XSTR (SaveSizeTitle), *sizeData, kCriticalSizeThreshold);
	
	if(auto documentDuration = documentData.getSaveDuration ())
		documentDuration->getValue (documentTotal, 0);

	exportSaveDurationParam = paramList.addParam ("exportSaveDuration");
	exportSaveSizeParam = paramList.addParam ("exportSaveSize");
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDiagnosticSaveDialog::paramChanged (IParameter* param)
{
	if(param == exportSaveDurationParam)
	{
		exportCsvWithFileSelector (*static_cast<DiagnosticList*> (listModels.at (0)));
		return true;
	}
	else if(param == exportSaveSizeParam)
	{
		exportCsvWithFileSelector (*static_cast<DiagnosticList*> (listModels.at (1)));
		return true;
	}

	return SuperClass::paramChanged (param);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticSaveDialog::writeOverviewContent (TextBlock& block) const
{
	block << Text::Paragraph (String ().appendFormat (XSTR (DocumentSaveDuration),
							  documentName,
							  DiagnosticPresentation::printDuration (documentTotal)));

	String saveDurationDistribution;
	double pluginPercentage = pluginsTotal / documentTotal.asDouble ();
	if(pluginPercentage >= 0.01)
	{
		saveDurationDistribution.appendFormat (XSTR (SaveDurationPlugins),
									Format::Percent::print (pluginPercentage));
	}
		
	double dataPercentage = dataTotal / documentTotal.asDouble ();
	if(dataPercentage >= 0.01)
	{
		if(!saveDurationDistribution.isEmpty ())
			saveDurationDistribution << " ";
		saveDurationDistribution.appendFormat (XSTR (SaveDurationData),
									Format::Percent::print (dataPercentage));
	}
	
	if(!saveDurationDistribution.isEmpty ())
		block << Text::Paragraph (saveDurationDistribution);

	writeCriticalItems (block, XSTR (CriticalSaveItems));
	writeTopItems (block);
}

//************************************************************************************************
// DocumentDiagnosticDataProvider
//************************************************************************************************

DEFINE_SINGLETON (DocumentDiagnosticDataProvider)

const String DocumentDiagnosticDataProvider::kSubFolder = "Document Diagnostics";
const String DocumentDiagnosticDataProvider::kLoadFileName = "Load Diagnostics";
const String DocumentDiagnosticDataProvider::kSaveFileName = "Save Diagnostics";
const String DocumentDiagnosticDataProvider::kLoadDurationName = "Load Duration";
const String DocumentDiagnosticDataProvider::kSaveDurationName = "Save Duration";
const String DocumentDiagnosticDataProvider::kSaveSizeName = "Save Size";

////////////////////////////////////////////////////////////////////////////////////////////////////

DocumentDiagnosticDataProvider::DocumentDiagnosticDataProvider ()
: reportsValid (false)
{}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::getBasePath (IUrl& path) const
{
	AppSafetyManager::instance ().getDiagnosticsFolder (path);
	path.descend (kSubFolder);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::getReportFilePath (IUrl& path, StringRef documentName, DocumentDiagnosticData::Type type) const
{
	getBasePath (path);
	String fileName;
	getReportFileName (fileName, documentName, type);
	path.descend (fileName);
	path.setFileType (FileTypes::Html (), true);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::getDataFilePath (IUrl& path, StringRef documentName, StringID key) const
{
	getBasePath (path);
	String fileName;
	getDataFileName (fileName, documentName, key);
	path.descend (fileName);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::getReportFileName (String& fileName, StringRef documentName, DocumentDiagnosticData::Type type) const
{
	switch(type)
	{
	case DocumentDiagnosticData::kLoadData :
		fileName.append (kLoadFileName);
		break;
	case DocumentDiagnosticData::kSaveData :
		fileName.append (kSaveFileName);
		break;
	default :
		ASSERT (0)
		break;
	}
	fileName.appendFormat ("_%(1).%(2)", documentName, FileTypes::Html ().getExtension ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::getDataFileName (String& fileName, StringRef documentName, StringID key) const
{
	if(key == DiagnosticID::kLoadDuration)
		fileName.append (kLoadDurationName);
	else if(key == DiagnosticID::kSaveDuration)
		fileName.append (kSaveDurationName);
	else if(key == DiagnosticID::kSaveSize)
		fileName.append (kSaveSizeName);
	else
	{
		ASSERT (0)
	}
	fileName.appendFormat ("_%(1).%(2)", documentName, FileTypes::Csv ().getExtension ());
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void DocumentDiagnosticDataProvider::scanReports ()
{
	savedReports.removeAll ();

	Url basePath;
	getBasePath (basePath);
	
	FileInfo info;
	int64 now = UnixTime::getTime ();
	ForEachFile (System::GetFileSystem ().newIterator (basePath, IFileIterator::kFiles), file)
		System::GetFileSystem ().getFileInfo (info, *file);
		String fileName;
		file->getName (fileName);
		if(!fileName.startsWith (kLoadFileName) 
			&& !fileName.startsWith (kSaveFileName) 
			&& !fileName.startsWith (kLoadDurationName)
			&& !fileName.startsWith (kSaveDurationName)
			&& !fileName.startsWith (kSaveSizeName)
		)
			continue;
		if(now - UnixTime::fromLocal (info.modifiedTime) >= 7 * DateTime::kSecondsInDay)
			savedReports.add (*file);
	EndFor

	reportsValid = true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API DocumentDiagnosticDataProvider::countDiagnosticData () const
{
	if(!reportsValid)
	{
		DocumentDiagnosticDataProvider* This = const_cast<DocumentDiagnosticDataProvider*> (this);
		This->scanReports ();
	}
	return savedReports.count ();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API DocumentDiagnosticDataProvider::getDiagnosticDescription (DiagnosticDescription& description, int index) const
{
	if(index >= 0 && index < savedReports.count ())
	{
		description.categoryFlags = DiagnosticDescription::kApplicationLogs;
		savedReports[index].getName (description.fileName);
		description.subFolder = kSubFolder;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

IStream* CCL_API DocumentDiagnosticDataProvider::createDiagnosticData (int index)
{	
	if(index >= 0 && index < savedReports.count ())
		return System::GetFileSystem ().openStream (savedReports[index]);
	return nullptr;
}
