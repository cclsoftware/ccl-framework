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
// Filename    : ccl/app/presets/presetparam.cpp
// Description : Preset Parameter
//
//************************************************************************************************

#include "ccl/app/presets/presetparam.h"
#include "ccl/app/presets/presetsystem.h"

#include "ccl/app/utilities/fileicons.h"

#include "ccl/public/app/ipreset.h"
#include "ccl/public/app/presetmetainfo.h"

#include "ccl/public/text/translation.h"

#include "ccl/public/gui/framework/iuserinterface.h"
#include "ccl/public/guiservices.h"
#include "ccl/public/plugservices.h"

namespace CCL {

//************************************************************************************************
// PresetReference
//************************************************************************************************

class PresetReference: public Object
{
public:
	DECLARE_CLASS (PresetReference, Object)

	PresetReference (IPreset* preset = nullptr);

	PROPERTY_SHARED_AUTO (IPreset, preset, Preset)

	// Object
	int compare (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;
	tresult CCL_API queryInterface (UIDRef iid, void** ptr) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

//************************************************************************************************
// PresetFolder
//************************************************************************************************

class PresetFolder: public Object
{
public:
	PresetFolder (StringRef name = nullptr);

	PROPERTY_STRING (name, Name)

	void addPreset (PresetReference* p, bool share = true);
	Iterator* getPresets () const;

	void addFolder (PresetFolder* folder);
	PresetFolder* findFolder (StringRef name) const;
	Iterator* getSubFolders () const;

	// Object
	int compare (const Object& obj) const override;
	bool toString (String& string, int flags = 0) const override;

protected:
	ObjectArray presets;
	ObjectArray subFolders;
};

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("Presets")
	XSTRING (NoPreset, "No Preset")
END_XSTRINGS

//************************************************************************************************
// PresetReference
//************************************************************************************************

DEFINE_CLASS_HIDDEN (PresetReference, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetReference::PresetReference (IPreset* preset)
{
	setPreset (preset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API PresetReference::queryInterface (UIDRef iid, void** ptr)
{
	if(iid == ccl_iid<IPreset> () || iid == ccl_iid<IPresetCollection> ())
	{
		ASSERT (preset != nullptr)
		return preset->queryInterface (iid, ptr);
	}
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetReference::getProperty (Variant& var, MemberID propertyId) const
{
	UnknownPtr<IObject> presetObject (preset);
	if(presetObject && presetObject->getProperty (var, propertyId))
		return true;

	if(propertyId == "relativePath")
	{
		if(!preset)
			return false;

		String pathString;

		if(IAttributeList* metaInfo = preset->getMetaInfo ())
		{
			pathString = PresetMetaAttributes (*metaInfo).getSubFolder ();
			if(pathString.startsWith (Url::strPathChar))
				pathString.remove (0, 1);
			
			if(!pathString.isEmpty ())
				pathString.append (Url::strPathChar);
		}

		pathString.append (preset->getPresetName ());

		var = pathString;
		var.share ();
		return true;
	}

	return SuperClass::getProperty (var, propertyId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetReference::compare (const Object& obj) const
{
	String name;
	toString (name);
	String name2;
	obj.toString (name2);
	return name.compare (name2);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetReference::toString (String& string, int flags) const
{
	ASSERT (preset != nullptr)
	string = preset->getPresetName ();
	return true;
}

//************************************************************************************************
// PresetFolder
//************************************************************************************************

PresetFolder::PresetFolder (StringRef name)
: name (name)
{
	presets.objectCleanup (true);
	subFolders.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFolder::addPreset (PresetReference* p, bool share)
{
	presets.addSorted (share ? return_shared (p) : p);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PresetFolder::getPresets () const
{
	return presets.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetFolder::addFolder (PresetFolder* folder)
{
	subFolders.addSorted (folder);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetFolder* PresetFolder::findFolder (StringRef name) const
{
	ArrayForEach (subFolders, PresetFolder, folder)
		if(folder->getName () == name)
			return folder;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Iterator* PresetFolder::getSubFolders () const
{
	return subFolders.newIterator ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool PresetFolder::toString (String& string, int flags) const
{
	string = name;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int PresetFolder::compare (const Object& obj) const
{
	String name;
	toString (name);
	String name2;
	obj.toString (name2);
	return name.compare (name2);
}

//************************************************************************************************
// PresetParam
//************************************************************************************************

void PresetParam::registerClass () // force linkage
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (PresetParam, MenuParam)
DEFINE_CLASS_UID (PresetParam, 0x827d9a8e, 0xe871, 0x4681, 0x80, 0x39, 0xed, 0xdf, 0x8f, 0xf5, 0x32, 0x30)
DEFINE_CLASS_NAMESPACE (PresetParam, "Host")

//////////////////////////////////////////////////////////////////////////////////////////////////

PresetParam::PresetParam (StringID name)
: MenuParam (name),
  displayStyle (0)
{
	hasNoPresetItem (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::setMetaInfo (IAttributeList* _metaInfo)
{
	metaInfo.share (_metaInfo);

	if(metaInfo)
	{
		// Extend meta info if only a plain classID is given. PresetStore needs meta info for knowing the read/write locations,
		// e.g. to distinguish user presets from factory presets, see PresetStore::PresetFilter::isHiddenByUserPreset
		PresetMetaAttributes metaAttribs (*metaInfo);
		UID cid;
		if(metaAttribs.getClassID (cid)
			&& metaAttribs.getClassName ().isEmpty ()
			&& metaAttribs.getVendor ().isEmpty ()
			&& metaAttribs.getCategory ().isEmpty ())
		{
			if(const IClassDescription* description = System::GetPlugInManager ().getClassDescription (cid))
				metaAttribs.assign (*description);
		}
	}
	checkRebuild ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::setPresetFilter (IObjectFilter* _presetFilter)
{
	presetFilter.assign (_presetFilter);
	checkRebuild ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::hasNoPresetItem (bool state)
{
	if(state != noPresetItem ())
	{
		noPresetItem (state);
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::isAutoRebuild (bool state)
{
	if(state != autoRebuild ())
	{
		autoRebuild (state);
		setSignalAlways (state);
		setOutOfRange (state); // no indicator for current value
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::shouldShowFolders (bool state)
{
	if(state != showFolders ())
	{
		showFolders (state);
		checkRebuild ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPreset* PresetParam::getSelectedPreset () const
{
	PresetReference* ref = getObject<PresetReference> (getValue ());
	return ref ? ref->getPreset () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::checkRebuild ()
{
	if(autoRebuild ())
		removeAll ();
	else
		updateList ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::updateList ()
{
	removeAll ();

	if(metaInfo)
		if(AutoPtr<IUnknownList> presets = System::GetPresetManager ().getPresets (metaInfo))
			ForEachUnknown (*presets, unk)
				if(UnknownPtr<IPreset> preset = unk)
					if(!presetFilter || presetFilter->matches (unk))
						appendObject (NEW PresetReference (preset));
			EndFor

	list.sort ();

	if(noPresetItem ())
		appendString (XSTR (NoPreset), 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetParam::prepareStructure ()
{
	if(autoRebuild ())
	{
		WaitCursor waitCursor (System::GetGUI ());
		updateList ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API PresetParam::cleanupStructure ()
{
	if(autoRebuild ())
		removeAll ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID CCL_API PresetParam::getMenuType () const
{
	return MenuPresentation::kTree;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetParam::onMenuKeyDown (const KeyEvent& event)
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void PresetParam::buildMenu (PresetFolder& parent, IMenu& menu, IParameterMenuBuilder& builder)
{
	auto& icons = FileIcons::instance ();

	IterForEach (parent.getSubFolders (), PresetFolder, child)
		IMenuItem* item = builder.addSubMenu (menu, *this, child->getName ());
		item->setItemAttribute (IMenuItem::kItemIcon, icons.getDefaultFolderIcon ());
		buildMenu (*child, *item->getItemMenu (), builder);
	EndFor

	IterForEach (parent.getPresets (), PresetReference, ref)
		int value = list.index (ref);
		ASSERT (value != -1)
		IMenuItem* item = builder.addValueItem (menu, *this, value);
		
		ASSERT (ref->getPreset ())

		Url path;
		ref->getPreset ()->getUrl (path);
		AutoPtr<IImage> icon = icons.createIcon (path);

		item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetParam::buildMenu (IMenu& menu, IParameterMenuBuilder& builder)
{
	if(showFolders ())
	{
		// build preset folders
		PresetFolder root;
		ArrayForEach (list, Object, obj)
			if(PresetReference* ref = ccl_cast<PresetReference> (obj))
			{
				ASSERT (ref->getPreset ())
				
				String folderName;
				if(IAttributeList* presetInfo = ref->getPreset ()->getMetaInfo ())
				{
					folderName = PresetMetaAttributes (*presetInfo).getSubFolder ();
					if(folderName.startsWith (Url::strPathChar))
						folderName.remove (0, 1);
				}

				PresetFolder* parent = &root;
				if(!folderName.isEmpty ())
				{
					ForEachStringToken (folderName, Url::strPathChar, subFolderName)
						PresetFolder* subFolder = parent->findFolder (subFolderName);
						if(subFolder == nullptr)
							parent->addFolder (subFolder = NEW PresetFolder (subFolderName));
						parent = subFolder;
					EndFor					
				}

				parent->addPreset (ref);
			}
		EndFor

		// build menu
		if(noPresetItem ())
			builder.addValueItem (menu, *this, 0);

		buildMenu (root, menu, builder);
	}
	else
	{
		int v = 0;
		ArrayForEach (list, Object, obj)
			IMenuItem* item = builder.addValueItem (menu, *this, v);
			if(PresetReference* ref = ccl_cast<PresetReference> (obj))
			{
				ASSERT (ref->getPreset ())

				Url path;
				ref->getPreset ()->getUrl (path);
				AutoPtr<IImage> icon = FileIcons::instance ().createIcon (path);

				item->setItemAttribute (IMenuItem::kItemIcon, Variant (icon));
			}
			v++;
		EndFor
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_METHOD_NAMES (PresetParam)
	DEFINE_METHOD_ARGS ("setMetaInfo", "metaInfo: Attributes")
	DEFINE_METHOD_ARGS ("shouldShowFolders", "state: boolean")
	DEFINE_METHOD_ARGS ("selectRelativePath", "path: string")
END_METHOD_NAMES (PresetParam)

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API PresetParam::invokeMethod (Variant& returnValue, MessageRef msg)
{
	if(msg == "setMetaInfo")
	{
		UnknownPtr<IAttributeList> metaInfo (msg[0]);
		setMetaInfo (metaInfo);
		return true;
	}
	else if(msg == "shouldShowFolders")
	{
		shouldShowFolders (msg[0]);
		return true;
	}
	else if(msg == "selectRelativePath")
	{
		String name = msg[0].asString ();
		String folder;

		int slashIndex = name.lastIndex (Url::strPathChar);
		if(slashIndex >= 0)
		{
			folder = name.subString (0, slashIndex);
			name = name.subString (slashIndex + 1);
		}

		int index = 0;
		ArrayForEach (list, Object, obj)
			if(PresetReference* ref = ccl_cast<PresetReference> (obj))
			{
				ASSERT (ref->getPreset ())
				
				String folderName;
				if(IAttributeList* presetInfo = ref->getPreset ()->getMetaInfo ())
				{
					folderName = PresetMetaAttributes (*presetInfo).getSubFolder ();
					if(folderName.startsWith (Url::strPathChar))
						folderName.remove (0, 1);
				}

				if(ref->getPreset ()->getPresetName () == name && folderName == folder)
				{
					setValue (index, true);
					break;
				}
			}
			index++;
		EndFor
		return true;
	}
	else
		return SuperClass::invokeMethod (returnValue, msg);
}
