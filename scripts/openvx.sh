#!/bin/bash

while [ $# -gt 0 ];
do
    if [ "${1}" == "install" ]; then
        if [ -n "${ANDROID_PRODUCT_OUT}" ]; then
            adb push ${ANDROID_PRODUCT_OUT}/system/lib/libopenvx.so       /system/lib/
            adb push ${ANDROID_PRODUCT_OUT}/system/lib/libvxu.so          /system/lib/
            adb push ${ANDROID_PRODUCT_OUT}/system/lib/libopenvx-c_model.so  /system/lib/
            adb push ${ANDROID_PRODUCT_OUT}/system/lib/libopenvx-debug.so /system/lib/
            adb push ${ANDROID_PRODUCT_OUT}/system/lib/libxyz.so          /system/lib/
            adb push ${ANDROID_PRODUCT_OUT}/system/bin/vx_test            /system/bin/
            adb push ${ANDROID_PRODUCT_OUT}/system/bin/vx_query           /system/bin/
            adb push ${ANDROID_PRODUCT_OUT}/system/bin/vx_conformance     /system/bin/
            adb install -r ${ANDROID_PRODUCT_OUT}/system/app/OpenVXActivity.apk
        else
            echo ANDROID_PRODUCT_OUT not set.
            exit 1
        fi
    fi
    if [ "${1}" == "gdb" ] || [ "${1}" == "ddd" ]; then
        if [ -n "${LD_LIBRARY_PATH}" ]; then
            TYPE="${1}"
            PROCESS="${2}"
            shift
            cat > gdb_cmds.txt << EOF
set solib-absolute-prefix ${LD_LIBRARY_PATH}
set solib-search-path ${LD_LIBRARY_PATH}
EOF
            if [ "${TYPE}" == "gdb" ]; then
                gdb ${PROCESS} --command=gdb_cmds.txt
            elif [ "${TYPE}" == "ddd" ]; then
                ddd --debugger gdb -command=gdb_cmds.txt ${PROCESS} &
            fi
        else
            echo LD_LIBRARY_PATH not set!
            exit 1
        fi
    fi
    shift
done
