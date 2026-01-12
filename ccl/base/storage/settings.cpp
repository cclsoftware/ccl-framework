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
// Filename    : ccl/base/storage/settings.cpp
// Description : User Settings
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/base/storage/settings.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"
#include "ccl/base/storage/storage.h"
#include "ccl/base/storage/file.h"
#include "ccl/base/storage/xmlarchive.h"
#include "ccl/base/storage/configuration.h"

#include "ccl/public/text/language.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ilocalemanager.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_STRINGID_ (Signals::kSettings, "CCL.Settings")
DEFINE_STRINGID_ (Signals::kAutoSaveSettings, "AutoSaveSettings")
DEFINE_STRINGID_ (Signals::kBackupSettings, "BackupSettings")

//************************************************************************************************
// ISettingsSaver
//************************************************************************************************

DEFINE_IID_ (ISettingsSaver, 0xf374b83a, 0x5841, 0x4512, 0xb8, 0x76, 0xc1, 0x45, 0xa4, 0x9a, 0xd5, 0x28)

//************************************************************************************************
// ConfigurationSaver
//************************************************************************************************

Attributes& ConfigurationSaver::getAttributes (Settings& settings)
{
	static const String configurationPath = CCLSTR ("Configuration");
	Attributes& section = settings.getAttributes (configurationPath);
	PersistentAttributes* a = section.getObject<PersistentAttributes> ("values");
	if(a == nullptr)
		section.set ("values", a = NEW PersistentAttributes, Attributes::kOwns);
	return *a;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigurationSaver::ConfigurationSaver (StringID section, StringID key, Configuration::IRegistry* registry)
: section (section),
  key (key),
  registry (registry)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Configuration::IRegistry& ConfigurationSaver::getRegistry ()
{
	if(registry)
		return *registry;
	else
		return Configuration::Registry::instance ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MutableCString ConfigurationSaver::getAttributeName () const
{
	MutableCString attrName (section);
	attrName += ".";
	attrName += key.str ();
	return attrName;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationSaver::restore (Settings& settings)
{
	Attributes& a = getAttributes (settings);
	MutableCString attrName = getAttributeName ();

	Variant value;
	if(a.getAttribute (value, attrName))
		getRegistry ().setValue (section, key, value);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ConfigurationSaver::flush (Settings& settings)
{
	Attributes& a = getAttributes (settings);
	MutableCString attrName = getAttributeName ();

	Variant value;
	if(getRegistry ().getValue (value, section, key))
		a.setAttribute (attrName, value, Attributes::kTemp);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ConfigurationSaver* ConfigurationSaver::migrateSettingFrom (Settings& settings, StringID oldSection, StringID oldKey)
{
	// take old value only if new value has not been set in settings yet
	Attributes& a = getAttributes (settings);
	MutableCString newAttributeName = ConfigurationSaver (section, key).getAttributeName ();
	
	Variant value;
	if(!a.getAttribute (value, newAttributeName))
	{
		MutableCString oldAttributeName = ConfigurationSaver (oldSection, oldKey).getAttributeName ();
		if(a.getAttribute (value, oldAttributeName))
			a.setAttribute (newAttributeName, value);
	}
	return this;
}

//************************************************************************************************
// Settings::Section
//************************************************************************************************

DEFINE_CLASS_PERSISTENT (Settings::Section, Object, "Section")
DEFINE_CLASS_NAMESPACE (Settings::Section, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section::Section (StringRef path)
: path (path),
  attributes (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section::Section (const Section& other)
: path (other.path),
  attributes (nullptr)
{
	if(other.attributes)
		attributes = NEW Attributes (*other.attributes);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section::~Section ()
{
	if(attributes)
		attributes->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int Settings::Section::compare (const Object& obj) const
{
	const Section* s = ccl_cast<Section> (&obj);
	return s ? path.compare (s->path) : Object::compare (obj);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::Section::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	a.get (path, "path");

	ASSERT (attributes == nullptr)
	attributes = a.unqueueAttributes ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::Section::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	a.set ("path", path);

	if(attributes)
		a.queue (nullptr, attributes);
	return true;
}

//************************************************************************************************
// Settings
//************************************************************************************************

Settings& Settings::instance ()
{
	static Settings* settings = nullptr;
	if(!settings)
	{
		settings = NEW XmlSettings;
		settings->isAutoSaveEnabled (true);
		settings->enableSignals (true);
		Object::addGarbageCollected (settings);
	}
	return *settings;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::autoSaveAll ()
{
	SignalSource (Signals::kSettings).signal (Message (Signals::kAutoSaveSettings));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::backupAll (IUnknownList& pathList)
{
	SignalSource (Signals::kSettings).signal (Message (Signals::kBackupSettings, &pathList));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (Settings, Object)
DEFINE_CLASS_NAMESPACE (Settings, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Settings (StringRef name, int version)
: name (name),
  version (version),
  flags (0),
  signalSink (nullptr)
{
	checkName (true);
	checkVersion (true);
	settings.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::~Settings ()
{
	enableSignals (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::init (StringRef _name, int _version)
{
	companyName.empty ();
	productName.empty ();
	name = _name;
	version = _version;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::init (StringRef _companyName, StringRef _productName, StringRef _settingsName, int _version)
{
	companyName = _companyName;
	productName = _productName;
	name = _settingsName;
	version = _version;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::copyFrom (const Settings& other)
{
	removeAll ();

	ForEach (other.settings, Section, section)
		settings.add (NEW Section (*section));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::addSaver (ISettingsSaver* saver)
{
	ASSERT (saver != nullptr)
	if(saver == nullptr)
		return;

	savers.append (saver);

	// settings might be restored already at this stage, so...
	saver->restore (*this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::removeSaver (ISettingsSaver* saver)
{
	ASSERT (saver != nullptr)
	if(saver == nullptr)
		return;

	// flush before remove
	saver->flush (*this);
	savers.remove (saver);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::containsSaver (ISettingsSaver* saver) const
{
	return saver ? savers.contains (saver) : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::enableSignals (bool state)
{
	if(state)
	{
		if(signalSink == nullptr)
		{
			signalSink = NEW SignalSink (Signals::kSettings);
			signalSink->setObserver (this);
			signalSink->enable (true);
		}
	}
	else
	{
		if(signalSink)
		{
			signalSink->enable (false);
			delete signalSink;
			signalSink = nullptr;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API Settings::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kAutoSaveSettings)
	{
		if(isAutoSaveEnabled ())
		{
			CCL_PRINT ("Auto-saving settings ")
			CCL_PRINTLN (name)
			flush ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section* Settings::lookup (StringRef path) const
{
	return static_cast<Section*> (settings.search (Section (path)));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (Settings)
	DEFINE_METHOD_ARGR ("getAttributes", "Attributes", "string")
END_METHOD_NAMES (Settings)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API Settings::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "getAttributes")
	{
		returnValue = getAttributes (msg.getArg (0)).asUnknown ();
		return true;
	}

	return SuperClass::invokeMethod (returnValue, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Settings::Section* Settings::getSection (StringRef path, bool create)
{
	Section* group = lookup (path);
	if(!group && create)
	{
		group = NEW Section (path);
		settings.addSorted (group); // sort for XML cosmetics ;-)
	}
	return group;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::isEmpty () const
{
	ForEach (settings, Section, group)
		if(!group->isEmpty ())
			return false;
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::isEmpty (StringRef path) const
{
	Section* g = lookup (path);
	return !g || (g && g->isEmpty ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::remove (StringRef path)
{
	Section* g = lookup (path);
	if(g)
	{
		settings.remove (g);
		g->release ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void Settings::removeAll ()
{
	settings.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::load (const Storage& storage)
{
	Attributes& a = storage.getAttributes ();

	String savedName;
	a.get (savedName, "name");

	int savedVersion = a.getInt ("version");

	if(!checkName ())
		name = savedName;
	else if(savedName != name)
		return false;

	if(!checkVersion ())
		version = savedVersion;
	else if(savedVersion != version)
		return false;

	if(checkLanguage ())
	{
		MutableCString savedLanguage = a.getCString ("language");
		if(savedLanguage != System::GetLocaleManager ().getLanguage ())
			return false;
	}

	settings.removeAll ();

	Section* g;
	while((g = (Section*)a.unqueueObject (nullptr, ccl_typeid<Section> ())) != nullptr)
		settings.addSorted (g);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::save (const Storage& storage) const
{
	Attributes& a = storage.getAttributes ();

	if(!name.isEmpty ())
		a.set ("name", name);
	a.set ("version", version);

	if(checkLanguage ())
		a.set ("language", System::GetLocaleManager ().getLanguage ());

	ForEach (settings, Section, group)
		a.queue (nullptr, group, Attributes::kShare); // share sections for settings created on stack
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::restore ()
{
	CCL_NOT_IMPL ("Settings::restore")
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool Settings::flush ()
{
	CCL_NOT_IMPL ("Settings::flush")
	return false;
}

//************************************************************************************************
// XmlSettings
//************************************************************************************************

const FileType& XmlSettings::getFileType ()
{
	static const FileType settingsType ("User Settings", "settings", "text/xml");
	return settingsType;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String XmlSettings::getNameWithLanguage (StringRef name)
{
	StringID language = System::GetLocaleManager ().getLanguage ();
	return String () << name << "-" << language;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlSettings::removeSettings (StringRef name, bool anyLanguage, bool anyPlatform)
{
	auto removeFiles = [&] (UrlRef folder)
	{
		if(anyLanguage)
		{
			String searchPattern;
			searchPattern << name << "-*." << XmlSettings::getFileType ().getExtension ();
			ForEachFile (File::findFiles (folder, searchPattern, IFileIterator::kFiles), path)
				System::GetFileSystem ().removeFile (*path);
			EndFor
		}
		else
		{
			Url path (folder);
			path.descend (name);
			path.setFileType (XmlSettings::getFileType (), true);
			if(System::GetFileSystem ().fileExists (path))
				System::GetFileSystem ().removeFile (path);
		}
	};

	Url settingsFolder;
	System::GetSystem ().getLocation (settingsFolder, System::kAppSettingsFolder);
	removeFiles (settingsFolder);

	ForEachFile (System::GetFileSystem ().newIterator (settingsFolder), current)
		if(current->getType () == Url::kFolder)
			removeFiles (*current);
	EndFor

	return;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (XmlSettings, Settings)
DEFINE_CLASS_NAMESPACE (XmlSettings, NAMESPACE_CCL)

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlSettings::XmlSettings (StringRef name, int version)
: Settings (name, version),
  path (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

XmlSettings::~XmlSettings ()
{
	if(path)
		path->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlSettings::setPath (UrlRef url)
{
	if(!path)
		path = NEW Url (url);
	else
		path->assign (url);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void XmlSettings::setFileName (StringRef _fileName)
{
	String fileName (_fileName);
	if(fileName.isEmpty ())
		fileName = LegalFileName (name);

	ASSERT (fileName.isEmpty () == false)
	if(fileName.isEmpty ())
		fileName = CCLSTR ("UserSettings");

	if(!path)
		path = NEW Url;

	if(!companyName.isEmpty () || !productName.isEmpty ())
	{
		ASSERT (isPlatformSpecific () == false) // not implemented!
		ASSERT (isApplicationIndependent () == false) // must not mix
		System::GetSystem ().getLocation (*path, System::kUserSettingsFolder);
		if(!companyName.isEmpty ())
			path->descend (companyName, Url::kFolder);
		if(!productName.isEmpty ())
			path->descend (productName, Url::kFolder);
	}
	else
	{
		if(isApplicationIndependent ())
			System::GetSystem ().getLocation (*path, System::kCompanySettingsFolder);
		else
			System::GetSystem ().getLocation (*path, isPlatformSpecific () ? System::kAppSettingsPlatformFolder : System::kAppSettingsFolder);
	}

	path->descend (fileName);
	path->setFileType (getFileType (), true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UrlRef XmlSettings::getPath ()
{
	if(!path)
		setFileName (String::kEmpty);
	return *path;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlSettings::loadSettings (UrlRef url)
{
	bool result = false;
	AutoPtr<IStream> stream = System::GetFileSystem ().openStream (url, IStream::kOpenMode);
	if(stream)
	{
		XmlArchive archive (*stream);
		result = archive.loadObject ("Settings", *this);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlSettings::saveSettings (UrlRef url)
{
	bool result = false;

	Url temp (url);
	temp.setName ("tempfile");

	String name;
	url.getName (name, true);

	IStream* stream = System::GetFileSystem ().openStream (temp, IStream::kCreateMode);
	if(stream)
	{
		XmlArchive archive (*stream);
		archive.defineNamespace (true);
		result = archive.saveObject ("Settings", *this);
		stream->release ();

		if(result == true)
		{
			if(System::GetFileSystem ().fileExists (url))
				System::GetFileSystem ().removeFile (url);
			System::GetFileSystem ().renameFile (temp, name);
		}
		else
			System::GetFileSystem ().removeFile (temp);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlSettings::restore ()
{
	if(!System::GetFileSystem ().fileExists (getPath ()))
		return true;

	if(!loadSettings (getPath ()))
		return false;

	ListForEach (savers, ISettingsSaver*, saver)
		saver->restore (*this);
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool XmlSettings::flush ()
{
	ListForEach (savers, ISettingsSaver*, saver)
		saver->flush (*this);
	EndFor

	return saveSettings (getPath ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API XmlSettings::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kBackupSettings)
	{
		if(isBackupEnabled ())
		{
			CCL_PRINT ("Backup settings ")
			CCL_PRINTLN (name)

			UnknownPtr<IUnknownList> list (msg[0].asUnknown ());
			ASSERT (list.isValid ())
			if(list)
				list->add (ccl_as_unknown (NEW Url (getPath ())));
		}
	}
	else
		SuperClass::notify (subject, msg);
}
