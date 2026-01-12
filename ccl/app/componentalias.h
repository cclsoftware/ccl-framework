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
// Filename    : ccl/app/componentalias.h
// Description : Component Alias
//
//************************************************************************************************

#ifndef _ccl_componentalias_h
#define _ccl_componentalias_h

#include "ccl/app/component.h"

namespace CCL {

interface IClassDescription;

//************************************************************************************************
// ComponentAlias
/** Alias class for external components. */
//************************************************************************************************

class ComponentAlias: public Component,
					  public IComponentAlias
{
public:
	DECLARE_CLASS (ComponentAlias, Component)

	ComponentAlias ();
	~ComponentAlias ();

	/** Assign plug-in object to alias. */
	virtual bool assignAlias (IUnknown* object);

	/** Release plug-in object references. */
	virtual void detachAlias ();

	/** Verify alias. */
	virtual bool verifyAlias () const;

	/** Get class description of plug-in object. */
	const IClassDescription* getClassDescription () const;

	// IComponentAlias
	IUnknown* CCL_API getPlugInUnknown () const override;
	IUnknown* CCL_API getHostContext () override;

	// Component
	tresult CCL_API initialize (IUnknown* context = nullptr) override;
	tresult CCL_API terminate () override;
	tbool CCL_API canTerminate () const override;
	IUnknown* CCL_API getExtension (StringID id) override;
	IParameter* CCL_API findParameter (StringID name) const override;
	int CCL_API countParameters () const override;
	IParameter* CCL_API getParameterAt (int index) const override;
	IParameter* CCL_API getParameterByTag (int tag) const override;
	IView* CCL_API createView (StringID name, VariantRef data, const Rect& bounds) override;
	tbool CCL_API checkCommandCategory (CStringRef category) const override;
	tbool CCL_API interpretCommand (const CommandMsg& msg) override;
	tresult CCL_API appendContextMenu (IContextMenu& contextMenu) override;
	UIDRef CCL_API getClassUID () const override;
	IObjectNode* CCL_API findChild (StringRef id) const override;
	IUnknown* CCL_API getObject (StringID name, UIDRef classID) override;
	bool load (const Storage& storage) override;
	bool save (const Storage& storage) const override;

	CLASS_INTERFACE (IComponentAlias, Component)

protected:
	UnknownPtr<IUnknown> unknownPtr;

	// Component
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
};

} // namespace CCL

#endif // _ccl_componentalias_h
