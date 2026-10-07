#!/bin/bash

OUT_DIR="./out"
TEMP_BUILD_DIR="./.build_tmp"
RELEASE_NAME="DixiOS-v0.1"

# ---------------------------------------------------------
# 1. إعداد شجرة الملفات المؤقتة بناءً على الإضافات والاستثناءات
# ---------------------------------------------------------
prepare_sources() {
    rm -rf "$TEMP_BUILD_DIR"
    mkdir -p "$TEMP_BUILD_DIR"

    DEFAULT_DIRS=("Apps" "UI" "UserSpace" "Kernel" "VirtSpace" "Drivers" "ext")

    for dir in "${DEFAULT_DIRS[@]}"; do
        if [ -d "./$dir" ]; then
            cp -r "./$dir" "$TEMP_BUILD_DIR/"
        fi
    done

    for arg in "$@"; do
        case "$arg" in
            -* )
                REMOVE_PATH="${arg#-}"
                rm -rf "${TEMP_BUILD_DIR}${REMOVE_PATH}"
                echo "[DixiOS Build]: تم استثناء المسار: ${REMOVE_PATH}"
                ;;
            +* )
                ADD_PATH="${arg#+}"
                TARGET_DIR=$(dirname "${TEMP_BUILD_DIR}${ADD_PATH}")
                mkdir -p "$TARGET_DIR"
                if [ -e ".${ADD_PATH}" ]; then
                    cp -r ".${ADD_PATH}" "$TARGET_DIR/"
                    echo "[DixiOS Build]: تم تضمين المسار المخصص: ${ADD_PATH}"
                fi
                ;;
        esac
    done
}

# ---------------------------------------------------------
# 2. دوال إنشاء التوزيعات (.tar.xz / .img / .iso)
# ---------------------------------------------------------
build_tar_xz() {
    echo "[DixiOS Build]: جاري إنشاء حزمة tar.xz..."
    mkdir -p "$OUT_DIR"
    tar -cJvf "${OUT_DIR}/${RELEASE_NAME}.tar.xz" -C "$TEMP_BUILD_DIR" .
    echo "[DixiOS Build]: تم الحفظ في: ${OUT_DIR}/${RELEASE_NAME}.tar.xz"
}

build_img() {
    echo "[DixiOS Build]: جاري إنشاء ملف الصورة .img..."
    mkdir -p "$OUT_DIR"
    
    dd if=/dev/zero of="${OUT_DIR}/${RELEASE_NAME}.img" bs=1M count=512 status=progress
    mkfs.ext4 -F "${OUT_DIR}/${RELEASE_NAME}.img"
    
    MOUNT_POINT="./.mnt_tmp"
    mkdir -p "$MOUNT_POINT"
    sudo mount "${OUT_DIR}/${RELEASE_NAME}.img" "$MOUNT_POINT"
    sudo cp -r "$TEMP_BUILD_DIR"/* "$MOUNT_POINT/"
    sudo umount "$MOUNT_POINT"
    rm -rf "$MOUNT_POINT"
    
    echo "[DixiOS Build]: تم الحفظ في: ${OUT_DIR}/${RELEASE_NAME}.img"
}

build_iso() {
    echo "[DixiOS Build]: جاري إنشاء ملف الـ ISO الإقلاعي..."
    mkdir -p "$OUT_DIR"
    
    if command -v xorriso &> /dev/null; then
        xorriso -as mkisofs -R -J -o "${OUT_DIR}/${RELEASE_NAME}.iso" "$TEMP_BUILD_DIR"
    elif command -v genisoimage &> /dev/null; then
        genisoimage -R -J -o "${OUT_DIR}/${RELEASE_NAME}.iso" "$TEMP_BUILD_DIR"
    else
        echo "[DixiOS Error]: يرجى تثبيت xorriso أو genisoimage لإنشاء الـ ISO"
        return 1
    fi
    
    echo "[DixiOS Build]: تم الحفظ في: ${OUT_DIR}/${RELEASE_NAME}.iso"
}

# ---------------------------------------------------------
# 3. أوامر Makefile الأساسية
# ---------------------------------------------------------
clean() {
    echo "[DixiOS Build]: تنظيف مجلد out والملفات المؤقتة..."
    rm -rf "${OUT_DIR:?}"/* "$TEMP_BUILD_DIR"
    echo "[DixiOS Build]: اكتمل التنظيف."
}

build_bridge() {
    echo "[DixiOS Build]: إعادة بناء DixiBridge.jar..."
    if [ -d "./UserSpace/System/Bridge" ]; then
        cd ./UserSpace/System/Bridge || return 1
        
        # ترجمة ملفات الجافا أولاً في حال عدم وجود ملفات الكلاس
        if ls *.java 1> /dev/null 2>&1; then
            javac *.java 2>/dev/null
        fi

        echo "Main-Class: DixiBridge" > manifest.txt
        jar cfm DixiBridge.jar manifest.txt *.class 2>/dev/null
        
        # تنظيف الكلاسات والمانيفست المؤقتة
        rm -f manifest.txt
        cd - > /dev/null
        echo "[DixiOS Build]: تم بناء الجسر بنجاح."
    else
        echo "[DixiOS Error]: لم يتم العثور على مجلد الجسر."
    fi
}

show_help() {
    echo "DixiOS Build System"
    echo "الاستخدام:"
    echo "  bash build.sh clean                           - تنظيف المخرجات"
    echo "  bash build.sh bridge                          - بناء الجسر فقط"
    echo "  bash build.sh tar.xz [المسارات]                - تصدير أرشيف مضغوط"
    echo "  bash build.sh img [المسارات]                   - تصدير صورة قرص .img"
    echo "  bash build.sh iso [المسارات]                   - تصدير ملف ISO"
    echo "  bash build.sh all [المسارات] [صيغة]            - تنفيذ تنظيف وبناء وتجميع"
}

# ---------------------------------------------------------
# 4. معالج الأوامر الرئيسي
# ---------------------------------------------------------
ACTION="$1"
shift 2>/dev/null

case "$ACTION" in
    clean)
        clean
        ;;
    bridge)
        build_bridge
        ;;
    tar.xz)
        prepare_sources "$@"
        build_tar_xz
        rm -rf "$TEMP_BUILD_DIR"
        ;;
    img)
        prepare_sources "$@"
        build_img
        rm -rf "$TEMP_BUILD_DIR"
        ;;
    iso)
        prepare_sources "$@"
        build_iso
        rm -rf "$TEMP_BUILD_DIR"
        ;;
    all)
        clean
        build_bridge
        
        FORMAT="tar.xz"
        ARGS=()

        for arg in "$@"; do
            if [[ "$arg" == "tar.xz" || "$arg" == "img" || "$arg" == "iso" ]]; then
                FORMAT="$arg"
            else
                ARGS+=("$arg")
            fi
        done
        
        prepare_sources "${ARGS[@]}"
        
        case "$FORMAT" in
            tar.xz) build_tar_xz ;;
            img)    build_img ;;
            iso)    build_iso ;;
        esac
        
        rm -rf "$TEMP_BUILD_DIR"
        echo "[DixiOS Build]: اكتمل البناء الشامل بنجاح!"
        ;;
    *)
        show_help
        ;;
esac
