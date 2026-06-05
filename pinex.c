#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <ctype.h>

// Fixed: Renamed to avoid collision with standard Termux/Linux header macro names
#define SOS_MAX_INPUT 512
#define SHELL_MAX_INPUT 512
#define MAX_ARGS 64
#define HISTORY_SIZE 100
#define WIDTH 20
#define HEIGHT 10

char history[HISTORY_SIZE][SHELL_MAX_INPUT];
int historyCount = 0;

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Fixed: Signal Handler to capture Ctrl+C (SIGINT) so it doesn't crash the shell
void handle_sigint(int sig) {
    printf("\n\033[93m[!] Operation interrupted. Returning to S OS prompt...\033[0m\n");
    // This allows the terminal loop to gracefully repaint the command prompt
}

void getCleanInput(char *buffer, size_t size) {
    if (fgets(buffer, size, stdin) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
    }
}

int isSafePath(const char *path) {
    if (strstr(path, "..") != NULL) {
        return 0; 
    }
    for (size_t i = 0; i < strlen(path); i++) {
        char c = path[i];
        if (!(isalnum(c) || c == '/' || c == '.' || c == '_' || c == '-')) {
            return 0;
        }
    }
    return 1;
}

int commandExists(const char *cmd) {
    char *env_path = getenv("PATH");
    if (!env_path) {
        env_path = "/data/data/com.termux/files/usr/bin:/bin:/usr/bin";
    }
    
    char *path_copy = strdup(env_path);
    char *dir = strtok(path_copy, ":");
    char full_path[1024];

    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return 1;
        }
        dir = strtok(NULL, ":");
    }

    free(path_copy);
    return 0;
}

void executeSafe(char *const args[]) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process restores standard interaction behavior
        signal(SIGINT, SIG_DFL);
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    } else if (pid > 0) {
        wait(NULL);
    } else {
        perror("fork failed");
    }
}

void executeCommand(char *args[]) {
    executeSafe(args);
}

void addHistory(char cmd[]) {
    if(historyCount < HISTORY_SIZE) {
        strcpy(history[historyCount], cmd);
        historyCount++;
    }
}

void showHistory() {
    printf("\n=== COMMAND HISTORY ===\n");
    for(int i = 0; i < historyCount; i++) {
        printf("%d. %s\n", i + 1, history[i]);
    }
}

void bootAnimation() {
    const char *logs[] = {
        "[ OK ] Starting S OS Kernel",
        "[ OK ] Loading Drivers",
        "[ OK ] Initializing Memory",
        "[ OK ] Mounting Storage",
        "[ OK ] Starting Package Manager",
        "[ OK ] Loading Games",
        "[ OK ] Launching Shell"
    };

    printf("\033[92m");
    printf("\n#################################\n");
    printf("#           S OS SHELL          #\n");
    printf("#################################\n\n");

    for(int i = 0; i < 7; i++) {
        printf("%s\n", logs[i]);
        fflush(stdout);
        usleep(150000);
    }
    printf("\033[0m\n");
}

void setColor(char code[]) {
    if (strcmp(code, "a") == 0) printf("\033[92m");
    else if (strcmp(code, "c") == 0) printf("\033[91m");
    else if (strcmp(code, "e") == 0) printf("\033[93m");
    else if (strcmp(code, "f") == 0) printf("\033[97m");
    else printf("Unknown color\n");
}

void calculator() {
    char op_str[10], count_str[32], num_str[64];
    char op;
    int n, i;
    double num, ans;
    char *endptr;

    printf("Operator (+ - * /): ");
    getCleanInput(op_str, sizeof(op_str));
    op = op_str[0];

    printf("How many numbers: ");
    getCleanInput(count_str, sizeof(count_str));
    n = strtol(count_str, &endptr, 10);

    if (n <= 0 || *endptr != '\0') {
        printf("Invalid choice count.\n");
        return;
    }

    printf("Enter number 1: ");
    getCleanInput(num_str, sizeof(num_str));
    ans = strtod(num_str, &endptr);
    if (*endptr != '\0') { printf("Invalid number token.\n"); return; }

    for (i = 2; i <= n; i++) {
        printf("Enter number %d: ", i);
        getCleanInput(num_str, sizeof(num_str));
        num = strtod(num_str, &endptr);
        if (*endptr != '\0') { printf("Invalid token. Calculation aborted.\n"); return; }

        if (op == '+') ans += num;
        else if (op == '-') ans -= num;
        else if (op == '*') ans *= num;
        else if (op == '/') {
            if (num == 0) {
                printf("Division by zero blocked.\n");
                return;
            }
            ans /= num;
        }
    }
    printf("Answer = %.2lf\n", ans);
}

