/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#include "common.h"
#include "install.h"
#include "mincrypt/rsa.h"
#include "minui/minui.h"
#include "minzip/SysUtil.h"
#include "minzip/Zip.h"
#include "minzipwrappers.h"
#include "mtdutils/mounts.h"
#include "mtdutils/mtdutils.h"
#include "mtdutilswrappers.h"
#include "roots.h"
#include "verifier.h"
#include "ui.h"

extern RecoveryUI* ui;

#define ASSUMED_UPDATE_BINARY_NAME  "META-INF/com/google/android/update-binary"
#define PUBLIC_KEYS_FILE "/res/keys"

// Default allocation of progress bar segments to operations
static const int VERIFICATION_PROGRESS_TIME = 60;
static const float VERIFICATION_PROGRESS_FRACTION = 0.25;
static const float DEFAULT_FILES_PROGRESS_FRACTION = 0.4;
static const float DEFAULT_IMAGE_PROGRESS_FRACTION = 0.1;

// If the package contains an update binary, extract it and run it.
static int
try_update_binary(const char* path, ZipArchive* zip, bool* wipe_cache) {
    LOGUI("Trying to update binary %s...\n", path);
    const ZipEntry* binary_entry =
            LOGUI_mzFindZipEntry(zip, ASSUMED_UPDATE_BINARY_NAME);
    if (binary_entry == NULL) {
        LOGUI_mzCloseZipArchive(zip);
        return INSTALL_CORRUPT;
    }

    const char* binary = "/tmp/update_binary";
    unlink(binary);
    int fd = creat(binary, 0755);
    if (fd < 0) {
        LOGUI_mzCloseZipArchive(zip);
        LOGE("Can't make %s\n", binary);
        return INSTALL_ERROR;
    }
    bool ok = LOGUI_mzExtractZipEntryToFile(zip, binary_entry, fd);
    close(fd);
    LOGUI_mzCloseZipArchive(zip);

    if (!ok) {
        LOGE("Can't copy %s\n", ASSUMED_UPDATE_BINARY_NAME);
        return INSTALL_ERROR;
    }

    int pipefd[2];
    pipe(pipefd);

    // When executing the update binary contained in the package, the
    // arguments passed are:
    //
    //   - the version number for this interface
    //
    //   - an fd to which the program can write in order to update the
    //     progress bar.  The program can write single-line commands:
    //
    //        progress <frac> <secs>
    //            fill up the next <frac> part of of the progress bar
    //            over <secs> seconds.  If <secs> is zero, use
    //            set_progress commands to manually control the
    //            progress of this segment of the bar.
    //
    //        set_progress <frac>
    //            <frac> should be between 0.0 and 1.0; sets the
    //            progress bar within the segment defined by the most
    //            recent progress command.
    //
    //        firmware <"hboot"|"radio"> <filename>
    //            arrange to install the contents of <filename> in the
    //            given partition on reboot.
    //
    //            (API v2: <filename> may start with "PACKAGE:" to
    //            indicate taking a file from the OTA package.)
    //
    //            (API v3: this command no longer exists.)
    //
    //        ui_print <string>
    //            display <string> on the screen.
    //
    //        wipe_cache
    //            a wipe of cache will be performed following a successful
    //            installation.
    //
    //        clear_display
    //            turn off the text display.
    //
    //        enable_reboot
    //            packages can explicitly request that they want the user
    //            to be able to reboot during installation (useful for
    //            debugging packages that don't exit).
    //
    //   - the name of the package zip file.
    //

    const char** args = (const char**)malloc(sizeof(char*) * 5);
    args[0] = binary;
    args[1] = EXPAND(RECOVERY_API_VERSION);   // defined in Android.mk
    char* temp = (char*)malloc(10);
    sprintf(temp, "%d", pipefd[1]);
    args[2] = temp;
    args[3] = (char*)path;
    args[4] = NULL;

    LOGUI("Fork recovery process: %s %s %s %s\n", binary, args[1], args[2], args[3]);
    pid_t pid = fork();
    if (pid == 0) {
        umask(022);
        close(pipefd[0]);
        execv(binary, (char* const*)args);
        fprintf(stdout, "E:Can't run %s (%s)\n", binary, strerror(errno));
        _exit(-1);
    }
    close(pipefd[1]);

    *wipe_cache = false;

    char buffer[1024];
    FILE* from_child = fdopen(pipefd[0], "r");
    struct timespec tv;
    tv.tv_sec = 1;
    tv.tv_nsec = 0;
    bool updateBinaryFinish = false;
    bool updatingBinaryDisplayed = false;
    int status;
    while(!waitpid(pid, &status, WNOHANG))
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(pipefd[0], &fds);
        pselect(pipefd[0]+1, &fds, NULL, NULL, &tv, NULL);
        if(FD_ISSET(pipefd[0], &fds))
        {
            if(fgets(buffer, sizeof(buffer), from_child) != NULL)
            {
                char* command = strtok(buffer, " \n");
                if (command == NULL) {
                    continue;
                } else if (strcmp(command, "progress") == 0) {
                    char* fraction_s = strtok(NULL, " \n");
                    char* seconds_s = strtok(NULL, " \n");

                    float fraction = strtof(fraction_s, NULL);
                    int seconds = strtol(seconds_s, NULL, 10);

                    ui->ShowProgress(fraction * (1-VERIFICATION_PROGRESS_FRACTION), seconds);
                } else if (strcmp(command, "set_progress") == 0) {
                    char* fraction_s = strtok(NULL, " \n");
                    float fraction = strtof(fraction_s, NULL);
                    ui->SetProgress(fraction);
                } else if (strcmp(command, "ui_print") == 0) {
                    char* str = strtok(NULL, "\n");
                    if (str) {
                       if(updatingBinaryDisplayed)
                       {
                           LOGUI("\n");
                       }
                        ui->Print("%s", str);
                    } else {
                        ui->Print("\n");
                    }
                    updatingBinaryDisplayed = false;
                } else if (strcmp(command, "wipe_cache") == 0) {
                    *wipe_cache = true;
                } else if (strcmp(command, "clear_display") == 0) {
                    ui->SetBackground(RecoveryUI::NONE);
                } else if (strcmp(command, "enable_reboot") == 0) {
                    // packages can explicitly request that they want the user
                    // to be able to reboot during installation (useful for
                    // debugging packages that don't exit).
                    ui->SetEnableReboot(true);
                } else {
                    LOGE("unknown command [%s]\n", command);
                }
            }
        }
        else
        {
            if(!updatingBinaryDisplayed)
            {
                LOGUI("Updating binary");
                updatingBinaryDisplayed = true;
            }
            LOGUI(".");
        }
        fflush(stdout);
    }
    if(updatingBinaryDisplayed)
    {
       LOGUI("\n");
    }
    fclose(from_child);

    // process is still valid
    LOGUI("Waiting for finish update binary process\n");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        LOGE("Error in %s\n(Status %d)\n", path, WEXITSTATUS(status));
        return INSTALL_ERROR;
    }
    LOGUI("Updated with result code: %d\n", status);

    return INSTALL_SUCCESS;
}

