//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl_native.d.ts
// Description : CCL framework native type information
//
//************************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////////////
// Global functions
//////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Runtime include a JavaScript file from a relative or resource path.
 *
 * ```
 * // Relative include example:
 * include_file ("path/file.js")
 *
 * // Resource based include example:
 * include_file ("resource://com.vendor.app/file.js")
 * ```
 *
 * @param string  path to file to include, relative or as absolute resource.
 */
declare var include_file: (path: string) => void;

//////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declared namespaces (for TypeDoc)
//////////////////////////////////////////////////////////////////////////////////////////////////

declare namespace CCL {}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Type aliases
//////////////////////////////////////////////////////////////////////////////////////////////////

declare namespace CCL
{

export type Coord = number; // mask Core type
export type Color = number; // mask Core type
export type tresult = number; // bool incomatible!
export type Variant = boolean | number | string | Object;

} // namespace CCL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Generated type information
//////////////////////////////////////////////////////////////////////////////////////////////////

declare namespace CCL
{
  export class ActionExecuter extends Object
  {
    private CCL_ActionExecuter: never;
    executeImmediately: any;
    beginMultiple: (description: string) => void;
    endMultiple: (cancel?: boolean) => void;
    setJournalEnabled: (enabled?: boolean) => void;
    isJournalEnabled: () => boolean;
  }
  export class AliasParam extends Object
  {
    private CCL_AliasParam: never;
    value: any;
    default: any;
    type: number;
    min: any;
    max: any;
    name: string;
    string: string;
    enabled: boolean;
    signalAlways: boolean;
    reverse: boolean;
    setOriginal: (original: Parameter | null) => void;
    hasOriginal: () => boolean;
    setFeedbackNeeded: (needed: boolean) => void;
    setValue: (value: any, update?: boolean) => void;
  }
  export class Application extends Component
  {
    private CCL_Application: never;
    readonly Configuration: Registry;
  }
  export class Attributes extends Object
  {
    private CCL_Attributes: never;
    countAttributes: () => number;
    getAttributeName: (index: number) => string;
    getAttributeValue: (index: number) => any;
    getAttribute: (name: string) => any;
    setAttribute: (name: string, value: any) => boolean;
    queueAttribute: (id: string, value: any) => boolean;
    newQueueIterator: (id: string) => Iterator;
    contains: (id: string) => boolean;
  }
  export class BitmapFilter extends Object
  {
    private CCL_BitmapFilter: never;
  }
  export class ClassDescription extends Object
  {
    private CCL_ClassDescription: never;
    getClassAttribute: () => void;
  }
  export class ColorParam extends Parameter
  {
    private CCL_ColorParam: never;
    palette: PaletteBase;
  }
  export class ColorPicker extends Component
  {
    private CCL_ColorPicker: never;
    construct: (colorParam: Parameter, applyPresetPalette?: boolean) => void;
    popup: (popupStyle?: VisualStyle | null, useMousePos?: boolean) => boolean;
  }
  export class Command extends Object
  {
    private CCL_Command: never;
  }
  export class CommandParam extends Parameter
  {
    private CCL_CommandParam: never;
  }
  export class CommandTable extends Object
  {
    private CCL_CommandTable: never;
    interpretCommand: (category: string, name: string, checkOnly?: boolean, invoker?: Object | null) => boolean;
    deferCommand: (category: string, name: string, checkOnly?: boolean, invoker?: Object | null) => boolean;
    addHandler: (ICommandHandler: any) => void;
    removeHandler: (ICommandHandler: any) => void;
    findCommand: (category: string, name: string) => Object;
    registerCommand: (category: string, name: string, displayCategory: string, displayName: string, englishName: string, arguments?: string) => void;
    unregisterCommand: (category: string, name: string) => void;
    newCommandIterator: () => void;
    newCategoryIterator: () => void;
    lookupBindings: (command: any) => Object;
    lookupKeyEvent: (command: Command, layoutSensitive?: boolean) => KeyEvent;
    assignKey: (command: any, key: any) => void;
    beginTransaction: () => void;
    endTransaction: () => void;
  }
  export class Component extends ObjectNode
  {
    private CCL_Component: never;
    readonly name: string;
    readonly title: string;
    readonly parent: Component;
    findParameter: (name: string) => Parameter;
    interpretCommand: (category: string, name: string, checkOnly?: boolean, invoker?: Object | null) => boolean;
  }
  export class Container extends Object
  {
    private CCL_Container: never;
    readonly count: number;
    at: (index: number) => Object;
    findEqual: (object_or_string: Object | string) => Object;
    newIterator: () => Iterator;
  }
  export class ContentPackageManager extends PackageManager
  {
    private CCL_ContentPackageManager: never;
    finishStartup: (deferred?: boolean) => void;
    findPackage: (packageId: string) => UnifiedPackage;
    canInstall: (packageId: string) => boolean;
    isInstalled: (packageId: string) => boolean;
  }
  export class DateTime extends Object
  {
    private CCL_DateTime: never;
    toOrdinal: () => void;
  }
  export class Document extends Object
  {
    private CCL_Document: never;
    title: any;
    path: any;
  }
  export class DocumentClass extends Object
  {
    private CCL_DocumentClass: never;
    isNative: () => boolean;
    isPrivate: () => boolean;
    getFileType: () => FileType;
  }
  export class DocumentTemplate extends StorableObject
  {
    private CCL_DocumentTemplate: never;
    readonly title: string;
    readonly subTitle: string;
    readonly description: string;
    readonly additionalData: string;
    readonly tutorialId: string;
    readonly icon: Image;
  }
  export class DocumentTemplateList extends Object
  {
    private CCL_DocumentTemplateList: never;
    getTemplateCount: () => number;
    getTemplate: (index: number) => DocumentTemplate;
  }
  export class EditEnvironment extends Component
  {
    private CCL_EditEnvironment: never;
    mainEditor: any;
    activeEditor: any;
    actionSelection: any;
    focusItem: any;
    focusItemPropertyEditor: any;
    runEditTask: () => void;
  }
  export class EditHandler extends UserControl.MouseHandler
  {
    private CCL_EditHandler: never;
    setEditTooltip: () => void;
  }
  export class EditModel extends Object
  {
    private CCL_EditModel: never;
    updateHighlightItem: (editView: EditView, where: Point) => void;
    hideHighlight: () => void;
    setDocumentDirty: () => void;
  }
  export class EditTaskContext extends Attributes
  {
    private CCL_EditTaskContext: never;
    parameters: ParamContainer;
    editor: Object;
    isArgumentDialog: boolean;
    restore: (force?: boolean) => void;
    runDialog: (formName: string, themeID?: string) => tresult;
    isSilentMode: () => boolean;
    getArguments: () => Attributes;
  }
  export class EditView extends UserControl
  {
    private CCL_EditView: never;
    readonly selection: Selection;
    readonly model: EditModel;
    readonly environment: EditEnvironment;
    findItem: (loc: Point) => Object;
    findItemPart: (item: Object, loc: Point) => Object;
    findItemDeep: (loc: Point) => Object;
    isSameItem: (item1: Object, item2: Object) => boolean;
    getItemType: (item: Object) => string;
    getEditArea: (loc: Point) => string;
    getSelectionSize: () => Rect;
    setFocusItem: (item: Object) => void;
    setAnchorItem: (item: Object) => void;
    select: (item: Object) => void;
    unselect: (item: Object) => void;
    canSelect: (item: Object) => boolean;
    dragSelection: (event: MouseEvent) => void;
    drawSelection: (event: MouseEvent, hook?: Object | null, hint?: string) => EditHandler;
    dragEraser: (event: MouseEvent) => EditHandler;
    deleteSelected: () => void;
    deleteItem: (item: Object) => void;
    editItem: (item: Object) => boolean;
    createEditHandler: (itemPart: Object, event: MouseEvent) => EditHandler;
    getItemSize: (item: Object) => Rect;
    detectDoubleClick: (event: MouseEvent) => boolean;
    detectDrag: (event: MouseEvent) => boolean;
    takeFocus: () => boolean;
    createSelectFunctions: (functions: Object) => SelectFunctions;
    showSelection: (show?: boolean, redraw?: boolean) => void;
    setCursor: (name: string) => void;
    moveCrossCursor: (position: Point) => void;
  }
  export class FileArchive extends FileStreamResource
  {
    private CCL_FileArchive: never;
    setOption: () => void;
    create: () => void;
    embedd: () => void;
    extract: () => void;
    extractAll: () => void;
    flush: () => void;
    close: () => void;
  }
  export class FileResource extends Object
  {
    private CCL_FileResource: never;
  }
  export class FileStreamResource extends Object
  {
    private CCL_FileStreamResource: never;
  }
  export class FileType extends Object
  {
    private CCL_FileType: never;
    description: string;
    extension: string;
    mimetype: string;
  }
  export class FileTypeRegistry extends Object
  {
    private CCL_FileTypeRegistry: never;
    registerFileType: (fileType: Object | string) => tresult;
    getFileTypeByExtension: (extension: string) => Object;
    getFileTypeByMimeType: (mimetype: string) => Object;
    registerHandler: (fileType: Object | string, location: Url, observer: Object) => tresult;
    unregisterHandler: (fileType: Object | string) => tresult;
  }
  export class FloatParam extends Parameter
  {
    private CCL_FloatParam: never;
  }
  export class Formatter extends Object
  {
    private CCL_Formatter: never;
    print: (value: any) => string;
    scan: (value: string) => any;
  }
  export class GraphicsHelper extends Object
  {
    private CCL_GraphicsHelper: never;
    loadImage: (path: any) => Image;
    saveImage: (path: any, image: any, encoderOptions?: any) => void;
    createBitmap: (width: any, height: any) => Image;
    createFilmstrip: (image: any, frames: any) => Image;
    createImagePart: (image: any, left: any, top: any, width: any, height: any) => Image;
    copyBitmap: (dstBitmap: any, srcBitmap: any, offsetX: any, offsetY: any) => void;
    createBitmapFilter: (name: any) => BitmapFilter;
    processBitmap: (bitmap: any, filter: any, inplace?: any) => Image;
  }
  export class HelpTutorial extends Object
  {
    private CCL_HelpTutorial: never;
    getID: () => string;
    getTitle: () => string;
    getCategory: () => string;
  }
  export class Image extends Object
  {
    private CCL_Image: never;
  }
  export class ImageProvider extends Parameter
  {
    private CCL_ImageProvider: never;
    setImage: (image: Object, update?: boolean) => void;
  }
  export class IntParam extends Parameter
  {
    private CCL_IntParam: never;
  }
  export class Iterator extends Object
  {
    private CCL_Iterator: never;
    done: () => boolean;
    first: () => void;
    last: () => void;
    next: () => Object;
    previous: () => Object;
  }
  export class Key extends KeyEvent
  {
    private CCL_Key: never;
  }
  export class KeyEvent extends Object
  {
    private CCL_KeyEvent: never;
    toString: (translated?: boolean) => String;
  }
  export class Linkable extends Object
  {
    private CCL_Linkable: never;
  }
  export class ListParam extends IntParam
  {
    private CCL_ListParam: never;
    appendString: (str: string) => void;
    appendValue: (value: any) => void;
    removeAll: () => void;
    getValueAt: (index: number) => any;
    getSelectedValue: () => any;
    selectValue: (value: any, update?: boolean) => void;
  }
  export class LocaleManager extends Object
  {
    private CCL_LocaleManager: never;
    getStrings: (tableId: any) => TranslationTable;
  }
  export class MenuParam extends ListParam
  {
    private CCL_MenuParam: never;
  }
  export class Message extends Object
  {
    private CCL_Message: never;
    id: string;
    readonly argCount: number;
    getArg: (index: number) => any;
    getArgCount: () => number;
  }
  export class MouseEvent extends Object
  {
    private CCL_MouseEvent: never;
  }
  export class NewDocumentAssistant extends Component
  {
    private CCL_NewDocumentAssistant: never;
    loadSecondaryTemplates: (path: Url) => DocumentTemplateList;
    selectSecondaryTemplate: (index: number) => void;
    setDropFileTypes: (fileTypes: Container | FileType[]) => void;
    setExclusiveDropFileTypes: (fileTypes: Container | FileType[]) => void;
    getClassIcon: (className: string) => Object;
    setDropImportFile: (path: Url, temporaryDocument?: boolean) => void;
    setConfirmEnabled: (state: boolean) => void;
    closeDialog: (apply?: boolean) => void;
  }
  export class Object
  {
    private CCL_Object: never;
  }
  export class ObjectNode extends Object
  {
    private CCL_ObjectNode: never;
    find: (childName: string) => ObjectNode;
  }
  export class ObjectTable extends Object
  {
    private CCL_ObjectTable: never;
    registerObject: (obj: any, name: any) => tresult;
    unregisterObject: (obj: any) => tresult;
    getObjectByName: (name: any) => Object;
    getObjectByID: (uidString: any) => Object;
    getObjectByUrl: (url: any, optional: any) => Object;
  }
  export class PackageDocument extends Document
  {
    private CCL_PackageDocument: never;
  }
  export class PackageFile extends FileArchive
  {
    private CCL_PackageFile: never;
  }
  export class PackageManager extends Component
  {
    private CCL_PackageManager: never;
  }
  export class PaletteBase extends Object
  {
    private CCL_PaletteBase: never;
    count: any;
    getAt: () => void;
    getNext: () => void;
  }
  export class PaletteParam extends ListParam
  {
    private CCL_PaletteParam: never;
  }
  export class ParamContainer extends Object
  {
    private CCL_ParamContainer: never;
    add: (param: Parameter) => Parameter;
    addParam: (name: string) => Parameter;
    addFloat: (min: number, max: number, name: string) => FloatParam;
    addInteger: (min: number, max: number, name: string) => IntParam;
    addString: (name: string) => StringParam;
    addList: (name: string) => ListParam;
    addMenu: (name: string) => MenuParam;
    addCommand: (commandCategory: string, commandName: string, name: string) => CommandParam;
    addColor: (name: string) => ColorParam;
    addAlias: (name: string) => AliasParam;
    addImage: (name: string) => ImageProvider;
    remove: (name: string) => Parameter;
    lookup: (name: string) => Parameter;
    findParameter: (name: string) => Parameter;
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
  export class PlugInCollection extends Object
  {
    private CCL_PlugInCollection: never;
  }
  export class PlugInManager extends PlugInCollection
  {
    private CCL_PlugInManager: never;
    createInstance: (className_or_cid: UID | string) => Object;
    getClassDescription: (className_or_cid: UID | string) => ClassDescription;
    newIterator: (category: any) => Iterator;
  }
  export class PlugInMenuParam extends MenuParam
  {
    private CCL_PlugInMenuParam: never;
    setCategory: (category: string) => void;
    setCategory2: (category: string) => void;
    setDisplaySorted: (state: boolean) => void;
    setDisplayUnselectItem: (state: boolean) => void;
    getSelectedClass: () => string;
    selectClass: (cid: UID | string) => boolean;
  }
  export class PlugInSelector extends Component
  {
    private CCL_PlugInSelector: never;
    setCategories: (classCategory1: string, classCategory2: string) => void;
    getSelected: () => string;
  }
  export class Point extends Object
  {
    private CCL_Point: never;
    equals: () => void;
  }
  export class Preset extends Object
  {
    private CCL_Preset: never;
    presetName: string;
    relativePath: string;
  }
  export class PresetManager extends Component
  {
    private CCL_PresetManager: never;
    presetExists: (metaInfo: Attributes, presetName: string, fileType?: FileType | null) => boolean;
  }
  export class PresetParam extends MenuParam
  {
    private CCL_PresetParam: never;
    setMetaInfo: (metaInfo: Attributes) => void;
    shouldShowFolders: (state: boolean) => void;
    selectRelativePath: (path: string) => void;
  }
  export class ProgressStep extends Object
  {
    private CCL_ProgressStep: never;
    canceled: boolean;
    detailCount: number;
    value: number;
    text: string;
    title: string;
    cancelEnabled: boolean;
    updateAnimated: () => void;
    beginProgress: () => void;
    endProgress: () => void;
  }
  export class Rect extends Object
  {
    private CCL_Rect: never;
    equals: () => void;
    pointInside: () => void;
    clone: () => void;
  }
  export class Registry extends Object
  {
    private CCL_Registry: never;
    getValue: (section: string, key: string) => any;
  }
  export class ScriptGUIHost extends Object
  {
    private CCL_ScriptGUIHost: never;
    Constants: any;
    readonly Commands: CommandTable;
    Themes: any;
    Desktop: any;
    Help: any;
    Configuration: any;
    Clipboard: any;
    Alerts: any;
    flushUpdates: (wait?: boolean) => void;
    alert: (text: any) => void;
    ask: (text: any) => void;
    runDialog: (theme: Object, formName: string, controller?: Object | null, buttons?: number) => number;
    runDialogWithParameters: (paramList: any, title: any) => number;
    addIdleTask: () => void;
    removeIdleTask: () => void;
    openUrl: () => void;
    showFile: () => void;
    keyStateToString: () => void;
  }
  export class ScriptingHost extends Object
  {
    private CCL_ScriptingHost: never;
    readonly Interfaces: ScriptingHost.InterfaceList;
    readonly Results: ScriptingHost.ResultsList;
    readonly Classes: PlugInManager;
    readonly Objects: ObjectTable;
    readonly Services: ServiceManager;
    readonly Console: ScriptingHost.Console;
    readonly Signals: ScriptingHost.Signals;
    readonly Locales: LocaleManager;
    readonly SystemInfo: SystemInformation;
    readonly IO: ScriptingHost.ScriptIO;
    readonly FileTypes: FileTypeRegistry;
    retain: (obj: any) => void;
    release: (obj: any) => void;
    UID: () => UID;
    Url: (url: Url | string) => Url;
    Attributes: (...vargs: any[]) => any;
    DateTime: () => DateTime;
    getPlatform: () => string;
    sleep: (ms: number) => void;
  }
  export namespace ScriptingHost
  {
    export class Console extends Object
    {
      private CCL_ScriptingHost_Console: never;
      writeLine: (string: any) => void;
    }
    export class InterfaceList extends Object
    {
      private CCL_ScriptingHost_InterfaceList: never;
      readonly IUnknown: UID;
      readonly IBrowserExtension: UID;
      readonly IDocumentTemplateHandler: UID;
      readonly IDocumentEventHandler: UID;
      readonly IEditTask: UID;
      readonly IToolConfiguration: UID;
      readonly IToolMode: UID;
      readonly IToolHelp: UID;
      readonly IToolSet: UID;
      readonly IToolAction: UID;
      readonly IEditHandlerHook: UID;
      readonly IEditHandler: UID;
      readonly IPresetMediator: UID;
      readonly IExtensionHandler: UID;
      readonly ICommandHandler: UID;
      readonly IContextMenuHandler: UID;
      readonly IParamObserver: UID;
      readonly IViewStateHandler: UID;
      readonly ITimerTask: UID;
      readonly IHelpTutorialHandler: UID;
      readonly IController: UID;
      readonly IScriptComponent: UID;
      readonly IClassFactory: UID;
      readonly IComponent: UID;
      readonly IObjectNode: UID;
      readonly IObserver: UID;
      readonly IPersistAttributes: UID;
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
    export class ScriptIO extends Object
    {
      private CCL_ScriptingHost_ScriptIO: never;
      File: () => void;
      findFiles: (startPoint: any, searchPattern: any) => Iterator;
      openTextFile: (path: any, [encoding]: any) => TextFile;
      createTextFile: (path: any, [encoding]: any) => TextFile;
      loadJsonFile: (path: Url) => Attributes;
      XmlTree: () => void;
      toBase64: (string: any, [encoding]: any) => string;
      fromBase64: (string: any, [encoding]: any) => string;
      openPackage: (path: any) => PackageFile;
      createPackage: (path: any, [mimeType]: any) => PackageFile;
      getDevelopmentFileLocation: (root: string, relativePath: string) => Url;
    }
    export class Signals extends Object
    {
      private CCL_ScriptingHost_Signals: never;
      signal: (subject_or_atom: Object | string, messageId: string, arg0?: string, arg1?: string) => void;
      flush: () => void;
      advise: (subject_or_atom: Object | string, observer: Object) => void;
      unadvise: (subject_or_atom: Object | string, observer: Object) => void;
      postMessage: (observer: Object, delay: number, messageId: string, arg1?: any, arg2?: any, arg3?: any) => void;
    }
  }
  export class SecurityHost extends Object
  {
    private CCL_SecurityHost: never;
    checkAccess: (resourceSid: any, itemSid: any) => boolean;
  }
  export class SelectFunctions extends ActionExecuter
  {
    private CCL_SelectFunctions: never;
    selectExclusive: boolean;
    makeItemsVisible: boolean;
    focusItem: Object;
    select: (item: Object) => boolean;
    selectMultiple: (items: Container | Object[]) => boolean;
    saveSelection: () => boolean;
    takeSnapshot: () => boolean;
  }
  export class Selection extends SelectionContainer
  {
    private CCL_Selection: never;
    isSelected: () => void;
    isEmpty: () => void;
    isMultiple: () => void;
    newIterator: () => void;
    unselectAll: () => void;
    unselectType: () => void;
    ignoreChanges: () => void;
  }
  export class SelectionContainer extends Object
  {
    private CCL_SelectionContainer: never;
  }
  export class ServiceManager extends Object
  {
    private CCL_ServiceManager: never;
    getInstance: (cid: any) => void;
  }
  export class Settings extends Object
  {
    private CCL_Settings: never;
    getAttributes: (Attributes: any) => string;
  }
  export class StorableObject extends Object
  {
    private CCL_StorableObject: never;
    saveToFile: (path: any) => boolean;
    loadFromFile: (path: any) => boolean;
  }
  export class StringParam extends Parameter
  {
    private CCL_StringParam: never;
  }
  export class SystemInformation extends Object
  {
    private CCL_SystemInformation: never;
    getLocalTime: () => DateTime;
  }
  export class TextFile extends Object
  {
    private CCL_TextFile: never;
    endOfStream: any;
    readLine: () => void;
    writeLine: () => void;
    writeString: () => void;
    close: () => void;
  }
  export class TranslationTable extends Object
  {
    private CCL_TranslationTable: never;
    getString: () => void;
  }
  export class UID extends Object
  {
    private CCL_UID: never;
    equals: (id: UID | string) => boolean;
  }
  export class UnifiedPackage extends Object
  {
    private CCL_UnifiedPackage: never;
    isLocalPackage: () => boolean;
  }
  export class Url extends Object
  {
    private CCL_Url: never;
    name: any;
    url: any;
    extension: any;
    protocol: any;
    hostname: any;
    ascend: () => void;
    descend: (name: any, folder?: any) => void;
    makeUnique: (forceSuffix?: any) => void;
    toDisplayString: () => string;
    fromDisplayString: (string: any, folder?: any) => void;
    contains: (childUrl: Url) => void;
    getName: (withExtension?: boolean) => string;
  }
  export class UserControl extends Object
  {
    private CCL_UserControl: never;
  }
  export namespace UserControl
  {
    export class MouseHandler extends Object
    {
      private CCL_UserControl_MouseHandler: never;
    }
  }
  export class VisualStyle extends VisualStyleData
  {
    private CCL_VisualStyle: never;
  }
  export class VisualStyleData extends Object
  {
    private CCL_VisualStyleData: never;
  }
  export class WindowManager extends Object
  {
    private CCL_WindowManager: never;
    isWindowOpen: (classID: string) => boolean;
    openWindow: (classID: string, toggle?: boolean) => boolean;
    closeWindow: (classID: string) => boolean;
    centerWindow: (classID: string) => boolean;
    findParameter: (name: string) => Parameter;
  }
  export class XmlSettings extends Settings
  {
    private CCL_XmlSettings: never;
  }
}
