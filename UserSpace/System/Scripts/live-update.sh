#!/bin/bash
# DixiOS Live Update Engine - Precise User Migration

echo "[DixiOS]: 1. نقل البيئة الحالية مؤقتاً إلى VirtSpace..."
mkdir -p /VirtSpace/working_env
rsync -a --delete /UserSpace/ /VirtSpace/working_env/

echo "[DixiOS]: 2. تهيئة وتجهيز UserSpace الجديد للتحديث..."
mkdir -p /UserSpace_new/System
mkdir -p /UserSpace_new/User

# تطبيق تحديثات النظام الجديدة داخل /UserSpace_new/System
if [ -d "/tmp/dixios_patch" ]; then
    cp -rf /tmp/dixios_patch/* /UserSpace_new/System/
fi

echo "[DixiOS]: 3. نقل بيانات المستخدم الشخصية من VirtSpace إلى UserSpace الجديد..."
rsync -a /VirtSpace/working_env/User/ /UserSpace_new/User/

echo "[DixiOS]: 4. التبديل اللحظي وتفعيل UserSpace الجديد..."
ln -sfn /UserSpace_new /UserSpace_tmp
mv -Tf /UserSpace_tmp /UserSpace

echo "[DixiOS]: 5. تنظيف VirtSpace بعد اكتمال الانتقال..."
rm -rf /VirtSpace/working_env

echo "[DixiOS]: اكتمل التحديث وانتقل المستخدم إلى UserSpace الجديد بنجاح!"
