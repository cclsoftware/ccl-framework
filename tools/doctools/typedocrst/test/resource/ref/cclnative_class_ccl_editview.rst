.. _details-cclnative-1011:

################
EditView (Class)
################

:ref:`Global <cclnative-home>` > :ref:`CCL <details-cclnative-4>` > :ref:`EditView <details-cclnative-1011>` 

*******
Summary
*******

**Constructors**: [:ref:`constructor <cclnative-1012>`] **Properties**: [:ref:`createEditHandler <cclnative-1087>`] [:ref:`createSelectFunctions <cclnative-1104>`] [:ref:`deleteItem <cclnative-1079>`] [:ref:`deleteSelected <cclnative-1076>`] [:ref:`detectDoubleClick <cclnative-1100>`] [:ref:`detectDrag <cclnative-1096>`] [:ref:`dragEraser <cclnative-1072>`] [:ref:`dragSelection <cclnative-1062>`] [:ref:`drawSelection <cclnative-1066>`] [:ref:`editItem <cclnative-1083>`] [:ref:`findItem <cclnative-1017>`] [:ref:`findItemDeep <cclnative-1026>`] [:ref:`findItemPart <cclnative-1021>`] [:ref:`getEditArea <cclnative-1039>`] [:ref:`getItemSize <cclnative-1092>`] [:ref:`getItemType <cclnative-1035>`] [:ref:`getSelectionSize <cclnative-1043>`] [:ref:`isSameItem <cclnative-1030>`] [:ref:`moveCrossCursor <cclnative-1117>`] [:ref:`select <cclnative-1054>`] [:ref:`selection <cclnative-1016>`] [:ref:`setAnchorItem <cclnative-1050>`] [:ref:`setCursor <cclnative-1113>`] [:ref:`setFocusItem <cclnative-1046>`] [:ref:`showSelection <cclnative-1108>`] [:ref:`unselect <cclnative-1058>`] 

************
Constructors
************

.. _cclnative-1012:

**new EditView** (): **EditView**





**********
Properties
**********

.. _cclnative-1087:

createEditHandler (obj: **Object**, event: **MouseEvent**): EditHandler





-----

.. _cclnative-1104:

createSelectFunctions (functions: **ActionExecuter**): SelectFunctions





-----

.. _cclnative-1079:

deleteItem (obj: **Object**): void





-----

.. _cclnative-1076:

deleteSelected (): void





-----

.. _cclnative-1100:

detectDoubleClick (event: **MouseEvent**): boolean





-----

.. _cclnative-1096:

detectDrag (event: **MouseEvent**): boolean





-----

.. _cclnative-1072:

dragEraser (event: **MouseEvent**): EditHandler





-----

.. _cclnative-1062:

dragSelection (event: **MouseEvent**): void





-----

.. _cclnative-1066:

drawSelection (event: **MouseEvent**, hook: **EditHandlerHook**, hint: **string**): EditHandler





-----

.. _cclnative-1083:

editItem (obj: **Object**): void





-----

.. _cclnative-1017:

findItem (loc: **Point**): Object





-----

.. _cclnative-1026:

findItemDeep (loc: **Point**): Object





-----

.. _cclnative-1021:

findItemPart (obj: **Object**, loc: **Point**): Object





-----

.. _cclnative-1039:

getEditArea (loc: **Point**): string





-----

.. _cclnative-1092:

getItemSize (obj: **Object**): Rect





-----

.. _cclnative-1035:

getItemType (obj: **Object**): string





-----

.. _cclnative-1043:

getSelectionSize (): Rect





-----

.. _cclnative-1030:

isSameItem (obj1: **Object**, obj2: **Object**): boolean





-----

.. _cclnative-1117:

moveCrossCursor (position: **Point**): void





-----

.. _cclnative-1054:

select (obj: **Object**): void





-----

.. _cclnative-1016:

**selection**: Selection *[read-only]*



-----

.. _cclnative-1050:

setAnchorItem (obj: **Object**): void





-----

.. _cclnative-1113:

setCursor (cursorName: **string**): void





-----

.. _cclnative-1046:

setFocusItem (obj: **Object**): void





-----

.. _cclnative-1108:

showSelection (show: **boolean**, redraw: **boolean**): void





-----

.. _cclnative-1058:

unselect (obj: **Object**): void