void deviceInfo() {
    printf("\033[96m\n=== DEVICE INFO ===\n");

    char *unameArgs[] = {"uname", "-r", NULL};
    executeSafe(unameArgs);

    printf("\nCPU Info:\n");
    char *cpuArgs[] = {"sh", "-c", "cat /proc/cpuinfo | head -15", NULL};
    executeSafe(cpuArgs);

    printf("\nRAM Info:\n");
    char *ramArgs[] = {"sh", "-c", "cat /proc/meminfo | head -5", NULL};
    executeSafe(ramArgs);

    printf("\nStorage Info:\n");
    char *dfArgs[] = {"df", "-h", "/", NULL};
    executeSafe(dfArgs);

    printf("\033[0m");
}

void duckDuckGo() {
    char choice[10];
    char query[150];

    printf("\n1. Open Homepage\n2. Search\nChoose: ");
    getCleanInput(choice, sizeof(choice));

    if (choice[0] == '1') {
        char *args[] = {"am", "start", "-a", "android.intent.action.VIEW", "-d", "https://duckduckgo.com", NULL};
        executeSafe(args);
    } else if (choice[0] == '2') {
        printf("Search query: ");
        getCleanInput(query, sizeof(query));

        for (int i = 0; query[i]; i++) {
            if (query[i] == ' ') query[i] = '+';
        }

        char url[256];
        snprintf(url, sizeof(url), "https://duckduckgo.com/?q=%s", query);

        char *args[] = {"am", "start", "-a", "android.intent.action.VIEW", "-d", url, NULL};
        executeSafe(args);
    }
}

void systemControl(char target[], char action[]) {
    if (strcmp(target, "wifi") == 0) {
        printf("Opening WiFi settings...\n");
        char *args[] = { "am", "start", "-n", "com.android.settings/.wifi.WifiSettings", NULL };
        executeSafe(args);
    }
    else if (strcmp(target, "bluetooth") == 0) {
        printf("Opening Bluetooth settings...\n");
        char *args[] = { "am", "start", "-a", "android.settings.BLUETOOTH_SETTINGS", NULL };
        executeSafe(args);
    }
}

void flashKit() {
    char mode[10];
    char company[10];
    char rom_path[150];

    printf("\n=====================================\n");
    printf("      S OS SAFE FLASH KIT v2.0\n");
    printf("=====================================\n");
    printf("1. Unlock Bootloader\n");
    printf("2. Flash Recovery / ROM\n");
    printf("Choose: ");

    getCleanInput(mode, sizeof(mode));

    if (mode[0] == '1') {
        printf("\nChecking fastboot devices...\n");
        char *devArgs[] = {"fastboot", "devices", NULL};
        executeSafe(devArgs);

        printf("\n1. Pixel/Moto/OnePlus\n");
        printf("2. Xiaomi/Redmi/POCO\n");
        printf("3. Samsung\n");
        printf("Choose brand: ");

        getCleanInput(company, sizeof(company));

        printf("\nWARNING: Unlocking wipes data.\n");
        printf("Continue? (yes/no): ");

        char confirm[10];
        getCleanInput(confirm, sizeof(confirm));

        if (strncmp(confirm, "yes", 3) != 0) {
            printf("Cancelled.\n");
            return;
        }

        if (company[0] == '1') {
            char *unlockArgs[] = { "fastboot", "flashing", "unlock", NULL };
            executeSafe(unlockArgs);
        }
        else if (company[0] == '2') {
            printf("Xiaomi path selected via prompt menu.\n");
        }
        else if (company[0] == '3') {
            printf("Samsung requires Download Mode manipulation.\n");
        }
    }
    else if (mode[0] == '2') {
        printf("\n1. Flash Recovery\n");
        printf("2. Run flash-all script\n");
        printf("Choose: ");

        char choice[10];
        getCleanInput(choice, sizeof(choice));

        if (choice[0] == '1') {
            printf("Enter image path: ");
            getCleanInput(rom_path, sizeof(rom_path));

            if (!isSafePath(rom_path)) {
                printf("Unsafe layout structure blocked.\n");
                return;
            }

            printf("Confirm flash? (yes/no): ");
            char confirm[10];
            getCleanInput(confirm, sizeof(confirm));

            if (strncmp(confirm, "yes", 3) != 0) {
                printf("Cancelled.\n");
                return;
            }

            char *flashArgs[] = { "fastboot", "flash", "recovery", rom_path, NULL };
            executeSafe(flashArgs);

            char *rebootArgs[] = { "fastboot", "reboot-recovery", NULL };
            executeSafe(rebootArgs);
        }
        else if (choice[0] == '2') {
            if (access("flash-all.sh", F_OK) == 0) {
                char *args[] = {"sh", "flash-all.sh", NULL};
                executeSafe(args);
            } else {
                printf("flash-all.sh script file missing.\n");
            }
        }
    }
}

