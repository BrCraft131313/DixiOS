// init.c - DixiOS i386 User-Space Init Process (v0.2.3)

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 512

typedef struct {
    char name[MAX_FILENAME];
    char content[MAX_FILE_SIZE];
    int size;
    int is_folder;
    int used;
} FileEntry;

FileEntry fs[MAX_FILES];
char current_dir[MAX_FILENAME] = "/";

// دالة memcpy عادية لتجنب أخطاء الـ Linker
void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// دالة memset عادية لتجنب أخطاء تصفير البيانات
void* memset(void* dest, int c, unsigned long n) {
    unsigned char* s = (unsigned char*)dest;
    for (unsigned long i = 0; i < n; i++) {
        s[i] = (unsigned char)c;
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

int str_starts_with(const char *str, const char *prefix) {
    int i = 0;
    while (prefix[i] != '\0') {
        if (str[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

void str_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs[i].used && str_compare(fs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void execute_command(const char *cmd_line);

// مفسر السكربتات (.dxs) بالساينتاكس الجديد [name: ..., cmd]
void run_dxs_script(const char *content) {
    int i = 0;
    while (content[i] != '\0') {
        if (content[i] == '[') {
            i++;
            char inner[128];
            int idx = 0;
            while (content[i] != '\0' && content[i] != ']' && idx < 127) {
                inner[idx++] = content[i];
                i++;
            }
            inner[idx] = '\0';
            if (content[i] == ']') i++;

            // البحث عن الفاصل (,) لفصل الـ name عن الـ command
            char *cmd_ptr = 0;
            for (int k = 0; inner[k] != '\0'; k++) {
                if (inner[k] == ',') {
                    inner[k] = '\0';
                    cmd_ptr = &inner[k + 1];
                    break;
                }
            }

            if (cmd_ptr != 0) {
                // إزالة المسافات الزائدة من بداية الأمر
                while (*cmd_ptr == ' ' || *cmd_ptr == '\t' || *cmd_ptr == '\r' || *cmd_ptr == '\n') {
                    cmd_ptr++;
                }
                // إزالة المسافات والرموز الزائدة من نهاية الأمر
                int len = str_len(cmd_ptr);
                while (len > 0 && (cmd_ptr[len - 1] == ' ' || cmd_ptr[len - 1] == '\t' || cmd_ptr[len - 1] == '\r' || cmd_ptr[len - 1] == '\n')) {
                    cmd_ptr[len - 1] = '\0';
                    len--;
                }
                if (cmd_ptr[0] != '\0') {
                    execute_command(cmd_ptr);
                }
            }
        } else {
            i++;
        }
    }
}

// تنفيذ الأوامر الرئيسية
void execute_command(const char *buf) {
    if (buf[0] == '\0') return;

    if (str_compare(buf, "help") == 0) {
        const char *help_msg = "DixiOS Commands:\n"
                               "  help                  - Display this help list\n"
                               "  info                  - Show OS info\n"
                               "  print <text>          - Print text\n"
                               "  create <name[/]>      - Create file or folder/\n"
                               "  edit <fileName>       - Text editor\n"
                               "  run <fileName>        - Execute file or .dxs script\n"
                               "  list                  - List files & folders\n"
                               "  goto <folderName/>    - Change directory\n"
                               "  remove <name[/]>      - Remove file or folder\n"
                               "  clear                 - Clear screen\n\n";
        print_string(help_msg, str_len(help_msg));
    } 
    else if (str_compare(buf, "info") == 0) {
        const char *info_msg = "OS : DixiOS\n"
                              "Version : v0.2.3\n"
                              "Architecture : x86_32\n"
                              "Script Engine : DXS Supported (.dxs)\n"
                              "Author : BrCraft131313\n\n";
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
    else if (str_starts_with(buf, "create ")) {
        const char *name = buf + 7;
        int is_dir = 0;
        int len = str_len(name);

        if (len > 0 && name[len - 1] == '/') {
            is_dir = 1;
        }

        int slot = -1;
        for (int i = 0; i < MAX_FILES; i++) {
            if (!fs[i].used) {
                slot = i;
                break;
            }
        }

        if (slot != -1) {
            fs[slot].used = 1;
            fs[slot].is_folder = is_dir;
            fs[slot].size = 0;
            fs[slot].content[0] = '\0';
            str_copy(fs[slot].name, name);
            if (is_dir) {
                print_string("Folder created successfully.\n", 29);
            } else {
                print_string("File created successfully.\n", 27);
            }
        } else {
            print_string("Error: Memory Full!\n", 20);
        }
    }
    else if (str_starts_with(buf, "edit ")) {
        const char *filename = buf + 5;
        int idx = find_file(filename);

        if (idx == -1) {
            print_string("Error: File not found! Create it first with 'create'.\n", 54);
            return;
        }

        if (fs[idx].is_folder) {
            print_string("Error: Cannot edit a directory!\n", 32);
            return;
        }

        print_string("--- DixiOS Text Editor ---\n", 27);
        print_string("Options: [S] Save | [C] Cancel | [E] Exit\n", 43);
        print_string("Enter file content below:\n", 26);

        char edit_buf[MAX_FILE_SIZE];
        for (int i = 0; i < MAX_FILE_SIZE; i++) edit_buf[i] = 0;

        int bytes = read_string(edit_buf, MAX_FILE_SIZE - 1);
        if (bytes > 0) {
            for (int i = 0; i < bytes; i++) {
                if (edit_buf[i] == '\n' || edit_buf[i] == '\r') {
                    edit_buf[i] = '\0';
                    break;
                }
            }
        }

        print_string("Action (S/C/E): ", 16);
        char action[10];
        read_string(action, 9);

        if (action[0] == 'S' || action[0] == 's') {
            str_copy(fs[idx].content, edit_buf);
            fs[idx].size = str_len(edit_buf);
            print_string("File saved successfully.\n", 25);
        } else if (action[0] == 'C' || action[0] == 'c') {
            print_string("Edit cancelled.\n", 16);
        } else if (action[0] == 'E' || action[0] == 'e') {
            print_string("Exited editor.\n", 15);
        }
    }
    else if (str_starts_with(buf, "run ")) {
        const char *filename = buf + 4;
        int idx = find_file(filename);

        if (idx == -1) {
            print_string("Error: File not found!\n", 23);
            return;
        }

        int len = str_len(filename);
        if (len > 4 && filename[len - 4] == '.' && filename[len - 3] == 'd' && filename[len - 2] == 'x' && filename[len - 1] == 's') {
            run_dxs_script(fs[idx].content);
        } else {
            print_string("Executing file content:\n", 24);
            print_string(fs[idx].content, str_len(fs[idx].content));
            print_string("\n", 1);
        }
    }
    else if (str_compare(buf, "list") == 0) {
        print_string("Directory Listing:\n", 19);
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs[i].used) {
                print_string("  ", 2);
                print_string(fs[i].name, str_len(fs[i].name));
                if (fs[i].is_folder) {
                    print_string(" <DIR>", 6);
                }
                print_string("\n", 1);
            }
        }
    }
    else if (str_starts_with(buf, "goto ")) {
        const char *foldername = buf + 5;
        int idx = find_file(foldername);

        if (idx != -1 && fs[idx].is_folder) {
            str_copy(current_dir, foldername);
            print_string("Moved to directory.\n", 20);
        } else {
            print_string("Error: Directory not found!\n", 28);
        }
    }
    else if (str_starts_with(buf, "remove ")) {
        const char *name = buf + 7;
        int idx = find_file(name);

        if (idx != -1) {
            fs[idx].used = 0;
            print_string("Removed successfully.\n", 22);
        } else {
            print_string("Error: File or folder not found!\n", 33);
        }
    }
    else if (str_compare(buf, "clear") == 0) {
        const char *clear_msg = "\033[H\033[J";
        print_string(clear_msg, str_len(clear_msg));
    }
    else {
        print_string("Error: Unknown command '", 24);
        print_string(buf, str_len(buf));
        print_string("'\n", 2);
    }
}

void _start() {
    for (int i = 0; i < MAX_FILES; i++) {
        fs[i].used = 0;
    }

    const char banner[] = "\n====================================\n"
                          " DixiOS i386 Init Started (v0.2.3)\n"
                          "====================================\n\n"
                          "Type 'help' to see available commands.\n\n";
    print_string(banner, str_len(banner));

    char buf[128];

    while (1) {
        print_string("DixiOS:", 7);
        print_string(current_dir, str_len(current_dir));
        print_string("# ", 2);
        
        for (int i = 0; i < 128; i++) buf[i] = 0;
        
        int n = read_string(buf, 127);
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n' || buf[i] == '\r') {
                buf[i] = '\0';
                break;
            }
        }

        execute_command(buf);
    }
}
