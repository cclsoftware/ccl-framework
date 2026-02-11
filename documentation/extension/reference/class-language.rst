Use as an if-statement, followed by the configuration option to set for that language. Supported languages are: de, es, fr, it, ja, pt, zh.

.. code-block:: xml
   :caption: <language> element format

   <?language [LANG]?>
   <!-- option -->


.. code-block:: xml
   :caption: Set language specific package attributes.

   <?xml version="1.0" encoding="UTF-8"?>
   <MetaInformation>
      <?language de?>
      <Attribute id="Package:LocalizedDescription" value="Programmerweiterung..."/>
      <?language es?>
      <Attribute id="Package:LocalizedDescription" value="Ampliación del programa..."/>	
      <!-- Other languages ... -->
   </MetaInformation>