void runCommand(char input[]) {
    char *args[MAX_ARGS];
    int i = 0;

    args[i] = strtok(input, " \t\r\n\a");

    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " \t\r\n\a");
    }

    if (args[0] == NULL)
        return;

    executeSafe(args);
}

void runExternal(char input[]) {
    runCommand(input);
}

void runSudo(char input[]) {
    char command[SHELL_MAX_INPUT];
    char *cmd_payload = input + 5;

    if (!isSafePath(cmd_payload)) {
        printf("Security restriction active.\n");
        return;
    }

    snprintf(command, sizeof(command), "tsu -c \"%s\"", cmd_payload);
    system(command);
}

void sosPackageManager(char input[]) {
    char action[50];
    char package[100];

    if (sscanf(input, "sos %49s %99s", action, package) < 2 && strcmp(action, "update") != 0) {
        printf("Usage: sos install/remove/search <package> or sos update\n");
        return;
    }

    if (!isSafePath(package) && strcmp(action, "update") != 0) {
        printf("Nomenclature context validation failed.\n");
        return;
    }

    if(strcmp(action, "install") == 0) {
        char *args[] = { "pkg", "install", package, "-y", NULL };
        executeCommand(args);
    }
    else if(strcmp(action, "remove") == 0) {
        char *args[] = { "pkg", "uninstall", package, "-y", NULL };
        executeCommand(args);
    }
    else if(strcmp(action, "search") == 0) {
        char *args[] = { "pkg", "search", package, NULL };
        executeCommand(args);
    }
    else if(strcmp(action, "update") == 0) {
        char *args[] = { "pkg", "update", "-y", NULL };
        executeCommand(args);
    }
}

void smartOpen(char input[]) {
    char app[100];
    char command[512];
    char package[256];

    if (sscanf(input, "open %99[^\n]", app) < 1) return;

    if (!isSafePath(app)) {
        printf("Alphanumeric restrictions apply.\n");
        return;
    }

    printf("\nSearching for %s...\n", app);
    snprintf(command, sizeof(command),
        "pm list packages | grep -i '%s' > /data/data/com.termux/files/home/.sospkg", app);
    
    if (system(command) != 0) {
        printf("Resolution matching module failed.\n");
        return;
    }

    FILE *fp = fopen("/data/data/com.termux/files/home/.sospkg", "r");
    if(fp == NULL) {
        return;
    }

    if(fgets(package, sizeof(package), fp) == NULL) {
        printf("App descriptor trace map empty.\n");
        fclose(fp);
        return;
    }
    fclose(fp);

    char *pkg = strstr(package, ":");
    if(pkg == NULL) return;
    pkg++;
    pkg[strcspn(pkg, "\n")] = 0;

    printf("Opening target link identity: %s\n", pkg);
    char *args[] = {"monkey", "-p", pkg, "-c", "android.intent.category.LAUNCHER", "1", NULL};
    executeSafe(args);
}

void numberGuessGame() {
    int secret, guess, tries = 0;
    char input_buf[32];
    char *endptr;

    srand(time(NULL));
    secret = rand() % 100 + 1;

    printf("\n=== NUMBER GUESS GAME ===\n");
    printf("Guess number between 1-100\n");

    while(1) {
        printf("Enter guess: ");
        getCleanInput(input_buf, sizeof(input_buf));
        guess = strtol(input_buf, &endptr, 10);

        if (*endptr != '\0' || input_buf[0] == '\0') {
            printf("Please punch in numbers only.\n");
            continue;
        }

        tries++;

        if(guess > secret) printf("Too High!\n");
        else if(guess < secret) printf("Too Low!\n");
        else {
            printf("Correct! Total calculation tries: %d\n", tries);
            break;
        }
    }
}

