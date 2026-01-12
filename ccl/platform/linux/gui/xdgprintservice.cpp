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
// Filename    : ccl/platform/linux/gui/printservice.linux.cpp
// Description : Print Service implementation using XDG Desktop Portal
//
//************************************************************************************************

#include "ccl/platform/linux/gui/xdgprintservice.h"

#include "ccl/platform/linux/gui/platformdialog.linux.h"
#include "ccl/platform/linux/interfaces/ilinuxsystem.h"

#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/base/storage/file.h"

#include "ccl/public/base/memorystream.h"
#include "ccl/public/text/cclstdstring.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/ifileutilities.h"

#include "ccl/platform/linux/shared/xdgportalrequest.h"

#include "org-freedesktop-portal-print-client.h"

#include <fcntl.h>
#include <unistd.h>

namespace CCL {
namespace Linux {

//************************************************************************************************
// XdgPrintJob
//************************************************************************************************

class XdgPrintJob: public PrintJob,
				   public LinuxPlatformDialog,
				   public DBusProxy<org::freedesktop::portal::Print_proxy>,
				   public PlatformIntegration::IPlatformDialog,
				   public IXdgPortalResponseHandler
{
public:
	DECLARE_CLASS_ABSTRACT (XdgPrintJob, PrintJob)
	
	XdgPrintJob (IDBusSupport& dbusSupport);

	PROPERTY_OBJECT (Url, pdfUrl, PdfUrl)

	bool runPageSetup (PageSetup& pageSetup, IWindow* window);
	bool prepare (IWindow* window, StringRef title = nullptr);

	// PrintJob
	tresult CCL_API run (const PrinterDocumentInfo& documentInfo, IPageRenderer* renderer, JobMode mode, IWindow* window) override;

	// IPlatformDialog
	void setParent (void* nativeWindowHandle) override;

	// IXdgPortalResponseHandler
	void onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results) override;

private:
	static const int kDefaultResolution = 72;

	std::string parentWindowId;
	XdgPrintSettings& settings;
	XdgPrintSettings& setup;
	uint32 token;
	uint32 lastResponse;

	void applyPageSetup (const PageSetup& pageSetup);
	void applyDocumentInfo (const PrinterDocumentInfo& documentInfo);
	void getPageSetup (PageSetup& pageSetup) const;
	bool renderDocument (MemoryStream& stream, IPageRenderer* renderer, const PrinterDocumentInfo& documentInfo);
};

//************************************************************************************************
// XdgPageSetupDialog
//************************************************************************************************

class XdgPageSetupDialog: public PageSetupDialog
{
	DECLARE_CLASS_ABSTRACT (XdgPageSetupDialog, PageSetupDialog)

