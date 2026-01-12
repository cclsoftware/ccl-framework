#!/bin/bash

codesign () {
	TARGET="$1"
	FLAGS="$2"

	if [ "$3" ]; then
		CERTIFICATE="$3"
	fi
	
	if [ -z "$CERTIFICATE"  ]; then
		CERTIFICATE=`xcrun codesign -dvv "$TARGET" 2>&1 | sed -n -E 's/Authority=(.*)/\1/p' | head -1`
	fi
	
	if [ -z "$CERTIFICATE"  ]; then
        return 0
	fi

    # Entitlements that have to be allow-listed must not be applied with --deep
	if [ "$4" ]; then
		RESTRICTED_ENTITLEMENTS="$4"
	fi
        
	STATUS=1
	RETRY_COUNT=0
	SLEEP_INTERVAL=5

	export CODESIGN_ALLOCATE=`xcrun -f codesign_allocate`
	while true; do
		rm -f targets.txt
		
		# most subfolders in a bundle are not covered by the --deep flag
		find "$TARGET/Contents" -name "*.bundle" -print0 >>targets.txt 2>/dev/null
		find "$TARGET/Contents" -name "*.dylib" -print0 >>targets.txt 2>/dev/null
		find "$TARGET/Contents" -name "*.app" -print0 >>targets.txt 2>/dev/null
		find "$TARGET/Frameworks" -name "*.framework" -print0 >>targets.txt 2>/dev/null

		printf "${TARGET}\0" >>targets.txt
		if [ -z "$RESTRICTED_ENTITLEMENTS" ]; then
			cat targets.txt | xargs -t -0 xcrun codesign --sign "$CERTIFICATE" --force --timestamp -v --deep --preserve-metadata=identifier,entitlements,runtime $FLAGS
		else
			cat targets.txt | xargs -t -0 xcrun codesign --sign "$CERTIFICATE" --force --timestamp -v --deep --preserve-metadata=identifier,entitlements,runtime $FLAGS
			xcrun codesign --sign "$CERTIFICATE" --force --timestamp -v --preserve-metadata=identifier,entitlements,runtime $FLAGS --entitlements $RESTRICTED_ENTITLEMENTS "$TARGET"
		fi

		STATUS=$?
		rm targets.txt
		echo "codesign status=$STATUS"
		if [ $STATUS == 0 ]; then
			break
		else
			if [ $RETRY_COUNT == 6 ]; then
				break
			fi
			sleep $SLEEP_INTERVAL
			let "RETRY_COUNT=RETRY_COUNT+1"
			let "SLEEP_INTERVAL=SLEEP_INTERVAL*2"
		fi
	done
	
	return $STATUS
}

codesign "$@"
