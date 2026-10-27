#!/bin/bash

echo "========================================="
echo "  Downloading DixiOS (Home Branch)..."
echo "========================================="

# التأكد من وجود أدوات git
if ! command -v git &> /dev/null; then
    echo "Error: git is not installed!"
    exit 1
fi

# حذف المجلد القديم إذا كان موجوداً لتجنب تداخل الملفات
if [ -d "DixiOS-Latest" ]; then
    echo "Removing existing 'DixiOS-Latest' folder..."
    rm -rf DixiOS-Latest
fi

# استنساخ فرع Home وتوجيهه إلى مجلد DixiOS-Latest
git clone --branch Home --single-branch https://github.com/BrCraft131313/DixiOS.git DixiOS-Latest

echo "========================================="
echo " Done! All files are ready in 'DixiOS-Latest'."
echo "========================================="
