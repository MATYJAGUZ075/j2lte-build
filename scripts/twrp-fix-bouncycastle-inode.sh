#!/sbin/sh
set -eu

SYSTEM_BLOCK="${SYSTEM_BLOCK:-/dev/block/mmcblk0p20}"
SRC=/system_root/system/apex/com.android.art/javalib/bouncycastle.jar
BAK="/data/local/tmp/bouncycastle.jar.bak-$(date +%Y%m%d-%H%M%S)"
TMP="${SRC}.new"

if [ -z "$(getprop ro.twrp.version 2>/dev/null)" ]; then
    echo "ERROR: ejecutar desde TWRP" >&2
    exit 1
fi

if ! grep -q ' /system_root ' /proc/mounts; then
    mount -o ro "$SYSTEM_BLOCK" /system_root
fi

if [ ! -f "$SRC" ]; then
    echo "ERROR: no existe $SRC" >&2
    exit 1
fi

echo "== antes"
md5sum "$SRC"
ORIG=$(md5sum "$SRC" | cut -d' ' -f1)
cp -p "$SRC" "$BAK"
echo "== backup"
md5sum "$BAK"
ls -l "$BAK"

echo "== remount rw"
mount -o remount,rw /system_root
rm -f "$TMP"
cp -p "$BAK" "$TMP"
sync
chown 0:0 "$TMP"
chmod 644 "$TMP"
chcon u:object_r:system_file:s0 "$TMP" 2>/dev/null || restorecon "$TMP" 2>/dev/null || true
NEW=$(md5sum "$TMP" | cut -d' ' -f1)
if [ "$ORIG" != "$NEW" ]; then
    echo "ERROR: md5 distinto ($ORIG != $NEW)" >&2
    rm -f "$TMP"
    exit 1
fi
md5sum "$TMP"
ls -lZ "$TMP"

echo "== reemplazo"
mv -f "$TMP" "$SRC"
sync
md5sum "$SRC"
ls -lZ "$SRC"

echo "== remount ro"
mount -o remount,ro /system_root
sync
echo "OK: JAR reescrito; backup: $BAK"
echo "No rebootees desde este script; reinicia manualmente."
