#!/bin/bash
# Checks the pinned release hosts (src/network/ReleaseTrust.h) with the SAME wolfSSL
# source and settings as the reader, over the network. openssl is not enough: the
# reader's wolfSSL once lacked SHA-384 and could not verify github.com at all.
#
#   scripts/wolfssl_pin_check/check.sh
#
# Needs a C compiler, make and python3, and a prior `pio run` (it uses the wolfSSL
# library PlatformIO downloaded). Run it before a release and whenever GitHub's
# certificates change. For each host it must pass with the pinned root and FAIL with
# the other root, so a check that verifies nothing cannot pass.
set -u
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

grep -q -- '-DWOLFSSL_SHA384' "$ROOT/platformio.ini" || { echo "platformio.ini lost -DWOLFSSL_SHA384"; exit 1; }
SRC="$(ls -d "$ROOT"/.pio/libdeps/*/Arduino-wolfSSL/src 2>/dev/null | head -1)"
[ -n "$SRC" ] || { echo "wolfSSL source not found: run 'pio run -e x4-pro' once first"; exit 1; }

WORK="$(mktemp -d "${TMPDIR:-/tmp}/neuink-wolfssl-check.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
cp "$HERE/user_settings.h" "$HERE/client.c" "$WORK/"

cat > "$WORK/Makefile" <<EOF
L=$SRC
H=$WORK
SRCS=\$(wildcard \$(L)/src/*.c) \$(wildcard \$(L)/wolfcrypt/src/*.c)
OBJS=\$(patsubst \$(L)/%.c,\$(H)/obj/%.o,\$(SRCS))
CFLAGS=-O1 -w -DWOLFSSL_USER_SETTINGS -DWOLFSSL_OPTIONS_H -DWOLFSSL_CLIENT_EXAMPLE -I\$(H) -I\$(L)
all: \$(H)/libwolf.a
\$(H)/libwolf.a: \$(OBJS)
	ar rcs \$@ \$(OBJS)
\$(H)/obj/%.o: \$(L)/%.c
	@mkdir -p \$(dir \$@)
	cc \$(CFLAGS) -c \$< -o \$@
EOF
echo "building wolfSSL for the host (about a minute)..."
make -s -j8 -C "$WORK" -f Makefile >/dev/null 2>"$WORK/build.err" || { cat "$WORK/build.err"; exit 1; }
cc -O1 -w -DWOLFSSL_USER_SETTINGS -DWOLFSSL_OPTIONS_H -DWOLFSSL_CLIENT_EXAMPLE -I"$WORK" -I"$SRC" \
  "$WORK/client.c" "$WORK/libwolf.a" -lm -o "$WORK/client" || exit 1

python3 -I - "$ROOT/src/network/ReleaseTrust.h" "$WORK" <<'EOF'
import re, sys
text = open(sys.argv[1]).read()
for name, var in (("usertrust", "kUsertrustEccRoot"), ("isrg", "kIsrgRootX1")):
    block = re.search(var + r"\[\] =\n((?:\"[^\n]*\n)+?);", text).group(1)
    lines = [l[:-2] if l.endswith("\\n") else l for l in re.findall(r'"(.*?)"\n', block)]
    open(f"{sys.argv[2]}/{name}.pem", "w").write("\n".join(lines) + "\n")
EOF

status=0
check() {  # host pinned-root other-root
  for m in tls13 tls12 auto; do
    good=$("$WORK/client" "$1" "$WORK/$2.pem" $m); rc_good=$?
    bad=$("$WORK/client" "$1" "$WORK/$3.pem" $m); rc_bad=$?
    if [ $rc_good -eq 0 ] && [ $rc_bad -ne 0 ]; then
      printf "  ok    %-38s %-6s pinned %-9s %s\n" "$1" $m "$2" "${good#OK    }"
    else
      printf "  FAIL  %-38s %-6s pinned %-9s -> %s | wrong root -> %s\n" "$1" $m "$2" "$good" "$bad"
      status=1
    fi
  done
}
check github.com usertrust isrg
check release-assets.githubusercontent.com isrg usertrust
check objects.githubusercontent.com isrg usertrust
[ $status -eq 0 ] && echo "all pinned hosts verify with the reader's wolfSSL settings" || echo "PROBLEM: see FAIL lines above"
exit $status
