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
// Filename    : ccl/extras/javascript/ccl_native.d.ts
// Description : Script type declarations for native types.
//
//************************************************************************************************

declare var include_file: (path: string) => void;

declare namespace CCL
{

export type Coord = number; // mask Core type
export type Color = number; // mask Core type

////////////////////////////////////////////////////////////
// app
////////////////////////////////////////////////////////////

export class AliasParam extends Object
{
	private CCL_AliasParam: never;

	setOriginal: (param: Parameter) => void;
}

export class Application extends Component
{
	private CCL_Application: never;
	readonly Configuration: Configuration.Registry;
}

export class ColorParam extends Parameter
{
	private CCL_ColorParam: never;
	palette: PaletteBase<Color>;
}

export class FloatParam extends Parameter
{
	private CCL_FloatParam: never;
}

export class IntParam extends Parameter
{
	private CCL_IntParam: never;
}

export class StringParam extends Parameter
{
	private CCL_StringParam: never;
}

export class ImageProvider extends Parameter
{
	private CCL_ImageProvider: never;
	setImage: (image: Object, update?: boolean) => void;
}

export class ListParam extends Parameter
{
	private CCL_ListParam: never;

	appendString: (value: string) => void;
	appendValue: (value: number) => void;
	removeAll: () => void;
	getValueAt: (index: number) => Variant;
	getSelectedValue: () => number;
	selectValue: (value: number, update?: boolean) => void;
}

export class MenuParam extends ListParam
{
	private CCL_MenuParam: never;
}

export class Component extends ObjectNode
{
	private CCL_Component: never;

	readonly name: string;
	readonly title: string;
	readonly parent: ObjectNode;
	readonly self: Component;
	readonly children: { [id: string]: ObjectNode };
	readonly hasChild: { [id: string]: ObjectNode };
	readonly numChildren: number;
	readonly hasParam: { [id: string]: Parameter };
	readonly paramValue: { [id: string]: Variant };
	readonly paramEnabled: { [id: string]: boolean };
	readonly paramEditable: { [id: string]: boolean };
	readonly hasObject: { [id: string]: Component | null };

	findParameter: (name: string) => Parameter;
	interpretCommand: (category: string, name: string, checkOnly?: boolean, invoker?: Object) => boolean;
}

export class PaletteParam extends ListParam
{
	private CCL_PaletteParam: never;
}

export class ParamContainer
{
	private CCL_ParamContainer: never;

	add: (param: Parameter) => Parameter;
	addParam: (name: string) => Parameter;
	addFloat: (rangeFrom: number, rangeTo: number, name: string) => FloatParam;
	addInteger: (rangeFrom: number, rangeTo: number, name: string) => IntParam;
	addString: (name: string) => StringParam;
	addList: (name: string) => ListParam;
	addMenu: (name: string) => ListParam;
	addCommand: (commandCategory: string, commandName: string, name: string) => Parameter;
	addColor: (name: string) => ColorParam;
	addAlias: (name: string) => AliasParam;
	addImage: (name: string) => ImageProvider;
	remove: (name: string) => boolean;
	lookup: (name: string) => Parameter | null;
}

export class Parameter extends Object
{
  private CCL_Parameter: never;
  type: number;
  value: any;
  min: any;
  max: any;
  default: any;
  name: string;
  string: string;
  enabled: boolean;
  signalAlways: boolean;
  reverse: boolean;

  setValue: (value: any, update?: boolean) => void;
  fromString: (str: string, update?: boolean) => void;
  setNormalized: (value: number, upate?: boolean) => void;
  getNormalized: () => number;
  setFormatter: (formatter: Formatter) => void;
  setCurve: (curve: string) => void;
  isType: (type: string) => boolean;
  setSignalAlways: (state?: boolean) => void;
}

export class Preset extends Object
{
	private CCL_Preset: never;
	readonly presetName: string;
	readonly relativePath: string;
}

export class RuntimeEnvironment extends Component
{
	private CCL_RuntimeEnvironment: never;
}

export class ScrollParam extends IntParam
{
	private CCL_ScrollParam: never;
	readonly numPages: number;
	readonly currentPage: number;
}

////////////////////////////////////////////////////////////
// app/actions
////////////////////////////////////////////////////////////

export class ActionExecuter extends Object
{
	private CCL_ActionExecuter: never;
	executeImmediately: boolean;

