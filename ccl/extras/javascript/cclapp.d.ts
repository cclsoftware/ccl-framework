/** Function to serve as CCL namespace. */
declare function CCL(): void;
declare namespace CCL {
    /** Function to serve as CCL.JS namespace. */
    function JS(): void;
    namespace JS {
        /** Get platform line ending. */
        function EndLine(): "\r\n" | "\r" | "\n";
        /** Get host application. */
        function getApplication(): any;
        /**
         * Lookup window manager object in host.
         * @returns {object} window manager object, may be null
         */
        function getWindowManager(): any;
        /** Convert Variant value to bool, supporting {false, 0, "0"} and {true, 1, "1"}. */
        function variantToBool(value: any): boolean;
        /** Helper function to construct a resource URL. */
        function ResourceUrl(fileName: any, isFolder: any): any;
        /** Helper function to make a legal file name. */
        function LegalFileName(fileName: any): any;
        namespace Columns {
            let kIconID: string;
            let kTitleID: string;
            let kCheckBoxID: string;
            let kSizable: number;
            let kMoveable: number;
            let kSortable: number;
            let kCanFit: number;
        }
        let kChanged: string;
        let kExtendMenu: string;
        let kRequestFocus: string;
        let kSelectionChanged: string;
        let kItemOpened: string;
        let kItemFocused: string;
        let kEditItemCell: string;
        let kCommandSelected: string;
        let kOpenFile: string;
        let kWindowOpened: string;
        let kWindowClosed: string;
        namespace DocumentEvent {
            let kCreated: number;
            let kBeforeLoad: number;
            let kLoadFinished: number;
            let kLoadFailed: number;
            let kBeforeSave: number;
            let kSaveFinished: number;
            let kClose: number;
            let kActivate: number;
            let kDeactivate: number;
            let kViewActivated: number;
            let kDestroyed: number;
            let kBeforeAutoSave: number;
            let kAutoSaveFinished: number;
        }
        /** Basic GUI component definition. */
        function Component(): void;
        class Component {
            getTheme(): any;
            initialize(context: any): any;
            terminate(): any;
            paramChanged(param: any): void;
            notify(subject: any, msg: any): void;
            onExtendMenu(param: any, menu: any): void;
            checkCommandCategory(category: any): boolean;
            interpretCommand(msg: any): boolean;
        }
        /**
         * Highlight a control in the host UI.
         *
         * @param {string} helpId  UI element help identifier
         * @param {boolean} exclusive  highlight exclusively, default true
         */
        function highlightControl(helpId: string, exclusive?: boolean): void;
        /**
         * Discard any active control highlights.
         */
        function discardHighlights(): void;
        /**
         * Indicate the begin / end of (multiple) modifications to control highlights.
         */
        function modifyHighlights(begin?: boolean): void;
        /**
         * Dim all windows, use discardHighlights () to cancel.
         */
        function dimAllWindows(): void;
        /**
         * Open tutorial.
         *
         * @param {string} tutorialId  tutorial identifier
         * @param {number} delay  open delay in ms
         */
        function showTutorial(tutorialId: string, delay: number): void;
        /**
         * Align tutorial viewer with a specified control.
         *
         * @param {string} helpId  UI element help identifier
         */
        function alignActiveTutorial(helpId: string): void;
        /**
         * Center tutorial viewer (reset position).
         */
        function centerActiveTutorial(): void;
        /**
         * Focus tutorial viewer (activate window).
         */
        function focusActiveTutorial(): void;
    }
}
/**
 * Shorcut to create instance via Plug-in Manager.
 * @param {string} className  name of class to instantiate
 */
declare function ccl_new(className: string): any;
/** Script initialization function. */
declare function __init(args: any): void;
/**
 * Translate key string.
 * JSTRANSLATE is parsed by String Extractor.
 *
 * @param {string} key  translation table id
 */
declare function JSTRANSLATE(key: string): any;
/** Package identifier. */
declare var kPackageID: string;
