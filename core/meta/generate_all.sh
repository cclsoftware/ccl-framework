#!/bin/sh

# Tool
scriptdir=$(dirname "$0")
gentool=$(${scriptdir}/../../tools/bin/findtool.sh ccl cclgenerator)
metamodeldir=${scriptdir}/models
workdir=.
templatedir=${workdir}/templates
generateddir=${workdir}/generated

# coregui-constants-generated.h
${gentool} -g \
-input "${metamodeldir}/coregui-constants.json" \
-output "${generateddir}/cpp/coregui-constants-generated.h" \
-template "${templatedir}/cpp/coregui-constants.h.in"

# CoreGuiConstants.h
${gentool} -g \
-input "${metamodeldir}/coregui-constants.json" \
-output "${generateddir}/java/CoreGuiConstants.java" \
-template "${templatedir}/java/coregui-constants.java.in"