	beginMultiple: (description: string) => void;
	endMultiple: (cancel?: boolean) => void;
	isJournalEnabled: () => boolean;
	setJournalEnabled: (enabled: boolean) => void;
}

////////////////////////////////////////////////////////////
// app/browser
////////////////////////////////////////////////////////////

export class PlugInMenuParam extends Parameter
{
	private CCL_PlugInMenuParam: never;

	getSelectedClass: () => string;
	selectClass: (cid: UID | string) => boolean;
	setCategory: (category: string) => void;
	setDisplaySorted: (state: boolean) => void;
}

export class PlugInSelector extends Component
{
	private CCL_PlugInSelector: never;

	setCategories: (classCategory1: string, classCategory2: string) => void;
	getSelected: () => string;
}

////////////////////////////////////////////////////////////
// app/components
////////////////////////////////////////////////////////////

export class ColorPicker extends Object
{
	private CCL_ColorPicker: never;

	construct: (color: ColorParam, applyPresetPalette?: boolean) => void;
	popup: (popupStyle?: any, useMousePos?: boolean) => boolean;
}

////////////////////////////////////////////////////////////
// app/documents
////////////////////////////////////////////////////////////

export class Document
{
	private CCL_Document: never;
	readonly title: string;
	readonly path: Url;
}

export class DocumentClass
{
	private CCL_DocumentClass: never;
	isNative(): boolean;
	isPrivate(): boolean;
	getFileType(): FileType
}

export class DocumentTemplate
{
	private CCL_DocumentTemplate: never;
	readonly icon: CCL.Image;
	readonly title: string; // localized
	readonly subTitle: string; // localized
	readonly description: string; // localized
	readonly additionalData: string;
	readonly tutorialId: string;
}

export class DocumentTemplateList
{
	private CCL_DocumentTemplateList: never;
	getTemplateCount (): number;
	getTemplate (index: number): DocumentTemplate;
}

export class NewDocumentAssistant extends Component
{
	private CCL_DocumentTemplateList: never;
	loadSecondaryTemplates (url: string): DocumentTemplateList;
	selectSecondaryTemplate (index: number): void;
	setDropFileTypes (fileTypes: Container<FileType> | Array<FileType>): void;
	getClassIcon (className: string): CCL.Image;
	setExclusiveDropFileTypes (fileTypes: Container<FileType> | Array<FileType>): void;
	getClassIcon (className: string): Object;
	setDropImportFile (path: Url, temporaryDocument?: boolean): void;
	setConfirmEnabled (state: boolean): void;
	closeDialog (apply?: boolean): void;	
}

////////////////////////////////////////////////////////////
// app/editing
////////////////////////////////////////////////////////////

export class EditTask extends Object
{
	private CCL_EditTask: never;
	readonly formName: string;
	readonly themeID: string;
}

export class EditHandlerHook extends Object
{
	private CCL_EditHandlerHook: never;
}

export class EditHandler
{
	private CCL_EditHandler: never;

	setEditTooltip: (tooltip: string) => void;
}

export class EditTaskContext extends Attributes
{
	private CCL_EditTaskContext: never;
	parameters: ParamContainer;
	isArgumentDialog?: boolean;

	isSilentMode: () => boolean;
	restore: (force?: boolean) => boolean;
	getArguments: () => Attributes;
	runDialog: (name?: string, themeID?: string) => tresult;
}

export class EditView<T extends Object> extends Object
{
	private CCL_EditView: T;
	readonly selection: Selection;

