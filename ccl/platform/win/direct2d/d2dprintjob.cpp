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
// Filename    : ccl/platform/win/direct2d/d2dprintjob.cpp
// Description : Direct2D Print Job
//
//************************************************************************************************

#include "ccl/platform/win/direct2d/d2dprintjob.h"
#include "ccl/platform/win/direct2d/d2ddevice.h"
#include "ccl/platform/win/direct2d/dxgiengine.h"
#include "ccl/platform/win/direct2d/wicbitmaphandler.h"

#include "ccl/platform/win/system/comstream.h"

#include "ccl/public/math/mathprimitives.h"

#include "ccl/public/system/threadsync.h"
#include "ccl/public/gui/graphics/dpiscale.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"

#include "ccl/base/message.h"

#include <DocumentTarget.h>
#include <Prntvpt.h>

#pragma comment(lib, "Prntvpt.lib")

using namespace CCL;
using namespace Win32;

//************************************************************************************************
// D2DPrintJob::Status
//************************************************************************************************

class D2DPrintJob::Status: public Object,
                           public ::IPrintDocumentPackageStatusEvent
{
public:
	DECLARE_CLASS_ABSTRACT (Status, Object)

	Status ();
	~Status ();

	bool init (ComPtr<IPrintDocumentPackageTarget>& documentPackageTarget);
	bool terminate (bool force);	
	bool waitDone (double maxWaitSeconds);

	const ::PrintDocumentPackageStatus& get () const { return status; }

	void setRenderer (IPageRenderer* renderer);

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	DELEGATE_COM_IUNKNOWN
	CLASS_INTERFACES (Object)

private:
	::PrintDocumentPackageStatus status;
	SharedPtr<IPageRenderer> renderer;

	Threading::CriticalSection guard;
    DWORD eventCookie;
	ComPtr<IConnectionPoint> connectionPoint;

	STDMETHODIMP GetTypeInfoCount (_Out_ UINT *pctinfo) override;
	STDMETHODIMP GetTypeInfo (UINT iTInfo, LCID lcid, ::ITypeInfo **ppTInfo) override;
    STDMETHODIMP GetIDsOfNames (REFIID  riid, LPOLESTR* rgszNames,  UINT cNames, LCID lcid, DISPID*  rgDispId) override;
    STDMETHODIMP Invoke (DISPID dispIdMember, REFIID  riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) override;

    // Implement virtual functions from interface IPrintDocumentPackageStatusEvent.
    STDMETHODIMP PackageStatusUpdated(PrintDocumentPackageStatus* packageStatus) override;
};

//************************************************************************************************
// D2DPrintJob::RenderTarget
//************************************************************************************************

class D2DPrintJob::RenderTarget: public Object,
							     public D2DRenderTarget
{
public:
	RenderTarget (ID2D1DeviceContext* printerContext, ID2D1Image* _outputImage)
	: D2DRenderTarget (printerContext)
	{
		outputImage.share (_outputImage);
	}

	// D2DRenderTarget
	bool isAlphChannelUsed () const override { return false; }
	float getContentScaleFactor () const override { return 2.f; } // use best resolution
};

