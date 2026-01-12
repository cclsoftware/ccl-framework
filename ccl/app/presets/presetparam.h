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
// Filename    : ccl/app/presets/presetparam.h
// Description : Preset Parameter
//
//************************************************************************************************

#ifndef _ccl_presetparam_h
#define _ccl_presetparam_h

#include "ccl/app/params.h"

#include "ccl/public/storage/iattributelist.h"
#include "ccl/public/gui/framework/iparametermenu.h"

namespace CCL {

interface IPreset;
class PresetFolder;

//************************************************************************************************
// PresetParam
//************************************************************************************************

class PresetParam: public MenuParam,
				   public StructuredParameter,
				   public IParameterMenuCustomize
{
public:
	DECLARE_CLASS (PresetParam, MenuParam)
	DECLARE_METHOD_NAMES (Parameter)

	enum DisplayStyle
	{
		kNoPresetItem = 1<<0,
		kAutoRebuild  = 1<<1,
		kShowFolders  = 1<<2
	};

	PresetParam (StringID name = nullptr);

	static void registerClass ();

	void setMetaInfo (IAttributeList* metaInfo);
	void setPresetFilter (IObjectFilter* presetFilter); // takes ownership

	void hasNoPresetItem (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kNoPresetItem, hasNoPresetItem)

	void isAutoRebuild (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kAutoRebuild, isAutoRebuild)

	void shouldShowFolders (bool state);
	PROPERTY_READONLY_FLAG (displayStyle, kShowFolders, shouldShowFolders)

	IPreset* getSelectedPreset () const;

	// StructuredParameter
	void CCL_API prepareStructure () override;
	void CCL_API cleanupStructure () override;

	CLASS_INTERFACE2 (IStructuredParameter, IParameterMenuCustomize, MenuParam)

protected:
	AutoPtr<IAttributeList> metaInfo;
	AutoPtr<IObjectFilter> presetFilter;
	int displayStyle;

	void updateList ();
	void checkRebuild ();

	PROPERTY_FLAG (displayStyle, kNoPresetItem, noPresetItem)
	PROPERTY_FLAG (displayStyle, kAutoRebuild, autoRebuild)
	PROPERTY_FLAG (displayStyle, kShowFolders, showFolders)

	void buildMenu (PresetFolder& parent, IMenu& menu, IParameterMenuBuilder& builder);

	// IParameterMenuCustomize
	StringID CCL_API getMenuType () const override;
	tbool CCL_API buildMenu (IMenu& menu, IParameterMenuBuilder& builder) override;
	tbool CCL_API onMenuKeyDown (const CCL::KeyEvent& event) override;

	// IObject
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

} // namespace CCL

#endif // _ccl_presetparam_h