void snakeGame() {
    int x = WIDTH / 2;
    int y = HEIGHT / 2;
    char input_buf[10];
    char move;

    while(1) {
        char *clearArgs[] = {"clear", NULL};
        executeSafe(clearArgs);

        printf("=== MINI SNAKE ===\n");
        printf("Use WASD + Enter, q to backout\n\n");

        for(int i = 0; i < HEIGHT; i++) {
            for(int j = 0; j < WIDTH; j++) {
                if(i == y && j == x) printf("O");
                else printf(".");
            }
            printf("\n");
        }

        getCleanInput(input_buf, sizeof(input_buf));
        move = input_buf[0];

        if(move == 'q') break;
        if(move == 'w' && y > 0) y--;
        if(move == 's' && y < HEIGHT - 1) y++;
        if(move == 'a' && x > 0) x--;
        if(move == 'd' && x < WIDTH - 1) x++;
    }
}

void showHelp() {
    printf("\n=========== S OS HELP ===========\n");
    printf("\nBASIC METRICS:\n");
    printf("  help                 Show operational mappings\n");
    printf("  clear                Refresh user interface view\n");
    printf("  exit                 Terminate parent terminal engine\n");
    printf("  history              Show executed instruction trace logs\n");
    printf("  time                 Query current chronological data\n");
    printf("  calc                 Calculator routine\n");
    printf("  deviceinfo           Query local platform kernel metrics\n");
    printf("  duckduckgo           Launch search intention wrapper\n");
    printf("\nPACKAGE AUTOMATION:\n");
    printf("  sos install <pkg>    Automate local background software assembly\n");
    printf("  sos remove <pkg>     Wipe specific application frameworks\n");
    printf("  sos search <pkg>     Scan available platform repos\n");
    printf("  sos update           Synchronize background binary pools\n");
    printf("\nINTELLIGENT INTEGRATION:\n");
    printf("  open <app_name>      Extract system strings to wake up apps\n");
    printf("\nADMIN ESCALATION:\n");
    printf("  sudo <command>       Forward threads directly via administrative tsu hooks\n");
    printf("\nGAME SUBSYSTEMS:\n");
    printf("  guess                Launch guess tracking core interface\n");
    printf("  snake                Launch simple navigational canvas grid\n");
    printf("\nFIRMWARE DEPLOYMENT UTILITIES:\n");
    printf("  unlock bootloader    Invoke automated flashing tool blocks\n");
    printf("  flash rom            Flash full device software environments cleanly\n");
    printf("  flashkit             Advanced manual verification boot option module\n");
    printf("\n=================================\n");
}

