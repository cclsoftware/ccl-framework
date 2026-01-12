//************************************************************************************************
//
// CCL Spy
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
// Filename    : viewproperty.cpp
// Description : 
//
//************************************************************************************************

#include "viewproperty.h"
#include "viewclass.h"

#include "ccl/app/utilities/boxedguitypes.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/settings.h"

#include "ccl/public/gui/framework/iskinmodel.h"
#include "ccl/public/gui/framework/iview.h"
#include "ccl/public/gui/framework/iform.h"
#include "ccl/public/gui/framework/iview3d.h"
#include "ccl/public/gui/framework/viewbox.h"
#include "ccl/public/gui/framework/designsize.h"
#include "ccl/public/gui/graphics/igraphics.h"
#include "ccl/public/gui/graphics/3d/iscene3d.h"
#include "ccl/public/gui/framework/isystemshell.h"
#include "ccl/public/gui/framework/skinxmldefs.h"
#include "ccl/public/gui/iparameter.h"
#include "ccl/public/text/stringbuilder.h"
#include "ccl/public/collections/vector.h"
#include "ccl/public/base/variant.h"
#include "ccl/public/base/iobjectnode.h"
#include "ccl/public/guiservices.h"

#include "ccl/main/cclargs.h"
#include "ccl/public/system/iexecutable.h"
#include "ccl/public/systemservices.h"

using namespace CCL;
using namespace Spy;

//////////////////////////////////////////////////////////////////////////////////////////////////

#if CCL_PLATFORM_WINDOWS
#define kEditorPath "C:\\Program Files\\Notepad++\\notepad++.exe"
#define kEditorArgs "-n%(2) %(1)"
#else
#define kEditorPath "" // todo
#define kEditorArgs ""
#endif

static Configuration::StringValue editorPath ("xmlEditor", "path", kEditorPath);
static Configuration::StringValue editorArgs ("xmlEditor", "args", kEditorArgs);

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT_LEVEL (xmlEditorOption, kFirstRun)
{
	Settings::instance ().addSaver (NEW ConfigurationSaver ("xmlEditor", "path"));
	Settings::instance ().addSaver (NEW ConfigurationSaver ("xmlEditor", "args"));
	return true;
}

//************************************************************************************************
// SizeModeProperty
//************************************************************************************************

void SizeModeProperty::toString (String& string, VariantRef value)
{
	int sizeMode = value.asInt ();

	if((sizeMode & IView::kAttachAll) == IView::kAttachAll)
		string = "all ";
	else
	{
		if(sizeMode & IView::kAttachLeft)
			string << "left ";
		if(sizeMode & IView::kAttachTop)
			string << "top ";
		if(sizeMode & IView::kAttachRight)
			string << "right ";
		if(sizeMode & IView::kAttachBottom)
			string << "bottom ";
	}
	if(sizeMode & IView::kHCenter)
		string << "hcenter ";
	if(sizeMode & IView::kVCenter)
		string << "vcenter ";

	if((sizeMode & IView::kFitSize) == IView::kFitSize)
		string << "fitsize ";
	else
	{
		if(sizeMode & IView::kHFitSize)
			string << "hfit ";
		if(sizeMode & IView::kVFitSize)
			string << "vfit ";
	}

	if(sizeMode & IView::kPreferCurrentSize)
		string << "prefercurrent ";

	if(sizeMode & IView::kFill)
		string << "fill ";
}

//************************************************************************************************
// SizeProperty
//************************************************************************************************

