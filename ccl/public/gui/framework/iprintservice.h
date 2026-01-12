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
// Filename    : ccl/public/gui/framework/iprintservice.h
// Description : Printer Interface
//
//************************************************************************************************

#ifndef _ccl_iprintservice_h
#define _ccl_iprintservice_h

#include "ccl/public/base/iunknown.h"
#include "ccl/public/base/cclmacros.h"
#include "ccl/public/text/cclstring.h"
#include "ccl/public/gui/graphics/rect.h"

namespace CCL {

interface IWindow;
interface IGraphics;

//************************************************************************************************
// PageOrientation
//************************************************************************************************

DEFINE_ENUM (PageOrientation)
{
	kPageOrientationPortrait,
	kPageOrientationLandscape,
	kPageOrientationUnknown
};

//************************************************************************************************
// PageSetup
//************************************************************************************************

struct PageSetup
{
	PointF size; // in mm
	RectF margins; // in mm
	PageOrientation orientation = kPageOrientationPortrait;

	bool isValid () const { return !size.isNull (); }
};

//************************************************************************************************
// SymbolicPaperFormat - See PaperFormat
//************************************************************************************************

DEFINE_ENUM (SymbolicPaperFormat)
{
	kPaperFormatUnknown,
	kPaperFormatLetter,
	kPaperFormatLetterExtra,
	kPaperFormatLetterPlus,
	kPaperFormatTabloid,
	kPaperFormatTabloidExtra,
	kPaperFormatLedger,
	kPaperFormatLegal,
	kPaperFormatLegalExtra,
	kPaperFormatStatement,
	kPaperFormatExecutive,
	kPaperFormatA2,
	kPaperFormatA3,
	kPaperFormatA3Extra,
	kPaperFormatA4,
	kPaperFormatA4Plus,
	kPaperFormatA4Extra,
	kPaperFormatA5,
	kPaperFormatA5Extra,
	kPaperFormatA6,
	kPaperFormatAPlus,
	kPaperFormatB4,
	kPaperFormatB5,
	kPaperFormatB5Extra,
	kPaperFormatBPlus,
	kPaperFormatIsoB4,
	kPaperFormatFolio,
	kPaperFormatQuarto,
	kPaperFormatNote,
	kPaperFormatJapanesePostcard,
	kPaperFormat9x11Inch,
	kPaperFormat10x11Inch,
	kPaperFormat15x11Inch,
	kPaperFormat10x14Inch,
	kPaperFormat11x17Inch
};

//************************************************************************************************
// PaperFormat - see IPrintService::getPaperFormat
/** The format size can be used to initialize documents or the page setup dialog. This is optional. */
//************************************************************************************************

struct PaperFormat
{
	SymbolicPaperFormat symbolic = kPaperFormatUnknown;	///< symbolic format
	String name;					                    ///< name of format (not localized)
	PointF size;					                    ///< size of format in mm

	bool isValid () const { return symbolic != kPaperFormatUnknown; }
	PageOrientation getFormatOrientation () const { return size.x > size.y ? kPageOrientationLandscape : kPageOrientationPortrait; }
};

//************************************************************************************************
// IPageSetupDialog 
//************************************************************************************************

interface IPageSetupDialog: IUnknown
{
	/** Run platform page setup dialog (blocking) */
	virtual tbool CCL_API run (PageSetup& pageSetup, IWindow* window = nullptr) = 0;

	DECLARE_IID (IPageSetupDialog)
};

DEFINE_IID (IPageSetupDialog, 0x5ee39736, 0xaf2f, 0x4ce4, 0xbb, 0xb2, 0xfd, 0x9e, 0x2, 0xc3, 0xa5, 0x87)

//************************************************************************************************
// IPageRenderer
/** Printer Page Renderer. Used as callback interface for a print job. See IPrintJob */
//************************************************************************************************

interface IPageRenderer: IUnknown
{
	struct PageRenderData
	{
		IGraphics& graphics;			///< printer graphics context
		int pageNumber;					///< page number starting at 0	
		float dpi;						///< graphics DPI (not physical printer DPI)
		PointF pageSize;				///< in mm, respecting orientation
		RectF printableArea;			///< area on page that can actually be printed on (in mm, respecting orientation)
		PageOrientation orientation;	///< print orientation - this is realized by the print system 
	};

	DEFINE_ENUM (PrintJobStatus)
	{
		kPrinting,	///< print start notification
		kFinished,	///< printing finished successfully
		kCanceled,	///< printing canceled by user or system
		kFailed		///< printing failed (fatal error)
	};

	/** Called as print start notification and when finished, canceled, or failed. */
	virtual tresult CCL_API updateStatus (PrintJobStatus status) = 0;