	findItem: (loc: Point) => Object;
	findItemPart: (obj: Object, loc: Point) => Object;
	findItemDeep: (loc: Point) => Object
	isSameItem: (obj1: Object, obj2: Object) => boolean;
	getItemType: (obj: Object) => string;
	getEditArea: (loc: Point) => string;
	getSelectionSize: () => Rect;
	setFocusItem: (obj: Object) => void;
	setAnchorItem: (obj: Object) => void;
	select: (obj: Object) => void;
	unselect: (obj: Object) => void;
	dragSelection: (event: MouseEvent) => void;
	drawSelection: (event: MouseEvent, hook: EditHandlerHook, hint: string) => EditHandler;
	dragEraser: (event: MouseEvent) => EditHandler;
	deleteSelected: () => void;
	deleteItem: (obj: Object) => void;
	editItem: (obj: Object) => void;
	createEditHandler: (obj: Object, event: MouseEvent) => EditHandler;
	getItemSize: (obj: Object) => Rect;
	detectDrag: (event: MouseEvent) => boolean;
	detectDoubleClick: (event: MouseEvent) => boolean;
	takeFocus: () => boolean;
	createSelectFunctions: (functions: ActionExecuter) => SelectFunctions<T>;
	showSelection: (show: boolean, redraw: boolean) => void;
	setCursor: (cursorName: string) => void;
	moveCrossCursor: (position: Point) => void;
}

export class SelectFunctions<T extends Object> extends ActionExecuter
{
	private CCL_SelectFunctions: T;
	selectExclusive: boolean;
	makeItemsVisible: boolean;
	focusItem: T;

	select: (event: T) => void;
	selectMultiple: (events: T[]) => void;
	saveSelection: () => void;
	takeSnapshot: () => boolean;
}

export class SelectionContainer extends Object
{
	private CCL_SelectionContainer: never;
}

export class Selection extends SelectionContainer
{
	private CCL_Selection: never;
	unselectAll: () => boolean;
}

////////////////////////////////////////////////////////////
// app/gui
////////////////////////////////////////////////////////////

export class HelpTutorial
{
	private CCL_HelpTutorial: never;
	getID (): string;
	getTitle (): string;
}

////////////////////////////////////////////////////////////
// app/presets
////////////////////////////////////////////////////////////

export class PresetManager
{
	private CCL_PresetManager: never;
	presetExists (metaInfo: Attributes, presetName: string, fileType?: FileType): boolean;
}

export class PresetParam extends MenuParam
{
	private CCL_PresetParam: never;
	enabled: boolean;

	setMetaInfo: (metaInfo: Attributes) => void;
	shouldShowFolders: (state: boolean) => void;
	selectRelativePath: (path: string) => void;
}

////////////////////////////////////////////////////////////
// app/utilities
////////////////////////////////////////////////////////////

// Boxed::Point
export class Point extends Object
{
	private CCL_Point: never;
	x: number;
	y: number;

	equals: (obj: Object) => boolean;
}

// Boxed::Rect
export class Rect extends Object
{
	private CCL_Rect: never;
	left: number;
	top: number;
	right: number;
	bottom: number;

	equals: (obj: Object) => boolean;
	pointInside: (point: Point) => boolean;
	clone: () => Rect;
}

// Boxed::MouseEvent
export class MouseEvent extends Object
{
	private CCL_MouseEvent: never;
}

// Boxed::DateTime
export class DateTime extends Object
{
	private CCL_DateTime: never;
	year: number;
	month: number;
	day: number;
	hour: number;
	minute: number;
	second: number;
	ms: number;

	toSeconds: () => number;
}

////////////////////////////////////////////////////////////
// base
////////////////////////////////////////////////////////////

export type tresult = number; // bool incomatible!
export type Variant = boolean | number | string | Object;

export class Formatter extends Object
{
	private CCL_Formatter: never;

	print: (value: number) => string;
	scan: (input: string) => number;
}

export class Message
{
	private CCL_Message: never;
	readonly id: string;

	getArgCount (): number;
	getArg (index: number): any;
}

export class Object
{
	private CCL_Object: never;
}

export class ObjectNode extends Object
{
	private CCL_ObjectNode: never;
	find: (path: string) => ObjectNode | null;
}

// Boxed::UID
export class UID extends Object
{
	private CCL_UID: never;

	equals: (obj: Object | string) => boolean;
}

////////////////////////////////////////////////////////////
// base/collections
////////////////////////////////////////////////////////////

export class Container<T> extends Object
{
	private CCL_Container: T;
	readonly count: number;

	at: (index: number) => T;
	findEqual: (key: T | string) => T;
	newIterator: () => Iterator<T>;
}

export class Iterator<T> extends Object
{
	private CCL_Iterator: T;

