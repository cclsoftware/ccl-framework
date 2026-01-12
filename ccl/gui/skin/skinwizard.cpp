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
// Filename    : ccl/gui/skin/skinwizard.cpp
// Description : Skin Wizard
//
//************************************************************************************************

#define DEBUG_LOG 0
#define REUSE_IMAGES_ON_RELOAD 1

#include "ccl/gui/skin/skinwizard.h"
#include "ccl/gui/skin/skinmodel.h"
#include "ccl/gui/skin/skinparser.h"
#include "ccl/gui/skin/skinregistry.h"
#include "ccl/gui/skin/skincontrols.h"
#include "ccl/gui/skin/skinexpression.h"

#include "ccl/base/singleton.h"
#include "ccl/base/boxedtypes.h"
#include "ccl/base/trigger.h"
#include "ccl/base/development.h"
#include "ccl/base/storage/url.h"
#include "ccl/base/storage/filefilter.h"
#include "ccl/base/storage/attributes.h"
#include "ccl/base/storage/configuration.h"
#include "ccl/base/storage/protocolhandler.h"
#include "ccl/public/base/iobjectnode.h"

#include "ccl/gui/views/view.h"
#include "ccl/gui/theme/visualstyleselector.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/multiimage.h"

#include "ccl/public/text/translation.h"
#include "ccl/public/text/itranslationtable.h"
#include "ccl/public/system/isysteminfo.h"
#include "ccl/public/system/inativefilesystem.h"
#include "ccl/public/system/ipackagefile.h"
#include "ccl/public/system/ipackagehandler.h"
#include "ccl/public/plugins/iobjecttable.h"
#include "ccl/public/systemservices.h"
#include "ccl/public/plugservices.h"
#include "ccl/public/cclversion.h"

namespace CCL {

//************************************************************************************************
// SkinProtocol
//************************************************************************************************

class SkinProtocol: public Object,
					public SharedSingleton<SkinProtocol>
{
public:
	class Handler: public MountProtocolHandler
	{
	public:
		StringRef CCL_API getProtocol () const override
		{
			static const String protocol = CCLSTR ("skin");
			return protocol;
		}
	};

	SkinProtocol ()
	: handler (NEW Handler)
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		ASSERT (registry != nullptr)
		if(registry)
			registry->registerProtocol (handler);
	}

	~SkinProtocol ()
	{
		UnknownPtr<IProtocolHandlerRegistry> registry (&System::GetFileSystem ());
		if(registry)
			registry->unregisterProtocol (handler);
	}

	Handler* getHandler () { return handler; }

protected:
	AutoPtr<Handler> handler;
};
} // namespace CCL

DEFINE_SHARED_SINGLETON (SkinProtocol)

using namespace CCL;
using namespace SkinElements;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Strings
//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_XSTRINGS ("FileType")
	XSTRING (SkinFile, "Skin File")
END_XSTRINGS

//************************************************************************************************
// SkinVariable
//************************************************************************************************

const CString SkinVariable::kPrefix (CCL_VARIABLE_PREFIX);
const String SkinVariable::prefix (CCL_VARIABLE_PREFIX);
const String SkinVariable::themePrefix ("$Theme.");

//************************************************************************************************
// ISkinContext
//************************************************************************************************

DEFINE_STRINGID_MEMBER_ (ISkinContext, kImportID, "~import")

//************************************************************************************************
// SkinWizard
//************************************************************************************************

