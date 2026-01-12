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
// Filename    : ccl/extras/web/transfermanagerui.h
// Description : Transfer Manager UI
//
//************************************************************************************************

#ifndef _ccl_transfermanagerui_h
#define _ccl_transfermanagerui_h

#include "ccl/public/collections/vector.h"

#include "ccl/public/network/web/itransfermanager.h"

#include "ccl/app/component.h"

namespace CCL {

class SignalSink;

namespace Web {

//************************************************************************************************
// TransferFormatter
//************************************************************************************************

class TransferFormatter: public Object,
						 public ITransferFormatter
{
public:
	// ITransferFormatter
	void CCL_API printState (String& string, ITransfer& transfer, ITransfer::State state, double progress, double speed) override;

	CLASS_INTERFACE (ITransferFormatter, Object)
};

//************************************************************************************************
// TransferManagerUI
//************************************************************************************************

class TransferManagerUI: public Component,
					     public ComponentSingleton<TransferManagerUI>
{
public:
	DECLARE_CLASS (TransferManagerUI, Component)

	TransferManagerUI ();
	~TransferManagerUI ();

	bool canShutdown () const;
	void updateActivity ();

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	tbool CCL_API paramChanged (IParameter* param) override;

protected:
	class TransferList;
	
	enum State { kEmpty, kActive, kCompleted, kLastState = kCompleted };

	SignalSink& signalSink;
	TransferList* transferList;
	Vector<ITransfer*> suspendedTransfers;

	void onApplicationSuspend ();
	void onApplicationResume ();
};

} // naemspace Web
} // namespace CCL

#endif // _ccl_transfermanagerui_h
