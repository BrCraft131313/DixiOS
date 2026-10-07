#!/bin/bash
# DixiOS System Shutdown Script

echo "[DixiOS]: جاري الحفظ وإيقاف الخدمات..."
sync # حفظ البيانات المعلقة في الذاكرة إلى القرص
systemctl poweroff || poweroff
