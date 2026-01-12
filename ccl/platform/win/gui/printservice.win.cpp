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
// Filename    : ccl/platform/win/gui/printerservice.win.cpp
// Description : platform-specific printer dialgog code
//
//************************************************************************************************

#include "ccl/platform/win/gui/printservice.win.h"

#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/systemwindow.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/systemservices.h"

#include "ccl/public/math/mathprimitives.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// Win32PageSetupDialog
//************************************************************************************************

class Win32PageSetupDialog: public PageSetupDialog
{
public:
	DECLARE_CLASS (Win32PageSetupDialog, PageSetupDialog)

	// PageSetupDialog
	tbool CCL_API run (PageSetup& pageSetup, IWindow* window) override;
};

//************************************************************************************************
// Win32PrintJob
//************************************************************************************************

class Win32PrintJob: public PrintJob
{
public:
	DECLARE_CLASS_ABSTRACT (Win32PrintJob, PrintJob)

	Win32PrintJob (CCL::IStream* pdfStream = nullptr)
	: pdfStream (pdfStream) {}

	// PrintJob
	tresult CCL_API run (const PrinterDocumentInfo& docData, IPageRenderer* renderer, JobMode mode, IWindow* window) override;

protected:
	SharedPtr<CCL::IStream> pdfStream;	
};

} // namespace Win32
} // namespace CCL

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// Win32PrintService
//************************************************************************************************

