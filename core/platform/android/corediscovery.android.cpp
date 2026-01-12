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
// Filename    : core/platform/android/corediscovery.android.cpp
// Description : DNS Service Discovery Android implementation
//
//************************************************************************************************

#include "core/platform/android/corediscovery.android.h"

#include "core/platform/shared/jni/corejnihelper.h"

#include "core/network/coresocket.h"
#include "core/system/corethread.h"

namespace Core {
namespace Platform {

//************************************************************************************************
// android.net.nsd.NsdServiceInfo
//************************************************************************************************

DECLARE_JNI_CLASS (NsdServiceInfo, "android/net/nsd/NsdServiceInfo")
	DECLARE_JNI_CONSTRUCTOR (construct)
	DECLARE_JNI_METHOD (jobject, getHost)
	DECLARE_JNI_METHOD (void, setHost, jobject)
	DECLARE_JNI_METHOD (int, getPort)
	DECLARE_JNI_METHOD (void, setPort, int)
	DECLARE_JNI_METHOD (jstring, getServiceName)
	DECLARE_JNI_METHOD (void, setServiceName, jstring)
	DECLARE_JNI_METHOD (jstring, getServiceType)
	DECLARE_JNI_METHOD (void, setServiceType, jstring)
	DECLARE_JNI_METHOD (jobject, getAttributes)
	DECLARE_JNI_METHOD (void, setAttribute, jstring, jstring)
	DECLARE_JNI_METHOD (void, removeAttribute, jstring)
END_DECLARE_JNI_CLASS (NsdServiceInfo)

DEFINE_JNI_CLASS (NsdServiceInfo)
	DEFINE_JNI_CONSTRUCTOR (construct, "()V")
	DEFINE_JNI_METHOD (getHost, "()Ljava/net/InetAddress;")
	DEFINE_JNI_METHOD (setHost, "(Ljava/net/InetAddress;)V")
	DEFINE_JNI_METHOD (getPort, "()I")
	DEFINE_JNI_METHOD (setPort, "(I)V")
	DEFINE_JNI_METHOD (getServiceName, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (setServiceName, "(Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (getServiceType, "()Ljava/lang/String;")
	DEFINE_JNI_METHOD (setServiceType, "(Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (getAttributes, "()Ljava/util/Map;")
	DEFINE_JNI_METHOD (setAttribute, "(Ljava/lang/String;Ljava/lang/String;)V")
	DEFINE_JNI_METHOD (removeAttribute, "(Ljava/lang/String;)V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// dev.ccl.core.NsdAdapter
//************************************************************************************************

DECLARE_JNI_CLASS (NsdAdapter, CORE_CLASS_PREFIX "NsdAdapter")
	DECLARE_JNI_CONSTRUCTOR (construct, Java::JniIntPtr)
	DECLARE_JNI_METHOD (jobject, discoverServices, jstring)
	DECLARE_JNI_METHOD (void, stopDiscovery, jobject)
	DECLARE_JNI_METHOD (jobject, resolveService, jobject)
	DECLARE_JNI_METHOD (jobject, registerService, jobject)
	DECLARE_JNI_METHOD (void, unregisterService, jobject)
END_DECLARE_JNI_CLASS (NsdAdapter)

DEFINE_JNI_CLASS (NsdAdapter)
	DEFINE_JNI_CONSTRUCTOR (construct, "(J)V")
	DEFINE_JNI_METHOD (discoverServices, "(Ljava/lang/String;)L" CORE_CLASS_PREFIX "NsdDiscoveryHandler;")
	DEFINE_JNI_METHOD (stopDiscovery, "(L" CORE_CLASS_PREFIX "NsdDiscoveryHandler;)V")
	DEFINE_JNI_METHOD (resolveService, "(Landroid/net/nsd/NsdServiceInfo;)L" CORE_CLASS_PREFIX "NsdResolveHandler;")
	DEFINE_JNI_METHOD (registerService, "(Landroid/net/nsd/NsdServiceInfo;)L" CORE_CLASS_PREFIX "NsdRegistrationHandler;")
	DEFINE_JNI_METHOD (unregisterService, "(L" CORE_CLASS_PREFIX "NsdRegistrationHandler;)V")
END_DEFINE_JNI_CLASS

//************************************************************************************************
// RegTypeString
//************************************************************************************************

class RegTypeString: public CString64
{
public:
	RegTypeString (const TypeDescriptor& descriptor)
	{
		ASSERT (descriptor.protocol == Sockets::kTCP || descriptor.protocol == Sockets::kUDP)
		appendFormat ("_%s._%s.", descriptor.type, descriptor.protocol == Sockets::kTCP ? "tcp" : "udp");
	}

	RegTypeString (CStringPtr regtype)
	: CString64 (regtype)
	{}

	void getType (CString64& type) const { subString (type, 1, index (".") - 1); }
	Sockets::ProtocolType getProtocol () const { return contains ("._tcp") ? Sockets::kTCP : Sockets::kUDP; }
};

//************************************************************************************************
// DiscoveryContext
//************************************************************************************************

class DiscoveryContext
{
public:
	DiscoveryContext ();
	virtual ~DiscoveryContext () {}

	virtual bool processResults () = 0;

protected:
	Java::JniObject nsdAdapter;
};

//************************************************************************************************
// DiscoveryBrowseContext
//************************************************************************************************

class DiscoveryBrowseContext: public DiscoveryContext
{
public:
	DiscoveryBrowseContext (const TypeDescriptor& descriptor, BrowseReplyHandler* browseHandler);
	~DiscoveryBrowseContext ();

	bool registerBrowser ();

	void onServiceFound (jobject serviceInfo);
	void onServiceLost (jobject serviceInfo);
	void onServiceResolved (jobject serviceInfo);

	// DiscoveryContext
	bool processResults () override;

private:
	RegTypeString regType;
	BrowseReplyHandler* browseHandler;

	Java::JniObject nsdDiscoveryHandler;

	Threads::Lock lock;
	Vector<Java::JniObject*> resolved;
	Vector<Java::JniObject*> removed;
};

//************************************************************************************************
// DiscoveryServiceContext
//************************************************************************************************

class DiscoveryServiceContext: public DiscoveryContext
{
public:
	DiscoveryServiceContext (const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* registerHandler);
	~DiscoveryServiceContext ();

	void onServiceRegistered ();
	void onRegistrationFailed ();

	bool registerService ();

	// DiscoveryContext
	bool processResults () override;

private:
	RegisterReplyHandler* registerHandler;

	Java::JniObject nsdServiceInfo;
	Java::JniObject nsdRegistrationHandler;

	void setServiceInformation (const ServiceTargetDescriptor& descriptor);
	void setInstanceName (CStringPtr service, const TypeDescriptor& type);
	void setHostName (CStringPtr host);
	void useLocalHostName ();
	void setTextRecord (const TextRecord& textRecord);

	static Sockets::PortNumber findPort ();

	Threads::Lock lock;
	bool processed;
	bool registered;
};

}}

using namespace Core;
using namespace Java;
using namespace Sockets;
using namespace Platform;

//************************************************************************************************
// DiscoveryContext
//************************************************************************************************

DiscoveryContext::DiscoveryContext ()
{
	JniAccessor jni;

	// create Java NsdAdapter object
	nsdAdapter.assign (jni, jni.newObject (NsdAdapter, NsdAdapter.construct, JniIntPtr (this)));
}

//************************************************************************************************
// DiscoveryBrowseContext
//************************************************************************************************

DiscoveryBrowseContext::DiscoveryBrowseContext (const TypeDescriptor& descriptor, BrowseReplyHandler* browseHandler)
: regType (descriptor),
  browseHandler (browseHandler)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DiscoveryBrowseContext::~DiscoveryBrowseContext ()
{
	if(!nsdDiscoveryHandler)
		return;

	NsdAdapter.stopDiscovery (nsdAdapter, nsdDiscoveryHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryBrowseContext::registerBrowser ()
{
	JniAccessor jni;
	JniString jQueryString (jni, regType);
	nsdDiscoveryHandler.assign (jni, NsdAdapter.discoverServices (nsdAdapter, jQueryString));

	return nsdDiscoveryHandler != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryBrowseContext::processResults ()
{
	Threads::ScopedLock scope (lock);

	if(resolved.count () == 0 && removed.count () == 0)
		return false;

	JniAccessor jni;

	for(JniObject* serviceInfo : resolved)
	{
		JniCStringChars jServiceType (jni, NsdServiceInfo.getServiceType (*serviceInfo));
		JniCStringChars jServiceName (jni, NsdServiceInfo.getServiceName (*serviceInfo));

		JniObject jHostAddr (jni, NsdServiceInfo.getHost (*serviceInfo));
		JniCStringChars jHostName (jni, InetAddress.getHostName (jHostAddr));

		RegTypeString regType (jServiceType);

		CString64 type;
		regType.getType (type);

		ServiceTargetDescriptor descriptor;

		descriptor.type = type;
		descriptor.protocol = regType.getProtocol ();
		descriptor.serviceName = jServiceName;
		descriptor.hostname = jHostName;
		descriptor.port = NsdServiceInfo.getPort (*serviceInfo);

		AndroidTextRecordBuilder builder;
		JniObject jAttributes (jni, NsdServiceInfo.getAttributes (*serviceInfo));
		JniObject jAttributesSet (jni, Map.entrySet (jAttributes));
		JniObject jAttributesIterator (jni, Set.iterator (jAttributesSet));

		while(Iterator.hasNext (jAttributesIterator))
		{
			jobject jEntry = Iterator.next (jAttributesIterator);

			JniCStringChars jKey (jni, jobject_cast<jstring> (MapEntry.getKey (jEntry)));
			JniByteArray jValue (jni, jobject_cast<jbyteArray> (MapEntry.getValue (jEntry)));

			CString256 value;
			jValue.getData (value.getBuffer (),	jValue.getLength ());
			value.getBuffer ()[jValue.getLength ()] = 0;

			builder.setValue (jKey, value);
		}

		TextRecord textRecord = builder.getTextRecord ();
		descriptor.textRecord = &textRecord;

		browseHandler->onServiceResolved (this, descriptor);

		delete serviceInfo;
	}

	for(JniObject* serviceInfo : removed)
	{
		JniCStringChars jServiceType (jni, NsdServiceInfo.getServiceType (*serviceInfo));
		JniCStringChars jServiceName (jni, NsdServiceInfo.getServiceName (*serviceInfo));

		RegTypeString regType (jServiceType);

		CString64 type;
		regType.getType (type);

		ServiceDescriptor descriptor;

		descriptor.type = type;
		descriptor.protocol = regType.getProtocol ();
		descriptor.serviceName = jServiceName;

		browseHandler->onServiceRemoved (this, descriptor);

		delete serviceInfo;
	}

	resolved.empty ();
	removed.empty ();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryBrowseContext::onServiceFound (jobject serviceInfo)
{
	NsdAdapter.resolveService (nsdAdapter, serviceInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryBrowseContext::onServiceLost (jobject serviceInfo)
{
	Threads::ScopedLock scope (lock);

	removed.add (NEW JniObject (JniAccessor (), serviceInfo));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryBrowseContext::onServiceResolved (jobject serviceInfo)
{
	Threads::ScopedLock scope (lock);

	resolved.add (NEW JniObject (JniAccessor (), serviceInfo));
}

//************************************************************************************************
// NsdDiscoveryHandler Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CORE (void, NsdDiscoveryHandler, onServiceFound, JniIntPtr nativeHandler, jobject serviceInfo)
{
	DiscoveryBrowseContext* context = JniCast<DiscoveryBrowseContext>::fromIntPtr (nativeHandler);
	if(!context)
		return;

	context->onServiceFound (serviceInfo);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CORE (void, NsdDiscoveryHandler, onServiceLost, JniIntPtr nativeHandler, jobject serviceInfo)
{
	DiscoveryBrowseContext* context = JniCast<DiscoveryBrowseContext>::fromIntPtr (nativeHandler);
	if(!context)
		return;

	context->onServiceLost (serviceInfo);
}

//************************************************************************************************
// NsdResolveHandler Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CORE (void, NsdResolveHandler, onServiceResolved, JniIntPtr nativeHandler, jobject serviceInfo)
{
	DiscoveryBrowseContext* context = JniCast<DiscoveryBrowseContext>::fromIntPtr (nativeHandler);
	if(!context)
		return;

	context->onServiceResolved (serviceInfo);
}

//************************************************************************************************
// DiscoveryServiceContext
//************************************************************************************************

DiscoveryServiceContext::DiscoveryServiceContext (const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* registerHandler)
: registerHandler (registerHandler),
  processed (false),
  registered (false)
{
	JniAccessor jni;
	nsdServiceInfo.assign (jni, jni.newObject (NsdServiceInfo, NsdServiceInfo.construct));

	setServiceInformation (descriptor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DiscoveryServiceContext::~DiscoveryServiceContext ()
{
	if(!nsdRegistrationHandler)
		return;

	NsdAdapter.unregisterService (nsdAdapter, nsdRegistrationHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryServiceContext::registerService ()
{
	JniAccessor jni;
	nsdRegistrationHandler.assign (jni, NsdAdapter.registerService (nsdAdapter, nsdServiceInfo));

	return nsdRegistrationHandler != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setServiceInformation (const ServiceTargetDescriptor& descriptor)
{
	// instance name
	setInstanceName (descriptor.serviceName, descriptor);

	// host name
	if(descriptor.hostname && descriptor.hostname[0] != 0)
		setHostName (descriptor.hostname);
	else
		useLocalHostName ();

	// port
	if(descriptor.port)
		NsdServiceInfo.setPort (nsdServiceInfo, descriptor.port);
	else
		NsdServiceInfo.setPort (nsdServiceInfo, findPort ());

	// txt data
	setTextRecord (*descriptor.textRecord);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setInstanceName (CStringPtr service, const TypeDescriptor& type)
{
	CString256 serviceType;
	serviceType.appendFormat ("_%s._%s", type.type, type.protocol == kTCP ? "tcp" : "udp");

	JniAccessor jni;
	JniString jServiceName (jni, service);
	JniString jServiceType (jni, serviceType);
	NsdServiceInfo.setServiceName (nsdServiceInfo, jServiceName);
	NsdServiceInfo.setServiceType (nsdServiceInfo, jServiceType);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setHostName (CStringPtr host)
{
	IPAddress address;
	Sockets::Network::getAddressByHost (address, host);

	JniAccessor jni;
	JniString jHostName (jni, host);
	JniByteArray jIpAddr (jni, (jbyte*) address.ip.address, 4);
	JniObject jHostAddr (jni, InetAddress.getByAddress (jIpAddr));
	NsdServiceInfo.setHost (nsdServiceInfo, jHostAddr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::useLocalHostName ()
{
	CString256 hostName;
	Sockets::Network::getLocalHostname (hostName);
	setHostName (hostName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::setTextRecord (const TextRecord& textRecord)
{
	JniAccessor jni;

	for(int i = 0; i < textRecord.getCount (); i++)
	{
		CString64 key;
		CString64 value;
		textRecord.getItemAt (key, value, i);

		JniString jKey (jni, key);
		JniString jValue (jni, value);
		NsdServiceInfo.setAttribute (nsdServiceInfo, jKey, jValue);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Sockets::PortNumber DiscoveryServiceContext::findPort ()
{
	IPAddress address;
	address.setIP (127, 0, 0, 1);

	// use bind to figure out a free port
	Sockets::Socket socket (kInternet, kStream, kTCP);
	if(socket.bind (address))
	{
		socket.getLocalAddress (address);
		socket.disconnect ();

		return address.port;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool DiscoveryServiceContext::processResults ()
{
	Threads::ScopedLock scope (lock);

	if(!processed)
		return false;

	if(registered)
	{
		JniAccessor jni;

		JniCStringChars jServiceType (jni, NsdServiceInfo.getServiceType (nsdServiceInfo));
		JniCStringChars jServiceName (jni, NsdServiceInfo.getServiceName (nsdServiceInfo));

		RegTypeString regType (jServiceType);

		CString64 type;
		regType.getType (type);

		ServiceDescriptor descriptor;

		descriptor.type = type;
		descriptor.protocol = regType.getProtocol ();
		descriptor.serviceName = jServiceName;

		registerHandler->onServiceRegistered (this, descriptor);
	}
	else
		registerHandler->onServiceRegistrationFailed (this);

	processed = false;
	registered = false;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::onServiceRegistered ()
{
	Threads::ScopedLock scope (lock);

	processed = true;
	registered = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DiscoveryServiceContext::onRegistrationFailed ()
{
	Threads::ScopedLock scope (lock);

	processed = true;
}

//************************************************************************************************
// NsdRegistrationHandler Java native methods
//************************************************************************************************

DECLARE_JNI_CLASS_METHOD_CORE (void, NsdRegistrationHandler, onRegistrationFailed, JniIntPtr nativeHandler, jobject serviceInfo, int errorCode)
{
	DiscoveryServiceContext* context = JniCast<DiscoveryServiceContext>::fromIntPtr (nativeHandler);
	if(!context)
		return;

	context->onRegistrationFailed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_JNI_CLASS_METHOD_CORE (void, NsdRegistrationHandler, onServiceRegistered, JniIntPtr nativeHandler, jobject serviceInfo)
{
	DiscoveryServiceContext* context = JniCast<DiscoveryServiceContext>::fromIntPtr (nativeHandler);
	if(!context)
		return;

	context->onServiceRegistered ();
}

//************************************************************************************************
// DiscoveryHandler
//************************************************************************************************

bool DiscoveryHandler::isInstalled ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerBrowser (DiscoveryRef& sdRef, const TypeDescriptor& descriptor, BrowseReplyHandler* replyHandler)
{
	DiscoveryBrowseContext* context = NEW DiscoveryBrowseContext (descriptor, replyHandler);

	sdRef = context;

	return context->registerBrowser ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::registerService (DiscoveryRef& sdRef, const ServiceTargetDescriptor& descriptor, RegisterReplyHandler* replyHandler)
{
	DiscoveryServiceContext* context = NEW DiscoveryServiceContext (descriptor, replyHandler);

	sdRef = context;

	return context->registerService ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
void DiscoveryHandler::unregisterReference (DiscoveryRef sdRef)
{
	delete sdRef;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

template<>
bool DiscoveryHandler::processResults (const DiscoveryRef sdRefList[], int count)
{
	bool anyActivity = false;
	for(int i = 0; i < count; i++)
		anyActivity |= const_cast<DiscoveryContext*> (sdRefList[i])->processResults ();

	return anyActivity;
}

//************************************************************************************************
// AndroidTextRecord
//************************************************************************************************

AndroidTextRecord::AndroidTextRecord (const ConstVector<AndroidTextRecordData*>& records)
: records (records)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

int AndroidTextRecord::getCount () const
{
	return records.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTextRecord::getItemAt (CString64& key, CString64& value, int index) const
{
	if(records.count () <= index)
		return false;

	key = records[index]->key;
	value = records[index]->value;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTextRecord::getValue (CString64& value, CStringPtr key) const
{
	int count = getCount ();
	for(int i = 0; i < count; i++)
	{
		CString64 iKey, iValue;
		getItemAt (iKey, iValue, i);
		if(iKey != key)
			continue;

		value = iValue;
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool AndroidTextRecord::getIntValue (int64& value, CStringPtr key) const
{
	CString64 string;
	if(!getValue (string, key))
		return false;

	return string.getIntValue (value);
}

//************************************************************************************************
// AndroidTextRecordBuilder
//************************************************************************************************

AndroidTextRecordBuilder::~AndroidTextRecordBuilder ()
{
	for(int i = 0; i < records.count (); i++)
		delete records[i];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextRecordBuilder::setValue (CStringPtr key, CStringPtr value)
{
	records.add (NEW AndroidTextRecordData { key, value });
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void AndroidTextRecordBuilder::setIntValue (CStringPtr key, int64 value)
{
	CString64 string;
	string.appendFormat ("%" FORMAT_INT64 "d", value);
	setValue (key, string);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AndroidTextRecord AndroidTextRecordBuilder::getTextRecord () const
{
	return AndroidTextRecord (records);
}