	done: () => boolean;
	first: () => void;
	last: () => void;
	next: () => T;
	previous: () => T;
}

export class Linkable extends Object
{
	private CCL_Linkable: never;
}

////////////////////////////////////////////////////////////
// base/storage
////////////////////////////////////////////////////////////

export class Attributes extends Object
{
	private CCL_Attributes: never;

	countAttributes: () => number;
	getAttributeName: (index: number) => string;
	getAttributeValue: (index: number) => Variant;
	getAttribute: (id: string) => Variant;
	setAttribute: (id: string, value: Variant) => boolean;
	newQueueIterator: (id: string) => CCL.Iterator<Object>;
	queueAttribute: (id: string, value: Variant) => boolean;
	contains: (id: string) => boolean;
}

export class File extends Object
{
	private CCL_File: never;
	path: Url;

	exists: () => boolean;
	remove: (deleteToTrashBin?: boolean, deleteRecursively?: boolean) => boolean;
	rename: (newName: string) => boolean;
	moveTo: (destination: Url) => boolean;
	copyTo: (destination: Url) => boolean;
}

export class TextFile extends Object
{
	private CCL_TextFile: never;
	readonly endOfStream: boolean;

	readLine: () => string;
	writeLine: (line: string) => boolean;
	close: () => void;
}

export class Url extends Object
{
	private CCL_Url: never;
	readonly name: string;
	readonly url: string;
	readonly extension: string;

	ascend: () => boolean;
	descend: (name: string, isFolder?: boolean) => boolean;
	makeUnique: (forceSuffix?: boolean) => void;
}

export namespace Configuration
{
	export class Registry extends Object
	{
		private CCL_Configuration_Registry: never;

		getValue: (section: string, key: string) => Variant;
	}
}

////////////////////////////////////////////////////////////
// extras/packages
////////////////////////////////////////////////////////////

export class ContentPackageManager
{
	private CCL_ContentPackageManager: never;
	finishStartup (deferred?: boolean): void;
	findPackage (packageId: string): UnifiedPackage;
	canInstall (packageId: string): boolean;
	isInstalled (packageId: string): boolean;
}

export class UnifiedPackage extends Object
{
	private CCL_UnifiedPackage: never;
	isLocalPackage: () => boolean;
}

////////////////////////////////////////////////////////////
// gui
////////////////////////////////////////////////////////////

export class PaletteBase<T = Variant>
{
	private CCL_PaletteBase: never;
	readonly count: number;

	getAt: (index: number) => T;
	getNext: () => T;
}

export class FileSelector extends Object
{
	private CCL_FileSelector: never;
	fileName: string;

	addFilter: (fileType: FileType) => void;
	runOpen: (title?: string) => boolean;
	runSave: (title?: string) => boolean;
	setFileName: (name?: string) => void;
	countPaths: () => number;
	getPath: (index?: number) => Url;
}

export class Image extends Object
{
	private CCL_Image: never;
	readonly width: number;
	readonly height: number;
	readonly frameCount: number;
	currentFrame: number;
	isTemplate: boolean;
	isAdaptive: boolean;
}

export class ProgressStep extends Object
{
	private CCL_ProgressStep: never;
	text: string;
	readonly canceled: boolean;
	readonly detailCount: number;
	value: number;
	title: string;
	cancelEnabled: boolean;

	beginProgress: () => void;
	endProgress: () => void;
	updateAnimated: (text?: string) => void;
}

export class ProgressDialog extends ProgressStep
{
	private CCL_ProgressDialog: never;
	readonly isCancelEnabled: boolean;
}

export class CommandTable extends Object
{
	private CCL_CommandTable: never;

	interpretCommand: (category: string, name: string, checkOnly?: boolean, invoker?: Object) => boolean;
}

export class ScriptGUIHost extends Object
{
	private CCL_ScriptGUIHost: never;
	readonly Configuration: Configuration.Registry;
	readonly Commands: CommandTable;