	/** Called during preview (optional) and for printing. 
		Return kResultAborted to stop printing, this will in turn call updateStatus() with kCanceled. */
	virtual tresult CCL_API renderPage (PageRenderData& data) = 0;

	DECLARE_IID (IPageRenderer)
};

DEFINE_IID (IPageRenderer, 0x4b7651f, 0x6361, 0x417b, 0x93, 0xe5, 0xa2, 0xbb, 0x3, 0xa1, 0xb0, 0x4c)

//************************************************************************************************
// PrinterDocumentInfo
//************************************************************************************************

struct PrinterDocumentInfo
{
	String name;		///< document name
	int minPage;		///< first page available for printing (starting at 0)
	int maxPage;		///< last page available for printing - when specified a print dialog would allow user input for page range. In silent mode, minPage + maxPage define the rage to be printed.
	PointF pageSize;	///< page size in mm (optional) - when specified a corresponding paper format is selected for the printer otherwise the previously selected format will be chosen.
	int currentPage;	///< current page of document (optional) - when specified a print dialog would display a 'current page' option. In silent mode, this page supersedes minPage and maxPage

	PrinterDocumentInfo (StringRef name = nullptr, int minP = 0, int maxP = 0, int currentPage = -1)
	: name (name),
	  minPage (minP),
	  maxPage (maxP),
	  currentPage (currentPage)
	{}

	bool hasValidPageRange () const { return (minPage <= maxPage && minPage >= 0); }
	bool hasValidCurrentPage () const { return currentPage >= 0; }
	bool isValid () const { return hasValidPageRange () || hasValidCurrentPage (); }
};

//************************************************************************************************
// PrinterInfo
//************************************************************************************************

struct PrinterInfo
{
	String name; ///< printer name
	PageOrientation orientation = kPageOrientationUnknown; ///< print orientation
	SymbolicPaperFormat paperFormat = kPaperFormatUnknown; ///< paper format 
};

//************************************************************************************************
// IPrintJob 
/** Print Job (created via IPrintService::createPrintJob) */
//************************************************************************************************

interface IPrintJob: IUnknown
{	
	DEFINE_ENUM (JobMode)
	{
		kJobModeNormal, ///< show print dialog
		kJobModeSilent  ///< no print dialog
	};
	
	/** Run print job, can be asynchronously depending on platform. Renderer will be shared and may outlive the job instance. */
	virtual tresult CCL_API run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode = kJobModeNormal, IWindow* window = nullptr) = 0;

	DECLARE_IID (IPrintJob)
};

DEFINE_IID (IPrintJob, 0x935189e7, 0xe162, 0x4f69, 0x89, 0xfa, 0xc9, 0xfd, 0xdb, 0x65, 0xdb, 0xaa)

//************************************************************************************************
// IPrintService (singleton, see GetPrintService in ccl/public/guiservices.h)
//************************************************************************************************

interface IPrintService: IUnknown
{
	DEFINE_ENUM (Features)
	{
		kPrinting = 1 << 0, 
		kPdfCreation = 1 << 1
	};

	/** Create print job*/
	virtual IPrintJob* CCL_API createPrintJob () = 0;
	
	/** Check if a print job is active */
	virtual tbool CCL_API isAnyPrintJobActive () = 0;
	
	/** Get name of default printer (used for default page size or when print job is performed with kJobModeSilent)  */
	virtual tresult CCL_API getDefaultPrinterInfo (PrinterInfo& info) = 0;

	/** Create page setup dialog.  */
	virtual IPageSetupDialog* CCL_API createPageSetupDialog () = 0;

	/** Get paper format info. */
	virtual const PaperFormat& CCL_API getPaperFormat (SymbolicPaperFormat symbolicFormat) const = 0;

	/** Lookup matching paper format for given size. When no matching format is found, returns format with kPaperFormatUnknown.
	    The format orientation can be used to filter formats */
	virtual const PaperFormat& CCL_API lookupPaperFormatBySize (PointF size, PageOrientation formatOrientation = kPageOrientationUnknown) const = 0;

	/** Get supported features. */
	virtual Features CCL_API getSupportedFeatures () const = 0;
	
	/** Create PDF print job. */
	virtual IPrintJob* CCL_API createPdfPrintJob (UrlRef path) = 0;

	DECLARE_IID (IPrintService)
};

DEFINE_IID (IPrintService, 0x22496a77, 0xb0e0, 0x4084, 0x86, 0x41, 0xac, 0x6a, 0x74, 0x8d, 0x97, 0x2)

} // namespace CCL

#endif // _ccl_iprintservice_h