bool SizeProperty::assignSize (Variant& value, RectRef size)
{
	AutoPtr<Boxed::Rect> r (NEW Boxed::Rect (size));
	value = Variant (r->asUnknown (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SizeProperty::getValue (Variant& value, IView* view)
{
	return assignSize (value, view->getSize ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SizeProperty::toString (String& string, VariantRef value)
{
	if(Boxed::Rect* r = unknown_cast<Boxed::Rect> (value))
	{
		Variant args[] = { r->left, r->top, r->right, r->bottom, r->getWidth (), r->getHeight () };
		string.appendFormat ("%(1), %(2), %(3), %(4) (%(5) x %(6))",  args, ARRAY_COUNT(args));
	}
}

//************************************************************************************************
// FlexProperty
//************************************************************************************************

FlexProperty::FlexProperty (StringID attributeID)
: attributeID (attributeID)
{
	MutableCString _name (attributeID);
    MutableCString firstCharacter = _name.subString (0, 1).toUppercase ();
	_name.replace (0, 1, firstCharacter);
	setName (_name);
}

//************************************************************************************************
// FlexItemProperty
//************************************************************************************************

bool FlexItemProperty::getValue (Variant& value, IView* view)
{
	IView* parentView = view->getParentByClass (ClassID::LayoutView);
	if(parentView == nullptr)
		return false;
	
	UnknownPtr<ILayoutView> layoutView (parentView);
	if(!layoutView.isValid ())
		return false;

	Attributes attributes;
	layoutView->getChildLayoutAttributes (attributes, view);
	
	if(!attributes.contains (attributeID))
		return false;
	
	attributes.getAttribute (value, attributeID);
	if(value.asString () == DesignCoord::kStrUndefined)
		return false;
	
	return true;
}

//************************************************************************************************
// FlexContainerProperty
//************************************************************************************************

bool FlexContainerProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<ILayoutView> layoutView (view);
	if(!layoutView.isValid ())
		return false;

	Attributes attributes;
	layoutView->getLayoutAttributes (attributes);
	StringID propertyID = getName ();
	if(!attributes.contains (attributeID))
		return false;
	
	attributes.getAttribute (value, attributeID);
	if(value.asString () == DesignCoord::kStrUndefined)
		return false;
	
	return true;
}

//************************************************************************************************
// SizeLimitsProperty
//************************************************************************************************

bool SizeLimitsProperty::getValue (Variant& value, IView* view)
{
	AutoPtr<Boxed::Rect> r (NEW Boxed::Rect ((view->getSizeLimits ())));
	value = Variant (r->asUnknown (), true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SizeLimitsProperty::toString (String& string, VariantRef value)
{
	if(Boxed::Rect* r = unknown_cast<Boxed::Rect> (value))
	{
		const SizeLimit& limits (*r);
		SizeLimit unlimited;
		unlimited.setUnlimited ();
		if(limits == unlimited)
			string = "none";
		else
		{
			Variant args2[] = {limits.minWidth, limits.minHeight };
			string.appendFormat ("%(1), %(2), ",  args2, 2);

			if(limits.maxWidth == kMaxCoord)
				string.append ("oo");
			else
				string.appendIntValue (limits.maxWidth);
			string.append (", ");
			if(limits.maxHeight == kMaxCoord)
				string.append ("oo");
			else
				string.appendIntValue (limits.maxHeight);
		}
	}
}

//************************************************************************************************
// StyleFlagsProperty
//************************************************************************************************

void StyleFlagsProperty::toString (String& string, VariantRef value)
{
	StyleFlags style;
	style.fromLargeInt (value);

	#define CHECK_STYLE(flag,s)if(style.isCommonStyle (Styles::flag)) string << s" ";
	CHECK_STYLE (kHorizontal,	"horizontal")
	CHECK_STYLE (kVertical,		"vertical")
	CHECK_STYLE (kBorder,		"border")
	CHECK_STYLE (kTransparent,	"transparent")
	CHECK_STYLE (kComposited,	"composited")
	CHECK_STYLE (kTranslucent,	"translucent")
	CHECK_STYLE (kTrigger,		"trigger")
	if(!style.isComposited ())
		CHECK_STYLE (kDirectUpdate,	"directupdate")
	CHECK_STYLE (kSmall,		"small")
	CHECK_STYLE (kLeft,			"left")
	CHECK_STYLE (kRight,		"right")
	CHECK_STYLE (kLayerUpdate,	"layerupdate")
	CHECK_STYLE (kNoHelpId,		"nohelp")
	#undef CHECK_STYLE
	if(style.custom != 0)
	{
		string << "Custom: 0x";
		string.appendHexValue (style.custom);
		// todo: decode custom styles
	}
}

//************************************************************************************************
// VisualStyleProperty
//************************************************************************************************

bool VisualStyleProperty::getValue (Variant& value, IView* view)
{
	const IVisualStyle* vs = &view->getVisualStyle ();
	if(vs && !vs->getName ().isEmpty ())
	{
		value = Variant (const_cast<IVisualStyle*> (vs), true);
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void VisualStyleProperty::toString (String& string, VariantRef value)
{
	UnknownPtr<IVisualStyle> vs (value);
	if(vs)
	{
		string = String (vs->getName ());

		// check for VisualStyleAlias from StyleSelector
		const IVisualStyle* style = vs;
		while(true)
		{
			// a style alias has a different original
			const IVisualStyle* original = style->getOriginal ();
			if(original == style || original == nullptr)
				break;

			string << String (Text::kUTF8, u8" \u2192 ") << original->getName (); // right arrow
			style = original;
		}
	}
}

//************************************************************************************************
// ZoomFactorProperty
//************************************************************************************************

bool ZoomFactorProperty::getValue (Variant& value, IView* view)
{
	value = view->getZoomFactor ();
	return true;
}

//************************************************************************************************
// ControllerPathProperty
//************************************************************************************************

bool ControllerPathProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<IObjectNode> controller = view->getController ();
	if(!controller)
	{
		UnknownPtr<IControl> control (view);
		IParameter* param = control ? control->getParameter () : nullptr;
		if(param)
			controller = param->getController ();
	}
	if(controller)
	{
		String path;
		controller->getChildPath (path);
		value = Variant (path, true);
		return true;
	}
	return false;
}

//************************************************************************************************
// FormNameProperty
//************************************************************************************************

bool FormNameProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<IForm> form (view);
	if(form)
		value = Variant (String (form->getFormName ()), true);
	return true;
}

//************************************************************************************************
// SourceCodeProperty
//************************************************************************************************

bool SourceCodeProperty::getValue (Variant& value, IView* view)
{
	while(view)
	{
		UnknownPtr<IForm> form (view);
		if(form)
		{
			AutoPtr<SourceInfo> source (NEW SourceInfo);
			ISkinElement* element = form->getISkinElement ();
            if(element != nullptr)
            {
                element->getSourceInfo (source->fileName, source->line, &source->packageUrl);
                source->description << "<Form name=\"" << String (form->getFormName ()) << "\">";
                value = Variant (source->asUnknown (), true);
            }
			return true;
		}
		view = view->getParentView ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceCodeProperty::draw (VariantRef value, const DrawInfo& info)
{
	SourceInfo* source = (SourceInfo*)unknown_cast<Object> (value.asUnknown ());
	if(!source)
		return false;

	SolidBrush linkBrush (Color (0x00, 0xCE, 0x00));
	Rect r (info.rect);
	String string (source->fileName);
	string << ": " << source->line;
	r.setWidth (Font::getStringWidth (string, info.style.font));
	ccl_upper_limit (r.right, info.rect.right);
	info.graphics.drawString (r, string, info.style.font, linkBrush, Alignment::kLeft|Alignment::kVCenter);

	r.left = r.right + 3;
	r.right = info.rect.right;
	string = source->description;
	info.graphics.drawString (r, string, info.style.font, info.style.textBrush, Alignment::kLeft|Alignment::kVCenter);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SourceCodeProperty::edit (VariantRef value, EditContext& context)
{
	SourceInfo* source = (SourceInfo*)unknown_cast<Object> (value.asUnknown ());
	if(!source)
		return false;

	Url xmlFile (source->packageUrl);
	xmlFile.descend (source->fileName);
	UrlDisplayString xmlFileString (xmlFile);

	// try to open xml file in text editor
	Url editorUrl;
	editorUrl.fromDisplayString (editorPath);

	Vector<String> args (2);
	ForEachStringToken (editorArgs.getValue (), " ", arg)
		String string;
		string.appendFormat (arg, Variant (xmlFileString), Variant (source->line));
		args.add (string);
	EndFor

	Threading::ProcessID processId = 0;
	if(System::GetExecutableLoader ().execute (processId, editorUrl, ArgumentList (args.count (), args)) == kResultOk)
		return true;

	// fallback: show in shell browser
	System::GetSystemShell ().showFile (xmlFile);
	return true;
}

//************************************************************************************************
// ParamNameProperty
//************************************************************************************************

bool ParamNameProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<IControl> control (view);
	IParameter* param = control ? control->getParameter () : nullptr;
	if(!param)
		return false;

	value = Variant (param, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamNameProperty::toString (String& string, VariantRef value)
{
	UnknownPtr<IParameter> param (value);
	if(param)
	{
		if(param->getName ().isEmpty () && param->getTag () != -1)
			string << "[Tag #" << param->getTag () << "]";
		else
			param->getName ().toUnicode (string);
	}
}

//************************************************************************************************
// ParamValueProperty
//************************************************************************************************

bool ParamValueProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<IControl> control (view);
	IParameter* param = control ? control->getParameter () : nullptr;
	if(!param)
		return false;

	value = Variant (param, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamValueProperty::toString (String& string, VariantRef value)
{
	UnknownPtr<IParameter> param (value);
	if(param)
	{
		param->getValue ().toString (string);

		if(param->getType () < IParameter::kString)
		{
			String str;
			param->getMin ().toString (str);
			string << "        (Range: " << str << " .. ";
			param->getMax ().toString (str);
			string << str << "; Default: ";
			param->getDefaultValue ().toString (str);
			string << str << ")";

			if(param->isEnabled ())
				string << " enabled";
			else
				string << " disabled";

			if(IFormatter* formatter = param->getFormatter ())
				if(!CString (formatter->getFactoryName ()).isEmpty ())
					string << " \"" << formatter->getFactoryName () << "\"";
		}
	}
}

//************************************************************************************************
// ParamCommandProperty
//************************************************************************************************

bool ParamCommandProperty::getValue (Variant& value, IView* view)
{
	UnknownPtr<IControl> control (view);
	UnknownPtr<ICommandParameter> commandParam (control ? control->getParameter () : nullptr);
	if(!commandParam)
		return false;

	value = Variant (commandParam, true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void ParamCommandProperty::toString (String& string, VariantRef value)
{
	UnknownPtr<ICommandParameter> commandParam (value);
	if(commandParam)
	{
		String category, name;
		commandParam->getCommandCategory ().toUnicode (category);
		commandParam->getCommandName ().toUnicode (name);
		string << category << " - " << name;
	}
}

//************************************************************************************************
// SceneNode3DProperty
//************************************************************************************************

bool SceneNode3DProperty::getValue (Variant& value, IView* view)
{
	if(UnknownPtr<ISceneView3D> sceneView = view)
	{
		value.takeShared (sceneView->getSceneRenderer ().getIScene ());
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SceneNode3DProperty::toString (String& string, VariantRef value)
{
	if(UnknownPtr<ISceneNode3D> sceneNode = value.asUnknown ())
		string = String (sceneNode->getNodeName ());
}
