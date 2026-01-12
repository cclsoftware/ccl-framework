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
// Filename    : ccl/platform/win/direct2d/d2dprintjob.h
// Description : Direct2D Print Job
//
//************************************************************************************************

#ifndef _ccl_direct2d_printjob_h
#define _ccl_direct2d_printjob_h

#include "ccl/platform/win/gui/printservice.win.h"
#include "ccl/platform/win/direct2d/d2dinterop.h"

#include "ccl/public/gui/framework/iprintservice.h"

interface IStream;
interface IPrintDocumentPackageTarget;
interface ID2D1PrintControl;

namespace CCL {
class NativeGraphicsDevice;

namespace Win32 {

//************************************************************************************************
// D2DPrintJob
//************************************************************************************************

class D2DPrintJob: public Win32PrintJobExecutor
{
public:
	DECLARE_CLASS_ABSTRACT (D2DPrintJob, Win32PrintJobExecutor)

	D2DPrintJob ();
	~D2DPrintJob ();

	// Win32PrintJob
	bool init (Win32PrintJobData* jobData) override;
	tresult runPrintJob (IPageRenderer* renderer) override;

private:
	CCL::SharedPtr<Win32PrintJobData> jobData;

	Win32::ComPtr<::IStream> ticketStream;
	Win32::ComPtr<::IPrintDocumentPackageTarget> documentTarget;
	Win32::ComPtr<::ID2D1PrintControl> printControl;
	Win32::ComPtr<::ID2D1DeviceContext> d2dContextForPrint;
	PointF paperSize; // millimeter
	RectF printableArea; // millimeter
	PageOrientation orientation;

	class Status;
	CCL::AutoPtr<Status> status;
	class RenderTarget;

	bool finish ();
};

} // namespace Win32
} // namespace CCL

#endif // _ccl_direct2d_printjob_h
