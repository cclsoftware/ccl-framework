#!/bin/bash
#
# Normalize a po file
#
# Usage       : ./normalize.sh <translations path> <language code>
#               You usually won't run this file on its own. Use the helper scripts 
#               in your project subdirectory instead
#
#************************************************************************************************

scriptdir=$(dirname "$0")

translations=$1
lang=$2

moduledir="${translations}/modules"
workdir="${moduledir}/strings/${lang}"

for filepath in "${workdir}"/*.po; do
	python ${scriptdir}/normalizepo.py -s "$filepath"
done
