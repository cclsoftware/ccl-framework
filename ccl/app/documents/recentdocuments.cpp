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
// Filename    : ccl/app/documents/recentdocuments.cpp
// Description : Recent Document Management
//
//************************************************************************************************

#include "ccl/app/documents/recentdocuments.h"

#include "ccl/app/utilities/fileicons.h"

#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/system/formatter.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/gui/framework/icommandtable.h"
#include "ccl/public/gui/framework/imenu.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/itheme.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/ialert.h"
#include "ccl/public/gui/framework/controlproperties.h"
#include "ccl/public/text/translation.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/system/isysteminfo.h"

namespace CCL {

//************************************************************************************************
// RecentDocuments::Saver
//************************************************************************************************

class RecentDocuments::Saver: public SettingsSaver
{
public:
	Saver (RecentDocuments* owner): owner (owner) {}
	
	PROPERTY_POINTER (RecentDocuments, owner, Owner)

	// SettingsSaver
	void restore (Settings&) override { /*nothing here*/ }
	void flush (Settings&) override { if(owner) owner->commitPaths (); }
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Documents")
	XSTRING (RecentFiles, "Recent Files")
	XSTRING (ClearRecentFiles, "Clear Recent Files")
	XSTRING (AskClearRecentFiles, "Do you really want to clear the list of recent files?\n\nThis action can not be undone.")
END_XSTRINGS

//************************************************************************************************
// RecentDocuments
//************************************************************************************************

StringRef RecentDocuments::getTranslatedTitle ()
{
	return XSTR (RecentFiles);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::shouldSaveRelative ()
{	
	#if CCL_PLATFORM_IOS // User documents path could change on iOS between app updates
	return true;
	#else
	return false;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::getRelativeLocation (Url& folder)
{
	return System::GetSystem ().getLocation (folder, System::kUserDocumentFolder) != 0;	
}


//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_HIDDEN (RecentDocuments, Component)

//////////////////////////////////////////////////////////////////////////////////////////////////

RecentDocuments::RecentDocuments (StringRef name, int maxPathCount, int maxMenuEntries, int options)
: Component (name),
  maxPathCount (maxPathCount),
  maxMenuEntries (maxMenuEntries),
  options (options),
  settings (*NEW XmlSettings ("RecentDocuments")),
  saver (nullptr),
  restoredOnce (false)
{
	paths.objectCleanup (true);
	pinnedPaths.objectCleanup (true);

	saver = NEW Saver (this);
	settings.addSaver (saver);
	settings.enableSignals (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RecentDocuments::~RecentDocuments ()
{
	saver->setOwner (nullptr);
	settings.removeSaver (saver);
	safe_release (saver);	
	settings.enableSignals (false);
	settings.release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::hasMenus () const
{
	return !menus.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::addMenu (IMenu* menu)
{
	menus.add (menu, true);
	updateMenus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::removeMenus ()
{
	menus.removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::updateMenus ()
{
	if(menus.isEmpty ())
		return;

	IImage* pinIcon = getTheme ()->getImage ("RecentDocuments.PinMenuIcon");

	ForEachUnknown (menus, unk)
		UnknownPtr<IMenu> menu (unk);
		if(!menu)
			continue;

		menu->removeAll ();

		int i = 0;
		AutoPtr<Iterator> iter (newRecentPathsIterator (true));
		for(auto url : iterate_as<Url> (*iter))
		{
			String title;
			if(options & kShowFullPathInMenu)
			{
				Url urlCopy (*url);
				
				String fileName;
				urlCopy.getName (fileName);
				
				String filePath;
				urlCopy.ascend ();
				urlCopy.toDisplayString (filePath);

				FontRef font = getTheme ()->getThemeFont (ThemeElements::kMenuFont);
				Font::collapseString (filePath, 300.f, font);
				
				title.appendFormat ("%(1) (%(2))", fileName, filePath);
			}
			else
				url->getName (title);

			int index = paths.index (url); // position in plain recent paths order (we are iterating pinned first)

			MutableCString commandName;
			commandName.appendFormat ("%d", index + 1);
			IMenuItem* menuItem = menu->addCommandItem (title, CSTR ("Recent File"), commandName);
			if(pinIcon && isPathPinned (*url))
				menuItem->setItemAttribute (IMenuItem::kItemIcon, pinIcon);

			if(++i >= maxMenuEntries)
				break;
		}

		menu->addSeparatorItem ();
		menu->addCommandItem (XSTR (ClearRecentFiles), CSTR ("File"), CSTR ("Clear Recent Files"), nullptr);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::changed (bool saveNeeded)
{
	if(frame)
		UnknownPtr<IForm> (frame)->reload ();

	updateMenus ();

	signal (Message (kChanged));

	if(saveNeeded)
		store ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int RecentDocuments::getPathIndex (UrlRef url, ObjectArray& container) const
{
	// don't compare parameters
	for(int i = 0; i < container.count (); i++)
	{
		Url* p = (Url*)container.at (i);
		if(p->isEqualUrl (url, false))
			return i;	
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::setRecentPath (UrlRef path)
{
	int index = getPathIndex (path, paths);
	if(index != -1)
	{
		if(index != 0) // move to top
		{
			Url* p = (Url*)paths.at (index);
			paths.remove (p);
			paths.insertAt (0, p);
		}
	}
	else
	{
		Url* path2 = NEW Url (path);
		paths.insertAt (0, path2);

		bool ignorePinned = false;
		while(paths.count () > maxPathCount)
		{
			for(int lastIndex = paths.count ()-1; lastIndex > 1; lastIndex--)
			{
				Url* p = (Url*)paths.at (lastIndex);
				if(ignorePinned || isPathPinned (*p) == false)
				{
					paths.remove (p);
					p->release ();
					break;
				}
			}
			ignorePinned = true; // ignore pinned on second try
		}
	}

	System::GetSystemShell ().addRecentFile (path);

	changed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::removeRecentPath (UrlRef path)
{
	setPathPinned (path, false);

	int index = getPathIndex (path, paths);
	if(index != -1)
	{
		Url* p = (Url*)paths.at (index);
		paths.remove (p);
		p->release ();
	}

	changed ();

	return index != -1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* RecentDocuments::newRecentPathsIterator (bool pinnedFirst) const
{
	if(pinnedFirst)
	{
		auto makeIterator = [&] (bool pinned)
		{
			// create iterator that filters only pinned or unpinned documents
			return makeFilteringIterator (paths.newIterator (), [&, pinned] (IUnknown* obj)
			{
				auto url = unknown_cast<Url> (obj);
				return pinnedPaths.contains (*url) == pinned;
			});
		};

		return createConcatenatedIterator (makeIterator (true), makeIterator (false)); // 1.) only pinned, 2.) only unpinned
	}
	else
		return paths.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::isPathPinned (UrlRef path) const
{
	return pinnedPaths.contains (Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::setPathPinned (UrlRef path, bool state)
{
	if(state)
	{
		if(!pinnedPaths.contains (Url (path)))
		{
			pinnedPaths.add (NEW Url (path));
			changed ();
		}
	}
	else
	{
		if(Url* p = (Url*)pinnedPaths.findEqual (Url (path)))
		{
			pinnedPaths.remove (p);
			p->release ();
			changed ();
		}
	}

	updateMenus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* RecentDocuments::newPinnedPathsIterator () const
{
	return pinnedPaths.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::relocate (UrlRef oldUrl, UrlRef newUrl)
{
	bool isFolder = oldUrl.isFolder ();
	ASSERT (newUrl.isFolder () == isFolder)

	auto relocateRecent = [&] (Url& recentPath, UrlRef newDocPath)
	{
		// replace if the file exists in the new location
		if(System::GetFileSystem ().fileExists (newDocPath))
		{
			recentPath = newDocPath;
			CCL_PRINTF ("relocate document: %s\n", MutableCString (UrlFullString (newDocPath)).str ())
		}
	};

	// relocate documents
	ArrayForEach (paths, Url, docPath)
		if(isFolder)
		{
			Url newDocPath (*docPath);
			if(newDocPath.makeRelative (oldUrl))
			{
				newDocPath.makeAbsolute (newUrl);
				relocateRecent (*docPath, newDocPath);
			}
		}
		else if(*docPath == oldUrl)
			relocateRecent (*docPath, newUrl);
	EndFor

	// relocate pinned files
	ObjectList missingPinned;
	auto relocatePinned = [&] (Url* pinnedPath, UrlRef newPath)
	{
		// replace if the file exists in the new location; unpin if it doesn't exist in either location
		if(System::GetFileSystem ().fileExists (newPath))
		{
			CCL_PRINTF ("relocate pinned: %s\n", MutableCString (UrlFullString (newPath)).str ())
			*pinnedPath = newPath;
		}
		else if(!System::GetFileSystem ().fileExists (*pinnedPath))
			missingPinned.add (pinnedPath);
	};

	ArrayForEach (pinnedPaths, Url, pinnedPath)
		if(isFolder)
		{
			Url newPath (*pinnedPath);
			if(newPath.makeRelative (oldUrl))
			{
				newPath.makeAbsolute (newUrl);
				relocatePinned (pinnedPath, newPath);
			}
		}
		else
			relocatePinned (pinnedPath, newUrl);
	EndFor

	ListForEachObject (missingPinned, Url, p)
		if(pinnedPaths.remove (p))
			p->release ();
	EndFor

	changed ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url RecentDocuments::getSettingsPath () const
{
	XmlSettings* xmlSettings = ccl_cast<XmlSettings> (&settings);
	ASSERT (xmlSettings)
	return xmlSettings ? Url (xmlSettings->getPath ()) : Url ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Url RecentDocuments::makeBackupPath () const
{
	Url backupPath (getSettingsPath ());
	String fileName;
	backupPath.getName (fileName);
	backupPath.setName (fileName << ".bak");
	return backupPath;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::clearAll ()
{
	if(paths.isEmpty ())
		return;

	if(Alert::ask (XSTR (AskClearRecentFiles)) != Alert::kYes)
		return;

	// copy old settings file as backup
	System::GetFileSystem ().copyFile (makeBackupPath (), getSettingsPath ());

	paths.removeAll ();
	pinnedPaths.removeAll ();

	changed (); // (immediately save the empty file)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int RecentDocuments::count () const
{
	return paths.count ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const Url* RecentDocuments::at (int index) const
{
	return (Url*)paths.at (index);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RecentDocuments::contains (UrlRef path) const
{
	return paths.contains (Url (path));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API RecentDocuments::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "numRecent")
	{
		var = count ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IView* CCL_API RecentDocuments::createView (StringID name, VariantRef data, const Rect& bounds)
{
	if(name == "RecentFilesFrame")
	{
		ITheme* theme = getTheme ();
		ASSERT (theme != nullptr)
		frame = theme ? theme->createView ("RecentFileList", this->asUnknown ()) : nullptr;
		if(frame)
		{
			SizeLimit limits (0, 0, kMaxCoord, frame->getSize ().getHeight ());
			frame->setSizeLimits (limits);
		}
		return frame;
	}
	else if(name.startsWith ("@recent"))
	{
		int index = 0;
		::sscanf (name, "@recent[%d]", &index);
		if(const Url* url = at (index))
		{
			if(name.contains (".icon"))
			{
				ViewBox imageView (ClassID::ImageView, bounds);
				AutoPtr<IImage> icon (FileIcons::instance ().createIcon (*url));
				imageView.setAttribute (kImageViewBackground, icon);
				imageView.setStyle (StyleFlags (0, Styles::kImageViewAppearanceFitImage));
				return imageView;
			}
			else if(name.contains (".age"))
			{
				ViewBox label (ClassID::Label, bounds);
				String title, tooltip;

				FileInfo fileInfo;
				if(System::GetFileSystem ().getFileInfo (fileInfo, *url))
				{
					title = Format::TimeAgo::print (fileInfo.modifiedTime);
					tooltip = Format::DateTime::print (fileInfo.modifiedTime, Format::DateTime::kFriendlyDateTime);
				}

				label.setTitle (title);
				label.setTooltip (tooltip);
				return label;
			}
			else
			{
				String title;
				url->getName (title, false);
				MutableCString cmdName;
				cmdName.appendFormat ("%d", index + 1);

				ControlBox linkView (ClassID::LinkView, System::GetCommandTable ().getCommandParam (CSTR ("Recent File"), cmdName), 
									 bounds, StyleFlags (0, Styles::kLinkViewAppearanceFitTitle), title);
				linkView.setTooltip (UrlDisplayString (*url));
				return linkView;
			}
		}
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API RecentDocuments::appendContextMenu (IContextMenu& contextMenu)
{
	contextMenu.addCommandItem (XSTR (ClearRecentFiles), CSTR ("File"), CSTR ("Clear Recent Files"), nullptr);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::commitPaths ()
{
	ASSERT (restoredOnce == true)

	// make copy - do not modify data for relative storage
	ObjectArray savePaths (paths); 
	ObjectArray savePinnedPaths (pinnedPaths);
	
	if(shouldSaveRelative ())
	{	
		Url relativeFolder;
		if(getRelativeLocation (relativeFolder))
		{
			ArrayForEach (savePaths, Url, docPath)
				docPath->makeRelative (relativeFolder);
			EndFor
			ArrayForEach (savePinnedPaths, Url, pinnedPath)
				pinnedPath->makeRelative (relativeFolder);
			EndFor
		}
	}
			
	ASSERT (!getName ().isEmpty ())
	Attributes& a = settings.getAttributes (getName ());
	a.removeAll ();
	a.queue (nullptr, savePaths, Attributes::kOwns);

	Attributes& a2 = settings.getAttributes (String () << getName () << ".pinned");
	a2.removeAll ();
	a2.queue (nullptr, savePinnedPaths, Attributes::kOwns);

	savePaths.objectCleanup (false); // owership passed to attributes
	savePinnedPaths.objectCleanup (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::store ()
{
	ASSERT (restoredOnce == true)
	if(restoredOnce == false) // workaround for early program exit
		return;

	settings.flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RecentDocuments::restore ()
{
	ASSERT (!getName ().isEmpty ())

	if(!restoredOnce)
	{
		// make sure settings aren't auto-saved before restore
		restoredOnce = true;
		settings.isBackupEnabled (true);
		settings.isAutoSaveEnabled (true);
	}

	settings.restore ();

	// was previously stored in the global settings instance; fallback if own file does not exist yet, but ignore if clearAll has already created a backup
	bool loadPreviousLocation = settings.isEmpty () && !System::GetFileSystem ().fileExists (makeBackupPath ());
	Settings& loadedSettings = loadPreviousLocation ? Settings::instance () : settings;

	Attributes& a = loadedSettings.getAttributes (getName ());
	paths.removeAll ();
	a.unqueue (paths, nullptr, ccl_typeid<Url> ());
	
	Attributes& a2 = loadedSettings.getAttributes (String () << getName () << ".pinned");
	pinnedPaths.removeAll ();
	a2.unqueue (pinnedPaths, nullptr, ccl_typeid<Url> ());

	Url relativeFolder;
	if(getRelativeLocation (relativeFolder)) // check for any relative paths 
	{
		ArrayForEach (paths, Url, docPath)
			if(docPath->isRelative ())
				docPath->makeAbsolute (relativeFolder);
		EndFor		
		ArrayForEach (pinnedPaths, Url, pinnedPath)
			if(pinnedPath->isRelative ())
				pinnedPath->makeAbsolute (relativeFolder);
		EndFor
	}

	if(loadPreviousLocation)
	{
		// create new settings file now
		store ();

		// remove empty group from global settings
		loadedSettings.remove ("RecentDocuments");
		loadedSettings.remove ("RecentDocuments.pinned");
	}

	changed (false);
}
