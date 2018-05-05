/*
    Copyright 2011, 2016 Alex Margarit

    This file is part of a2x-framework.

    a2x-framework is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    a2x-framework is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with a2x-framework.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "a2x_system_includes.h"
#include "a2x_pack_screenshot.v.h"

#include "a2x_pack_dir.v.h"
#include "a2x_pack_input.v.h"
#include "a2x_pack_input_button.v.h"
#include "a2x_pack_out.v.h"
#include "a2x_pack_png.v.h"
#include "a2x_pack_screen.v.h"
#include "a2x_pack_settings.v.h"
#include "a2x_pack_str.v.h"

#define SCREENSHOTS_LIMIT 99999

static bool g_isInit;
static char* g_filePrefix;
static char* g_title;
static char* g_description;
static unsigned g_screenshotNumber;
static AInputButton* g_button;

static bool lazy_init(void)
{
    const char* screensDir = a_settings_getString("screenshot.dir");

    if(a_dir_exists(screensDir)) {
        ADir* dir = a_dir_open(screensDir);

        // Only interested in the last file, to get the number from its name
        ADirEntry* entry = a_list_getLast(a_dir_getEntries(dir));

        if(entry == NULL) {
            g_isInit = true;
        } else {
            const char* file = a_dir_entryGetName(entry);
            const char* fullPath = a_dir_entryGetPath(entry);

            int start = a_str_getLastIndex(file, '-');
            int end = a_str_getLastIndex(file, '.');

            if(start != -1 && end != -1 && end - start == 6) {
                char* numberStr = a_str_getSub(file, start + 1, end);
                int number = atoi(numberStr);
                free(numberStr);

                if(number > 0) {
                    g_screenshotNumber = (unsigned)number;
                    g_isInit = true;
                }
            }

            if(!g_isInit) {
                a_out__error("Invalid file name %s", fullPath);
            }
        }

        a_dir_close(dir);
    } else {
        a_out__message("Making screenshots dir: %s", screensDir);

        if(a_dir_make(screensDir)) {
            g_isInit = true;
        }
    }

    if(g_isInit) {
        g_filePrefix = a_str_merge(screensDir,
                                   "/",
                                   a_settings_getString("app.title"),
                                   "-",
                                   NULL);

        // No spaces in file name
        for(char* s = g_filePrefix; *s != '\0'; s++) {
            if(*s == ' ') {
                *s = '-';
            }
        }

        g_title = a_str_merge(a_settings_getString("app.title"),
                              " ",
                              a_settings_getString("app.version"),
                              " by ",
                              a_settings_getString("app.author"),
                              NULL);

        g_description = a_str_merge(a_settings_getString("app.title"),
                                    " ",
                                    a_settings_getString("app.version"),
                                    " by ",
                                    a_settings_getString("app.author"),
                                    ", built on ",
                                    a_settings_getString("app.buildtime"),
                                    ". Running on a2x ",
                                    A__MAKE_CURRENT_GIT_BRANCH,
                                    ", built on ",
                                    A__MAKE_COMPILE_TIME,
                                    ".",
                                    NULL);
    }

    return g_isInit;
}

static void takeScreenshot(void)
{
    if(!g_isInit && !lazy_init()) {
        return;
    }

    if(++g_screenshotNumber > SCREENSHOTS_LIMIT) {
        a_out__error("%d screenshots limit exceeded", SCREENSHOTS_LIMIT);
        return;
    }

    char num[6];
    snprintf(num, sizeof(num), "%05d", g_screenshotNumber);
    char* name = a_str_merge(g_filePrefix, num, ".png", NULL);

    a_out__message("Saving screenshot '%s'", name);
    a_png_write(name,
                a_screen_getPixels(),
                a__screen.width,
                a__screen.height,
                g_title,
                g_description);

    free(name);
}

static void inputCallback(void)
{
    if(a_button_getPressedOnce(g_button)) {
        takeScreenshot();
    }
}

void a_screenshot__init(void)
{
    g_isInit = false;
    g_filePrefix = NULL;
    g_title = NULL;
    g_description = NULL;
    g_screenshotNumber = 0;
    g_button = a_button_new(a_settings_getString("screenshot.button"));
    a_input__addCallback(inputCallback);
}

void a_screenshot__uninit(void)
{
    if(g_isInit) {
        free(g_filePrefix);
        free(g_title);
        free(g_description);
    }
}

void a_screenshot_take(void)
{
    takeScreenshot();
}
