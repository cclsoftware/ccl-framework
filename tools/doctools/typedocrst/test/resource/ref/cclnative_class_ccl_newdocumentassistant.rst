.. _details-cclnative-865:

############################
NewDocumentAssistant (Class)
############################

:ref:`Global <cclnative-home>` > :ref:`CCL <details-cclnative-4>` > :ref:`NewDocumentAssistant <details-cclnative-865>` 

*******
Summary
*******

**Constructors**: [:ref:`constructor <cclnative-866>`] **Properties**: [:ref:`children <cclnative-899>`] [:ref:`find <cclnative-939>`] [:ref:`findParameter <cclnative-928>`] [:ref:`hasChild <cclnative-903>`] [:ref:`hasObject <cclnative-924>`] [:ref:`hasParam <cclnative-908>`] [:ref:`interpretCommand <cclnative-932>`] [:ref:`name <cclnative-895>`] [:ref:`numChildren <cclnative-907>`] [:ref:`paramEditable <cclnative-920>`] [:ref:`paramEnabled <cclnative-916>`] [:ref:`paramValue <cclnative-912>`] [:ref:`parent <cclnative-897>`] [:ref:`self <cclnative-898>`] [:ref:`title <cclnative-896>`] **Methods**: [:ref:`closeDialog <cclnative-893>`] [:ref:`getClassIcon <cclnative-878>`] [:ref:`loadSecondaryTemplates <cclnative-869>`] [:ref:`selectSecondaryTemplate <cclnative-872>`] [:ref:`setConfirmEnabled <cclnative-890>`] [:ref:`setDropFileTypes <cclnative-875>`] [:ref:`setDropImportFile <cclnative-886>`] [:ref:`setExclusiveDropFileTypes <cclnative-883>`] 

************
Constructors
************

.. _cclnative-866:

**new NewDocumentAssistant** (): **NewDocumentAssistant**





**********
Properties
**********

.. _cclnative-899:

children (id: **string**): ObjectNode

inherited from [:ref:`Component.children <cclnative-464>`]  





-----

.. _cclnative-939:

find (path: **string**): ObjectNode

inherited from [:ref:`Component.find <cclnative-504>`]  





-----

.. _cclnative-928:

findParameter (name: **string**): Parameter

inherited from [:ref:`Component.findParameter <cclnative-493>`]  





-----

.. _cclnative-903:

hasChild (id: **string**): ObjectNode

inherited from [:ref:`Component.hasChild <cclnative-468>`]  





-----

.. _cclnative-924:

hasObject (id: **string**): None

inherited from [:ref:`Component.hasObject <cclnative-489>`]  





-----

.. _cclnative-908:

hasParam (id: **string**): Parameter

inherited from [:ref:`Component.hasParam <cclnative-473>`]  





-----

.. _cclnative-932:

interpretCommand (category: **string**, name: **string**, checkOnly: **boolean**, invoker: **Object**): boolean

inherited from [:ref:`Component.interpretCommand <cclnative-497>`]  





-----

.. _cclnative-895:

**name**: string *[read-only]*

inherited from [:ref:`Component.name <cclnative-460>`]  



-----

.. _cclnative-907:

**numChildren**: number *[read-only]*

inherited from [:ref:`Component.numChildren <cclnative-472>`]  



-----

.. _cclnative-920:

paramEditable (id: **string**): boolean

inherited from [:ref:`Component.paramEditable <cclnative-485>`]  





-----

.. _cclnative-916:

paramEnabled (id: **string**): boolean

inherited from [:ref:`Component.paramEnabled <cclnative-481>`]  





-----

.. _cclnative-912:

paramValue (id: **string**): Variant

inherited from [:ref:`Component.paramValue <cclnative-477>`]  





-----

.. _cclnative-897:

**parent**: ObjectNode *[read-only]*

inherited from [:ref:`Component.parent <cclnative-462>`]  



-----

.. _cclnative-898:

**self**: Component *[read-only]*

inherited from [:ref:`Component.self <cclnative-463>`]  



-----

.. _cclnative-896:

**title**: string *[read-only]*

inherited from [:ref:`Component.title <cclnative-461>`]  

*******
Methods
*******

.. _cclnative-893:

**closeDialog** (): **void**







-----

.. _cclnative-878:

**getClassIcon** (className: **string**): **Image**

**Parameters**

* className: **string** 


**getClassIcon** (className: **string**): **Object**

**Parameters**

* className: **string** 








-----

.. _cclnative-869:

**loadSecondaryTemplates** (url: **string**): **DocumentTemplateList**

**Parameters**

* url: **string** 








-----

.. _cclnative-872:

**selectSecondaryTemplate** (index: **number**): **void**

**Parameters**

* index: **number** 








-----

.. _cclnative-890:

**setConfirmEnabled** (state: **boolean**): **void**

**Parameters**

* state: **boolean** 








-----

.. _cclnative-875:

**setDropFileTypes** (fileTypes: **Container | FileType []**): **void**

**Parameters**

* fileTypes: **Container | FileType []** 








-----

.. _cclnative-886:

**setDropImportFile** (path: **Url**, temporaryDocument: **boolean**): **void**

**Parameters**

* path: **Url** 
* temporaryDocument: **boolean** 








-----

.. _cclnative-883:

**setExclusiveDropFileTypes** (fileTypes: **Container | FileType []**): **void**

**Parameters**

* fileTypes: **Container | FileType []** 






