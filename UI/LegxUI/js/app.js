// تحديث الساعة تلقائياً
function updateClock() {
    const now = new Date();
    let hours = now.getHours();
    const minutes = String(now.getMinutes()).padStart(2, '0');
    const ampm = hours >= 12 ? 'PM' : 'AM';
    hours = hours % 12 || 12;
    document.getElementById('time-widget').innerText = `${hours}:${minutes} ${ampm}`;
}
setInterval(updateClock, 1000);
updateClock();

// التحرك يميناً ويساراً بين التطبيقات
function scrollApps(direction) {
    const container = document.getElementById('apps-list');
    const scrollAmount = 100;
    if (direction === 'left') {
        container.scrollLeft += scrollAmount;
    } else {
        container.scrollLeft -= scrollAmount;
    }
}

// فتح التطبيق داخل المساحة البيضاء
function openApp(appId) {
    const viewport = document.getElementById('app-frame');
    console.log(`جارٍ فتح التطبيق: ${appId}`);
    
    // توجيه إطار الـ iframe لمسار التطبيق المطلوبة
    viewport.src = `/Apps/System-Apps/${appId}/index.html`;
}
