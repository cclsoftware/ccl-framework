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
// Filename    : ccl/system/plugins/scriptinghost.cpp
// Description : Scripting Host
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/system/plugins/scriptinghost.h"

#include "ccl/system/plugins/plugmanager.h"
#include "ccl/system/plugins/objecttable.h"
#include "ccl/system/plugins/stubclasses.h"
#include "ccl/system/plugins/servicemanager.h"
#include "ccl/system/system.h"
#include "ccl/system/fileutilities.h"
#include "ccl/system/localization/localemanager.h"
#include "ccl/system/packaging/packagehandler.h"

#include "ccl/base/message.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/xmltree.h"
#include "ccl/base/storage/textfile.h"
#include "ccl/base/storage/jsonarchive.h"
#include "ccl/base/security/cryptomaterial.h"

#include "ccl/public/text/cstring.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/base/iarrayobject.h"
#include "ccl/public/system/iconsole.h"
#include "ccl/public/system/iatomtable.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/formatter.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/systemservices.h"

using namespace CCL;

//************************************************************************************************
// ScriptingHost::InterfaceList
//************************************************************************************************

DEFINE_CLASS (ScriptingHost::InterfaceList, Object)
DEFINE_CLASS_FLAGS (ScriptingHost::InterfaceList, CCL::ITypeInfo::kMutable)
DEFINE_CLASS_NAMESPACE (ScriptingHost::InterfaceList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ScriptingHost::InterfaceList)
	DEFINE_PROPERTY_CLASS_ ("IUnknown", "UID", CCL::ITypeInfo::kReadOnly)
