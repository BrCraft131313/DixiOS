// init.c - DixiOS i386 User-Space Init Process (v0.2.7 - Part 1)

#define MAX_FILES 16
#define MAX_FILENAME 32
#define MAX_FILE_SIZE 512

#define MAX_ALIASES 16
#define MAX_VARS 16

typedef struct {
    char name[MAX_FILENAME];
    char content[MAX_FILE_SIZE];
    int size;
    int is_folder;
    int used;
} FileEntry;

typedef struct {
    char name[32];
    char command[64];
    int used;
} AliasEntry;

typedef struct {
    char name[32];
    char value[64];
    int used;
} VarEntry;

FileEntry fs[MAX_FILES];
AliasEntry aliases[MAX_ALIASES];
VarEntry vars[MAX_VARS];
char current_dir[MAX_FILENAME] = "/";

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

void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (unsigned long i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* dest, int c, unsigned long n) {
    unsigned char* s = (unsigned char*)dest;
    for (unsigned long i = 0; i < n; i++) {
        s[i] = (unsigned char)c;
    }
    return dest;
}

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

void trim_string(char *str) {
    int start = 0;
    while (str[start] == ' ' || str[start] == '\t' || str[start] == '\r' || str[start] == '\n') {
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
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n')) {
        str[len - 1] = '\0';
        len--;
    }
}

