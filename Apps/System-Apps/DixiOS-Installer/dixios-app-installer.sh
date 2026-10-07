#!/bin/bash
# DixiOS App Installer Engine (Custom Apps Architecture)

PACKAGE_PATH=$1
TARGET_TYPE=$2 # التوجيه إلى Pre-Installed أو System-Apps

if [ -z "$PACKAGE_PATH" ]; then
    echo "[DixiOS Installer Error]: يرجى تحديد مسار الحزمة!"
    exit 1
fi

# تحديد المسار المستهدف بناءً على تقسيم مجلداتك
if [ "$TARGET_TYPE" == "system" ]; then
    BASE_DIR="/Apps/System-Apps"
else
    BASE_DIR="/Apps/Pre-Installed"
fi

# استخراج امتداد واسم التطبيق
EXTENSION="${PACKAGE_PATH##*.}"
FILENAME=$(basename "$PACKAGE_PATH" ."$EXTENSION")
APP_DIR="$BASE_DIR/$FILENAME"

mkdir -p "$APP_DIR"

case "$EXTENSION" in
    "webappXi")
        echo "[DixiOS Installer]: جاري تثبيت WebApp ($FILENAME) داخل $BASE_DIR..."
        unzip -q "$PACKAGE_PATH" -d "$APP_DIR"
        echo "[DixiOS Installer]: تم تثبيت الـ WebApp بنجاح!"
        ;;

    "appXi")
        echo "[DixiOS Installer]: جاري تثبيت Native App ($FILENAME) داخل $BASE_DIR..."
        unzip -q "$PACKAGE_PATH" -d "$APP_DIR"
        chmod +x "$APP_DIR/"*
        echo "[DixiOS Installer]: تم تثبيت الـ Native App وتفعيل صلاحيات التنفيذ بنجاح!"
        ;;

    *)
        echo "[DixiOS Installer Error]: صيغة غير مدعومة ($EXTENSION)! يرجى التثبيت بصيغة .webappXi أو .appXi"
        rm -rf "$APP_DIR"
        exit 1
        ;;
esac
