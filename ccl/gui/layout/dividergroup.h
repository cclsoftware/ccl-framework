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
// Filename    : ccl/gui/layout/dividergroup.h
// Description : Groups of synchronized Dividers
//
//************************************************************************************************

#include "ccl/base/objectnode.h"
#include "ccl/base/collections/objectlist.h"

#include "ccl/public/gui/icontroller.h"
#include "ccl/public/gui/iparamobserver.h"

#include "ccl/app/params.h"

namespace CCL {

//************************************************************************************************
// DividerGroups
/** 
	Every workspace perspective has a set of divider groups. This "DividerGroups" controller can be adressed as
		"://Workspace/workspaceID/perspectiveID/DividerGroups" or
		"://Workspace/workspaceID/~/DividerGroups" (for the current perspective)

	A divider group can be established by using this controller and the same parameter name for each divider:
		<Divider name="://Workspace/workspaceID/~/DividerGroups/groupName">
*/
//************************************************************************************************

class DividerGroups: public ObjectNode,
					 public AbstractController
{
public:
	DividerGroups ();

	class Group;
	class DividerParam;

	PROPERTY_POINTER (Object, dirtySink, DirtySink)

	void flush ();

	// IController
	IParameter* CCL_API findParameter (StringID name) const override;
	
	CLASS_INTERFACE (IController, ObjectNode)
		
private:
	ObjectList groups;
    
	Group* getGroup (StringID name);
};

//************************************************************************************************
// DividerGroups::Group
//************************************************************************************************

class DividerGroups::Group: public Object,
							public IParamObserver
{
public:
	DECLARE_CLASS (DividerGroups::Group, Object)

	Group (StringID name = nullptr);
	
	PROPERTY_MUTABLE_CSTRING (name, Name)
	PROPERTY_POINTER (Object, dirtySink, DirtySink)

	IParameter* newParameter ();
	void removeParameter (IParameter* param);
	Parameter* getAlignmentParam (bool needsConnectedParameter = true) const;
	
	void flush ();

	// IParamObserver
	void CCL_API paramEdit (IParameter* param, tbool begin) override;
	tbool CCL_API paramChanged (IParameter* param) override;

	// Object
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACE (IParamObserver, Object)

private:
	ObjectList params;
	IParameter* editParam;
	int lastValue;
	
	void synchronize (IParameter* param, bool absolute);
};

//************************************************************************************************
// DividerGroups::DividerParam
//************************************************************************************************

class DividerGroups::DividerParam: public IntParam
{
public:
	DECLARE_CLASS (DividerGroups::DividerParam, IntParam)
	
	DividerParam (StringID name = nullptr);

	// IntParam
	unsigned int CCL_API release () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
	PROPERTY_BOOL (initialized, Initialized)
	PROPERTY_BOOL (dividerConnected, DividerConnected)
};
} // namespace CCL
