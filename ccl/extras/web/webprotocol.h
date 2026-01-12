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
// Filename    : ccl/extras/web/webprotocol.h
// Description : Web Protocol
//
//************************************************************************************************

#ifndef _ccl_webprotocol_h
#define _ccl_webprotocol_h

#include "ccl/base/object.h"

#include "ccl/public/collections/vector.h"

#include "ccl/public/network/web/iwebclient.h"
#include "ccl/public/network/web/iwebfileclient.h"
#include "ccl/public/network/web/iwebprotocol.h"
#include "ccl/public/network/web/iwebcredentials.h"

namespace CCL {
namespace Web {

//************************************************************************************************
// WebClientProtocol
//************************************************************************************************

class WebClientProtocol: public Object,
						 public IWebClientProtocol
{
public:
	DECLARE_CLASS_ABSTRACT (WebClientProtocol, Object)

	WebClientProtocol (StringID protocol);
	~WebClientProtocol ();

	void registerHandler ();
	void unregisterHandler ();

	// WebClientProtocol
	CCL::StringID CCL_API getProtocol () const override;

	CLASS_INTERFACE (IWebClientProtocol, Object)

protected:
	MutableCString protocol;
	bool registered;
};

//************************************************************************************************
// WebClient
//************************************************************************************************

class WebClient: public Object,
				 public IWebClient
{
public:
	DECLARE_CLASS (WebClient, Object)

	WebClient ();

	// IWebClient
	void CCL_API setCredentials (IWebCredentials* credentials) override;
	tresult CCL_API connect (StringRef hostname) override;
	tresult CCL_API disconnect () override;
	tbool CCL_API isConnected () override;
	int CCL_API getLastStatus () override;
	tresult CCL_API downloadData (StringRef remotePath, IStream& localStream, IWebHeaderCollection* headers = nullptr, 
								  IProgressNotify* progress = nullptr) override;
	tresult CCL_API uploadData (IWebHeaderCollection* headers, IStream& localStream, StringRef remotePath, IStream& responseStream, 
								StringID method = nullptr, IProgressNotify* progress = nullptr) override;
	tresult CCL_API setOption (StringID optionId, VariantRef value) override;

	CLASS_INTERFACE (IWebClient, Object)

protected:
	SharedPtr<IWebCredentials> credentials;
	String hostname;
	bool connected;
	int lastStatus;
};

//************************************************************************************************
// WebFileClientImpl
//************************************************************************************************

class WebFileClientImpl: public IWebFileClient
{
public:
	tresult CCL_API getServerInfo (StringRef remotePath, ServerInfo& info) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	tresult CCL_API getFileInfo (StringRef remotePath, DirEntry& info) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	tresult CCL_API makeDirectory (String& resultPath, StringRef remotePath, StringRef name) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	IDirIterator* CCL_API openDirectory (StringRef remotePath, IProgressNotify* progress) override
	{ CCL_NOT_IMPL ("Implement me!") return nullptr; }
	tresult CCL_API deleteResource (StringRef remotePath) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	tresult CCL_API copyResource (String& resultPath, StringRef sourcePath, StringRef destPath) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	tresult CCL_API moveResource (String& resultPath, StringRef sourcePath, StringRef destPath, StringRef newName) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
	tresult CCL_API uploadResource (String& resultPath, IStream& localStream, StringRef remotePath,
									StringRef fileName, StringID contentType, IProgressNotify* progress) override
	{ CCL_NOT_IMPL ("Implement me!") return kResultNotImplemented; }
};

//************************************************************************************************
// SimpleFileClient
//************************************************************************************************

class SimpleFileClient: public WebClient,
						public WebFileClientImpl
{
public:
	DECLARE_CLASS (SimpleFileClient, WebClient)

	class DirIterator: public Unknown,
					   public IDirIterator
	{
	public:
		void add (const DirEntry& entry, IUnknown* object = nullptr);
		DirEntry& addDirectory (StringRef name, int flags = 0);
		
		// IDirIterator
		const DirEntry* CCL_API getEntry (int index) const override;
		IUnknown* CCL_API getObject (int index) const override;

		CLASS_INTERFACE (IDirIterator, Unknown)

	protected:
		struct Entry
		{
			DirEntry base;
			SharedPtr<IUnknown> object;
		};
		Vector<Entry> entries;
	};

	// IWebFileClient
	tresult CCL_API getServerInfo (StringRef remotePath, ServerInfo& info) override;

	CLASS_INTERFACE (IWebFileClient, WebClient)
};

} // namespace Web
} // namespace CCL

#endif // _ccl_webprotocol_h