DEFINE_EXTERNAL_SINGLETON (PrintService, Win32PrintService)

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32PrintService::Win32PrintService ()
: hRecentDevNames (nullptr),
  hRecentDevMode (nullptr), 
  hPdfDevMode (nullptr),
  pdfFeatureChecked (false)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32PrintService::~Win32PrintService ()
{
	if(hPdfDevMode)
		GlobalFree (hPdfDevMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintService::getDefaultPrinterData (HGLOBAL& hDevNames, HGLOBAL& hDevMode) const
{
	PRINTDLG printDlg = {0};
	printDlg.lStructSize = sizeof(PRINTDLG);
	printDlg.Flags = PD_RETURNDEFAULT;
	if(PrintDlg (&printDlg) == false)
		return false;

	hDevNames = printDlg.hDevNames;
	hDevMode = printDlg.hDevMode;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintService::getRecentPrinterData (HGLOBAL& hDevNames, HGLOBAL& hDevMode, bool useDefaultIfNotSet) const
{
	if((hRecentDevNames == nullptr || hRecentDevMode == nullptr) && useDefaultIfNotSet)
		return getDefaultPrinterData (hDevNames, hDevMode);

	hDevNames = hRecentDevNames;
	hDevMode = hRecentDevMode;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintService::setRecentPrinterData (HGLOBAL hDevNames, HGLOBAL hDevMode)
{
	ASSERT (hDevNames && hDevMode)
	if(hDevNames == nullptr || hDevMode == nullptr)
		return false;

	hRecentDevNames = hDevNames;
	hRecentDevMode = hDevMode;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintService::getPdfPrinterData (DeviceNames& devNames, HGLOBAL& devModeData) const
{
	if(hPdfDevMode)
	{
		devModeData = hPdfDevMode;
		devNames = pdfPrinterNames;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API Win32PrintService::createPrintJob ()
{
	return NEW Win32PrintJob;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Win32PrintService::getDefaultPrinterInfo (PrinterInfo& info)
{
	HGLOBAL hDevNames = nullptr;
	HGLOBAL hDevMode = nullptr;
	if(getDefaultPrinterData (hDevNames, hDevMode) == false)
		return kResultFailed;

	DevNames devNames (hDevNames);
	info.name = devNames.getDeviceName ();

	if(info.name.isEmpty ())
		return kResultFalse;

	DevMode devMode (hDevMode);
	PointF paperSize;
	if(devMode.getPaperSize (paperSize, false))
	{
		auto& format = lookupPaperFormatBySize (paperSize, devMode.getOrientation ());
		info.paperFormat = format.symbolic;
	}
	info.orientation = devMode.getOrientation ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPageSetupDialog* CCL_API Win32PrintService::createPageSetupDialog ()
{
	return NEW Win32PageSetupDialog;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintService::checkPdfSupport ()
{
	if(pdfFeatureChecked)
		return hPdfDevMode != nullptr;

	DWORD bytesNeeded = 0, count = 0;
	EnumPrinters (PRINTER_ENUM_LOCAL, nullptr, 2, nullptr , 0, &bytesNeeded, &count);
	if(bytesNeeded > 0)
	{			
		int infoCount = (bytesNeeded / sizeof(PRINTER_INFO_2)) + 1;
		Vector<PRINTER_INFO_2> infoData (infoCount, 1);
		int bytesAvail = sizeof(PRINTER_INFO_2) * infoCount;

		if(EnumPrinters(PRINTER_ENUM_LOCAL, nullptr, 2, (LPBYTE) infoData.getItems (), bytesAvail, &bytesNeeded, &count))
		{
			int infoCount = count;
			int infoIndex = -1;
				
			for(int i = infoCount - 1; i >= 0; i--)
			{
				// ignore terminal server (remote desktop) connected printers
				if(infoData[i].Attributes & PRINTER_ATTRIBUTE_TS)
					continue;

				String printerName (infoData[i].pPrinterName);
				if(printerName.contains (" PDF", false))
				{
					infoIndex = i;

					if(printerName.contains ("Microsoft "))
						break;
				}
			}

			if(infoIndex >= 0)
			{
				LPDEVMODE devMode = infoData[infoIndex].pDevMode;
				ULONG devModesize = devMode->dmSize + devMode->dmDriverExtra; // Size of DEVMODE in bytes, including private driver data.
				hPdfDevMode = GlobalAlloc (GMEM_FIXED, devModesize);
				memcpy (hPdfDevMode, devMode, devModesize);

				pdfPrinterNames.setDeviceName (infoData[infoIndex].pPrinterName);
				pdfPrinterNames.setDriverName (infoData[infoIndex].pDriverName);
				pdfPrinterNames.setOutputName (infoData[infoIndex].pPortName);
			}
		}
	}
	pdfFeatureChecked = true;

	return hPdfDevMode != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32PrintService::Features CCL_API Win32PrintService::getSupportedFeatures () const
{
	Features features = kPrinting;

	if(const_cast<Win32PrintService&> (*this).checkPdfSupport ())
		features |= kPdfCreation;

	return features;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API Win32PrintService::createPdfPrintJob (UrlRef path)
{
	if(checkPdfSupport ())
	{
		AutoPtr<IStream> stream = System::GetFileSystem ().openStream (path, CCL::IStream::kCreateMode);
		if(stream == nullptr)
			return nullptr;
		
		return NEW Win32PrintJob (stream);
	}

	return nullptr;
}

//************************************************************************************************
// Win32PageSetupDialog
//************************************************************************************************

DEFINE_CLASS (Win32PageSetupDialog, PageSetupDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Win32PageSetupDialog::run (PageSetup& setup, IWindow* window)
{
	if(!window)
		window = Desktop.getDialogParentWindow ();

	PAGESETUPDLG setupDlg = {0};
	setupDlg.lStructSize = sizeof(PAGESETUPDLG);
	setupDlg.hwndOwner = window ? (HWND)window->getSystemWindow () : nullptr;

	if(Win32PrintService::instance ().getRecentPrinterData (setupDlg.hDevNames, setupDlg.hDevMode, true) == false)
		return false;

	setupDlg.Flags = PSD_MARGINS;

	uchar info [2];
	GetLocaleInfo (LOCALE_USER_DEFAULT, LOCALE_IMEASURE, info, 2);
	bool useMetricsUS = (*info == '1'); // see LOCALE_IMEASURE documentation - means "US metrics"

	{
		DevMode devModeObj (setupDlg.hDevMode);
		devModeObj.setOrientation (setup.orientation);

		if(setup.size.x > 0 && setup.size.y > 0)
			devModeObj.setPaperSize (setup.size);
	}

	typedef LONG (* FromMMFunc)(float);
	typedef float (* ToMMFunc)(LONG);
	FromMMFunc convertFromMM = nullptr;
	ToMMFunc convertToMM = nullptr;

	if(useMetricsUS)
	{
		convertFromMM = [] (float mm)
		{
			return ccl_to_int<LONG> (Math::millimeterToInch (mm) * 1000.0);
		};
		convertToMM = [] (LONG thOfInches)
		{
			return Math::inchToMillimeter<float> (thOfInches / 1000.f);
		};
		setupDlg.Flags |= PSD_INTHOUSANDTHSOFINCHES;
	}
	else
	{
		convertFromMM = [] (float mm)
		{
			return ccl_to_int<LONG> (mm * 100.0);
		};
		convertToMM = [] (LONG thOfmm)
		{
			return thOfmm / 100.f;
		};
		setupDlg.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
	}

	auto& margins = setup.margins;

	setupDlg.rtMargin.left = convertFromMM (margins.left);
	setupDlg.rtMargin.top = convertFromMM (margins.top);
	setupDlg.rtMargin.right = convertFromMM (margins.right);
	setupDlg.rtMargin.bottom = convertFromMM (margins.bottom);

	if(PageSetupDlg (&setupDlg) == false)
		return false;

	Win32PrintService::instance ().setRecentPrinterData (setupDlg.hDevNames, setupDlg.hDevMode);

	{
		DevMode devModeObj (setupDlg.hDevMode);
		setup.orientation = devModeObj.getOrientation ();
	}

	setup.size.x = convertToMM (setupDlg.ptPaperSize.x);
	setup.size.y = convertToMM (setupDlg.ptPaperSize.y);
	margins.left = convertToMM (setupDlg.rtMargin.left);
	margins.top = convertToMM (setupDlg.rtMargin.top);
	margins.right = convertToMM (setupDlg.rtMargin.right);
	margins.bottom = convertToMM (setupDlg.rtMargin.bottom);

	return true;
}

//************************************************************************************************
// Win32PrintJobExecutor
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Win32PrintJobExecutor, Object)

//************************************************************************************************
// Win32PrintJob
//************************************************************************************************

DEFINE_CLASS_HIDDEN (Win32PrintJob, PrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API Win32PrintJob::run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode jobMode, IWindow* window)
{
	if(docInfo.isValid () == false)
		return kResultInvalidArgument;

	AutoPtr<Object> printJobObject = NativeGraphicsEngine::instance ().createPrintJob ();
	Win32PrintJobExecutor* jobExecutor = ccl_cast<Win32PrintJobExecutor> (printJobObject);
	if(jobExecutor == nullptr)
		return kResultFailed;

	AutoPtr<Win32PrintJobData> jobData = NEW Win32PrintJobData;
	tresult setupResult = jobData->setup (docInfo, jobMode, window, pdfStream);
	if(setupResult != kResultOk)
		return setupResult;

	if(jobExecutor->init (jobData) == false)
		return kResultFailed;

	return jobExecutor->runPrintJob (renderer);
}

//************************************************************************************************
// Win32PrintJobData
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (Win32PrintJobData, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32PrintJobData::Win32PrintJobData ()
: pageRangeCount (0),
  hDevMode (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Win32PrintJobData::~Win32PrintJobData ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Win32PrintJobData::getPageSizes (PointF& paperSize, RectF& printablePageArea) const
{
	DevMode devMode (hDevMode);

	if(devMode)
	{
		if(devMode.getPaperSize (paperSize, true) == false)
			return false;

		StringChars driverName (deviceNames.getDriverName ());
		StringChars deviceName (deviceNames.getDeviceName ());
		StringChars outputName (deviceNames.getOutputName ());

		HDC ic = CreateIC (driverName, deviceName, outputName, devMode);
		if(ic)
		{
			printablePageArea.left = float (GetDeviceCaps (ic, PHYSICALOFFSETX)) *
				  float (GetDeviceCaps (ic, HORZSIZE)) / float (GetDeviceCaps (ic, HORZRES));

			printablePageArea.top = float (GetDeviceCaps (ic, PHYSICALOFFSETY)) *
				  float (GetDeviceCaps (ic, VERTSIZE)) / float (GetDeviceCaps (ic, VERTRES));

			printablePageArea.setWidth (float (GetDeviceCaps (ic, HORZSIZE)) - 0.5f);
			printablePageArea.setHeight (float (GetDeviceCaps (ic, VERTSIZE)) - 0.5f);

			DeleteDC (ic);

			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PageOrientation Win32PrintJobData::getDocumentOrientation () const
{
	DevMode devMode (hDevMode);

	if(!devMode)
		return kPageOrientationUnknown;

	return devMode.getOrientation ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult Win32PrintJobData::setup (const PrinterDocumentInfo& docInfo, IPrintJob::JobMode jobMode, IWindow* window, IStream* _pdfStream)
{
	pdfOutputFile = _pdfStream;
	
	if(jobMode == IPrintJob::kJobModeNormal && pdfOutputFile == nullptr)
	{
		// Bring up Print Dialog and receive user print settings.

		if(!window)
			window = Desktop.getDialogParentWindow ();

		PRINTDLGEX printDialogEx = {0};
		printDialogEx.lStructSize = sizeof(PRINTDLGEX);
		printDialogEx.Flags = PD_HIDEPRINTTOFILE | PD_USEDEVMODECOPIESANDCOLLATE;

		Win32PrintService::instance ().getRecentPrinterData (printDialogEx.hDevNames, printDialogEx.hDevMode, false);

		printDialogEx.Flags |= PD_NOSELECTION;

		PRINTPAGERANGE dlgPageRanges[Win32PrintJobData::kMaxPageRanges];

		if(docInfo.hasValidPageRange () && docInfo.minPage != docInfo.maxPage)
		{
			printDialogEx.nMinPage = docInfo.minPage + 1;
			printDialogEx.nMaxPage = docInfo.maxPage + 1;

			printDialogEx.nPageRanges = 0;
			printDialogEx.nMaxPageRanges = Win32PrintJobData::kMaxPageRanges;
			printDialogEx.lpPageRanges = dlgPageRanges;
		}
		else
			printDialogEx.Flags |= PD_NOPAGENUMS;

		if(docInfo.hasValidCurrentPage () == false)
			printDialogEx.Flags |= PD_NOCURRENTPAGE;

		printDialogEx.hwndOwner = window ? (HWND)window->getSystemWindow () : nullptr;
		printDialogEx.nStartPage = START_PAGE_GENERAL;

		AutoPtr<IWindow> systemWindow (NEW ModalSystemWindow);

		HRESULT hrPrintDlgEx = PrintDlgEx(&printDialogEx);
		if(FAILED(hrPrintDlgEx))
			return kResultFailed;

		if(printDialogEx.hDevNames == nullptr || printDialogEx.hDevMode == nullptr)
			return kResultFailed;

		if(printDialogEx.dwResultAction == PD_RESULT_CANCEL)
		{
			// User clicks the Cancel button.
			return kResultAborted;
		}

		Win32PrintService::instance ().setRecentPrinterData (printDialogEx.hDevNames, printDialogEx.hDevMode);

		if(printDialogEx.dwResultAction == PD_RESULT_APPLY)
		{
			// User clicks the Apply button and later clicks the Cancel button.
			return kResultAborted;
		}

		deviceNames.fromHDevNames (printDialogEx.hDevNames);
		
		setHDevMode (printDialogEx.hDevMode);

		if(docInfo.hasValidCurrentPage () && (printDialogEx.Flags & PD_CURRENTPAGE) != 0)
		{
			pageRangeCount = 1;
			pageRanges[0].pageFrom = pageRanges[0].pageTo = docInfo.currentPage;
		}

		else if(docInfo.hasValidPageRange ())
		{
			// all pages
			pageRangeCount = 1;
			pageRanges[0].pageFrom = docInfo.minPage;
			pageRanges[0].pageTo = docInfo.maxPage;

			// range
			if((printDialogEx.Flags & PD_PAGENUMS) != 0)
			{
				pageRangeCount = printDialogEx.nPageRanges;

				for(int i = 0; i < pageRangeCount; i++)
				{
					pageRanges[i].pageFrom = dlgPageRanges[i].nFromPage - 1;
					pageRanges[i].pageTo = dlgPageRanges[i].nToPage - 1;
				}
			}
		}
	}
	else if(jobMode == IPrintJob::kJobModeSilent)
	{
		if(pdfOutputFile)
		{
			if(Win32PrintService::instance ().getPdfPrinterData (deviceNames, hDevMode) == false)
				return kResultFailed;
		}
		else
		{
			// Use default printer and take page range directly from doc info.
			HGLOBAL hDevNames;			
			if(Win32PrintService::instance ().getDefaultPrinterData (hDevNames, hDevMode) == false)
				return kResultFailed;

			deviceNames.fromHDevNames (hDevNames);				
		}
		
		pageRangeCount = 1;
		if(docInfo.hasValidCurrentPage ())
		{
			pageRanges[0].pageFrom = pageRanges[0].pageTo = docInfo.currentPage;
		}
		else if(docInfo.hasValidPageRange ())
		{
			pageRanges[0].pageFrom = docInfo.minPage;
			pageRanges[0].pageTo = docInfo.maxPage;
		}
	}
	else
	{
		ASSERT (0)
		return kResultNotImplemented;
	}

	setJobName (docInfo.name);
	setPageSize (docInfo.pageSize); // pass page size

	if(!pageSize.isNull ())
	{
		PointF size = pageSize;
		if(size.x > size.y)
			ccl_swap (size.x, size.y);

		DevMode devMode (hDevMode);
		devMode.setPaperSize (size);
		devMode.setOrientation (pageSize.x <= pageSize.y ? kPageOrientationPortrait : kPageOrientationLandscape);
	}

	return kResultOk;
}

//************************************************************************************************
// DevMode
//************************************************************************************************

struct DevMode::NativeSizeMapItem
{
	short win32Format;
	int cclFormat;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

DevMode::NativeSizeMapItem DevMode::formatMap[] =
{
	{DMPAPER_LETTER,		kPaperFormatLetter},
	{DMPAPER_LETTER_EXTRA,	kPaperFormatLetterExtra},
	{DMPAPER_LETTER_PLUS,	kPaperFormatLetterPlus},
	{DMPAPER_TABLOID,		kPaperFormatTabloid},
	{DMPAPER_TABLOID_EXTRA, kPaperFormatTabloidExtra},
	{DMPAPER_LEDGER,		kPaperFormatLedger},
	{DMPAPER_LEGAL,			kPaperFormatLegal},
	{DMPAPER_LEGAL_EXTRA,	kPaperFormatLegalExtra},
	{DMPAPER_STATEMENT,		kPaperFormatStatement},
	{DMPAPER_EXECUTIVE,		kPaperFormatExecutive},

	{DMPAPER_A2,			kPaperFormatA2},
	{DMPAPER_A3,			kPaperFormatA3},
	{DMPAPER_A3_EXTRA, 		kPaperFormatA3Extra},
	{DMPAPER_A4,			kPaperFormatA4},
	{DMPAPER_A4_EXTRA,		kPaperFormatA4Extra},
	{DMPAPER_A4_PLUS,		kPaperFormatA4Plus},
	{DMPAPER_A5,			kPaperFormatA5},
	{DMPAPER_A5_EXTRA,		kPaperFormatA5Extra},
	{DMPAPER_A6,			kPaperFormatA6},
	{DMPAPER_A_PLUS,		kPaperFormatAPlus},

	{DMPAPER_B4,			kPaperFormatB4},
	{DMPAPER_B5,			kPaperFormatB5},
	{DMPAPER_B_PLUS,		kPaperFormatBPlus},

	{DMPAPER_FOLIO, 		kPaperFormatFolio},
	{DMPAPER_QUARTO, 		kPaperFormatQuarto},
	{DMPAPER_JAPANESE_POSTCARD,	kPaperFormatJapanesePostcard},
	{DMPAPER_10X14,			kPaperFormat10x14Inch},
	{DMPAPER_11X17,			kPaperFormat11x17Inch},
	{DMPAPER_9X11,			kPaperFormat9x11Inch},
	{DMPAPER_10X11,			kPaperFormat10x11Inch},
	{DMPAPER_15X11,			kPaperFormat15x11Inch},
	{DMPAPER_NOTE, 			kPaperFormatNote},
	{DMPAPER_ISO_B4,		kPaperFormatIsoB4}


#if 0
	{DMPAPER_ENV_9, {Math::inchToMillimeter (3.f + (7.f/8.f)), Math::inchToMillimeter (8.f + (7.f/8.f))}},
	{DMPAPER_ENV_10, {Math::inchToMillimeter (4.f + (1.f/8.f)), Math::inchToMillimeter (9.5f)}},
	{DMPAPER_ENV_11, {Math::inchToMillimeter (4.5f), Math::inchToMillimeter (10.f + (3.f/8.f))}},
	{DMPAPER_ENV_12, {Math::inchToMillimeter (4.75f), Math::inchToMillimeter (11.f)}},
	{DMPAPER_ENV_14, {Math::inchToMillimeter (5.f), Math::inchToMillimeter (11.5f)}},
	{DMPAPER_ENV_DL, {110.f, 220.f}},
	{DMPAPER_ENV_C5, {162.f, 229.f}},
	{DMPAPER_ENV_C3, {324.f, 458.f}},
	{DMPAPER_ENV_C4, {229.f, 324.f}},
	{DMPAPER_ENV_C6, {114.f, 162.f}},
	{DMPAPER_ENV_C65, {114.f, 229.f}},
	{DMPAPER_ENV_B4, {250.f, 353.f}},
	{DMPAPER_ENV_B5, {176.f, 250.f}},
	{DMPAPER_ENV_B6, {176.f, 125.f}},
	{DMPAPER_ENV_ITALY, {110.f, 230.f}},
	{DMPAPER_ENV_MONARCH, {Math::inchToMillimeter (3.875f), Math::inchToMillimeter (7.5f)}},
	{DMPAPER_ENV_PERSONAL, {Math::inchToMillimeter (3.f + (5.f/8.f)), Math::inchToMillimeter (6.5f)}},
	{DMPAPER_FANFOLD_US, {Math::inchToMillimeter (14.f + (7.f/8.f)), Math::inchToMillimeter (11.f)}},
	{DMPAPER_FANFOLD_STD_GERMAN, {Math::inchToMillimeter (8.5f), Math::inchToMillimeter (12.f)}},
	{DMPAPER_FANFOLD_LGL_GERMAN, {Math::inchToMillimeter (8.5f), Math::inchToMillimeter (13.f)}},
	{DMPAPER_JAPANESE_POSTCARD, {100.f, 148.f}},
	{DMPAPER_ENV_INVITE, {220.f, 220.f}},
	{DMPAPER_LETTER_TRANSVERSE, {Math::inchToMillimeter (8.5f), Math::inchToMillimeter (11.f)}},
	{DMPAPER_A4_TRANSVERSE, {210.f, 297.f}},
	{DMPAPER_LETTER_EXTRA_TRANSVERSE, {Math::inchToMillimeter (9.5f), Math::inchToMillimeter (12.f)}},
	{DMPAPER_A3_TRANSVERSE, {297.f, 420.f}},
	{DMPAPER_A3_EXTRA_TRANSVERSE, {322.f, 445.f}},
	{DMPAPER_DBL_JAPANESE_POSTCARD, {200.f, 148.f}},

#endif
};

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::cclToWin32SymbolicPaperFormat (short& win32Format, int cclFormat)
{
	for(int i = 0; i < ARRAY_COUNT (formatMap); i++)
	{
		if(formatMap[i].cclFormat == cclFormat)
		{
			win32Format = formatMap[i].win32Format;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::win32ToCCLSymbolicPaperFormat (int& cclFormat, short win32Format)
{
	for(int i = 0; i < ARRAY_COUNT (formatMap); i++)
	{
		if(formatMap[i].win32Format == win32Format)
		{
			cclFormat = formatMap[i].cclFormat;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::getPhysicalPaperSize (PointF& point, short symbolicWin32Format)
{
	int cclPaperFormat;
	if(win32ToCCLSymbolicPaperFormat (cclPaperFormat, symbolicWin32Format))
	{
		auto& format = PrintService::instance ().getPaperFormat (cclPaperFormat);
		if(format.isValid ())
		{
			point = format.size;
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::getSymbolicPaperSize (short& symbolicWin32Format, PointFRef size)
{
	auto& format = PrintService::instance ().lookupPaperFormatBySize (size, getOrientation ());
	if(format.isValid ())
		return cclToWin32SymbolicPaperFormat (symbolicWin32Format, format.symbolic);

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DevMode::DevMode (HGLOBAL _hDevMode)
: hDevMode (_hDevMode)
{
	devMode = reinterpret_cast<DEVMODE*>(GlobalLock(hDevMode));   // retrieve DevMode
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DevMode::~DevMode ()
{
	if(hDevMode)
		GlobalUnlock (hDevMode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PageOrientation DevMode::getOrientation () const
{
	if(devMode)
		if(devMode->dmFields & DM_ORIENTATION)
			return devMode->dmOrientation == DMORIENT_PORTRAIT ? kPageOrientationPortrait : kPageOrientationLandscape;

	return kPageOrientationPortrait;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::setOrientation (PageOrientation orientation)
{
	if(devMode)
	{
		devMode->dmOrientation = (orientation == kPageOrientationPortrait ? DMORIENT_PORTRAIT : DMORIENT_LANDSCAPE);
		devMode->dmFields |= DM_ORIENTATION;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::getPaperSize (PointF& size, bool respectOrientation) const
{
	if(devMode)
	{
		bool success = false;
		if((devMode->dmFields & DM_PAPERLENGTH) && (devMode->dmFields & DM_PAPERWIDTH))
		{
			size.x = devMode->dmPaperWidth / 10.f; // in 10th mm
			size.y = devMode->dmPaperLength / 10.f; // in 10th mm
			success = true;
		}
		else if(devMode->dmFields & DM_PAPERSIZE)
		{
			success = getPhysicalPaperSize (size, devMode->dmPaperSize);
		}

		if(success && respectOrientation)
		{
			if(getOrientation () == kPageOrientationLandscape)
				ccl_swap (size.x, size.y);
		}

		return success;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::setSymblicPaperSize (short symbolicWin32Format)
{
	if(devMode)
	{
		devMode->dmPaperSize = symbolicWin32Format;
		devMode->dmFields |= DM_PAPERSIZE;
		devMode->dmFields &= ~DM_PAPERLENGTH;
		devMode->dmFields &= ~DM_PAPERWIDTH;
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DevMode::setPaperSize (PointFRef size)
{
	if(devMode)
	{
		short symbolicSize = 0;
		if(getSymbolicPaperSize (symbolicSize, size))
			return setSymblicPaperSize (symbolicSize);
		else
		{
			devMode->dmPaperWidth = ccl_to_int<short> (size.x * 10.f); // in 10th mm
			devMode->dmPaperLength = ccl_to_int<short> (size.y * 10.f); // in 10th mm
			devMode->dmFields &= ~DM_PAPERSIZE;
			devMode->dmFields |= DM_PAPERLENGTH;
			devMode->dmFields |= DM_PAPERWIDTH;
		}
		return true;
	}
	return false;
}

//************************************************************************************************
// DevNames
//************************************************************************************************

DevNames::DevNames (HGLOBAL _hDevNames)
: hDevNames (_hDevNames)
{
	devNames = reinterpret_cast<LPDEVNAMES>(GlobalLock(hDevNames));   // retrieve DEVNAMES
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DevNames::~DevNames ()
{
	if(hDevNames)
		GlobalUnlock (hDevNames);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PCWSTR DevNames::getDriverName () const
{
	if(devNames)
		return reinterpret_cast<PCWSTR>(devNames) + devNames->wDriverOffset;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PCWSTR DevNames::getDeviceName () const
{
	if(devNames)
		return reinterpret_cast<PCWSTR>(devNames) + devNames->wDeviceOffset;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PCWSTR DevNames::getOutputName () const
{
	if(devNames)
		return reinterpret_cast<PCWSTR>(devNames) + devNames->wOutputOffset;

	return nullptr;
}

//************************************************************************************************
// DeviceNames
//************************************************************************************************

void DeviceNames::fromHDevNames (HGLOBAL hDevNames)
{
	DevNames devNames (hDevNames);
	setDriverName (devNames.getDriverName ());
	setDeviceName (devNames.getDeviceName ());
	setOutputName (devNames.getOutputName ());
}
