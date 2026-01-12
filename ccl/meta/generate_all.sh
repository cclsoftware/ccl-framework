#!/bin/sh

# Tool
scriptdir=$(dirname "$0")
gentool=$(${scriptdir}/../../tools/bin/findtool.sh ccl cclgenerator)
classmodeldir=${scriptdir}/../../classmodels
metamodeldir=${scriptdir}/models
workdir=.
templatedir=${workdir}/templates
generateddir=${workdir}/generated

# controlstyles-generated.h
${gentool} -g \
-input "${metamodeldir}/controlstyles.json" \
-output "${generateddir}/cpp/controlstyles-generated.h" \
-template "${templatedir}/cpp/controlstyles.h.in"

# ControlStyles.java
${gentool} -g \
-input "${metamodeldir}/controlstyles.json" \
-output "${generateddir}/java/ControlStyles.java" \
-template "${templatedir}/java/controlstyles.java.in"

# graphics-constants-generated.h
${gentool} -g \
-input "${metamodeldir}/graphics-constants.json" \
-output "${generateddir}/cpp/graphics-constants-generated.h" \
-template "${templatedir}/cpp/graphics-constants.h.in"

# GraphicsConstants.java
${gentool} -g \
-input "${metamodeldir}/graphics-constants.json" \
-output "${generateddir}/java/GraphicsConstants.java" \
-template "${templatedir}/java/graphics-constants.java.in"

# gui-constants-generated.h
${gentool} -g -input "${metamodeldir}/gui-constants.json" \
-output "${generateddir}/cpp/gui-constants-generated.h" \
-template "${templatedir}/cpp/gui-constants.h.in"

# GuiConstants.java
${gentool} -g \
-input "${metamodeldir}/gui-constants.json" \
-output "${generateddir}/java/GuiConstants.java" \
-template "${templatedir}/java/gui-constants.java.in"

# skinenums-generated.ts
${gentool} -g \
-input "${classmodeldir}/Skin Elements.classModel" \
-output "${generateddir}/typescript/skinenums-generated.ts" \
-template "${templatedir}/typescript/skinenums.ts.in"

# skinxmldefs-generated.h
${gentool} -g \
-input "${metamodeldir}/skinxmldefs.json" \
-output "${generateddir}/cpp/skinxmldefs-generated.h" \
-template "${templatedir}/cpp/skinxmldefs.h.in"

# skinxmldefs-generated.ts
${gentool} -g \
-input "${metamodeldir}/skinxmldefs.json" \
-output "${generateddir}/typescript/skinxmldefs-generated.ts" \
-template "${templatedir}/typescript/skinxmldefs.ts.in"
