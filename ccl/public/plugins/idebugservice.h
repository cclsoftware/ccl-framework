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
// Filename    : ccl/public/plugins/idebugservice.h
// Description : Debug Service Interface
//
//************************************************************************************************

#ifndef _ccl_idebugservice_h
#define _ccl_idebugservice_h

#include "ccl/public/base/iunknown.h"

namespace CCL {

interface IContainer;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Component Categories
//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class category for debug services. */
#define PLUG_CATEGORY_DEBUGSERVICE CCLSTR ("DebugService")

//************************************************************************************************
// IDebugMessage
//************************************************************************************************

interface IDebugMessage: IUnknown
{
	static constexpr int kBroadcastThreadId = -1;

	virtual int CCL_API getThreadId () const = 0;
	
	virtual void CCL_API getRawData (String& data) const = 0;
	
	virtual void CCL_API setRawData (StringRef data) = 0;

	DECLARE_IID (IDebugMessage)
};

DEFINE_IID (IDebugMessage, 0x1eed5ab0, 0x2bdd, 0x48fb, 0xb7, 0x83, 0x20, 0x93, 0x32, 0x12, 0x3c, 0x69)

//************************************************************************************************
// IDebugMessageReceiver
//************************************************************************************************

interface IDebugMessageReceiver: IUnknown
{
	virtual void CCL_API receiveMessage (const IDebugMessage& message) = 0;
	
	virtual void CCL_API onDisconnected () = 0;

	DECLARE_IID (IDebugMessageReceiver)
};

DEFINE_IID (IDebugMessageReceiver, 0x433d9cd2, 0xd768, 0x48c4, 0xa2, 0x8b, 0xb4, 0x21, 0xa1, 0x22, 0x23, 0x18)

//************************************************************************************************
// IDebugMessageSender
//************************************************************************************************

interface IDebugMessageSender: IUnknown
{
	virtual tbool CCL_API sendMessage (const IDebugMessage& message) = 0;
	
	virtual IDebugMessage* CCL_API createMessage (StringRef rawData) = 0;

	DECLARE_IID (IDebugMessageSender)
};

DEFINE_IID (IDebugMessageSender, 0xa2b9be63, 0xe0c6, 0x4d59, 0x9f, 0x5e, 0x95, 0x46, 0xdb, 0x74, 0x2a, 0xd6)

//************************************************************************************************
// IDebuggable
//************************************************************************************************

interface IDebuggable: IDebugMessageReceiver
{
	virtual void CCL_API setSender (IDebugMessageSender* sender) = 0;
	
	virtual void CCL_API setThreadId (int threadId) = 0;
	
	virtual int CCL_API getThreadId () const = 0;
	
	virtual StringRef CCL_API getName () const = 0;

	DECLARE_IID (IDebuggable)
};

DEFINE_IID (IDebuggable, 0x10acda86, 0x9c78, 0x46ac, 0xb8, 0x85, 0x84, 0x57, 0x2d, 0x98, 0x65, 0x2e)

//************************************************************************************************
// IDebuggableManager
//************************************************************************************************

interface IDebuggableManager: IDebugMessageReceiver
{
	virtual const IContainer& CCL_API getDebuggables () const = 0;

	DECLARE_IID (IDebuggableManager)
};

DEFINE_IID (IDebuggableManager, 0x4c34d8b0, 0xdfc4, 0x4429, 0x8e, 0x23, 0xea, 0xce, 0x13, 0x1c, 0x79, 0xce)

//************************************************************************************************
// IDebugService
//************************************************************************************************

interface IDebugService: IDebugMessageSender
{
	virtual tbool CCL_API startup (StringRef arg, IDebuggableManager* manager) = 0;
	
	virtual tbool CCL_API shutdown () = 0;

	DECLARE_STRINGID_MEMBER (kProtocolAttribute) ///< used as class attribute

	DECLARE_IID (IDebugService)
};

DEFINE_IID (IDebugService, 0xb83dabe1, 0x504f, 0x4637, 0xb2, 0xd9, 0x99, 0xc8, 0x4d, 0xc1, 0x4f, 0xc4)
DEFINE_STRINGID_MEMBER (IDebugService, kProtocolAttribute, "protocolIdentifier")

} // namespace CCL

#endif // _ccl_idebugservice_h