int main() {
    unsetenv("LD_PRELOAD");

    char input[SHELL_MAX_INPUT];
    char cwd[1024];

    // Fixed: Catches Ctrl+C securely instead of abruptly exiting the whole process
    signal(SIGINT, handle_sigint);

    bootAnimation();

    while(1) {
        if(getcwd(cwd, sizeof(cwd)) != NULL)
            printf("\033[94m%s\033[0m > ", cwd);
        else
            printf("> ");

        getCleanInput(input, sizeof(input));

        if(strlen(input) == 0)
            continue;

        addHistory(input);

        if(strcmp(input, "exit") == 0) {
            printf("Exiting terminal context matrix loop cleanly...\n");
            break;
        }
        else if(strcmp(input, "help") == 0)
            showHelp();
        else if(strcmp(input, "history") == 0)
            showHistory();
        else if(strcmp(input, "guess") == 0)
            numberGuessGame();
        else if(strcmp(input, "snake") == 0)
            snakeGame();
        else if(strcmp(input, "calc") == 0)
            calculator();
        else if(strcmp(input, "deviceinfo") == 0)
            deviceInfo();
        else if(strcmp(input, "duckduckgo") == 0)
            duckDuckGo();
        else if(strcmp(input, "flashkit") == 0)
            flashKit();
        else if(strncmp(input, "wifi ", 5) == 0)
            systemControl("wifi", input + 5);
        else if(strncmp(input, "bluetooth ", 10) == 0)
            systemControl("bluetooth", input + 10);
        else if(strncmp(input, "bt ", 3) == 0)
            systemControl("bluetooth", input + 3);
        else if (strncmp(input, "cd ", 3) == 0) {
            if (chdir(input + 3) != 0) perror("Navigation pathway failed");
        }
        else if (strcmp(input, "cd") == 0) {
            char *home = getenv("HOME");
            if (home != NULL) chdir(home);
        }
        else if (strcmp(input, "time") == 0) {
            time_t t; time(&t);
            printf("%s", ctime(&t));
        }
        else if (strncmp(input, "color ", 6) == 0) {
            char arg[100];
            if (sscanf(input + 6, "%99s", arg) > 0) setColor(arg);
        }
        else if(strcmp(input, "clear") == 0) {
            char *args[] = {"clear", NULL};
            executeCommand(args);
        }
        else if(strncmp(input, "sos ", 4) == 0)
            sosPackageManager(input);
        else if(strncmp(input, "open ", 5) == 0)
            smartOpen(input);
        else if(strncmp(input, "sudo ", 5) == 0)
            runSudo(input);
        else if (strcmp(input, "unlock bootloader") == 0) {
            char company[10];
            printf("\nSelect Device Brand:\n");
            printf("1. Pixel/Moto/OnePlus\n");
            printf("2. Xiaomi/Redmi/POCO\n");
            printf("Choose: ");
            getCleanInput(company, sizeof(company));

            if (company[0] == '2') {
                printf("\033[91m\n=====================================\n         XIAOMI MITOOL MODE\n=====================================\n\nChecking required tools...\n");

                if (!commandExists("git")) {
                    printf("git missing in framework directories.\nRun: pkg install git\n\033[0m");
                    continue;
                }
                if (!commandExists("bash")) {
                    printf("bash missing in framework directories.\nRun: pkg install bash\n\033[0m");
                    continue;
                }
                if (!commandExists("fastboot")) {
                    printf("fastboot missing in framework directories.\nRun: pkg install android-tools\n\033[0m");
                    continue;
                }

                printf("\nThis uses unofficial MiTool dependencies.\nSuccess depends on explicit device model configurations.\n\nContinue? (yes/no): ");
                char xconfirm[20];
                getCleanInput(xconfirm, sizeof(xconfirm));

                if (strncmp(xconfirm, "yes", 3) != 0) {
                    printf("Cancelled.\n\033[0m");
                    continue;
                }

                int clone_failed = 0;
                if (access("MiTool", F_OK) != 0) {
                    printf("\nDownloading automated MiTool repository contents...\n");
                    if (system("git clone https://github.com/offici5l/MiTool.git") != 0) {
                        printf("Error: Network transaction timed out or aborted.\n");
                        clone_failed = 1;
                    }
                }

                if (!clone_failed) {
                    printf("\nLaunching custom automated flashing tools smoothly...\n");
                    // Fixed: Targets main.sh accurately inside the cloned repository layout
                    system("cd MiTool && chmod +x main.sh 2>/dev/null && bash main.sh");
                }
                printf("\033[0m");
                continue;
            }

            printf("\033[91m\n=====================================\n        BOOTLOADER UNLOCK\n=====================================\n\nDANGER WARNING:\nUnlocking the bootloader wipes all structural file profiles.\n\nContinue? (yes/no): ");
            char confirm[20];
            getCleanInput(confirm, sizeof(confirm));

            if (strncmp(confirm, "yes", 3) != 0) {
                printf("Cancelled.\n\033[0m");
                continue;
            }

            char *checkArgs[] = { "fastboot", "devices", NULL };
            executeSafe(checkArgs);
            char *unlockArgs[] = { "fastboot", "flashing", "unlock", NULL };
            executeSafe(unlockArgs);
            printf("\nVerify choice verification selections directly on the handset hardware screen structure.\n\033[0m");
        }
        else if (strcmp(input, "flash rom") == 0) {
            char rom[200];
            printf("\033[91m\n=====================================\n            FLASH ROM\n=====================================\n\nDANGER WARNING:\nFlashing wrong image variants can instantly drop target system states.\n\nEnter ROM ZIP/IMG path: ");
            getCleanInput(rom, sizeof(rom));

            if (!isSafePath(rom)) {
                printf("Risk parameters detected. Partition delivery blocked.\n\033[0m");
                continue;
            }

            printf("\nContinue flashing? (yes/no): ");
            char confirm[20];
            getCleanInput(confirm, sizeof(confirm));

            if (strncmp(confirm, "yes", 3) != 0) {
                printf("Cancelled.\n\033[0m");
                continue;
            }

            char *checkArgs[] = { "fastboot", "devices", NULL };
            executeSafe(checkArgs);
            char *flashArgs[] = { "fastboot", "update", rom, NULL };
            executeSafe(flashArgs);
            char *rebootArgs[] = { "fastboot", "reboot", NULL };
            executeSafe(rebootArgs);
            printf("\nFlash execution process chain concluded.\n\033[0m");
        }
        else {
            runExternal(input);
        }
    }

    return 0;
}