	// PageSetupDialog
	tbool CCL_API run (PageSetup& pageSetup, IWindow* window) override;
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// XdgPrintService
//************************************************************************************************

XdgPrintService::XdgPrintService ()
: dbusSupport (nullptr)
{
	UnknownPtr<ILinuxSystem> linuxSystem (&System::GetSystem ());
	if(linuxSystem.isValid ())
		dbusSupport = linuxSystem->getDBusSupport ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API XdgPrintService::createPdfPrintJob (UrlRef path)
{
	if(dbusSupport)
	{
		XdgPrintJob* printJob = NEW XdgPrintJob (*dbusSupport);
		printJob->setPdfUrl (path);
		return printJob;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPrintJob* CCL_API XdgPrintService::createPrintJob ()
{
	if(dbusSupport)
		return NEW XdgPrintJob (*dbusSupport);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPageSetupDialog* CCL_API XdgPrintService::createPageSetupDialog ()
{
	return NEW XdgPageSetupDialog;
}

//************************************************************************************************
// XdgPrintJob
//************************************************************************************************

DEFINE_CLASS_HIDDEN (XdgPrintJob, PrintJob)

//////////////////////////////////////////////////////////////////////////////////////////////////

XdgPrintJob::XdgPrintJob (IDBusSupport& dbusSupport)
: DBusProxy (dbusSupport, XdgPortalRequest::kDestination, XdgPortalRequest::kObjectPath),
  settings (XdgPrintService::instance ().getSettings ()),
  setup (XdgPrintService::instance ().getSetup ()),
  token (0),
  lastResponse (XdgPortalRequest::kUnknown)
{
	nativeDialog = this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPrintJob::setParent (void* nativeWindowHandle)
{
	PlatformIntegration::NativeWindowHandle* handle = static_cast<PlatformIntegration::NativeWindowHandle*> (nativeWindowHandle);
	if(!CString (handle->exportedHandle).isEmpty ())
	{
		parentWindowId = "wayland:";
		parentWindowId.append (handle->exportedHandle);
	}
	else if(!CString (handle->exportedHandleV1).isEmpty ())
	{
		parentWindowId = "wayland:";
		parentWindowId.append (handle->exportedHandleV1);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPrintJob::onResponse (const uint32_t& response, const std::map<std::string, sdbus::Variant>& results)
{
	if(results.find ("settings") != results.end ())
	{
		const sdbus::Variant& settingsData = results.at ("settings");
		settings = settingsData.get<std::map<std::string, sdbus::Variant>> ();
	}
	if(results.find ("page-setup") != results.end ())
	{
		const sdbus::Variant& pageSetupData = results.at ("page-setup");
		setup = pageSetupData.get<std::map<std::string, sdbus::Variant>> ();
	}
	if(results.find ("token") != results.end ())
	{
		const sdbus::Variant& tokenData = results.at ("token");
		token = tokenData.get<uint32> ();
	}
	lastResponse = response;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XdgPrintJob::runPageSetup (PageSetup& pageSetup, IWindow* window)
{
	applyPageSetup (pageSetup);
	
	if(!prepare (window))
		return false;

	getPageSetup (pageSetup);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XdgPrintJob::prepare (IWindow* window, StringRef title)
{
	Promise promise (setParentWindow (window));

	AutoPtr<XdgPortalRequest> request;
	promise.then ([&] (IAsyncOperation& op)
	{
		sdbus::ObjectPath result;
		try
		{
			result = PreparePrint (parentWindowId, toStdString (title), settings, setup, {});
		}
		CATCH_DBUS_ERROR

		if(result.empty ())
		{
			op.cancel ();
			return;
		}

		request = NEW XdgPortalRequest (dbusSupport, *this, result);
	});
	while(promise->getState () == AsyncOperation::kStarted || (request.isValid () && !request->receivedResponse ()))
		dbusSupport.flushUpdates ();

	return promise->getState () != IAsyncOperation::kCanceled && lastResponse == XdgPortalRequest::kSuccess;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API XdgPrintJob::run (const PrinterDocumentInfo& documentInfo, IPageRenderer* renderer, JobMode mode, IWindow* window)
{
	if(mode == kJobModeSilent && pdfUrl.isEmpty () && token == 0)
		return kResultFailed;

	applyDocumentInfo (documentInfo);

	if(mode == kJobModeNormal)
	{
		if(!prepare (window, documentInfo.name))
			return kResultFailed;
	}

	MemoryStream stream;
	if(!renderDocument (stream, renderer, documentInfo))
		return kResultFailed;

	stream.rewind ();
	if(!pdfUrl.isEmpty ())
	{
		if(File::save (pdfUrl, stream))
			return kResultOk;
	}

	Url tempFileUrl;
	System::GetFileUtilities ().makeUniqueTempFile (tempFileUrl, "ccl_print_document");
	if(!File::save (tempFileUrl, stream))
		return kResultFailed;

	MutableCString tempFilePath (UrlDisplayString (tempFileUrl), Text::kSystemEncoding);
	int fd = ::open (tempFilePath, O_RDONLY);
	if(fd < 0)
		return kResultFailed;

	XdgPrintSettings options;
	options["token"] = token;
	sdbus::ObjectPath result;
	try
	{
		result = Print (parentWindowId, toStdString (documentInfo.name), sdbus::UnixFd (fd), options);
	}
	CATCH_DBUS_ERROR

	if(result.empty ())
		return kResultFailed;

	AutoPtr<XdgPortalRequest> request = NEW XdgPortalRequest (dbusSupport, *this, result);
	while(!request->receivedResponse ())
		dbusSupport.flushUpdates ();
		
	::close (fd);
	::unlink (tempFilePath);

	return (lastResponse == XdgPortalRequest::kSuccess) ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPrintJob::applyPageSetup (const PageSetup& pageSetup)
{
	if(pageSetup.isValid ())
	{
		setup["Width"] = double(pageSetup.size.x);
		setup["Height"] = double(pageSetup.size.y);
		setup["MarginLeft"] = double(pageSetup.margins.left);
		setup["MarginTop"] = double(pageSetup.margins.top);
		setup["MarginRight"] = double(pageSetup.margins.right);
		setup["MarginBottom"] = double(pageSetup.margins.bottom);
		switch(pageSetup.orientation)
		{
		case kPageOrientationPortrait :
			setup["Orientation"] = "portrait";
			break;
		case kPageOrientationLandscape :
			setup["Orientation"] = "landscape";
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPrintJob::applyDocumentInfo (const PrinterDocumentInfo& documentInfo)
{
	if(!documentInfo.pageSize.isNull ())
	{
		settings["paper-width"] = double(documentInfo.pageSize.x);
		settings["paper-height"] = double(documentInfo.pageSize.y);
	}
	if(documentInfo.hasValidPageRange ())
	{
		settings["paper-ranges"] = MutableCString ().appendFormat ("%d-%d", documentInfo.minPage, documentInfo.maxPage).str ();
		settings["print-pages"] = "ranges";
	}
	else
		settings["print-pages"] = "all";
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XdgPrintJob::getPageSetup (PageSetup& pageSetup) const
{
	if(settings.find ("paper-width") != settings.end ())
		pageSetup.size.x = settings.at ("paper-width").get<double> ();
	else if(setup.find ("Width") != setup.end ())
		pageSetup.size.x = setup.at ("Width").get<double> ();
	if(settings.find ("paper-height") != settings.end ())
		pageSetup.size.y = settings.at ("paper-height").get<double> ();
	else if(setup.find ("Height") != setup.end ())
		pageSetup.size.y = setup.at ("Height").get<double> ();
	if(setup.find ("MarginLeft") != setup.end ())
		pageSetup.margins.left = setup.at ("MarginLeft").get<double> ();
	if(setup.find ("MarginTop") != setup.end ())
		pageSetup.margins.top = setup.at ("MarginTop").get<double> ();
	if(setup.find ("MarginRight") != setup.end ())
		pageSetup.margins.right = setup.at ("MarginRight").get<double> ();
	if(setup.find ("MarginBottom") != setup.end ())
		pageSetup.margins.bottom = setup.at ("MarginBottom").get<double> ();
	if(setup.find ("Orientation") != setup.end ())
	{
		std::string orientation = setup.at ("Orientation").get<std::string> ();
		if(orientation == "portrait")
			pageSetup.orientation = kPageOrientationPortrait;
		else if(orientation == "landscape")
			pageSetup.orientation = kPageOrientationLandscape;
		else if(orientation == "reverse-portrait")
			pageSetup.orientation = kPageOrientationPortrait;
		else if(orientation == "reverse-landscape")
			pageSetup.orientation = kPageOrientationLandscape;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XdgPrintJob::renderDocument (MemoryStream& stream, IPageRenderer* renderer, const PrinterDocumentInfo& documentInfo)
{
	PointF pixelSize;
	pixelSize.x = Math::millimeterToInch (documentInfo.pageSize.x) * kDefaultResolution;
	pixelSize.y = Math::millimeterToInch (documentInfo.pageSize.y) * kDefaultResolution;

	if(pixelSize.isNull ())
		return false;

	SkiaPDFRenderTarget renderTarget (stream, pixelSize.x, pixelSize.y);
	SkiaScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
	GraphicsDevice device;
	device.setNativeDevice (&nativeDevice);
	
	int minPage = documentInfo.hasValidPageRange () ? documentInfo.minPage : 0;
	int maxPage = documentInfo.hasValidPageRange () ? documentInfo.maxPage : 0;

	for(int pageNumber = minPage; pageNumber <= maxPage; pageNumber++)
	{
		IPageRenderer::PageRenderData data = {device, pageNumber, kDefaultResolution, documentInfo.pageSize, documentInfo.pageSize};
		if(renderer->renderPage (data) != kResultOk)
			return false;
		if(pageNumber < maxPage)
			renderTarget.nextPage ();
	}
	return true;
}

//************************************************************************************************
// XdgPageSetupDialog
//************************************************************************************************

DEFINE_CLASS_HIDDEN (XdgPageSetupDialog, PageSetupDialog)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API XdgPageSetupDialog::run (PageSetup& pageSetup, IWindow* window)
{	
	IDBusSupport* dbusSupport = XdgPrintService::instance ().getDBusSupport ();
	if(dbusSupport == nullptr)
		return false;

	XdgPrintJob dialog (*dbusSupport);
	return dialog.runPageSetup (pageSetup, window);
}
