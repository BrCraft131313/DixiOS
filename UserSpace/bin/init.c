// init.c - DixiOS i386 User-Space Init Process (v0.2.6)

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

// مصفوفة لتخزين آخر 5 أوامر (History)
char history[5][64];
int history_count = 0;

void str_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void add_to_history(const char *cmd) {
    if (history_count < 5) {
        int j = 0;
        while (cmd[j] != '\0' && j < 63) {
            history[history_count][j] = cmd[j];
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
        while (cmd[j] != '\0' && j < 63) {
            history[4][j] = cmd[j];
            j++;
        }
        history[4][j] = '\0';
    }
}

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
        : "a"(4),
          "b"(1),
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
        : "a"(3),
          "b"(0),
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

int find_file(const char *name) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (fs[i].used && str_compare(fs[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void execute_command(const char *cmd_line);
void process_chained_commands(const char *line);

// تنظيف الحواف والمسافات الزائدة في النصوص
void trim_string(char *str) {
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t' ||
           str[start] == '\r' || str[start] == '\n') {
        start++;
    }

    if (start > 0) {
        int i = 0;
        while (str[start + i] != '\0') {
            str[i] = str[start + i];
            i++;
        }
        str[i] = '\0';
    }

    int len = str_len(str);
    while (len > 0 &&
           (str[len - 1] == ' ' || str[len - 1] == '\t' ||
            str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
}

// دالة تفكيك السلسلة وتفريغ الأوامر المربوطة بالفاصلة (,)
void process_chained_commands(const char *line) {
    char token[128];
    int token_idx = 0;
    int i = 0;

    while (line[i] != '\0') {
        if (line[i] == ',') {
            token[token_idx] = '\0';
            trim_string(token);
            if (token[0] != '\0') {
                execute_command(token);
            }
            token_idx = 0;
        } else {
            if (token_idx < 127) {
                token[token_idx++] = line[i];
            }
        }
        i++;
    }

    if (token_idx > 0) {
        token[token_idx] = '\0';
        trim_string(token);
        if (token[0] != '\0') {
            execute_command(token);
        }
    }
}

// مفسر السكربتات (.dxs) يدعم أوامر متعددة مفصولة بـ (,)
void run_dxs_script(const char *content) {
    int i = 0;
    while (content[i] != '\0') {
        if (content[i] == '[') {
            i++;
            char inner[256];
            int idx = 0;

            while (content[i] != '\0' &&
                   content[i] != ']' && idx < 255) {
                inner[idx++] = content[i];
                i++;
            }

            inner[idx] = '\0';
            if (content[i] == ']') i++;

            process_chained_commands(inner);
        } else {
            i++;
        }
    }
}

// تنفيذ الأوامر الرئيسية
void execute_command(const char *buf) {
    if (buf[0] == '\0') return;

    if (str_compare(buf, "help") == 0) {
        const char *help_msg =
            "DixiOS Commands (v0.2.6):\n"
            "  help                  - Display this help list\n"
            "  info                  - Show OS info\n"
            "  print <text>          - Print text\n"
            "  create <name[/]>      - Create file or folder/\n"
            "  edit <fileName>       - Text editor\n"
            "  run <fileName>        - Execute file or .dxs script\n"
            "  list                  - List files & folders\n"
            "  goto <folderName/>    - Change directory\n"
            "  remove <name[/]>      - Remove file or folder\n"
            "  remove a              - Remove all files & folders\n"
            "  history               - Show last 5 commands\n"
            "  clear                 - Clear screen\n"
            "  Note: You can chain commands using ',' "
            "(e.g. help, print x)\n\n";

        print_string(help_msg, str_len(help_msg));
    }
    else if (str_compare(buf, "info") == 0) {
        const char *info_msg =
            "OS : DixiOS\n"
            "Version : v0.2.6\n"
            "Architecture : x86_32\n"
            "Script Engine : DXS Multi-Command Supported (.dxs)\n"
            "Chaining Support : Enabled (,) separator\n"
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
            print_string(
                "Error: File not found! Create it first with 'create'.\n",
                54
            );
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
        for (int i = 0; i < MAX_FILE_SIZE; i++) {
            edit_buf[i] = 0;
        }

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
        char action[10] = {0};
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
        if (len > 4 &&
            filename[len - 4] == '.' &&
            filename[len - 3] == 'd' &&
            filename[len - 2] == 'x' &&
            filename[len - 1] == 's') {
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

        // remove a يحذف جميع الملفات والمجلدات الافتراضية
        if (str_compare(name, "a") == 0) {
            int removed_count = 0;

            for (int i = 0; i < MAX_FILES; i++) {
                if (fs[i].used) {
                    fs[i].used = 0;
                    fs[i].size = 0;
                    fs[i].content[0] = '\0';
                    fs[i].name[0] = '\0';
                    fs[i].is_folder = 0;
                    removed_count++;
                }
            }

            if (removed_count > 0) {
                print_string("All files and folders removed.\n", 32);
            } else {
                print_string("No files or folders to remove.\n", 31);
            }
        } else {
            int idx = find_file(name);

            if (idx != -1) {
                fs[idx].used = 0;
                fs[idx].size = 0;
                fs[idx].content[0] = '\0';
                fs[idx].name[0] = '\0';
                fs[idx].is_folder = 0;
                print_string("Removed successfully.\n", 22);
            } else {
                print_string("Error: File or folder not found!\n", 33);
            }
        }
    }
    else if (str_compare(buf, "history") == 0) {
        print_string("Command History (Last 5):\n", 26);

        for (int i = 0; i < history_count; i++) {
            print_string("  ", 2);
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
        print_string("Error: Unknown command '", 24);
        print_string(buf, str_len(buf));
        print_string("'\n", 2);
    }
}

void _start() {
    for (int i = 0; i < MAX_FILES; i++) {
        fs[i].used = 0;
    }

    const char banner[] =
        "\n====================================\n"
        " DixiOS i386 Init Started (v0.2.6)\n"
        "====================================\n\n"
        "Type 'help' to see available commands.\n\n";

    print_string(banner, str_len(banner));

    char buf[128];

    while (1) {
        print_string("DixiOS:", 7);
        print_string(current_dir, str_len(current_dir));
        print_string("# ", 2);

        for (int i = 0; i < 128; i++) {
            buf[i] = 0;
        }

        int n = read_string(buf, 127);
        if (n <= 0) continue;

        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n' || buf[i] == '\r') {
                buf[i] = '\0';
                break;
            }
        }

        if (buf[0] != '\0') {
            add_to_history(buf);
            process_chained_commands(buf);
        }
    }
}