void int_to_str_signed(int val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int neg = 0;
    if (val < 0) {
        neg = 1;
        val = -val;
    }
    char temp[16];
    int i = 0;
    while (val > 0) {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }
    int j = 0;
    if (neg) {
        buf[j++] = '-';
    }
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

void expand_vars(const char *src, char *dest, int max_len) {
    int i = 0, j = 0;
    while (src[i] != '\0' && j < max_len - 1) {
        if (src[i] == '$') {
            i++;
            char vname[32];
            int v_idx = 0;
            while (src[i] != '\0' && src[i] != ' ' && src[i] != '\t' && src[i] != '\n' && src[i] != ',' && v_idx < 31) {
                vname[v_idx++] = src[i++];
            }
            vname[v_idx] = '\0';

            int found = 0;
            for (int v = 0; v < MAX_VARS; v++) {
                if (vars[v].used && str_compare(vars[v].name, vname) == 0) {
                    int k = 0;
                    while (vars[v].value[k] != '\0' && j < max_len - 1) {
                        dest[j++] = vars[v].value[k++];
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {}
        } else {
            dest[j++] = src[i++];
        }
    }
    dest[j] = '\0';
}

int parse_int(const char *str, int *out_val, int *next_idx) {
    int i = 0;
    while (str[i] == ' ' || str[i] == '\t') i++;
    int sign = 1;
    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }
    if (str[i] < '0' || str[i] > '9') return 0;
    int val = 0;
    while (str[i] >= '0' && str[i] <= '9') {
        val = val * 10 + (str[i] - '0');
        i++;
    }
    *out_val = val * sign;
    *next_idx = i;
    return 1;
}

int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

void set_terminal_color(const char *hex_str) {
    if (hex_str[0] == '#') hex_str++;
    int r = hex_val(hex_str[0]) * 16 + hex_val(hex_str[1]);
    int g = hex_val(hex_str[2]) * 16 + hex_val(hex_str[3]);
    int b = hex_val(hex_str[4]) * 16 + hex_val(hex_str[5]);

    char r_s[8], g_s[8], b_s[8];
    int_to_str_signed(r, r_s);
    int_to_str_signed(g, g_s);
    int_to_str_signed(b, b_s);

    char seq[64];
    int idx = 0;
    seq[idx++] = '\033';
    seq[idx++] = '[';
    seq[idx++] = '3';
    seq[idx++] = '8';
    seq[idx++] = ';';
    seq[idx++] = '2';
    seq[idx++] = ';';

    int k = 0;
    while(r_s[k] != '\0') seq[idx++] = r_s[k++];
    seq[idx++] = ';';
    k = 0;
    while(g_s[k] != '\0') seq[idx++] = g_s[k++];
    seq[idx++] = ';';
    k = 0;
    while(b_s[k] != '\0') seq[idx++] = b_s[k++];
    seq[idx++] = 'm';
    seq[idx] = '\0';

    print_string(seq, str_len(seq));
}

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

void run_dxs_script(const char *content) {
    int i = 0;
    while (content[i] != '\0') {
        if (content[i] == '[') {
            i++;
            char inner[256];
            int idx = 0;
            while (content[i] != '\0' && content[i] != ']' && idx < 255) {
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
// init.c - DixiOS i386 User-Space Init Process (v0.2.7 - Part 2)

void execute_command(const char *cmd_line) {
    char expanded_buf[128];
    expand_vars(cmd_line, expanded_buf, 128);
    trim_string(expanded_buf);

    if (expanded_buf[0] == '\0') return;

    for (int a = 0; a < MAX_ALIASES; a++) {
        if (aliases[a].used && str_compare(expanded_buf, aliases[a].name) == 0) {
            execute_command(aliases[a].command);
            return;
        }
    }

    if (str_compare(expanded_buf, "help") == 0) {
        const char *help_msg = 
            "DixiOS Commands (v0.2.7):\n"
            "  help [cmd]            - Display help or specific command guide\n"
            "  info                  - Show OS info\n"
            "  print <text>          - Print text (supports $var)\n"
            "  create <name[/]>      - Create file or folder/\n"
            "  edit <fileName>       - Text editor\n"
            "  run <fileName>        - Execute file or .dxs script\n"
            "  list                  - List files & folders\n"
            "  goto <folderName/>    - Change directory\n"
            "  remove <name[/]>      - Remove file or folder\n"
            "  remove a              - Remove all files & folders\n"
            "  alias <name> = <cmd>  - Create a custom shortcut\n"
            "  color <HEX>           - Change terminal color (e.g. #0397cd)\n"
            "  calc <n1> <op> <n2>   - Arithmetic calculator (+, -, *, /, %%)\n"
            "  set <var> = <val>     - Define a variable\n"
            "  history               - Show last 5 commands\n"
            "  clear                 - Clear screen\n\n";

        print_string(help_msg, str_len(help_msg));
    }
    else if (str_starts_with(expanded_buf, "help ")) {
        char target_cmd[32];
        int tc_idx = 0;
        const char *p = expanded_buf + 5;
        while (*p != '\0' && tc_idx < 31) {
            target_cmd[tc_idx++] = *p++;
        }
        target_cmd[tc_idx] = '\0';
        trim_string(target_cmd);

        if (str_compare(target_cmd, "help") == 0) {
            print_string("Help: Displays available commands or detailed info about a specific command.\nUsage: help [command]\n\n", 94);
        } else if (str_compare(target_cmd, "info") == 0) {
            print_string("Info: Shows system information, OS version, architecture, and author.\n\n", 74);
        } else if (str_compare(target_cmd, "print") == 0) {
            print_string("Print: Outputs the specified text to the terminal. Supports variables like $x.\nUsage: print <text>\n\n", 94);
        } else if (str_compare(target_cmd, "create") == 0) {
            print_string("Create: Creates a new file or folder (if name ends with '/').\nUsage: create <name[/]>\n\n", 89);
        } else if (str_compare(target_cmd, "edit") == 0) {
            print_string("Edit: Opens the text editor to write or modify file content.\nUsage: edit <fileName>\n\n", 84);
        } else if (str_compare(target_cmd, "run") == 0) {
            print_string("Run: Executes file content or a .dxs multi-command script block.\nUsage: run <fileName>\n\n", 86);
        } else if (str_compare(target_cmd, "list") == 0) {
            print_string("List: Lists all created files and directories in the virtual file system.\n\n", 78);
        } else if (str_compare(target_cmd, "goto") == 0) {
            print_string("Goto: Changes the current working directory.\nUsage: goto <folderName/>\n\n", 75);
        } else if (str_compare(target_cmd, "remove") == 0) {
            print_string("Remove: Deletes a specific file/folder or all files ('remove a').\nUsage: remove <name[/]> | remove a\n\n", 100);
        } else if (str_compare(target_cmd, "alias") == 0) {
            print_string("Alias: Creates a custom shortcut for any command.\nUsage: alias <name> = <command>\nExample: alias h = help\n\n", 109);
        } else if (str_compare(target_cmd, "color") == 0) {
            print_string("Color: Changes terminal text color using HEX code.\nUsage: color #RRGGBB\nExample: color #0397cd\n\n", 97);
        } else if (str_compare(target_cmd, "calc") == 0) {
            print_string("Calc: Performs basic arithmetic calculations (+, -, *, /, %%).\nUsage: calc <num> <op> <num>\nExample: calc 2 + 2\n\n", 104);
        } else if (str_compare(target_cmd, "set") == 0) {
            print_string("Set: Defines a variable for script and command use.\nUsage: set <var> = <value>\nUsage in print: print $var\n\n", 98);
        } else {
            print_string("Error: No detailed help available for '", 39);
            print_string(target_cmd, str_len(target_cmd));
            print_string("'.\n\n", 4);
        }
    }
    else if (str_compare(expanded_buf, "info") == 0) {
        const char *info_msg = 
            "OS : DixiOS\n"
            "Version : v0.2.7\n"
            "Architecture : x86_32\n"
            "Script Engine : DXS Multi-Command Supported (.dxs)\n"
            "Features : Aliases, Calculator, Variables, Custom Colors\n"
            "Author : BrCraft131313\n\n";

        print_string(info_msg, str_len(info_msg));
    }
    else if (str_starts_with(expanded_buf, "print ")) {
        const char *text_to_print = expanded_buf + 6;
        print_string(text_to_print, str_len(text_to_print));
        print_string("\n", 1);
    }
    else if (str_compare(expanded_buf, "print") == 0) {
        print_string("\n", 1);
    }
    else if (str_starts_with(expanded_buf, "alias ")) {
        char *p = (char*)(expanded_buf + 6);
        char alias_name[32];
        char alias_cmd[64];
        int an_idx = 0;
        while (*p != '\0' && *p != '=' && an_idx < 31) {
            alias_name[an_idx++] = *p++;
        }
        alias_name[an_idx] = '\0';
        trim_string(alias_name);

        if (*p == '=') {
            p++;
            int ac_idx = 0;
            while (*p != '\0' && ac_idx < 63) {
                alias_cmd[ac_idx++] = *p++;
            }
            alias_cmd[ac_idx] = '\0';
            trim_string(alias_cmd);

            if (alias_name[0] != '\0' && alias_cmd[0] != '\0') {
                int slot = -1;
                for (int i = 0; i < MAX_ALIASES; i++) {
                    if (!aliases[i].used || str_compare(aliases[i].name, alias_name) == 0) {
                        slot = i;
                        break;
                    }
                }
                if (slot != -1) {
                    aliases[slot].used = 1;
                    str_copy(aliases[slot].name, alias_name);
                    str_copy(aliases[slot].command, alias_cmd);
                    print_string("Alias created successfully.\n", 28);
                } else {
                    print_string("Error: Aliases memory full!\n", 29);
                }
            } else {
                print_string("Error: Invalid alias syntax. Use: alias name = command\n", 58);
            }
        } else {
            print_string("Error: Missing '=' in alias definition. Use: alias name = command\n", 67);
        }
    }
    else if (str_starts_with(expanded_buf, "set ")) {
        char *p = (char*)(expanded_buf + 4);
        char var_name[32];
        char var_val[64];
        int vn_idx = 0;
        while (*p != '\0' && *p != '=' && vn_idx < 31) {
            var_name[vn_idx++] = *p++;
        }
        var_name[vn_idx] = '\0';
        trim_string(var_name);

        if (*p == '=') {
            p++;
            int vv_idx = 0;
            while (*p != '\0' && vv_idx < 63) {
                var_val[vv_idx++] = *p++;
            }
            var_val[vv_idx] = '\0';
            trim_string(var_val);

            if (var_name[0] != '\0') {
                int slot = -1;
                for (int i = 0; i < MAX_VARS; i++) {
                    if (!vars[i].used || str_compare(vars[i].name, var_name) == 0) {
                        slot = i;
                        break;
                    }
                }
                if (slot != -1) {
                    vars[slot].used = 1;
                    str_copy(vars[slot].name, var_name);
                    str_copy(vars[slot].value, var_val);
                    print_string("Variable set successfully.\n", 29);
                } else {
                    print_string("Error: Variables memory full!\n", 30);
                }
            } else {
                print_string("Error: Invalid variable syntax. Use: set var = value\n", 58);
            }
        } else {
            print_string("Error: Missing '=' in set definition. Use: set var = value\n", 66);
        }
    }
    else if (str_starts_with(expanded_buf, "color ")) {
        const char *hex_code = expanded_buf + 6;
        set_terminal_color(hex_code);
        print_string("Color updated.\n", 15);
    }
    else if (str_starts_with(expanded_buf, "calc ")) {
        const char *p = expanded_buf + 5;
        int num1, idx1;
        if (!parse_int(p, &num1, &idx1)) {
            print_string("Error: Invalid calculator syntax. Use: calc <num> <op> <num>\n", 61);
            return;
        }
        p += idx1;
        while (*p == ' ' || *p == '\t') p++;
        char op = *p;
        p++;
        while (*p == ' ' || *p == '\t') p++;
        int num2, idx2;
        if (!parse_int(p, &num2, &idx2)) {
            print_string("Error: Invalid second number.\n", 30);
            return;
        }

        int res = 0;
        int error = 0;
        if (op == '+') res = num1 + num2;
        else if (op == '-') res = num1 - num2;
        else if (op == '*') res = num1 * num2;
        else if (op == '/') {
            if (num2 == 0) {
                print_string("Error: Division by zero!\n", 25);
                error = 1;
            } else {
                res = num1 / num2;
            }
        }
        else if (op == '%') {
            if (num2 == 0) {
                print_string("Error: Modulo by zero!\n", 23);
                error = 1;
            } else {
                res = num1 % num2;
            }
        } else {
            print_string("Error: Unknown operator. Use +, -, *, /, %\n", 45);
            error = 1;
        }

        if (!error) {
            char res_str[32];
            int_to_str_signed(res, res_str);
            print_string("Result: ", 8);
            print_string(res_str, str_len(res_str));
            print_string("\n", 1);
        }
    }
    else if (str_starts_with(expanded_buf, "create ")) {
        const char *name = expanded_buf + 7;
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
    else if (str_starts_with(expanded_buf, "edit ")) {
        const char *filename = expanded_buf + 5;
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
    else if (str_starts_with(expanded_buf, "run ")) {
        const char *filename = expanded_buf + 4;
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
    else if (str_compare(expanded_buf, "list") == 0) {
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
    else if (str_starts_with(expanded_buf, "goto ")) {
        const char *foldername = expanded_buf + 5;
        int idx = find_file(foldername);

        if (idx != -1 && fs[idx].is_folder) {
            str_copy(current_dir, foldername);
            print_string("Moved to directory.\n", 20);
        } else {
            print_string("Error: Directory not found!\n", 28);
        }
    }
    else if (str_starts_with(expanded_buf, "remove ")) {
        const char *name = expanded_buf + 7;

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
    else if (str_compare(expanded_buf, "history") == 0) {
        print_string("Command History (Last 5):\n", 26);

        for (int i = 0; i < history_count; i++) {
            print_string("  ", 2);
            print_string(history[i], str_len(history[i]));
            print_string("\n", 1);
        }

        print_string("\n", 1);
    }
    else if (str_compare(expanded_buf, "clear") == 0) {
        const char *clear_msg = "\033[H\033[J";
        print_string(clear_msg, str_len(clear_msg));
    }
    else {
        print_string("Error: Unknown command '", 24);
        print_string(expanded_buf, str_len(expanded_buf));
        print_string("'\n", 2);
    }
}

void _start() {
    for (int i = 0; i < MAX_FILES; i++) {
        fs[i].used = 0;
    }
    for (int i = 0; i < MAX_ALIASES; i++) {
        aliases[i].used = 0;
    }
    for (int i = 0; i < MAX_VARS; i++) {
        vars[i].used = 0;
    }

    const char banner[] = 
        "\n====================================\n"
        " DixiOS i386 Init Started (v0.2.7)\n"
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
