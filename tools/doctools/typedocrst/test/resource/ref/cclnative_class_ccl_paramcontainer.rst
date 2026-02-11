.. _details-cclnative-579:

######################
ParamContainer (Class)
######################

:ref:`Global <cclnative-home>` > :ref:`CCL <details-cclnative-4>` > :ref:`ParamContainer <details-cclnative-579>` 

*******
Summary
*******

**Constructors**: [:ref:`constructor <cclnative-580>`] **Properties**: [:ref:`add <cclnative-583>`] [:ref:`addAlias <cclnative-625>`] [:ref:`addColor <cclnative-621>`] [:ref:`addCommand <cclnative-615>`] [:ref:`addFloat <cclnative-591>`] [:ref:`addImage <cclnative-629>`] [:ref:`addInteger <cclnative-597>`] [:ref:`addList <cclnative-607>`] [:ref:`addMenu <cclnative-611>`] [:ref:`addParam <cclnative-587>`] [:ref:`addString <cclnative-603>`] [:ref:`lookup <cclnative-637>`] [:ref:`remove <cclnative-633>`] 

************
Constructors
************

.. _cclnative-580:

**new ParamContainer** (): **ParamContainer**





**********
Properties
**********

.. _cclnative-583:

add (param: **Parameter**): Parameter





-----

.. _cclnative-625:

addAlias (name: **string**): AliasParam





-----

.. _cclnative-621:

addColor (name: **string**): ColorParam





-----

.. _cclnative-615:

addCommand (commandCategory: **string**, commandName: **string**, name: **string**): Parameter





-----

.. _cclnative-591:

addFloat (rangeFrom: **number**, rangeTo: **number**, name: **string**): FloatParam





-----

.. _cclnative-629:

addImage (name: **string**): ImageProvider





-----

.. _cclnative-597:

addInteger (rangeFrom: **number**, rangeTo: **number**, name: **string**): IntParam





-----

.. _cclnative-607:

addList (name: **string**): ListParam





-----

.. _cclnative-611:

addMenu (name: **string**): ListParam





-----

.. _cclnative-587:

addParam (name: **string**): Parameter





-----

.. _cclnative-603:

addString (name: **string**): StringParam





-----

.. _cclnative-637:

lookup (name: **string**): Parameter





-----

.. _cclnative-633:

remove (name: **string**): boolean



