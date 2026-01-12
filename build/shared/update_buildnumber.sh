#!/bin/bash

scriptdir=$(dirname "$0")
outputfile="$1"
BUILD_NUMBER="$2"
DATE=$(date +"%Y/%m/%d %H:%M:%S")

echo "#ifndef _buildnumber_h
#define _buildnumber_h

//////////////////////////////////////////////////////////////////////////////////////////////////

#define BUILD_REVISION_NUMBER		${BUILD_NUMBER}
#define BUILD_REVISION_STRING		\"${BUILD_NUMBER}\"
#define BUILD_DATE_STRING			\"$DATE\"

//////////////////////////////////////////////////////////////////////////////////////////////////

#endif // _buildnumber_h
" > ${outputfile}

