#!/bin/bash
#
# Finds the repository root
#
# Usage       : ./get_repo_root.sh
#               The script will find the repository root upwards using the repo.json file
#               as 'marker' for the repository root and return the path
#
#************************************************************************************************

scriptdir=$(dirname "$0")
root_file="repo.json"

repo_root=$(${scriptdir}/find_upwards.sh ${root_file})
if [ -z ${repo_root} ]; then
	>&2 echo "Repository root not found!"
	exit 1
fi

echo ${repo_root}
exit 0