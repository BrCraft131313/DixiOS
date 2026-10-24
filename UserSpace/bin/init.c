// init.c - DixiOS i386 User-Space Init Process (Interactive Shell)

void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// دالة الكتابة (sys_write = 4)
void print_string(const char *str, unsigned int len) {
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(4),      // sys_write
          "b"(1),      // stdout
          "c"(str),
          "d"(len)
    );
}

// دالة القراءة لاستقبال إدخال المستخدم (sys_read = 3)
int read_string(char *buf, unsigned int max_len) {
    int bytes_read;
    __asm__ __volatile__ (
        "int $0x80"
        : "=a"(bytes_read)
        : "a"(3),      // sys_read
          "b"(0),      // stdin
          "c"(buf),
          "d"(max_len)
    );
    return bytes_read;
}

unsigned int str_len(const char *str) {
    unsigned int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// مقارنة النصوص البسيطة
int str_compare(const char *s1, const char *s2) {
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) return s1[i] - s2[i];
        i++;
    }
    if ((s1[i] == '\n' || s1[i] == '\r' || s1[i] == '\0') && 
        (s2[i] == '\n' || s2[i] == '\r' || s2[i] == '\0')) return 0;
    return s1[i] - s2[i];
}

// فحص البادئة للتعرف على أمر print
int str_starts_with(const char *str, const char *prefix) {
    int i = 0;
    while (prefix[i] != '\0') {
        if (str[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

void main();

void _start() {
    main();
}

void main() {
    // طبع رسالة الترحيب مرة واحدة فقط عند الإقلاع
    const char banner[] = "\n====================================\n"
                          " DixiOS i386 Init Started Successfully!\n"
                          "====================================\n\n"
                          "Type 'help' to see available commands.\n\n";
    print_string(banner, str_len(banner));

    char buf[64];

    // مصفوفة لتخزين آخر 5 أوامر
    char history[5][64];
    for (int i = 0; i < 5; i++) {
        history[i][0] = '\0';
    }
    int history_count = 0;

    // حلقة تفاعلية (Shell Loop)
    while (1) {
        print_string("DixiOS:/# ", 10);
        
        for (int i = 0; i < 64; i++) {
            buf[i] = 0;
        }
        
        int n = read_string(buf, 63);
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n' || buf[i] == '\r') {
                buf[i] = '\0';
                break;
            }
        }

        if (buf[0] == '\0') {
            continue;
        }

        // إضافة الأمر المدخل إلى الـ History
        if (history_count < 5) {
            int j = 0;
            while (buf[j] != '\0' && j < 63) {
                history[history_count][j] = buf[j];
                j++;
            }
            history[history_count][j] = '\0';
            history_count++;
        } else {
            for (int i = 0; i < 4; i++) {
                int j = 0;
                while (history[i + 1][j] != '\0' && j < 63) {
                    history[i][j] = history[i + 1][j];
                    j++;
                }
                history[i][j] = '\0';
            }
            int j = 0;
            while (buf[j] != '\0' && j < 63) {
                history[4][j] = buf[j];
                j++;
            }
            history[4][j] = '\0';
        }

        if (str_compare(buf, "help") == 0) {
            const char *help_msg = "Commands:\n"
                                   "  help    - Display this help list\n"
                                   "  info    - Show OS info\n"
                                   "  print   - Print text to screen\n"
                                   "  history - Show last 5 commands\n"
                                   "  clear   - Clear terminal screen\n\n";
            print_string(help_msg, str_len(help_msg));
        } 
        else if (str_compare(buf, "info") == 0) {
            const char *info_msg = "OS : DixiOS\n"
                                  "Version : v0.2.2\n"
                                  "Architecture : x86_32\n"
                                  "Author : BrCraft131313\n"
                                  "GitHub : https://github.com/BrCraft131313/DixiOS/\n"
                                  "Release in : October 24\n\n";
            print_string(info_msg, str_len(info_msg));
        }
        else if (str_starts_with(buf, "print ")) {
            const char *text_to_print = buf + 6;
            print_string(text_to_print, str_len(text_to_print));
            print_string("\n", 1);
        }
        else if (str_compare(buf, "print") == 0) {
            print_string("\n", 1);
        }
        else if (str_compare(buf, "history") == 0) {
            for (int i = 0; i < history_count; i++) {
                print_string(history[i], str_len(history[i]));
                print_string("\n", 1);
            }
            print_string("\n", 1);
        }
        else if (str_compare(buf, "clear") == 0) {
            const char *clear_msg = "\033[H\033[J";
            print_string(clear_msg, str_len(clear_msg));
        }
        else {
            const char *err_msg = "Unknown command. Type 'help' for options.\n\n";
            print_string(err_msg, str_len(err_msg));
        }
    }
}
