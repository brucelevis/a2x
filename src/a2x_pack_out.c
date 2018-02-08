/*
    Copyright 2016, 2017 Alex Margarit

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

#if A_PLATFORM_SYSTEM_EMSCRIPTEN
    #include <emscripten.h>
#endif

#include "a2x_pack_console.v.h"
#include "a2x_pack_out.v.h"
#include "a2x_pack_screen.v.h"
#include "a2x_pack_settings.v.h"
#include "a2x_pack_time.v.h"

typedef enum {
    A_COLOR_INVALID = -1,
    A_COLOR_BLACK = 30,
    A_COLOR_RED = 31,
    A_COLOR_GREEN = 32,
    A_COLOR_YELLOW = 33,
    A_COLOR_BLUE = 34,
    A_COLOR_MAGENTA = 35,
    A_COLOR_CYAN = 36,
    A_COLOR_WHITE = 37
} AColorCode;

static void outPrintHeader(const char* Source, const char* Title, AColorCode Color, FILE* Stream)
{
    #if A_PLATFORM_SYSTEM_LINUX && A_PLATFORM_SYSTEM_DESKTOP
        fprintf(Stream, "\033[1;%dm[%s][%s]\033[0m ", Color, Source, Title);
    #else
        A_UNUSED(Color);
        fprintf(Stream, "[%s][%s] ", Source, Title);
    #endif
}

static void outPrint(const char* Source, const char* Title, AColorCode Color, FILE* Stream, const char* Format, va_list Args)
{
    outPrintHeader(Source, Title, Color, Stream);
    vfprintf(Stream, Format, Args);
    fprintf(Stream, "\n");
}

static void outConsole(AConsoleOutType Type, const char* Format, va_list Args)
{
    char buffer[256];

    if(vsnprintf(buffer, sizeof(buffer), Format, Args) > 0) {
        a_console__write(Type, buffer);
    }
}

static void outConsoleOverwrite(AConsoleOutType Type, const char* Format, ...)
{
    char buffer[256];

    va_list args;
    va_start(args, Format);

    if(vsnprintf(buffer, sizeof(buffer), Format, args) > 0) {
        a_console__overwrite(Type, buffer);
    }

    va_end(args);
}

#define A_OUT__ARGS(FunctionCall) \
{                                 \
    va_list args;                 \
    va_start(args, Format);       \
                                  \
    FunctionCall;                 \
                                  \
    va_end(args);                 \
}

#define A_OUT__PRINT(Source, Title, Color, Stream)                     \
{                                                                      \
    A_OUT__ARGS(outPrint(Source, Title, Color, Stream, Format, args)); \
}

#define A_OUT__CONSOLE(Type)                     \
{                                                \
    A_OUT__ARGS(outConsole(Type, Format, args)); \
}

void a_out__message(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")) {
        A_OUT__PRINT("a2x", "Msg", A_COLOR_GREEN, stdout);
        A_OUT__CONSOLE(A_CONSOLE_MESSAGE);
    }
}

void a_out__warning(const char* Format, ...)
{
    A_OUT__PRINT("a2x", "Wrn", A_COLOR_YELLOW, stderr);
    A_OUT__CONSOLE(A_CONSOLE_WARNING);
}

void a_out__warningv(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")
        && a_settings_getBool("app.output.verbose")) {

        A_OUT__PRINT("a2x", "Wrn", A_COLOR_YELLOW, stderr);
        A_OUT__CONSOLE(A_CONSOLE_WARNING);
    }
}

void a_out__error(const char* Format, ...)
{
    A_OUT__PRINT("a2x", "Err", A_COLOR_RED, stderr);
    A_OUT__CONSOLE(A_CONSOLE_ERROR);
}

void a_out__fatal(const char* Format, ...)
{
    A_OUT__PRINT("a2x", "Ftl", A_COLOR_MAGENTA, stderr);
    A_OUT__CONSOLE(A_CONSOLE_ERROR);

    a_console__setShow(true);
    a_out__message("Exiting in 10 seconds");

    for(int s = 10; s > 0; s--) {
        outConsoleOverwrite(A_CONSOLE_MESSAGE, "Exiting in %d seconds", s);
        a_screen__show();
        a_time_waitMs(1000);
    }

    #if A_PLATFORM_SYSTEM_EMSCRIPTEN
        emscripten_force_exit(1);
    #endif

    exit(1);
}

void a_out__state(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")) {
        A_OUT__PRINT("a2x", "Stt", A_COLOR_BLUE, stdout);
        A_OUT__CONSOLE(A_CONSOLE_STATE);
    }
}

void a_out__stateVerbose(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")
        && a_settings_getBool("app.output.verbose")) {

        A_OUT__PRINT("a2x", "Stt", A_COLOR_BLUE, stdout);
        A_OUT__CONSOLE(A_CONSOLE_STATE);
    }
}

void a_out_print(const char* Text)
{
    if(a_settings_getBool("app.output.on")) {
        outPrintHeader("App", "Msg", A_COLOR_GREEN, stdout);
        puts(Text);

        a_console__write(A_CONSOLE_APP, Text);
    }
}

void a_out_printf(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")) {
        A_OUT__PRINT("App", "Msg", A_COLOR_GREEN, stdout);
        A_OUT__CONSOLE(A_CONSOLE_APP);
    }
}

void a_out_printv(const char* Format, va_list Args)
{
    if(a_settings_getBool("app.output.on")) {
        va_list consoleArgs;
        va_copy(consoleArgs, Args);

        outPrint("App", "Msg", A_COLOR_GREEN, stdout, Format, Args);
        outConsole(A_CONSOLE_APP, Format, consoleArgs);

        va_end(consoleArgs);
    }
}

void a_out_warning(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")) {
        A_OUT__PRINT("App", "Wrn", A_COLOR_YELLOW, stderr);
        A_OUT__CONSOLE(A_CONSOLE_APP);
    }
}

void a_out_error(const char* Format, ...)
{
    if(a_settings_getBool("app.output.on")) {
        A_OUT__PRINT("App", "Err", A_COLOR_RED, stderr);
        A_OUT__CONSOLE(A_CONSOLE_APP);
    }
}