const FileType& SkinWizard::getSkinFileType ()
{
	static FileType fileType (nullptr, "skin", CCL_MIME_TYPE "-skin");
	return FileTypes::init (fileType, XSTR (SkinFile));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::insideReloadSkin = false;
bool SkinWizard::isReloadingSkin () { return insideReloadSkin; }

//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS (SkinWizard, Object)

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinWizard::SkinWizard (StringID skinID, Theme* theme, ITranslationTable* table)
: skinID (skinID),
  theme (theme),
  stringTable (table),
  package (nullptr),
  model (nullptr),
  scopeModel (nullptr),
  skinProtocol (SkinProtocol::instance ()),
  moduleReference (nullptr),
  currentZoomFactor (1.f)
{
	// Note: theme is our owner, don't retain it here!
	if(stringTable)
		stringTable->retain ();

	SkinRegistry::instance ().addSkin (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinWizard::~SkinWizard ()
{
	SkinRegistry::instance ().removeSkin (this);

	if(model)
		model->release ();

	if(package)
	{
		skinProtocol->getHandler ()->unmount (String (skinID));
		package->release ();
	}

	if(stringTable)
		stringTable->release ();

	ASSERT (variables.isEmpty () == true)
	variables.objectCleanup (true);

	skinProtocol->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringID SkinWizard::getSkinID () const
{
	return skinID;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IFileSystem* SkinWizard::getFileSystem () const
{
	ASSERT (package != nullptr)
	return package ? package->getFileSystem () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITranslationTable* SkinWizard::getStringTable () const
{
	return stringTable;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Theme* SkinWizard::getTheme () const
{
	return theme;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IPackageFile* SkinWizard::getPackage () const
{
	return package;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::addVariable (CStringRef name, VariantRef value)
{
	SkinVariable* var = NEW SkinVariable (name, value);
	variables.add (var);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::removeVariable (CStringRef name)
{
	// remove last occurrence of variable
	ArrayForEachReverse (variables, SkinVariable, var)
		if(var->getName () == name)
		{
			variables.remove (var);
			var->release ();
			return true;
		}
	EndFor
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::addVariables (IAttributeList& list)
{
	int numAttribs = list.countAttributes ();
	for(int i = 0; i < numAttribs; i++)
	{
		MutableCString name;
		Variant value;
		list.getAttributeName (name, i);
		list.getAttributeValue (value, i);

		MutableCString varName (SkinVariable::kPrefix);
		varName.append (name);
		addVariable (varName, value);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::removeVariables (IAttributeList& list)
{
	int numAttribs = list.countAttributes ();
	for(int i = numAttribs - 1; i >= 0; i--)
	{
		MutableCString name;
		list.getAttributeName (name, i);

		MutableCString varName (SkinVariable::kPrefix);
		varName.append (name);
		bool removed = removeVariable (varName);
		ASSERT (removed == true)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::getVariables (IAttributeList& list) const
{
	list.removeAll ();
	ArrayForEachFast (variables, SkinVariable, v)
		ASSERT (v->getName ().startsWith (SkinVariable::kPrefix))
		MutableCString name = v->getName ().subString (1); // remove leading "$"
		list.setAttribute (name, v->getValue (), Attributes::kShare);
	EndFor
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const SkinVariable* SkinWizard::getVariable (CStringRef name) const
{
	ArrayForEachReverse (variables, SkinVariable, v)
		if(v->getName () == name)
			return v;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::addOverlay (SkinOverlay* overlay)
{
	overlays.add (overlay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::removeOverlay (SkinOverlay* overlay)
{
	overlays.remove (overlay);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinOverlay* SkinWizard::findOverlay (StringID scope, StringID name) const
{
	ArrayForEachFast (overlays, SkinOverlay, overlay)
		const FormReference& target = overlay->getTarget ();
		if(target.scope == scope && target.name == name)
			return overlay;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::loadSkin (UrlRef url, bool keepImages, bool loadAllImages)
{
	AutoPtr<IPackageFile> package = System::GetPackageHandler ().openPackage (url);
	if(!package)
		return false;

	IFileSystem* fileSys = package->getFileSystem ();
	ASSERT (fileSys != nullptr)

	AutoPtr<SkinModel> model;
	Url xmlUrl;
	xmlUrl.setPath (CCLSTR ("skin.xml"));
	AutoPtr<IStream> xmlStream = fileSys->openStream (xmlUrl);
	
	SkinParser p (this);
	p.setFileName (MutableCString (xmlUrl.getPath ()));

	if(xmlStream)
		model = p.parseSkin (*xmlStream);
	if(!model)
		return false;

	SharedPtr<SkinModel> oldModel (keepImages ? this->model : nullptr);
	setModel (model, package);

	// imports (other packages in file system)
	if(!model->getImports ().isEmpty ())
	{
		ObjectList resolvedImports;
		resolvedImports.objectCleanup ();
		Url currentDir (url);
		currentDir.ascend ();
		loadImports (model, currentDir, resolvedImports);
	}

	// includes (files in current package)
	if(!model->getIncludes ().isEmpty ())
	{
		ObjectList resolvedIncludes;
		resolvedIncludes.objectCleanup ();
		loadIncludes (model, resolvedIncludes);
	}

	if(oldModel)
		model->reuseResources (*oldModel);

	model->loadResources (loadAllImages);

	if(Element::isSkinWarningsEnabled ())
		checkResources ();

	#if (0 && DEBUG)
	String helpIdentifierList = model->dumpHelpIdentifiers ();
	if(!helpIdentifierList.isEmpty ())
	{
		Debugger::printf ("=== Help identifiers for %s ===\n", skinID.str ());
		Debugger::print (helpIdentifierList);
	}
	#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::checkResources ()
{
	struct ResourceChecker
	{
		Url packagePath;
		ObjectList files;

		struct ResourceUrl : public Url
		{
			ResourceUrl (UrlRef url) : Url (url), used (false) {}
			PROPERTY_BOOL (used, Used)
		};

		ResourceChecker (UrlRef packagePath)
		: packagePath (packagePath)
		{
			files.objectCleanup ();
		}

		void collectFiles (UrlRef folder)
		{
			FileFilter filter (folder);

			ForEachFile (System::GetFileSystem ().newIterator (folder, IFileIterator::kAll), p)
				if(!filter.matches (*p))
					continue;

				if(p->isFolder ())
				{
					String name;
					p->getName (name);
					if(name != ".svn")
						collectFiles (*p);
				}
				else
				{
					if(!Bitmap::isHighResolutionFile (*p))
						files.add (NEW ResourceUrl (*p));
				}
			EndFor
		}

		void checkResources (ResourceElement& resourceElement)
		{
			if(resourceElement.getUrl ().isEmpty ())
			{
				// empty url (e.g. MultiImage): check child elements
				ArrayForEachFast (resourceElement, Element, e)
					if(ResourceElement* re = ccl_cast<ResourceElement> (e))
						checkResources (*re);
				EndFor
			}
			else if(IconSetElement* iconSetElement = ccl_cast<IconSetElement> (&resourceElement))
			{
				Url iconSetFolder (packagePath);
				iconSetFolder.descend (resourceElement.getUrl ());

				// mark all matching icon files in IconSet folder as used
				ForEachFile (System::GetFileSystem ().newIterator (iconSetFolder, IFileIterator::kFiles), p)
					String fileName;
					p->getName (fileName, true);
					if(IconSetFormat2::isValidIconName (fileName))
						if(ResourceUrl* foundFile = (ResourceUrl*)files.findEqual (Url (*p)))
							foundFile->setUsed (true);
				EndFor
			}
			else
			{
				Url resourceUrl (packagePath);
				resourceUrl.descend (resourceElement.getUrl ());

				if(ResourceUrl* foundFile = (ResourceUrl*)files.findEqual (resourceUrl))
				{
					foundFile->setUsed (true);
					if(foundFile->getPath ().compare (resourceUrl.getPath (), true) != Text::kEqual)
					{
						SKIN_WARNING (&resourceElement, "Resource file has wrong case: '%s'", MutableCString (resourceElement.getUrl ()).str ())
					}
				}
			}
		}

		void checkResources (SkinModel* model)
		{
			ArrayForEachFast (model->getResources (), Element, e)
				if(ResourceElement* re = ccl_cast<ResourceElement> (e))
					checkResources (*re);
			EndFor

			ArrayForEachFast (model->getModels (), SkinModel, m)
				checkResources (m);
			EndFor
		}

		void checkUnusedResources ()
		{
			// checkResources must have been called before
			ListForEachObject (files, ResourceUrl, url)
				if(!url->isUsed () && Image::findHandler (url->getFileType ()))
				{
					SKIN_WARNING (nullptr, "Unused Resource: '%s'", MutableCString (UrlFullString (*url)).str ())
				}
			EndFor
		}
	};

	if(package && package->getPath ().isNativePath ())
	{
		ResourceChecker checker (package->getPath ());
		checker.collectFiles (checker.packagePath);
		checker.checkResources (model);
		checker.checkUnusedResources ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::loadIncludes (SkinModel* model, Container& resolved)
{
	ArrayForEachFast (model->getIncludes (), Element, e)
		IncludeElement* inc = ccl_cast<IncludeElement> (e);
		if(inc && !inc->getUrl ().isEmpty ())
		{
			if(resolved.contains (Boxed::String (inc->getUrl ())))
			{
				SKIN_WARNING (inc, "Crosswise include of \"%s\"!", MutableCString (inc->getUrl ()).str ())
				continue;
			}
			
			Url incUrl;
			incUrl.setPath (inc->getUrl ());
			resolved.add (NEW Boxed::String (inc->getUrl ()));

			SkinParser p (this);
			p.setFileName (MutableCString (incUrl.getPath ()));

			AutoPtr<IStream> stream = getFileSystem ()->openStream (incUrl);
			ASSERT (stream != nullptr)
			if(stream)
			{
				SkinModel* incModel = p.parseSkin (*stream);
				if(incModel)
				{
					CStringRef name = inc->getName ();
					incModel->setName (name);
				
					// 1) no scope specified -> merge to current script
					if(name.isEmpty ())
					{
						loadIncludes (incModel, resolved);
						model->merge (*incModel);
						incModel->release ();
					}
					else
					{
						// 2) merge with already existing scope
						SkinModel* existing = getScopeModel (name);
						if(existing)
						{
							loadIncludes (incModel, resolved);
							existing->merge (*incModel);
							incModel->release ();
						}
						// 3) add new scope to current script
						else
							model->getModels ().addChild (incModel);
					}
				}
			}
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::loadImports (SkinModel* model, const Url& currentDir, Container& resolved)
{
	Url skinsFolder;
	GET_DEVELOPMENT_FOLDER_LOCATION (skinsFolder, "skins", "")
	if(skinsFolder.isEmpty ())
	{
		System::GetSystem ().getLocation (skinsFolder, System::kAppSupportFolder);
		skinsFolder.descend (CCLSTR ("skins"), Url::kFolder);
	}

	ArrayForEachFast (model->getImports (), Element, e)
		ImportElement* imp = ccl_cast<ImportElement> (e);
		if(imp && !imp->getUrl ().isEmpty ())
		{
			String urlString (imp->getUrl ());
			if(resolved.contains (Boxed::String (urlString)))
			{
				SKIN_WARNING (imp, "Crosswise import of \"%s\"!", MutableCString (urlString).str ())
				continue;
			}
			
			resolved.add (NEW Boxed::String (urlString));

			Url impUrl;
			if(urlString.startsWith (CCLSTR ("@"))) // symbolic name
			{
				String name (urlString);
				name.remove (0, 1);
				resolveImport (impUrl, name, skinsFolder, currentDir);
			}
			else if(urlString.contains (CCLSTR ("://"))) // absolute path
			{
				impUrl.setUrl (urlString, Url::kDetect);
			}
			else // relative path
			{
				impUrl.setPath (urlString, Url::kDetect);
				impUrl.makeAbsolute (currentDir);
			}

			SkinRegistry::ImportContext importContext (getSkinID ());
			SkinWizard helper (kImportID, getTheme (), getStringTable ());
			if(!helper.loadSkin (impUrl, false, true)) // force all images to be loaded now, because the original file URL is lost
			{
				SKIN_WARNING (imp, "Import of %s failed!", MutableCString (urlString).str ())
				continue;
			}

			SkinModel& importedModel = helper.getRoot ();
			model->merge (importedModel);
			model->addImportedPath (impUrl);

			// handle sub-models
			if(!importedModel.getModels ().isEmpty ())
				model->takeSubModels (importedModel);
		}
	EndFor
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::resolveImport (Url& result, StringRef name, const Url& skinsFolder, const Url& currentDir)
{
#if SKIN_DEVELOPMENT_LOCATIONS_ENABLED
	if(SkinRegistry::instance ().getDevelopmentLocation (result, name))
		return true;
#endif

	String folderName (name);
	String fileName (String () << name << "." << getSkinFileType ().getExtension ());

	ResourceUrl appResources (System::GetMainModuleRef (), String::kEmpty, Url::kFolder);
	ResourceUrl frameResources (String::kEmpty, Url::kFolder); // cclgui
	
	ObjectArray searchPaths;
	searchPaths.objectCleanup (true);
	SkinRegistry::instance ().getSearchLocations (searchPaths);
	searchPaths.addOnce (skinsFolder);
	searchPaths.addOnce (currentDir);
	searchPaths.addOnce (appResources);
	searchPaths.addOnce (frameResources);

	ArrayForEachFast (searchPaths, Url, basePath)
		Url testFile (*basePath);
		testFile.descend (fileName, Url::kFile);
		if(System::GetFileSystem ().fileExists (testFile))
		{
			result = testFile;
			break;
		}

		// check for a folder that contains a skin.xml
		Url testFolder (*basePath);
		testFolder.descend (folderName, Url::kFolder);
		Url testXmlFile (testFolder);
		testXmlFile.descend (CCLSTR ("skin.xml"));
		if(System::GetFileSystem ().fileExists (testXmlFile))
		{
			result = testFolder;
			break;
		}
	EndFor
	return !result.isEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::reloadSkin (bool keepImages)
{
#if CCL_STATIC_LINKAGE
	ASSERT (package != 0 || skinID == "cclgui")
#else
	ASSERT (package != nullptr)
#endif
	if(!package)
		return false;

	//SKIN_WARNING (0, "*** Reloading skin '%s'... ***", skinID.str ())

	ScopedVar<bool> scope (insideReloadSkin, true);
	getTheme ()->resetStyles ();

	Url url (package->getPath ());
	return loadSkin (url, keepImages);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::isSkinLoaded () const
{
	return model != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::setModel (SkinModel* _model, IPackageFile* _package)
{
	if(package)
		skinProtocol->getHandler ()->unmount (String (skinID));

	take_shared<SkinModel> (model, _model);
	take_shared<IPackageFile> (package, _package);

	if(package)
		skinProtocol->getHandler ()->mount (String (skinID), package->getFileSystem ());

	scopeModel = nullptr;
	scopeName.empty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef SkinWizard::getCurrentScope () const 
{ 
	return scopeName; 
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::setCurrentScope (CStringRef scope)
{
	if(scope == scopeName)
		return true;

	if(!scope.isEmpty ())
	{
		SkinModel* newModel = getScopeModel (scope);
		if(!newModel)
			return false;

		scopeModel = newModel;
	}
	else
		scopeModel = nullptr; // select root
	
	scopeName = scope;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel& SkinWizard::getRoot ()
{
	if(!model)
		model = NEW SkinModel;
	return *model;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel& SkinWizard::getModel ()
{
	if(scopeModel)
		return *scopeModel;
	return getRoot ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkinModel* SkinWizard::getScopeModel (CStringRef scopeName)
{
	return getRoot ().getScopeModel (scopeName);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinWizard::createView (CStringRef scope, CStringRef name, IUnknown* controller)
{
	SkinScopeSelector sel (scope, *this);
	return createView (name, controller);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinWizard::createView (CStringRef name, IUnknown* controller)
{
	const char* p = strrchr (name.str (), '/');
	if(p)
	{
		MutableCString scopeName; scopeName.append (name.str (), (int)(p - name.str ()));
		CString viewName (++p);
		return createView (scopeName, viewName, controller);
	}

	// check if an overlay is registered for this form...
	if(!overlays.isEmpty ())
	{
		SkinOverlay* overlay = findOverlay (scopeName, name);
		if(overlay)
		{
			Attributes arguments;
			getVariables (arguments);
			return SkinRegistry::instance ().createView (overlay->getSource (), controller, &arguments);
		}
	}

	FormElement* element = getModel ().getForms ().findElement<FormElement> (name);
	#if DEBUG
	if(element == nullptr)
	{
		CCL_PRINT ("Form not found : ")
		CCL_PRINTLN (name)
	}
	#endif

//	CCL_PROFILE_START (createView)
	View* v = element ? createView (element, controller) : nullptr;
//	CCL_PROFILE_STOP (createView)
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinWizard::createView (Element* element, IUnknown* controller, View* parent, ViewElement* parentElement)
{
	View* v = nullptr;

	ControlStatement* statement = ccl_cast<ControlStatement> (element);
	if(statement)
	{
		// *** <using> Statement ***
		if(statement->isClass (ccl_typeid<UsingStatement> ()))
		{
			UsingStatement* usingStatement = (UsingStatement*)statement;
			if(usingStatement->getType () == UsingStatement::kController)
			{
				ResolvedName resolvedName (*this, statement->getName ());
				CString name = resolvedName.string ();

				IUnknown* child = lookupController (controller, name);
				if(child)
					v = createViewElements (element, child, parent, parentElement);
				#if DEBUG
				else if(usingStatement->isOptional () == false)
				{
					SKIN_WARNING (usingStatement, "Controller not found for using statement: '%s'", name.str ())
					CCL_DEBUGGER ("Controller not found for using statement.\n");
				}
				#endif
			}
		}
		// *** <switch>/<case> Statement ***
		else if(statement->canCast (ccl_typeid<SwitchStatement> ()))
		{
			SwitchStatement* switchStatement = (SwitchStatement*)statement;
			Element* newElement = nullptr;

			Variant value;
			bool valueFound = false;
			if(!switchStatement->getDefined ().isEmpty ())
			{
				bool isDefined = getVariable (switchStatement->getDefined ()) != nullptr;
				if((isDefined && !switchStatement->isDefineNegated ()) || (!isDefined && switchStatement->isDefineNegated ()))
				{
					value = 1;
					valueFound = true;
				}
			}
			else
			{
				// lookup controller for property (optional)
				IUnknown* propertyController = controller;
				if(!switchStatement->getController ().isEmpty ())
				{
					ResolvedName resolvedName (*this, switchStatement->getController ());
					CString name = resolvedName.string ();
					propertyController = lookupController (controller, name);
					if(!propertyController)
					{
						SKIN_WARNING (switchStatement, "Controller not found for switch statement: '%s'", name.str ())
						CCL_DEBUGGER ("Controller not found for switch statement.\n");
					}
				}

				UnknownPtr<IObject> iObject (propertyController);
				ResolvedName resolvedPropertyId (*this, switchStatement->getName ());
				CString propertyId = resolvedPropertyId.string ();
				valueFound = Property (iObject, propertyId).get (value);

				if(!valueFound && switchStatement->getName ().startsWith (SkinVariable::kPrefix)) // if name is a variable, try the resolved value
				{
					value =	propertyId;
					valueFound = true;
				}
			}

			if(valueFound)
				newElement = switchStatement->getCaseElement (value);

			if(!newElement)
				newElement = switchStatement->getDefaultElement ();

			if(newElement)
				v = createViewElements (newElement, controller, parent, parentElement);
		}
		// *** <foreach> Statement ***
		else if(statement->isClass (ccl_typeid<ForEachStatement> ()))
		{
			ForEachStatement* foreachStatement = (ForEachStatement*)statement;

			SkinVariable skinVariable (foreachStatement->getName ());
			variables.add (&skinVariable);

			if(!foreachStatement->getCountString ().isEmpty ())
			{
				Variant minValue;
				Variant maxValue;
				resolveNumber (minValue, foreachStatement->getStartString (), controller, foreachStatement);
				resolveNumber (maxValue, foreachStatement->getCountString (), controller, foreachStatement);

				int min = minValue.parseInt ();
				int max = maxValue.parseInt () + min;

				for(int i = min; i < max; i++)
				{
					skinVariable.setValue (Variant (i));
					createViewElements (foreachStatement, controller, parent, parentElement);
				}
			}
			else
			{
				MutableCString inString (foreachStatement->getInString ());
				ResolvedName resolvedString (*this, inString);

				ForEachStringToken (String (resolvedString.string ()), " ", token)
					skinVariable.setValue (Variant (token));
					createViewElements (foreachStatement, controller, parent, parentElement);
				EndFor
			}

			variables.remove (&skinVariable);
		}
		// *** <define> Statement ***
		else if(statement->isClass (ccl_typeid<DefineStatement> ()))
		{
			DefineStatement* defineStatement = (DefineStatement*)statement;

			// add variables
			ObjectArray definitions;
			definitions.objectCleanup (true);

			ArrayForEachFast (defineStatement->getVariables (), SkinVariable, variable)
				if(variable->getValue ().isString ())
				{
					Variant resolvedValue;
					if(resolveDefine (resolvedValue, variable->getValue ().asString (), controller))
					{
						SkinVariable* variable2 = NEW SkinVariable (variable->getName (), resolvedValue);
						definitions.add (variable2);
						variables.add (variable2);
						continue;
					}
				}
				definitions.add (return_shared (variable));
				variables.add (variable);
			EndFor

			v = createViewElements (element, controller, parent, parentElement);

			// remove variables
			ArrayForEachFast (definitions, SkinVariable, variable)
				variables.remove (variable);
			EndFor
		}
		// *** <zoom> statement ***
		else if(auto* zoomStatement = ccl_strict_cast<ZoomStatement> (statement))
		{
			// apply factor relative to current factor if no "absolute" mode set
			float factor = (zoomStatement->getMode () == ZoomStatement::kAbsolute) ? 1.0 : currentZoomFactor;
			ScopedVar<float> scope (currentZoomFactor, zoomStatement->getZoomFactor () * factor);
			v = createViewElements (element, controller, parent, parentElement);
		}
		// *** <styleselector> Statement ***
		else if(auto* variantStatement = ccl_strict_cast<VisualStyleSelectorElement> (statement))
		{
			AutoPtr<VisualStyleAlias> styleAlias (NEW VisualStyleAlias (variantStatement->getVariableName ())); // name is only for diagnostic purposes, e.g. CCL Spy
			AutoPtr<VisualStyleSelector> styleSelector (NEW VisualStyleSelector (styleAlias));

			// lookup styles to be selected based on parameter value
			for(CStringRef styleName : variantStatement->getStyleNames ())
			{
				ResolvedName resolvedStyleName (*this, styleName);
				VisualStyle* style = lookupStyle (resolvedStyleName.string (), variantStatement);
				
				ASSERT (style)
				if(style)
					styleSelector->addStyle (style);
				else
					styleSelector->addStyle (AutoPtr<VisualStyle> (NEW VisualStyle)); // dummy to keep indices as expected
			}

			// lookup parameter or property
			if(variantStatement->getPropertyId ().isEmpty ())
			{
				IParameter* param = SkinElements::ControlElement::getParameter (ViewElement::CreateArgs (*this, controller), variantStatement->getName (), variantStatement);
				ASSERT (param)
				if(param)
					styleSelector->setParameter (param);
			}
			else
			{
				IUnknown* propertyController = controller;
				if(!variantStatement->getController ().isEmpty ())
				{
					ResolvedName resolvedName (*this, variantStatement->getController ());
					CString name = resolvedName.string ();
					propertyController = lookupController (controller, name);
					if(!propertyController)
					{
						SKIN_WARNING (variantStatement, "Controller not found for VisualStyleSelector: '%s'", name.str ())
						CCL_DEBUGGER ("Controller not found for VisualStyleSelector.\n");
					}
				}
				
				ResolvedName resolvedPropertyId (*this, variantStatement->getPropertyId ());
				CString propertyString = resolvedPropertyId.string ();
				styleSelector->setSelectorProperty (propertyString, propertyController);
			}

			styleSelector->initialize ();

			// make style alias accessible to child elements via a temporary skin variable
			AutoPtr<SkinVariable> variable (NEW SkinVariable (variantStatement->getVariableName (), Variant (styleAlias->asUnknown (), true)));
			variables.add (variable);

			v = createViewElements (element, controller, parent, parentElement);

			variables.remove (variable);
		}
	}
	else
		v = createViewElements (element, controller, parent, parentElement);

	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* SkinWizard::createViewElements (Element* element, IUnknown* controller, View* parent, ViewElement* parentElement)
{
	// create current view...
	View* v = nullptr;
	ViewElement* viewElement = ccl_cast<ViewElement> (element);
	if(viewElement)
		v = viewElement->createView (ViewElement::CreateArgs (*this, controller));

	// ...create sub-elements...
	createChildElements (element, controller, v ? v : parent, v ? viewElement : parentElement);

	if(v)
	{
		viewElement->viewCreated (v);

		// ...and add to parent view
		if(parent)
		{
			parent->addView (v);

			if(parentElement)
				parentElement->viewAdded (parent, v, viewElement, *this);
		}
	}
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CStringRef SkinWizard::resolveName (MutableCString& resolvedName, CStringRef name) const
{
	if(!variables.isEmpty ())
	{
		// we try to avoid copying the name
		bool found = false;
		ArrayForEachReverse (variables, SkinVariable, v)
			if(v->getValue ().isObject ())
				continue;

			int idx = -1;
			if(found)
				idx = resolvedName.index (v->getName ());
			else
			{
				idx = name.index (v->getName ());
				if(idx >= 0)
				{
					resolvedName = name;
					found = true;
				}
			}

			while(idx != -1)
			{
				if(idx == 0 && resolvedName == v->getName ())
				{
					v->getValue ().toCString (resolvedName); // complete replace
					break;
				}
				else
				{
					MutableCString valueString;
					v->getValue ().toCString (valueString);

					resolvedName.replace (idx, v->getName ().length (), valueString);

					idx = resolvedName.index (v->getName ());
				}
			}
		EndFor
		if(found)
			return resolvedName;
	}
	return name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

String SkinWizard::resolveTitle (StringRef title) const
{
	String resolvedTitle (title);
	
	int idx = resolvedTitle.index (SkinVariable::themePrefix);
	while(idx != -1)
	{
		static const int prefixLength = SkinVariable::themePrefix.length ();
		
		bool resolved = false;
		for(int i = 0; i < ThemeElements::kNumMetrics; i++)
		{
			String themeMetricName (ThemeStatics::instance ().getThemeMetricName (i));
			int elementIndex = resolvedTitle.index (themeMetricName);
			if(elementIndex == (idx + prefixLength)) // element starts directly after the themePrefix?
			{
				String remainder = resolvedTitle.subString (elementIndex + themeMetricName.length ());
				resolvedTitle.truncate (idx);
				
				resolvedTitle.append (VariantString (getTheme ()->getThemeMetric (i)));
				
				if(!remainder.isEmpty ())
					resolvedTitle.append (remainder);
				
				resolved = true;
				break;
			}
		}
		
		if(!resolved)
		{
			ASSERT (false) // no themeElement definition found
			break;
		}
		
		idx = resolvedTitle.index (SkinVariable::themePrefix);
	}

	if(!variables.isEmpty ())
	{
		ArrayForEachReverse (variables, SkinVariable, v)
			if(v->getValue ().isObject ())
				continue;

			String variableName (v->getName ());
			int idx = resolvedTitle.index (variableName);
			while(idx != -1)
			{
				String remainder = resolvedTitle.subString (idx + v->getName ().length ());
				resolvedTitle.truncate (idx);

				String valueString;
				v->getValue ().toString (valueString);
				resolvedTitle.append (valueString);

				if(!remainder.isEmpty ())
					resolvedTitle.append (remainder);

				idx = resolvedTitle.index (variableName);
			}
		EndFor
	}
	return resolvedTitle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::resolveNumber (Variant& resolvedValue, StringRef valueString, IUnknown* controller, Element* element) const
{
	if(valueString.isEmpty ())
	{
		resolvedValue = 0;
		return true;
	}

	resolvedValue.fromString (valueString);
	switch(resolvedValue.getType ())
	{
	case Variant::kInt:
	case Variant::kFloat:
		return true;

	case Variant::kString:
	{
		MutableCString str (valueString);
		if(str.firstChar () == SkinVariable::kPrefix[0])
		{
			if(const SkinVariable* variable = getVariable (str))
			{
				resolvedValue = variable->getValue ();
				return true;
			}
			else
				SKIN_WARNING (element, "Variable not found: '%s'", str.str ()) 
		}
		else
		{
			UnknownPtr<IObject> iObject (controller);
			if(Property (iObject, str).get (resolvedValue))
				return true;
			else
				SKIN_WARNING (element, "Property not found: '%s'", str.str ()) 
		}
	}
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool SkinWizard::resolveDefine (Variant& resolvedValue, StringRef valueString, IUnknown* controller) const
{
	if(valueString.startsWith ("@"))
	{
		// resolve special instructions in the form "@instruction:arguments"
		int colonIndex = valueString.index (":");
		String instruction = valueString.subString (0, colonIndex);
		String arguments = valueString.subString (colonIndex + 1);

		if(instruction == "@property")
		{
			// @property:propertyPath
			// resolve a property (global path or relative to controller)
			MutableCString _propertyPath (arguments);
			ResolvedName resolvedPropertyPath (*this, _propertyPath, _propertyPath.contains (SkinVariable::kPrefix));
			MutableCString propertyPath = resolvedPropertyPath.string ();

			if(propertyPath.contains ("://"))
				resolvedValue = Property (propertyPath);
			else
			{
				IUnknown* propertyController = controller;
				if(propertyPath.contains ("/"))
				{
					int index = propertyPath.index ('.');
					if(index > -1)
					{
						// skip any ".." as part of the anchor path
						while(propertyPath[index + 1] == '.')
						{
							int subIndex = propertyPath.subString (index + 2).index ('.');
							if(subIndex < 0)
								break;
							else
								index += 2 + subIndex;
						}
					}
					propertyController = lookupController (controller, propertyPath.subString (0, index));
					propertyPath = propertyPath.subString (index + 1);
				}

				UnknownPtr<IObject> anchor (propertyController);
				resolvedValue = Property (anchor, propertyPath);
			}
			return true;
		}
		else if(instruction == "@select")
		{
			// @select:$variable:str0,str1,str2
			// select a string from a list, indexed by a given variable
			int colonIndex = arguments.index (":");
			if(colonIndex > 0)
			{
				MutableCString varName (arguments.subString (0, colonIndex));
				String stringList (arguments.subString (colonIndex + 1));

				// resolve variable (expecting an int)
				ResolvedName resolvedString (*this, varName);
				int64 resolvedIndex = 0;
				resolvedString.string ().getIntValue (resolvedIndex);

				int index = 0;
				ForEachStringTokenWithFlags (stringList, ",", token, Text::kPreserveEmptyToken)
					if(index++ == resolvedIndex)
					{
						resolvedValue = Variant (token, true);
						return true;
					}
				EndFor
			}
		}
		else if(instruction == "@eval")
		{
			// @eval:42* $i + 23 - 20 / (4+6)
			// evaluate an arithmetic expression
			SkinExpressionParser::evaluate (resolvedValue, arguments, *this);
			CCL_PRINTF ("SkinExpressionParser::evaluate %s, %d\n", MutableCString (arguments).str (), resolvedValue.asInt ())
			return true;
		}
	}
	else if(valueString.contains (SkinVariable::prefix))
	{
		// resolve variable with current values
		MutableCString valueCString (valueString);
		ResolvedName resolvedValueString (*this, valueCString);
		resolvedValue = resolvedValueString.string ();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* SkinWizard::lookupController (IUnknown* currentController, CStringRef path) const
{
	IUnknown* controller = nullptr;
	if(path.contains ("://")) // lookup from root
	{
		String _path (path);
		Url objectUrl (_path);
		controller = System::GetObjectTable ().getObjectByUrl (objectUrl);
	}
	else if(path.startsWith (SkinVariable::kPrefix))
	{
		// try object from skin variables
		if(const SkinVariable* var = getVariable (path))
			controller = var->getValue ().asUnknown ();
	}

	if(controller == nullptr)
	{
		// lookup relative to current controller
		UnknownPtr<IObjectNode> iNode (currentController);
		controller = iNode ? iNode->lookupChild (String (path)) : nullptr;
	}

	return controller;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

VisualStyle* SkinWizard::lookupStyle (StringID styleName, SkinElements::Element* caller) const
{
	VisualStyle* visualStyle = nullptr;
	if(styleName.startsWith (SkinVariable::kPrefix))
	{
		// try object from skin variables
		if(const SkinVariable* var = getVariable (styleName))
			visualStyle = unknown_cast<VisualStyle> (var->getValue ().asUnknown ());
	}

	if(visualStyle == nullptr)
	{
		ResolvedName resolvedName (*this, styleName);
		CString name = resolvedName.string ();
		visualStyle = const_cast<SkinWizard*> (this)->getModel ().getStyle (name, caller);
	}

	return visualStyle;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SkinWizard::setZoomFactor (float factor)
{
	currentZoomFactor = factor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float SkinWizard::getZoomFactor () const
{
	return currentZoomFactor;
}
