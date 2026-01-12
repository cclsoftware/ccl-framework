#!/bin/bash
#
# Scan source files for translatable strings and creates pot files
#
# Usage       : ./makepot.sh <config file>
#               You usually won't run this file on its own. Use the helper scripts 
#               in your project subdirectory instead
#
#************************************************************************************************

config="$1"
scriptdir=$(dirname "$0")
configdir=$(dirname "${config}")
xstring=$(${scriptdir}/../../tools/bin/findtool.sh ccl xstring)

if [ ! -f "${config}" ]; then
	>&2 echo "Usage: ./makepot.sh <config file>"
	exit 1
fi

while read line
do
	params=(${line})
	
	if [ "${#params[@]}" -lt '3' ]; then
		continue
	fi
	
	type=${params[0]}
	input=${params[1]}
	output=${params[2]}
	model=${params[3]}
	option=${params[4]}
	${xstring} -${type} ${configdir}/${input} -po ${configdir}/${output} ${configdir}/${model} ${option}
done < "${config}"
