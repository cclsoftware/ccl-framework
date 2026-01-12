#!/bin/bash
#
# Merge new strings from a pot file into an existing po file
#
# Usage       : ./merge.sh <translations path> <language code>
#               You usually won't run this file on its own. Use the helper scripts 
#               in your project subdirectory instead
#
#************************************************************************************************

scriptdir=$(dirname "$0")
msgmerge=$(${scriptdir}/../../tools/bin/findtool.sh gettext msgmerge)

translations=$1
lang=$2

moduledir="${translations}/modules"
workdir="${moduledir}/strings/${lang}"

for filepath in "${workdir}"/*.po; do
	filename=$(basename "$filepath")
	echo -n "${filename}: "
	${msgmerge} -U -N --backup=none --no-wrap $filepath "${moduledir}/templates/${filename%.*}.pot"
done
