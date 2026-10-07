#!/bin/bash
# DixiOS OTA Checker & Downloader

SERVER_URL="https://your-server.com/dixios/version.json"
LOCAL_VERSION="0.1"
PATCH_DIR="/tmp/dixios_patch"

echo "[DixiOS OTA]: جاري الفحص عن تحديثات..."

# 1. جلب معلومات الإصدار من السيرفر
LATEST_VERSION=$(curl -s $SERVER_URL | grep -oP '"version": "\K[^"]+')
DOWNLOAD_URL=$(curl -s $SERVER_URL | grep -oP '"url": "\K[^"]+')

if [ -z "$LATEST_VERSION" ]; then
    echo "ERROR_SERVER_UNREACHABLE"
    exit 1
fi

# 2. مقارنة الإصدارات
if [ "$LATEST_VERSION" != "$LOCAL_VERSION" ]; then
    echo "[DixiOS OTA]: تم العثور على إصدار جديد: $LATEST_VERSION"
    
    # 3. تنزيل حزمة التحديث وفكها في /tmp/dixios_patch
    rm -rf $PATCH_DIR && mkdir -p $PATCH_DIR
    curl -s -L "$DOWNLOAD_URL" -o /tmp/update.tar.xz
    tar -xf /tmp/update.tar.xz -C $PATCH_DIR
    rm -f /tmp/update.tar.xz
    
    echo "UPDATE_AVAILABLE:$LATEST_VERSION"
else
    echo "NO_UPDATE"
fi
