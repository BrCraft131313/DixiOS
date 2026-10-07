/**
 * DixiOS System Bridge (UI Side)
 * الجسر البرمجي للربط التام مع DixiBridge.java
 */

const DixiOSBridge = {
    // إرسال أمر تنفيذي مباشر إلى خلفية النظام (Port 8080)
    sendCommand: async function(formattedCommand) {
        console.log(`[DixiOS Bridge Out]: ${formattedCommand}`);

        try {
            const response = await fetch('http://localhost:8080/api/command', {
                method: 'POST',
                headers: { 'Content-Type': 'text/plain; charset=utf-8' },
                body: formattedCommand
            });
            const result = await response.text();
            console.log(`[DixiOS Bridge HTTP Response]: ${result}`);
            return result;
        } catch (error) {
            console.warn(`[DixiOS Bridge Error]: تعذر الاتصال بالجسر للأمر (${formattedCommand}).`);
            return "ERROR: Connection Failed";
        }
    },

    // تشغيل تطبيق حسب اسمه
    launchApp: function(appName) {
        return this.sendCommand(`LAUNCH_APP:${appName}`);
    },

    // تثبيت تطبيق (.webappXi / .appXi)
    installApp: function(packagePath, targetType = "preinstalled") {
        return this.sendCommand(`INSTALL_APP:${packagePath}:${targetType}`);
    },

    // إيقاف التشغيل
    powerOff: function() {
        return this.sendCommand("POWER_OFF");
    },

    // إعادة التشغيل
    reboot: function() {
        return this.sendCommand("REBOOT");
    },

    // الفحص عن التحديثات المتاحة
    checkUpdate: function() {
        return this.sendCommand("CHECK_UPDATE");
    },

    // التحديث الحي
    liveUpdate: function() {
        return this.sendCommand("LIVE_UPDATE");
    }
};

// ربط الدوال العامة للأزرار والواجهة
function systemCommand(action, param1, param2) {
    switch (action) {
        case 'shutdown':
            return DixiOSBridge.powerOff();
        case 'reboot':
            return DixiOSBridge.reboot();
        case 'check-update':
            return DixiOSBridge.checkUpdate();
        case 'live-update':
            return DixiOSBridge.liveUpdate();
        case 'launch':
            return DixiOSBridge.launchApp(param1);
        case 'install':
            return DixiOSBridge.installApp(param1, param2);
        default:
            return DixiOSBridge.sendCommand(action);
    }
}