//************************************************************************************************
// D2DPrintJob
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D2DPrintJob, Win32PrintJobExecutor)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPrintJob::D2DPrintJob ()
: orientation (kPageOrientationUnknown)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPrintJob::~D2DPrintJob ()
{
	finish ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPrintJob::init (Win32PrintJobData* _jobData)
{
	jobData = _jobData;

	ASSERT (jobData)
	if(jobData == nullptr)
		return false;

	ASSERT (jobData->pageRangeCount > 0)
	if(jobData->pageRangeCount <= 0)
		return false;

	ID2D1Device* d2dDevice = DXGIEngine::instance ().getDirect2dDevice ();
	if(d2dDevice == nullptr)
		return false;

	DevMode devMode (jobData->getHDevMode ());
	if(!devMode)
		return false;

	orientation = jobData->getDocumentOrientation ();
	ASSERT (orientation != kPageOrientationUnknown)

	if(jobData->getPageSizes (paperSize, printableArea) == false)
		return false;

	StringChars jobName (jobData->getJobName ());
	StringChars deviceName (jobData->deviceNames.getDeviceName ());

	// Create a print job ticket stream to define options for the next print job.
	HRESULT hr = S_OK;
    hr = CreateStreamOnHGlobal (nullptr, TRUE, ticketStream);
	HPTPROVIDER provider = nullptr;
    if(SUCCEEDED (hr))
	{
	    hr = PTOpenProvider (deviceName, 1, &provider);
	}

    if(SUCCEEDED (hr))
	{
		ULONG devModesize = devMode->dmSize + devMode->dmDriverExtra; // Size of DEVMODE in bytes, including private driver data.
		hr = PTConvertDevModeToPrintTicket (provider, devModesize, devMode, kPTJobScope, ticketStream);
	}

    if(provider)
		PTCloseProvider (provider);

	AutoPtr<CCL::Win32::ComStream> outputStream;
	if(jobData->pdfOutputFile)
		outputStream = NEW CCL::Win32::ComStream (jobData->pdfOutputFile);

    if(SUCCEEDED (hr))
	{
		ComPtr<IPrintDocumentPackageTargetFactory> documentTargetFactory;
		hr = ::CoCreateInstance(__uuidof(PrintDocumentPackageTargetFactory), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IPrintDocumentPackageTargetFactory), documentTargetFactory);

		if(SUCCEEDED (hr))
		{
			hr = documentTargetFactory->CreateDocumentPackageTargetForPrintJob (deviceName, jobName, outputStream, ticketStream, documentTarget);
		}

		if(SUCCEEDED (hr))
		{
			IWICImagingFactory* wicFactory = WICBitmapHandler::instance ().getFactory ();
			hr = d2dDevice->CreatePrintControl (wicFactory,	documentTarget,	nullptr, printControl);
		}
	}

	if(SUCCEEDED (hr))
	{
		status = NEW Status;
		if(status->init (documentTarget) == false)
			status = nullptr;

		hr = d2dDevice->CreateDeviceContext (D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2dContextForPrint);
	}

	if(SUCCEEDED (hr))
		return true;

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult D2DPrintJob::runPrintJob (IPageRenderer* renderer)
{
	if(jobData == nullptr || renderer == nullptr)
		return kResultInvalidArgument;

	if(d2dContextForPrint == 0 || printControl == 0 || documentTarget == 0)
		return kResultUnexpected;

	if(status)
		status->setRenderer (renderer);

	PrintService::instance ().onPrintJobStarted (); // is finished by D2DPrintJob::Status

	// ?? start thread here ??

	tresult printResult = kResultOk;

	float dpi = DpiScale::getDpi (1.f);

	PointF pageSize;
	pageSize.x = Math::millimeterToCoord (paperSize.x, dpi);
	pageSize.y = Math::millimeterToCoord (paperSize.y, dpi);

	renderer->updateStatus (IPageRenderer::kPrinting);

	for(int i = 0; i < jobData->pageRangeCount && printResult == kResultOk; i++)
	{
		const Win32PrintJobData::PageRage& range = jobData->pageRanges[i];

		for(int page = range.pageFrom; page <= range.pageTo && printResult == kResultOk; page++)
		{
			// create render target and graphics device for page
			CCL::AutoPtr<NativeGraphicsDevice> graphicsDevice;
			Win32::ComPtr<::ID2D1CommandList> pageCommandList;

			HRESULT hr = d2dContextForPrint->CreateCommandList (pageCommandList);
			if(SUCCEEDED(hr))
			{
				AutoPtr<RenderTarget> renderTarget = NEW RenderTarget (d2dContextForPrint, pageCommandList);
				graphicsDevice = NEW D2DScopedGraphicsDevice (*renderTarget, static_cast<Unknown*> (renderTarget));
			}

			if(graphicsDevice)
			{
				GraphicsDevice cclGraphics;
				cclGraphics.setNativeDevice (graphicsDevice);

				IPageRenderer::PageRenderData data = {cclGraphics, page, dpi, paperSize, printableArea, orientation};
				tresult renderResult = renderer->renderPage (data);
				if(renderResult != kResultOk)
					printResult = renderResult;

				graphicsDevice = nullptr;

				pageCommandList->Close ();

				if(printResult == kResultOk)
				{
					hr = printControl->AddPage (pageCommandList, D2D1::SizeF (pageSize.x, pageSize.y), nullptr);
					if(FAILED(hr))
						printResult = kResultFailed;
				}
			}
			else
				printResult = kResultFailed;
		}
	}

	if(printResult == kResultAborted)
		documentTarget->Cancel ();

	finish ();

	return printResult;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPrintJob::finish ()
{
	d2dContextForPrint = nullptr;

	if(printControl)
		printControl->Close ();

	ticketStream = nullptr;
	printControl = nullptr;

	if(status)
	{
		if(jobData && jobData->pdfOutputFile)
			status->waitDone (5.0);

		status->terminate (false);
		status = nullptr;
	}

	documentTarget = nullptr;

	return true;
}

//************************************************************************************************
// D2DPrintJob::Status
//************************************************************************************************

DEFINE_CLASS_HIDDEN (D2DPrintJob::Status, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPrintJob::Status::Status ()
: eventCookie (0)
{
	memset (&status, 0, sizeof(::PrintDocumentPackageStatus));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

D2DPrintJob::Status::~Status ()
{
	ASSERT (connectionPoint == 0)
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void D2DPrintJob::Status::setRenderer (IPageRenderer* _renderer)
{
	Threading::ScopedLock lock (guard);
	renderer = _renderer;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API D2DPrintJob::Status::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "terminate")
	{
		terminate (true);

		PrintService::instance ().onPrintJobDone ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPrintJob::Status::init (ComPtr<IPrintDocumentPackageTarget>& documentPackageTarget)
{
	if(documentPackageTarget)
	{
		ComPtr<::IConnectionPointContainer> connectionPointContainer;
		documentPackageTarget.as (connectionPointContainer);
		if(connectionPointContainer)
		{
			HRESULT hr = connectionPointContainer->FindConnectionPoint (__uuidof(IPrintDocumentPackageStatusEvent), connectionPoint);
			if(SUCCEEDED(hr) && connectionPoint)
			{
				::IUnknown* unk = (::IPrintDocumentPackageStatusEvent*) this;
				hr = connectionPoint->Advise (unk, &eventCookie);

				return SUCCEEDED (hr);
			}
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPrintJob::Status::terminate (bool force)
{
    if(connectionPoint)
    {
		if(force == false && status.Completion == PrintDocumentPackageCompletion_InProgress)
			return true;

		ComPtr<IConnectionPoint> localConnectionPoint (connectionPoint);
        connectionPoint = nullptr;

		if(eventCookie != 0)
            localConnectionPoint->Unadvise (eventCookie);
    }

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool D2DPrintJob::Status::waitDone (double maxWaitTime)
{
	double startTime = System::GetProfileTime ();	
	double waitTime = 0;

	while(connectionPoint && waitTime < maxWaitTime)
	{
		MSG msg;
		if(!::GetMessage (&msg, nullptr, 0, 0))
			return false;
		::TranslateMessage (&msg);
		::DispatchMessage (&msg);

		waitTime = System::GetProfileTime () - startTime;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL::tresult CCL_API D2DPrintJob::Status::queryInterface (CCL::UIDRef iid, void** ptr)
{
	QUERY_COM_INTERFACE (::IPrintDocumentPackageStatusEvent);
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DPrintJob::Status::GetTypeInfoCount (_Out_ UINT*)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DPrintJob::Status::GetTypeInfo ( UINT, LCID, ::ITypeInfo**)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DPrintJob::Status::GetIDsOfNames (_In_ REFIID, LPOLESTR*, UINT , LCID, DISPID*)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT D2DPrintJob::Status::Invoke (DISPID, REFIID, LCID, WORD, DISPPARAMS*,VARIANT*,EXCEPINFO*, UINT*)
{
    return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP D2DPrintJob::Status::PackageStatusUpdated (::PrintDocumentPackageStatus* packageStatus)
{
    HRESULT hr = (packageStatus == nullptr) ? E_INVALIDARG : S_OK;

    if(packageStatus)
    {
        // The package status update operation is guarded with a critical section,
        // since PackageStatusUpdated may be called in a very short time slot.
        Threading::ScopedLock lock (guard);
        status = *packageStatus;

		if(renderer)
		{
			IPageRenderer::PrintJobStatus jobStatus = IPageRenderer::kPrinting;
			switch(status.Completion)
			{
			case PrintDocumentPackageCompletion_InProgress: jobStatus = IPageRenderer::kPrinting; break;
			case PrintDocumentPackageCompletion_Completed: jobStatus = IPageRenderer::kFinished; break;
			case PrintDocumentPackageCompletion_Canceled: jobStatus = IPageRenderer::kCanceled; break;
			case PrintDocumentPackageCompletion_Failed: jobStatus = IPageRenderer::kFailed; break;
			}
			if(jobStatus != IPageRenderer::kPrinting)
				renderer->updateStatus (jobStatus);
		}

		if(status.Completion != PrintDocumentPackageCompletion_InProgress)
		{
			(NEW Message ("terminate"))->post (this);
		}
    }

    return S_OK;
}
