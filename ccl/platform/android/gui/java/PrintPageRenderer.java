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
// Filename    : ccl/platform/android/gui/java/PrintPageRenderer.java
// Description : PrintDocumentAdapter implementation
//
//************************************************************************************************

package dev.ccl.cclgui;

import dev.ccl.core.Logger;

import android.print.*;
import android.print.pdf.*;
import android.graphics.*;
import android.graphics.pdf.PdfDocument;
import android.content.Context;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.os.Bundle;

import androidx.annotation.Keep;

import java.util.ArrayList;
import java.io.FileOutputStream;

//************************************************************************************************
// PrintPageRenderer
//************************************************************************************************

@Keep
class PrintPageRenderer extends PrintDocumentAdapter
{
	private final long nativeRenderer;
	private final String jobName;
	private final int minPage;
	private final int maxPage;
	private final FrameworkGraphics graphics = new FrameworkGraphics ();
	private PrintedPdfDocument pdfDocument = null;
	private boolean canceled = false;

	private static final boolean DEBUG_LOG = true;

	public PrintPageRenderer (long nativeRenderer, String jobName, int minPage, int maxPage)
	{
		this.nativeRenderer = nativeRenderer;
		this.jobName = jobName;
		this.minPage = minPage;
		this.maxPage = maxPage;
	}

	private static FrameworkActivity getActivity ()
	{
		return FrameworkActivity.getActivity ();
	}

	public void run (int paperWidth, int paperHeight)
	{
		PrintAttributes.MediaSize orientation = paperWidth <= paperHeight ? PrintAttributes.MediaSize.UNKNOWN_PORTRAIT : PrintAttributes.MediaSize.UNKNOWN_LANDSCAPE;
		PrintAttributes.MediaSize mediaSize = new PrintAttributes.MediaSize ("custom", orientation.getLabel (getActivity ().getPackageManager ()), paperWidth, paperHeight);
		PrintAttributes printAttributes = new PrintAttributes.Builder ().setMediaSize (mediaSize).build ();

		PrintManager printManager = (PrintManager)getActivity ().getSystemService (Context.PRINT_SERVICE);
		printManager.print (jobName, this, printAttributes);
	}

	public boolean writePdf (String urlString, int paperWidth, int paperHeight)
	{
		if(DEBUG_LOG)
			Logger.log ("PageRenderer.writePdf (" + paperWidth + ", " + paperHeight + ") " + urlString);

		PdfDocument pdfDocument = new PdfDocument ();

		// generate pages
		int totalPages = maxPage - minPage + 1;
		for(int i = 0; i < totalPages; i++)
		{
			PdfDocument.PageInfo pageInfo = new PdfDocument.PageInfo.Builder (paperWidth, paperHeight, i + 1).create ();
			PdfDocument.Page page = pdfDocument.startPage (pageInfo);
		
			Rect content = pageInfo.getContentRect ();
			setLayout (nativeRenderer, pageInfo.getPageWidth (), pageInfo.getPageHeight (), content.left, content.top, content.right, content.bottom);

			// draw page
			graphics.setCanvas (page.getCanvas ());
			drawPage (nativeRenderer, graphics, i);
			graphics.setCanvas (null);

			pdfDocument.finishPage (page);
		}

		// write PDF document to file
		try
		{
			ParcelFileDescriptor fileDescriptor = getActivity ().openContentFile (urlString, "wt");
			FileOutputStream fileStream = new ParcelFileDescriptor.AutoCloseOutputStream (fileDescriptor);

			pdfDocument.writeTo (fileStream);

			fileStream.close ();
		}
		catch(Exception e)
		{
			Logger.logError ("PageRenderer.writePdf: ", e);
			return false;
		}
		finally
		{
			pdfDocument.close ();

			onFinish ();
		}
		return true;
	}

	@Override
	public void onStart ()
	{
		if(DEBUG_LOG)
			Logger.log ("PageRenderer onStart");
	}

