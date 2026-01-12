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
// Filename    : ccl/platform/win/gui/printservice.win.h
// Description : Win32 Native Print Job Data
//
//************************************************************************************************

#ifndef _ccl_printservice_win_h
#define _ccl_printservice_win_h

#include "ccl/base/object.h"
#include "ccl/gui/graphics/printservice.h"
#include "ccl/platform/win/cclwindows.h"
#include "ccl/public/base/buffer.h"

namespace CCL {
namespace Win32 {

//************************************************************************************************
// DeviceNames
//************************************************************************************************

struct DeviceNames
{
	PROPERTY_STRING (driverName, DriverName)
	PROPERTY_STRING (deviceName, DeviceName)
	PROPERTY_STRING (outputName, OutputName)

	void fromHDevNames (HGLOBAL hDevNames);
};

//************************************************************************************************
// Win32PrintService
//************************************************************************************************

class Win32PrintService: public PrintService
{
public:
	static Win32PrintService& instance () {return (Win32PrintService&) PrintService::instance (); }

	Win32PrintService ();
	~Win32PrintService ();

	bool getDefaultPrinterData (HGLOBAL& hDevNames, HGLOBAL& hDevMode) const;
	bool getRecentPrinterData (HGLOBAL& hDevNames, HGLOBAL& hDevMode, bool useDefaultIfNotSet) const;
	bool setRecentPrinterData (HGLOBAL hDevNames, HGLOBAL hDevMode);
	bool getPdfPrinterData (DeviceNames& devNames, HGLOBAL& devModeData) const;

	// PrintService
	IPrintJob* CCL_API createPrintJob () override;
	tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) override;
	IPageSetupDialog* CCL_API createPageSetupDialog () override;
	Features CCL_API getSupportedFeatures () const override;
	IPrintJob* CCL_API createPdfPrintJob (UrlRef path) override;

protected:
	HGLOBAL hRecentDevNames;
	HGLOBAL hRecentDevMode;

	DeviceNames pdfPrinterNames;
	HGLOBAL hPdfDevMode;	
	bool pdfFeatureChecked;

	bool checkPdfSupport ();
};

//************************************************************************************************
// Win32PrintJobData
//************************************************************************************************

class Win32PrintJobData: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Win32PrintJobData, Object)

	Win32PrintJobData ();
	~Win32PrintJobData ();
	tresult setup (const PrinterDocumentInfo& docInfo, IPrintJob::JobMode jobMode, IWindow* window, CCL::IStream* pdfOutputFile);

	PROPERTY_STRING (jobName, JobName)
	DeviceNames deviceNames;	
	PROPERTY_VARIABLE (HGLOBAL, hDevMode, HDevMode)
	PROPERTY_VARIABLE (PointF, pageSize, PageSize)
	SharedPtr<CCL::IStream> pdfOutputFile;

	struct PageRage
	{
		int pageFrom;
		int pageTo;
	};
	static const int kMaxPageRanges = 8;
	PageRage pageRanges[kMaxPageRanges];
	int pageRangeCount;

	bool getPageSizes (PointF& paperSize, RectF& printablePageArea) const; // in millimeter
	PageOrientation getDocumentOrientation () const;
};

//************************************************************************************************
// Win32PrintJobExecutor
//************************************************************************************************

class Win32PrintJobExecutor: public Object
{
public:
	DECLARE_CLASS_ABSTRACT (Win32PrintJobExecutor, Object)

	virtual bool init (Win32PrintJobData* jobData) = 0;
	virtual tresult runPrintJob (IPageRenderer* renderer) = 0;
};

//************************************************************************************************
// DevMode
//************************************************************************************************

class DevMode
{
public:
	DevMode (HGLOBAL hDevMode);

	~DevMode ();

	operator PDEVMODE () {return devMode;}
	PDEVMODE operator -> () {return devMode;}

	PageOrientation getOrientation () const;
	bool setOrientation (PageOrientation orientation);

	bool getPaperSize (PointF& size, bool respectOrientation) const; // mm
	bool setPaperSize (PointFRef size);
	bool setSymblicPaperSize (short symbolicWin32Format);
	bool getSymbolicPaperSize (short& symbolicWin32Format, PointFRef size);

	static bool getPhysicalPaperSize (PointF& size, short symbolicWin32Format);
	static bool win32ToCCLSymbolicPaperFormat (int& cclFormat, short win32Format);
	static bool cclToWin32SymbolicPaperFormat (short& win32Format, int cclFormat);

private:
	HGLOBAL hDevMode;
	PDEVMODE devMode;

	struct NativeSizeMapItem;
	static NativeSizeMapItem formatMap[];
};

//************************************************************************************************
// DevNames
//************************************************************************************************

class DevNames
{
public:
	DevNames (HGLOBAL hDevNames);
	~DevNames ();

	operator LPDEVNAMES () {return devNames;}
	LPDEVNAMES operator -> () {return devNames;}

	PCWSTR getDriverName () const;
	PCWSTR getDeviceName () const;
	PCWSTR getOutputName () const;

private:
	HGLOBAL hDevNames;
	LPDEVNAMES devNames;
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_printservice_win_h
