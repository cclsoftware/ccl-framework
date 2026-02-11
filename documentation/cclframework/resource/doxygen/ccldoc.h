//************************************************************************************************
//
// CCL Documentation with Doxygen
//
// Filename    : ccldoc.h
// Created by  : Mario Ewald, Matthias Juwan
// Description : Define Doxygen documentation module structure
//
//************************************************************************************************

//************************************************************************************************
//************************************************************************************************
// CCL 
//************************************************************************************************
//************************************************************************************************

namespace CCL {

//************************************************************************************************
// Base
//************************************************************************************************
/** \defgroup ccl CCL
Crystal Class Library 
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup ccl_base CCL-Base
Basic Interfaces and Structures - static lib.
\ingroup ccl
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup base_collect Collections
Container classes.
\ingroup ccl_base 
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup base_io IO
Persistence / File IO.
\ingroup ccl_base
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup base_plug Plugins
Plug-In System Interfaces.
\ingroup ccl_base 

Essential Classes & Interfaces
- System::GetPlugInManager / IPlugInManager
- IClassDescription
- \ref ccl_new / \ref ccl_release

*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \namespace CCL::Database 
Database interfaces.
\ingroup ccl_base */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** Class IDs to be used with ccl_new. 
	\ingroup ccl_base  */
namespace ClassID {
}

//////////////////////////////////////////////////////////////////////////////////////////////////

// 'using' does not seem to be known in doxygen so we import some types from Core to CCL using typedef...
typedef Core::UIDBytes UIDBytes;
typedef Core::UIDRef UIDRef;

/** Interface calling convention (platform dependent). */
#define CCL_API

//************************************************************************************************
// Text
//************************************************************************************************

/** \defgroup ccl_text CCL-Text
Unicode Text and XML (dynamic library).
\ingroup ccl

\section ec1 Essential Classes & Interfaces
- IXmlParser / System::CreateXmlParser / XmlContentParser / IXmlContentHandler
- IXmlWriter / System::CreateXmlWriter

- \ref translation
  - \ref BEGIN_XSTRINGS / \ref END_XSTRINGS / \ref XSTRING
  - \ref XSTR
  - \ref TRANSLATE
 */

/** \defgroup text_string Strings
String classes.
\ingroup ccl_text

Essential Classes & Interfaces
- CString / MutableCString
- String / StringChars / StringWriter
  - namespace Unicode
  - StringBuilder / StringUtils
*/

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \namespace CCL::Text 
Text definitions.
\ingroup ccl_text */

//************************************************************************************************
// System
//************************************************************************************************

/** \defgroup ccl_system CCL-System
System Function (dynamic library).
- System singletons can be accessed via \ref namespace CCL::System
\ingroup ccl */

//////////////////////////////////////////////////////////////////////////////////////////////////

/**	\namespace CCL::System 
System Functions.
\ingroup ccl_system */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \namespace CCL::Threading 
Threading interfaces and structures.
\ingroup ccl_system */

//************************************************************************************************
// GUI
//************************************************************************************************

/** \defgroup gui CCL-GUI
Graphical User Interface (dynamic library).
\ingroup ccl */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_graphics Graphics
Graphic engine interfaces.
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_graphics3d 3D Graphics
3D Graphics interfaces.
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_event User Interface Events
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_command Commands
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_help Help System
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_view Views
Interfaces for Views and Windows.
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_item List- and Treeviews
\ingroup gui_view */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_appview Custom Controls
Application Defined Views and Controls.
\ingroup gui_view */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_dialog Dialogs
\ingroup gui_view */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_skin Skin
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_param Parameters
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_menu Menu
Menu related interfaces and structures.
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_data Data Transfer
Drag & Drop and Clipboard.
\ingroup gui */

//////////////////////////////////////////////////////////////////////////////////////////////////

/** \defgroup gui_accessibility Accessibility
\ingroup gui */

//************************************************************************************************
// APP
//************************************************************************************************

/**	\defgroup ccl_app CCL-App
Application building blocks (static library)
\ingroup ccl */

//////////////////////////////////////////////////////////////////////////////////////////////////

/**	\defgroup app_inter Interfaces
Application interfaces
\ingroup ccl_app */

//////////////////////////////////////////////////////////////////////////////////////////////////

/**	\defgroup app_preset Presets
Application preset handling
\ingroup ccl_app */

} // namespace CCL