	@Override
	public void onLayout (PrintAttributes oldAttributes, PrintAttributes newAttributes, CancellationSignal cancellationSignal, PrintDocumentAdapter.LayoutResultCallback callback, Bundle extras)
	{
		if(DEBUG_LOG)
			Logger.log ("PageRenderer onLayout");

		pdfDocument = new PrintedPdfDocument (FrameworkActivity.getActivity (), newAttributes);

		if(cancellationSignal.isCanceled ())
		{
			callback.onLayoutCancelled();
		    return;
		}

		int pages = maxPage - minPage + 1;
		if(pages > 0)
		{
			PrintDocumentInfo info = new PrintDocumentInfo.Builder (jobName)
				.setContentType (PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
				.setPageCount (pages)
				.build ();

			Rect content = pdfDocument.getPageContentRect ();
			setLayout (nativeRenderer, pdfDocument.getPageWidth (), pdfDocument.getPageHeight (), content.left, content.top, content.right, content.bottom);

			callback.onLayoutFinished (info, true);
		}
		else
		{
			callback.onLayoutFailed ("No pages to print");
		}
    }

	private boolean containsPage (PageRange[] pageRanges, int page)
	{
		for(PageRange range : pageRanges)
			if(page >= range.getStart () && page <= range.getEnd ())
				return true;

		return false;
	}
	
	private PageRange[] determineWrittenPageRanges (ArrayList<Integer> writtenPages)
	{
		ArrayList<PageRange> pageRanges = new ArrayList<> ();

		int start = -1;
		int end = -1;
		for(int p : writtenPages)
		{
			if(start < 0)
				start = end = p; // start new range
			else if(p > end + 1)
			{
                pageRanges.add (new PageRange (start, end));
                start = end = p;
			}
			else
				end = p;
		}
		if(start >= 0)               
			pageRanges.add (new PageRange (start, end));

		PageRange[] rangesArray = new PageRange[pageRanges.size ()];
		pageRanges.toArray (rangesArray);
		return rangesArray;
	}

	@Override
	public void onWrite (PageRange[] pageRanges, ParcelFileDescriptor destination, CancellationSignal cancellationSignal, PrintDocumentAdapter.WriteResultCallback callback)
	{
		ArrayList<Integer> writtenPages = new ArrayList<> ();

		int totalPages = maxPage - minPage + 1;
		for(int i = 0; i < totalPages; i++)
		{
			if(containsPage (pageRanges, i))
			{
				PdfDocument.Page page = pdfDocument.startPage (i);

				// check for cancellation
				if(cancellationSignal.isCanceled ())
				{
					canceled = true;
					callback.onWriteCancelled ();
					pdfDocument.close ();
					pdfDocument = null;
					return;
				}

				// draw page
				graphics.setCanvas (page.getCanvas ());
				drawPage (nativeRenderer, graphics, i);
				graphics.setCanvas (null);

				pdfDocument.finishPage (page);

				writtenPages.add (i);
			}
		}

		// write PDF document to file
		try
		{
			pdfDocument.writeTo (new FileOutputStream (destination.getFileDescriptor ()));
		}
		catch(Exception e)
		{
			Logger.logError ("Exception", e);
			callback.onWriteFailed (e.toString ());
			return;
		}
		finally
		{
			pdfDocument.close ();
			pdfDocument = null;
		}

		callback.onWriteFinished (determineWrittenPageRanges (writtenPages));
	}

	public @Override
	void onFinish ()
	{
		if(DEBUG_LOG)
			Logger.log ("PageRenderer onFinish");

		finishNative (nativeRenderer, canceled);
	}

	public static native void setLayout (long nativeRenderer, int pageW, int pageH, int contentLeft, int contentTop, int contentRight, int contentBottom);
	public static native void drawPage (long nativeRenderer, FrameworkGraphics graphics, int page);
	public static native void finishNative (long nativeRenderer, boolean canceled);
}
