#!/bin/bash
# DixiOS System Reboot Script

echo "[DixiOS]: جاري إعادة تشغيل النظام..."
sync
systemctl reboot || reboot
