#!/bin/bash

dir="/usr/lib"

echo "Finding functions: sin|con|exp"

find "$dir" -type f -name "*.so*" 2>/dev/null | while read -r lib; do

	symbols=$(nm -D --defined-only "$lib" 2>/dev/null)

	if [[ -n "$symbols" ]]; then
		matches=$(echo "$symbols" | grep -E 'T[[:space:]](sin|cos|exp).*(@GLIBC|@@QUADMATH)')

		if [[ -n "$matches" ]]; then
			echo "Library: $lib"
			echo "$matches"
			echo "--------------------------------"
		fi
	fi
done
