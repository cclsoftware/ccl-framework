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
// Filename    : ccl/gui/graphics/printservice.cpp
// Description : Printer Dialogs and Job
//
//************************************************************************************************

#include "ccl/gui/graphics/printservice.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/guiservices.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// GUI Service APIs
//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_EXPORT IPrintService& CCL_API System::CCL_ISOLATED (GetPrintService) ()
{
	return PrintService::instance ();
}

//************************************************************************************************
// PrintService
//************************************************************************************************

const PaperFormat PrintService::paperFormatTable[] =
{
	{kPaperFormatLetter,			CCLSTR ("Letter"),				{Math::inchToMillimeter (8.5f), Math::inchToMillimeter (11.f)}},
	{kPaperFormatLetterExtra,		CCLSTR ("Letter Extra"),		{Math::inchToMillimeter (9.5f), Math::inchToMillimeter (12.f)}},
	{kPaperFormatLetterPlus,		CCLSTR ("Letter Plus"),			{Math::inchToMillimeter (8.5f), Math::inchToMillimeter (12.69f)}},

	{kPaperFormatTabloid,			CCLSTR ("Tabloid"),				{Math::inchToMillimeter (11.f), Math::inchToMillimeter (17.f)}},
	{kPaperFormatTabloidExtra,		CCLSTR ("Tabloid Extra"),		{Math::inchToMillimeter (11.69f), Math::inchToMillimeter (18.f)}},

	{kPaperFormatLedger,			CCLSTR ("Ledger"),				{Math::inchToMillimeter (17.f), Math::inchToMillimeter (11.f)}},
	{kPaperFormatLegal,				CCLSTR ("Legal"),				{Math::inchToMillimeter (8.5f), Math::inchToMillimeter (14.f)}},
	{kPaperFormatLegalExtra,		CCLSTR ("Legal Extra"),			{Math::inchToMillimeter (9.5f), Math::inchToMillimeter (15.f)}},
	{kPaperFormatStatement,			CCLSTR ("Statement"),			{Math::inchToMillimeter (5.5f), Math::inchToMillimeter (8.5f)}},
	{kPaperFormatExecutive,			CCLSTR ("Executive"),			{Math::inchToMillimeter (7.25f), Math::inchToMillimeter (10.5f)}},

	{kPaperFormatA2,				CCLSTR ("A2"),					{420.f, 594.f}},
	{kPaperFormatA3,				CCLSTR ("A3"),					{297.f, 420.f}},
	{kPaperFormatA3Extra,			CCLSTR ("A3 Extra"),			{322.f, 445.f}},
	{kPaperFormatA4,				CCLSTR ("A4"),					{210.f, 297.f}},
	{kPaperFormatA4Plus,			CCLSTR ("A4 Plus"),				{210.f, 330.f}},
	{kPaperFormatA4Extra,			CCLSTR ("A4 Extra"),			{Math::inchToMillimeter (9.27f), Math::inchToMillimeter (12.69f)}},
	{kPaperFormatA5,				CCLSTR ("A5"),					{148.f, 210.f}},
	{kPaperFormatA5Extra,			CCLSTR ("A5 Extra"),			{174.f, 235.f}},
	{kPaperFormatA6,				CCLSTR ("A6"),					{105.f, 148.f}},
	{kPaperFormatAPlus,				CCLSTR ("A Plus"),				{227.f, 356.f}},

	{kPaperFormatB4,				CCLSTR ("B4"),					{250.f, 354.f}},
	{kPaperFormatB5,				CCLSTR ("B5"),					{182.f, 257.f}},
	{kPaperFormatB5Extra,			CCLSTR ("B5 Extra"),			{201.f, 276.f}},
	{kPaperFormatIsoB4,				CCLSTR ("ISO B4"),				{250.f, 353.f}},
	{kPaperFormatBPlus,				CCLSTR ("B Plus"),				{305.f, 487.f}},

	{kPaperFormatFolio,				CCLSTR ("Folio"),				{Math::inchToMillimeter (8.5f), Math::inchToMillimeter (13.f)}},
	{kPaperFormatQuarto,			CCLSTR ("Quarto"),				{215.f, 275.f}},
	{kPaperFormatNote,				CCLSTR ("Note"),				{Math::inchToMillimeter (8.5f), Math::inchToMillimeter (11.f)}},
	{kPaperFormatJapanesePostcard,	CCLSTR ("Japanese Postcard"),	{100.f, 148.f}},

	{kPaperFormat9x11Inch,			CCLSTR ("9x11 Inch"),			{Math::inchToMillimeter (9.f), Math::inchToMillimeter (11.f)}},
	{kPaperFormat10x11Inch,			CCLSTR ("10x11 Inch"),			{Math::inchToMillimeter (10.f), Math::inchToMillimeter (11.f)}},
	{kPaperFormat15x11Inch,			CCLSTR ("15x11 Inch"),			{Math::inchToMillimeter (15.f), Math::inchToMillimeter (11.f)}},
	{kPaperFormat10x14Inch,			CCLSTR ("10x14 Inch"),			{Math::inchToMillimeter (10.f), Math::inchToMillimeter (14.f)}},
	{kPaperFormat11x17Inch,			CCLSTR ("11x17 Inch"),			{Math::inchToMillimeter (11.f), Math::inchToMillimeter (17.f)}}
};

//////////////////////////////////////////////////////////////////////////////////////////////////

const PaperFormat PrintService::kUnknownFormat = {kPaperFormatUnknown};

//////////////////////////////////////////////////////////////////////////////////////////////////

PrintService::PrintService ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PrintService::onPrintJobStarted ()
{
	printJobCounter.increment ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PrintService::onPrintJobDone ()
{
	printJobCounter.decrement ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PrintService::isAnyPrintJobActive ()
{
	return printJobCounter.getValue () > 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PaperFormat& CCL_API PrintService::getPaperFormat (SymbolicPaperFormat symbolicFormat) const
{
	for(int i = 0; i < ARRAY_COUNT (paperFormatTable); i++)
		if(paperFormatTable[i].symbolic == symbolicFormat)
			return paperFormatTable [i];

	return kUnknownFormat;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const PaperFormat& CCL_API PrintService::lookupPaperFormatBySize (PointF size, PageOrientation orientation) const
{
	for(int i = 0; i < ARRAY_COUNT (paperFormatTable); i++)
	{
		auto& format = paperFormatTable[i];

		if(ccl_equals (format.size.x, size.x, 0.5f) && ccl_equals (format.size.y, size.y, 0.5f))
		{
			if(orientation != kPageOrientationUnknown && format.getFormatOrientation () != orientation)
				continue;

			return format;
		}
	}
	return kUnknownFormat;
}

//************************************************************************************************
// PageSetupDialog
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PageSetupDialog, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PageSetupDialog::run (PageSetup& pageSetup, IWindow* window)
{
	CCL_NOT_IMPL ("PageSetupDialog::run implemented in derived class!")
	return false;
}

//************************************************************************************************
// PrintJob
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PrintJob, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PrintJob::run (const PrinterDocumentInfo& docInfo, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	CCL_NOT_IMPL ("PrintJob::run implemented in derived class!")
	return kResultNotImplemented;
}
