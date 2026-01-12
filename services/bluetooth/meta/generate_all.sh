#!/bin/sh

# Tool
scriptdir=$(dirname "$0")
gentool=$(${scriptdir}/../../../tools/bin/findtool.sh ccl cclgenerator)
metamodeldir=${scriptdir}/models
workdir=.
templatedir=${workdir}/templates
generateddir=${workdir}/generated

# bluetooth-constants-generated.h
${gentool} -g \
-input "${metamodeldir}/bluetooth-constants.json" \
-output "${generateddir}/cpp/bluetooth-constants-generated.h" \
-template "${templatedir}/cpp/bluetooth-constants.h.in"

# BluetoothPermissions.java
${gentool} -g \
-input "${metamodeldir}/bluetooth-constants.json" \
-output "${generateddir}/java/BluetoothConstants.java" \
-template "${templatedir}/java/bluetooth-constants.java.in"