	flushUpdates: (wait?: boolean) => void;
	alert: (text: string) => void;
	ask: (text: string) => tresult;
	openUrl: (url: Url) => tresult;
	showFile: (url: Url) => tresult;
	keyStateToString: (keyState: number) => string;
}

export class WindowManager extends Object
{
	isWindowOpen: (classID: string) => boolean;
	openWindow: (classID: string, toggle?: boolean) => boolean;
	closeWindow: (classID: string) => boolean;
	centerWindow: (classID: string) => boolean;
	findParameter: (name: string) => Parameter;
}

////////////////////////////////////////////////////////////
// io
////////////////////////////////////////////////////////////

// Boxed::FileType
export class FileType extends Object
{
	private CCL_FileType: never;

	readonly description: string;
	readonly extension: string;
	readonly mimetype: string;
}

////////////////////////////////////////////////////////////
// system/localization
////////////////////////////////////////////////////////////

export class LocaleManager extends Object
{
	private CCL_ScriptGUIHost_LocaleManager: never;
	getStrings: (tableID: string) => TranslationTable;
}

export class TranslationTable extends Object
{
	private CCL_ScriptGUIHost_TranslationTable: never;

	getString: (keyOrScope: string, key?: string) => string;
}

////////////////////////////////////////////////////////////
// system/plugins
////////////////////////////////////////////////////////////

export class PlugInManager extends Object
{
	private CCL_PlugInManager: never;

	createInstance: (className: string) => Object;
	newIterator: (category: string) => Object;
}

export namespace ScriptingHost
{
	export class Console extends Object
	{
		private CCL_ScriptingHost_Console: never;

		writeLine: (text: string) => void;
	}

	export class InterfaceList extends Object
	{
		private CCL_ScriptingHost_InterfaceList: never;
		IUnknown: UID;
		IComponent: UID;
		IObjectNode: UID;
		IEditTask: UID;
		IPersistentEditTask: UID;
		IParamObserver: UID;
		IToolAction: UID;
		IEditHandler: UID;
		IToolConfiguration: UID;
		IEditHandlerHook: UID;
		IToolHelp: UID;
		ICommandHandler: UID;
		IToolMode: UID;
		IToolSet: UID;
		IClassFactory: UID;
		IPortFilter: UID;
		IObserver: UID;
		ITimerTask: UID;
		IController: UID;
		IScriptComponent: UID;
		IPresetMediator: UID;
		IPersistEditTask: UID;
		IBrowserExtension: UID;
		IPersistAttributes: UID;
		IDocumentTemplateHandler: UID;
		IDocumentEventHandler: UID;
		IHelpTutorialHandler: UID;
	}

	export class Signals extends Object
	{
		private CCL_ScriptingHost_Signals: never;

		advise: (subject_or_atom: Object | string, observer: Object) => void;
		unadvise: (subject_or_atom: Object | string, observer: Object) => void;
		signal: (subject_or_atom: Object | string, messageId: string, arg0?: string, arg1?: string) => void;
		flush: () => void;
		postMessage: (observer: Object, delay: number, messageId: string, arg1?: string, arg2?: string, arg3?: string) => void;
	}

	export class ResultsList extends Object
	{
		private CCL_ScriptingHost_ResultsList: never;
		kResultOk: tresult;
		kResultTrue: tresult;
		kResultFalse: tresult;
		kResultNotImplemented: tresult;
		kResultNoInterface: tresult;
		kResultInvalidPointer: tresult;
		kResultFailed: tresult;
		kResultUnexpected: tresult;
		kResultClassNotFound: tresult;
		kResultOutOfMemory: tresult;
		kResultInvalidArgument: tresult;
		kResultWrongThread: tresult;
	}
}

export class ScriptingHost extends Object
{
	private CCL_ScriptingHost: never;
	readonly Interfaces: ScriptingHost.InterfaceList;
	readonly Results: ScriptingHost.ResultsList;
	readonly Classes: PlugInManager;
	readonly Console: ScriptingHost.Console;
	readonly Locales: LocaleManager;
	readonly GUI: ScriptGUIHost;
	readonly Signals: ScriptingHost.Signals;
	// Engine: Engine.EngineHost added in app derived ScriptingHost

	UID: (uid: string) => UID;
	Url: (inputUrl: Url | string, isFolder: boolean) => Url;
	Attributes: (attributes?: any[]) => Attributes;
	DateTime: (time?: string) => DateTime;
	getPlatform: () => string;
	sleep: (millis: number) => void;
}

} // namespace CCL
