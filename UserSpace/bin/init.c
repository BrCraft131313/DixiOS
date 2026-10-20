// init.c - DixiOS i386 User-Space Init Process

// دالة نسخ الذاكرة يدوياً لمنع المترجم من طلب مكتبة C القياسية
void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// دالة الطباعة المباشرة باستخدام نداء النظام write (sys_write = 4)
void print_string(const char *str, unsigned int len) {
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(4),      // رقم نداء النظام sys_write
          "b"(1),      // stdout (1)
          "c"(str),    // مؤشر النص
          "d"(len)     // الطول
    );
}

void main();

void _start() {
    main();
}

void main() {
    const char msg[] = "\n====================================\n"
                        " DixiOS i386 Init Started Successfully!\n"
                        "====================================\n\n";
    
    unsigned int len = 0;
    while (msg[len] != '\0') {
        len++;
    }

    print_string(msg, len);

    // حلقة لا نهائية لمنع العملية من الانتهاء
    while (1) {
        __asm__ __volatile__("pause");
    }
}

