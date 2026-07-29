#!/usr/bin/env bash
set -euo pipefail
binary=${1:?Pass the plugin .so path}
file "$binary"
readelf -h "$binary" | grep -E 'Class:|Machine:|Type:'
readelf -Ws "$binary" | awk '$5 == "GLOBAL" && $7 != "UND" { print }'
if readelf -d "$binary" | grep -E 'funchook|capstone'; then
	echo "Unexpected dynamic Funchook or Capstone dependency" >&2
	exit 1
fi
if ! readelf -Ws "$binary" | grep -q 'CreateInterface'; then
	echo "CreateInterface export missing" >&2
	exit 1
fi
sha256sum "$binary"