END_PROPERTY_NAMES (ScriptingHost::InterfaceList)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::InterfaceList::getProperty (Variant& var, MemberID propertyId) const
{
	Boxed::UID* iid = StubFactory::instance ().lookupInterface (propertyId);
	if(iid)
	{
		var = iid->asUnknown ();
		return true;
	}
	else if(propertyId == "IUnknown")
	{
		static Boxed::UID* iidUnknown = nullptr;
		if(!iidUnknown)
			addGarbageCollected (iidUnknown = NEW Boxed::UID (ccl_iid<IUnknown> ()));

		var = ccl_as_unknown (iidUnknown);
		return true;
	}
	
	CCL_PRINTLN (propertyId)
	CCL_DEBUGGER ("Invalid Interface ID called")

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::InterfaceList::getPropertyNames (IPropertyCollector& collector) const
{
	collector.addPropertyName ("IUnknown");
	StubFactory::instance ().getPropertyNames (collector);
	return true;
}

//************************************************************************************************
// ScriptingHost::ResultsList
//************************************************************************************************

DEFINE_CLASS (ScriptingHost::ResultsList, Object)
DEFINE_CLASS_NAMESPACE (ScriptingHost::ResultsList, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

#define RESULT_DEF(kName) {#kName, kName}
const ScriptingHost::ResultsList::ResultDef ScriptingHost::ResultsList::resultList[] =
{
	RESULT_DEF (kResultOk),
	RESULT_DEF (kResultTrue),
	RESULT_DEF (kResultFalse),
	RESULT_DEF (kResultNotImplemented),
	RESULT_DEF (kResultNoInterface),
	RESULT_DEF (kResultInvalidPointer),
	RESULT_DEF (kResultFailed),
	RESULT_DEF (kResultUnexpected),
	RESULT_DEF (kResultClassNotFound),
	RESULT_DEF (kResultOutOfMemory),
	RESULT_DEF (kResultInvalidArgument),
	RESULT_DEF (kResultWrongThread)
};
#undef RESULT_DEF

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::ResultsList::getProperty (Variant& var, MemberID propertyId) const
{
	for(int i = 0; i < ARRAY_COUNT (resultList); i++)
		if(propertyId == resultList[i].name)
		{
			var = resultList[i].result;
			return true;
		}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::ResultsList::getPropertyNames (IPropertyCollector& collector) const
{
	for(int i = 0; i < ARRAY_COUNT (resultList); i++)
		collector.addPropertyName (resultList[i].name);
	return true;
}

//************************************************************************************************
// ScriptingHost::Console
//************************************************************************************************

DEFINE_CLASS (ScriptingHost::Console, Object)
DEFINE_CLASS_NAMESPACE (ScriptingHost::Console, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptingHost::Console)
	DEFINE_METHOD_ARGS ("writeLine", "string")
END_METHOD_NAMES (ScriptingHost::Console)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::Console::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "writeLine")
	{
		#if (1 && DEBUG)
		Debugger::println (msg[0].asString ());
		#endif
		System::GetConsole ().writeLine (msg[0].asString ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ScriptingHost::Signals
//************************************************************************************************

DEFINE_CLASS (ScriptingHost::Signals, Object)
DEFINE_CLASS_NAMESPACE (ScriptingHost::Signals, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

ISubject* ScriptingHost::Signals::resolve (VariantRef var)
{
	UnknownPtr<ISubject> subject (var);
	if(!subject)
	{
		MutableCString name (var.asString ());
		ASSERT (name.isEmpty () == false)
		AutoPtr<IAtom> atom = System::GetAtomTable ().createAtom (name);
		subject = atom;
	}
	return subject;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptingHost::Signals)
	DEFINE_METHOD_ARGS ("signal", "subject_or_atom: Object | string, messageId: string, arg0: string = null, arg1: string = null")
	DEFINE_METHOD_NAME ("flush")
	DEFINE_METHOD_ARGS ("advise", "subject_or_atom: Object | string, observer: Object")
	DEFINE_METHOD_ARGS ("unadvise", "subject_or_atom: Object | string, observer: Object")
	DEFINE_METHOD_ARGS ("postMessage", "observer: Object, delay: int, messageId: string, arg1 = null, arg2 = null, arg3 = null")
END_METHOD_NAMES (ScriptingHost::Signals)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::Signals::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "signal")
	{
		ISubject* subject = resolve (msg[0]);
		MutableCString msgID = msg[1].asString ();
		if(subject && !msgID.isEmpty ())
		{
			switch(msg.getArgCount ())
			{
			default:
			case 2:
				System::GetSignalHandler ().performSignal (subject, Message (msgID));
				break;
			case 3:
				System::GetSignalHandler ().performSignal (subject, Message (msgID, msg[2]));
				break;
			case 4:
				System::GetSignalHandler ().performSignal (subject, Message (msgID, msg[2], msg[3]));
				break;
			}
		}
		return true;
	}
	else if(msg == "flush")
	{
		System::GetSignalHandler ().flush ();
		return true;
	}
	else if(msg == "advise")
	{
		ISubject* subject = resolve (msg[0]);
		UnknownPtr<IObserver> observer (msg[1]);
		ASSERT (subject && observer)
		if(subject && observer)
		{
			System::GetSignalHandler ().advise (subject, observer);
			observer->retain (); // keep alive!
		}
		return true;
	}
	else if(msg == "unadvise")
	{
		ISubject* subject = resolve (msg[0]);
		UnknownPtr<IObserver> observer (msg[1]);
		ASSERT (subject && observer)
		if(subject && observer)
		{
			System::GetSignalHandler ().unadvise (subject, observer);
			observer->release (); // see above!
		}
		return true;
	}
	else if(msg == "postMessage")
	{
		class ScriptMessage: public Message
		{
		public:
			using Message::Message;

			PROPERTY_SHARED_AUTO (IObserver, observer, Observer)
		};

		// args: observer, delay, messageId, arg1, arg2, ...
		constexpr int kMaxArgs = Message::kMaxMessageArgs - 3; // max. remaining args of incoming message (after observer, delay, id)
		int numArgs = msg.getArgCount () - 3;
		ASSERT (numArgs >= 0 && numArgs <= kMaxArgs)
		if(numArgs >= 0 && numArgs <= kMaxArgs)
		{
			UnknownPtr<IObserver> observer (msg[0]);
			int delay = msg[1];
			MutableCString messageId (msg[2].asString ());

			ASSERT (observer)
			ASSERT (!messageId.isEmpty ())
			if(observer && !messageId.isEmpty ())
			{
				// copy arguments
				Vector<Variant> args (numArgs);
				for(int i = 0; i < numArgs; i++)
					args[i] = msg[i + 3];

				auto* message = NEW ScriptMessage (messageId, args.getItems (), numArgs);

				if(UnknownPtr<IStubObject> (observer).isValid ())
					message->setObserver (observer); // keep observer alive if it's a script object

				message->post (observer, delay);
			}
		}
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ScriptingHost::ScriptableIO
//************************************************************************************************

DEFINE_CLASS (ScriptingHost::ScriptableIO, Object)
DEFINE_CLASS_NAMESPACE (ScriptingHost::ScriptableIO, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptingHost::ScriptableIO)
	DEFINE_METHOD_NAME ("File")
	DEFINE_METHOD_ARGR ("findFiles", "startPoint, searchPattern", "Iterator")
	DEFINE_METHOD_ARGR ("openTextFile", "path, [encoding]", "TextFile")
	DEFINE_METHOD_ARGR ("createTextFile", "path, [encoding]", "TextFile")
	DEFINE_METHOD_ARGR ("loadJsonFile", "path: Url", "Attributes")
	DEFINE_METHOD_NAME ("XmlTree")
	DEFINE_METHOD_ARGR ("toBase64", "string, [encoding]", "string")
	DEFINE_METHOD_ARGR ("fromBase64", "string, [encoding]", "string")
	DEFINE_METHOD_ARGR ("openPackage", "path", "PackageFile")
	DEFINE_METHOD_ARGR ("createPackage", "path, [mimeType]", "PackageFile")
	DEFINE_METHOD_ARGR ("getDevelopmentFileLocation", "root: string, relativePath: string", "Url")
END_METHOD_NAMES (ScriptingHost::ScriptableIO)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::ScriptableIO::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "File")
	{
		UnknownPtr<IUrl> path;
		if(msg.getArgCount () > 0)
			path = msg[0].asUnknown ();
		AutoPtr<IObject> file = path ? NEW File (*path) : NEW File;
		returnValue.takeShared (file);
		return true;
	}
	else if(msg == "findFiles")
	{
		UnknownPtr<IUrl> startPoint (msg[0].asUnknown ());
		ASSERT (startPoint)
		String searchPattern (msg[1].asString ());
		AutoPtr<IFileIterator> fileIterator = startPoint ? File::findFiles (*startPoint, searchPattern) : nullptr;

		AutoPtr<IObject> iter;
		iter.share (UnknownPtr<IObject> (fileIterator));
		if(!iter)
			iter = NEW NullIterator; // do not fail for scripts!
		returnValue.takeShared (iter);
		return true;
	}
	else if(msg == "createTextFile" || msg == "openTextFile")
	{
		bool openMode = msg == "openTextFile";

		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path)

		TextEncoding encoding = Text::kUnknownEncoding;
		if(msg.getArgCount () > 1)
			encoding = TextUtils::getEncodingByName (msg[1].asString ());
		if(openMode == false && encoding == Text::kUnknownEncoding)
			encoding = Text::kUTF8;

		AutoPtr<TextFile> textFile;
		if(path)
		{
			if(msg == "openTextFile")
				textFile = NEW TextFile (*path, TextFile::kOpen, encoding);
			else
				textFile = NEW TextFile (*path, encoding);
		}
		
		if(textFile && textFile->isValid ())
			returnValue.takeShared (textFile->asUnknown ());
		return true;
	}
	else if(msg == "loadJsonFile")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path)

		AutoPtr<Attributes> jsonContent;
		if(path)
		{
			AutoPtr<IStream> stream = File (*path).open (IStream::kOpenMode);
			if(stream)
			{
				jsonContent = NEW Attributes;
				JsonArchive (*stream).loadAttributes (nullptr, *jsonContent);
			}
		}

		if(jsonContent && jsonContent->isEmpty () == false)
			returnValue.takeShared (jsonContent->asUnknown ());

		return true;
	}
	else if(msg == "XmlTree")
	{
		returnValue.takeShared (AutoPtr<IObject> (NEW XmlTree));
		return true;
	}
	#if !CCL_PLATFORM_IOS
	else if(msg == "toBase64")
	{
		String string (msg[0].asString ());

		TextEncoding encoding = Text::kUnknownEncoding;
		if(msg.getArgCount () > 1)
			encoding = TextUtils::getEncodingByName (msg[1].asString ());
		if(encoding == Text::kUnknownEncoding)
			encoding = Text::kUTF8;

		String result;
		if(Text::isUTF16Encoding (encoding))
		{
			StringChars chars (string);
			Security::Crypto::Block block (chars, string.length () * sizeof(uchar));
			result = Security::Crypto::Material (block).toBase64 ();
		}
		else
		{
			MutableCString cString (string, encoding);
			Security::Crypto::Block block (cString.str (), cString.length ());
			result = Security::Crypto::Material (block).toBase64 ();
		}

		returnValue = result;
		returnValue.share ();
		return true;
	}
	else if(msg == "fromBase64")
	{
		Security::Crypto::Material material;
		material.fromBase64 (msg[0].asString ());
		Security::Crypto::Block data (material.asBlock ());

		TextEncoding encoding = Text::kUnknownEncoding;
		if(msg.getArgCount () > 1)
			encoding = TextUtils::getEncodingByName (msg[1].asString ());
		if(encoding == Text::kUnknownEncoding)
			encoding = Text::kUTF8;

		String result;
		if(Text::isUTF16Encoding (encoding))
			result.append ((uchar*)data.data, data.length / sizeof(uchar));
		else
			result.appendCString (encoding, (char*)data.data, data.length);

		returnValue = result;
		returnValue.share ();
		return true;
	}
	#endif
	else if(msg == "openPackage")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path.isValid ())
		AutoPtr<IPackageFile> packageFile = path.isValid () ? PackageHandler::instance ().openPackage (*path) : nullptr;
		returnValue.takeShared (packageFile);
		return true;
	}
	else if(msg == "createPackage")
	{
		UnknownPtr<IUrl> path (msg[0].asUnknown ());
		ASSERT (path.isValid ())
		String mimeType (msg.getArgCount () > 1 ? msg[1].asString () : String ());
		UIDRef cid = PackageHandler::instance ().getPackageClassForMimeType (mimeType);
		AutoPtr<IPackageFile> packageFile = path.isValid () ? PackageHandler::instance ().createPackage (*path, cid) : nullptr;
		returnValue.takeShared (packageFile);
		return true;
	}
	else if(msg == "getDevelopmentFileLocation")
	{
		MutableCString root (msg[0].asString ());
		MutableCString relativePath (msg[1].asString ());

		AutoPtr<Url> path = NEW Url;
		GET_DEVELOPMENT_FILE_LOCATION (*path, root, relativePath)
		if(!path->isEmpty ())
			returnValue.takeShared (path->asUnknown ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}

//************************************************************************************************
// ScriptingHost
//************************************************************************************************

DEFINE_CLASS (ScriptingHost, Object)
DEFINE_CLASS_NAMESPACE (ScriptingHost, NAMESPACE_CCL)
DEFINE_CLASS_FLAGS (ScriptingHost, CCL::ITypeInfo::kMutable)
DEFINE_SINGLETON (ScriptingHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptingHost::ScriptingHost ()
: interfaceList (NEW InterfaceList),
  resultsList (NEW ResultsList),
  console (NEW Console),
  signals (NEW Signals),
  scriptableIO (NEW ScriptableIO),
  children (NEW ObjectTable)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

ScriptingHost::~ScriptingHost ()
{
	interfaceList->release ();
	resultsList->release ();
	console->release ();
	signals->release ();
	scriptableIO->release ();
	children->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingHost::registerObject (StringID name, IObject& object)
{
	children->registerObject (&object, kNullUID, name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ScriptingHost::unregisterObject (IObject& object)
{
	children->unregisterObject (&object);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IObject* CCL_API ScriptingHost::getObject (StringID name) const
{
	return UnknownPtr<IObject> (children->getObjectByName (name));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_PROPERTY_NAMES (ScriptingHost)
	DEFINE_PROPERTY_CLASS_ ("Interfaces", "ScriptingHost.InterfaceList", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Results", "ScriptingHost.ResultsList", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Classes", "PlugInManager", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Objects", "ObjectTable", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Services", "ServiceManager", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Console", "ScriptingHost.Console", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Signals", "ScriptingHost.Signals", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("Locales", "LocaleManager", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("SystemInfo", "SystemInformation", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("IO", "ScriptingHost.ScriptableIO", CCL::ITypeInfo::kReadOnly)
	DEFINE_PROPERTY_CLASS_ ("FileTypes", "FileTypeRegistry", CCL::ITypeInfo::kReadOnly)
END_PROPERTY_NAMES (ScriptingHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "Interfaces")
	{
		var = ccl_as_unknown (interfaceList);
		return true;
	}
	else if(propertyId == "Results")
	{
		var = ccl_as_unknown (resultsList);
		return true;
	}
	else if(propertyId == "Classes")
	{
		var = PlugInManager::instance ().asUnknown ();
		return true;
	}
	else if(propertyId == "Objects")
	{
		var = ObjectTable::instance ().asUnknown ();
		return true;
	}
	else if(propertyId == "Services")
	{
		var = ServiceManager::instance ().asUnknown ();
		return true;
	}
	else if(propertyId == "Console")
	{
		var = ccl_as_unknown (console);
		return true;
	}
	else if(propertyId == "Signals")
	{
		var = ccl_as_unknown (signals);
		return true;
	}
	else if(propertyId == "Locales")
	{
		var = LocaleManager::instance ().asUnknown ();
		return true;
	}
	else if(propertyId == "SystemInfo")
	{
		var = SystemInformation::instance ().asUnknown ();
		return true;
	}
	else if(propertyId == "IO")
	{
		var = ccl_as_unknown (scriptableIO);
		return true;
	}
	else if(propertyId == "FileTypes")
	{
		var = FileTypeRegistry::instance ().asUnknown ();
		return true;
	}
	else
	{
		// try local object table
		if(children->getProperty (var, propertyId))
			return true;

		// try global object table
		// This allows shortcuts from scripts like Host.whatever instead of Host.Objects.getObjectByName ("whatever").
		if(ObjectTable::instance ().getProperty (var, propertyId))
			return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::setProperty (MemberID propertyId, const Variant& var)
{
	return SuperClass::setProperty (propertyId, var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::getPropertyNames (IPropertyCollector& collector) const
{
	SuperClass::getPropertyNames (collector);

	children->getObjectNames (collector);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (ScriptingHost)
	DEFINE_METHOD_ARGS ("retain", "obj")
	DEFINE_METHOD_ARGS ("release", "obj")
	DEFINE_METHOD_ARGR ("UID", "", "UID")
	DEFINE_METHOD_ARGR ("Url", "url: Url | string", "Url")
	DEFINE_METHOD_ARGR ("Attributes", "vargs", "variant")
	DEFINE_METHOD_ARGR ("DateTime", "", "DateTime")
	DEFINE_METHOD_ARGR ("getPlatform", "", "string")
	DEFINE_METHOD_ARGS ("sleep", "ms: int")
END_METHOD_NAMES (ScriptingHost)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ScriptingHost::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "retain")
	{
		if(IUnknown* unk = msg[0].asUnknown ())
			unk->retain ();
		return true;
	}
	else if(msg == "release")
	{
		if(IUnknown* unk = msg[0].asUnknown ())
			unk->release ();
		return true;
	}
	else if(msg == "UID")
	{
		AutoPtr<Boxed::UID> uid = NEW Boxed::UID;
		uid->fromString (msg[0]);
		returnValue = Variant (static_cast<IObject*> (uid), true);
		return true;
	}
	else if(msg == "Url")
	{
		AutoPtr<Url> url;
		if(msg.getArgCount () > 0)
		{
			if(msg[0].isObject ())
				if(UnknownPtr<IUrl> inputUrl = msg[0].asUnknown ())
					url = NEW Url (*inputUrl); // clone incoming URL

			if(!url)
			{
				bool isFolder = msg.getArgCount () > 1 ? msg[1].asBool () : false;
				url = NEW Url (msg[0].asString (), isFolder ? Url::kFolder : Url::kFile);
			}
		}
		else
			url = NEW Url;

		returnValue = Variant (static_cast<IObject*> (url), true);
		return true;
	}
	else if(msg == "Attributes")
	{
		AutoPtr<Attributes> attr = NEW Attributes;

		UnknownPtr<IArrayObject> arrayObject (msg.getArgCount () > 0 ? msg[0].asUnknown () : nullptr);
		if(arrayObject)
		{
			int length = arrayObject->getArrayLength ();
			for(int i = 0; i < length; i+=2)
			{
				Variant v;
				arrayObject->getArrayElement (v, i);
				VariantString string (v);
				MutableCString key (string);
				v.clear ();
				arrayObject->getArrayElement (v, i + 1);
				if(!key.isEmpty ())
					attr->setAttribute (key, v, Attributes::kShare);
			}
		}

		returnValue = Variant (static_cast<IObject*> (attr), true);
		return true;
	}
	else if(msg == "DateTime")
	{
		AutoPtr<Boxed::DateTime> dt = NEW Boxed::DateTime;
		if(msg.getArgCount () >= 1)
			Format::PortableDateTime::scan (*dt, msg[0].asString ());
		returnValue.takeShared (dt->asUnknown ());
		return true;
	}
	else if(msg == "getPlatform")
	{
		returnValue = CCLSTR (XmlProcessingInstructionHandler::getPlatform ());
		return true;
	}
	else if(msg == "sleep")
	{
		System::ThreadSleep (msg[0].asInt ());
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