static int
really_install_package(const char *path, bool* wipe_cache, bool needs_mount)
{
    LOGUI("Really install package %s\n", path);
    ui->SetBackground(RecoveryUI::INSTALLING_UPDATE);
    ui->Print("Finding update package...\n");
    // Give verification half the progress bar...
    ui->SetProgressType(RecoveryUI::DETERMINATE);
    ui->ShowProgress(VERIFICATION_PROGRESS_FRACTION, VERIFICATION_PROGRESS_TIME);
    LOGI("Update location: %s\n", path);

    // Map the update package into memory.
    ui->Print("Opening update package...\n");

    if (path && needs_mount) {
        if (path[0] == '@') {
            ensure_path_mounted(path+1);
        } else {
            ensure_path_mounted(path);
        }
    }

    MemMapping map;
    if (LOGUI_sysMapFile(path, &map) != 0) {
        LOGE("failed to map file\n");
        return INSTALL_CORRUPT;
    }

    int numKeys;
    Certificate* loadedKeys = load_keys(PUBLIC_KEYS_FILE, &numKeys);
    if (loadedKeys == NULL) {
        LOGE("Failed to load keys\n");
        return INSTALL_CORRUPT;
    }
    LOGI("%d key(s) loaded from %s\n", numKeys, PUBLIC_KEYS_FILE);

    ui->Print("Verifying update package...\n");

    int err;
    err = verify_file(map.addr, map.length, loadedKeys, numKeys);
    free(loadedKeys);
    LOGI("verify_file returned %d\n", err);
    if (err != VERIFY_SUCCESS) {
        LOGE("signature verification failed\n");
        LOGUI_sysReleaseMap(&map);
        return INSTALL_CORRUPT;
    }

    /* Try to open the package.
     */
    ZipArchive zip;
    err = LOGUI_mzOpenZipArchive(map.addr, map.length, &zip);
    if (err != 0) {
        LOGE("Can't open %s\n(%s)\n", path, err != -1 ? strerror(err) : "bad");
        LOGUI_sysReleaseMap(&map);
        return INSTALL_CORRUPT;
    }

    /* Verify and install the contents of the package.
     */
    ui->Print("Installing update...\n");
    ui->SetEnableReboot(false);
    int result = try_update_binary(path, &zip, wipe_cache);
    ui->SetEnableReboot(true);

    LOGUI_sysReleaseMap(&map);

    return result;
}

int
install_package(const char* path, bool* wipe_cache, const char* install_file,
                bool needs_mount)
{
    LOGUI("Install package %s\n", path);
    modified_flash = true;

    LOGUI("Open install log file %s\n", install_file);
    FILE* install_log = fopen_path(install_file, "w");
    if (install_log) {
        fputs(path, install_log);
        fputc('\n', install_log);
    } else {
        LOGE("failed to open last_install: %s\n", strerror(errno));
    }
    int result;
    if (setup_install_mounts() != 0) {
        LOGE("failed to set up expected mounts for install; aborting\n");
        result = INSTALL_ERROR;
    } else {
        result = really_install_package(path, wipe_cache, needs_mount);
    }
    if (install_log) {
        fputc(result == INSTALL_SUCCESS ? '1' : '0', install_log);
        fputc('\n', install_log);
        fclose(install_log);
    }
    return result;
}
