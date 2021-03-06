/*
    Copyright 2010, 2016-2019 Alex Margarit <alex@alxm.org>
    This file is part of a2x, a C video game framework.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "a2x_pack_sound.v.h"

#include "a2x_pack_draw.v.h"
#include "a2x_pack_file.v.h"
#include "a2x_pack_math.v.h"
#include "a2x_pack_mem.v.h"
#include "a2x_pack_platform.v.h"
#include "a2x_pack_screen.v.h"
#include "a2x_pack_timer.v.h"

static int g_volume;
static int g_musicVolume;
static int g_samplesVolume;
static int g_volumeMax;

#if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
    #define A__VOLUME_STEP 1
    #define A__VOLBAR_SHOW_MS 500
    static ATimer* g_volTimer;
    static AButton* g_volumeUpButton;
    static AButton* g_volumeDownButton;
#endif

#if A_CONFIG_TRAIT_KEYBOARD
    static AButton* g_muteButton;
#endif

static void adjustSoundVolume(int Volume)
{
    g_volume = a_math_clamp(Volume, 0, g_volumeMax);
    g_musicVolume = g_volume * A_CONFIG_SOUND_VOLUME_SCALE_MUSIC / 100;
    g_samplesVolume = g_volume * A_CONFIG_SOUND_VOLUME_SCALE_SAMPLE / 100;

    a_platform__soundSampleVolumeSetAll(g_samplesVolume);
    a_platform__soundMusicVolumeSet(g_musicVolume);
}

void a_sound__init(void)
{
    g_volumeMax = a_platform__soundVolumeGetMax();

    #if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
        adjustSoundVolume(g_volumeMax / 16);
        g_volTimer = a_timer_new(A_TIMER_MS, A__VOLBAR_SHOW_MS, false);
    #else
        adjustSoundVolume(g_volumeMax);
    #endif

    #if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
        g_volumeUpButton = a_button_new();
        a_button_bind(g_volumeUpButton, A_BUTTON_VOLUP);

        g_volumeDownButton = a_button_new();
        a_button_bind(g_volumeDownButton, A_BUTTON_VOLDOWN);
    #endif

    #if A_CONFIG_TRAIT_KEYBOARD
        g_muteButton = a_button_new();
        a_button_bind(g_muteButton, A_KEY_M);
    #endif
}

void a_sound__uninit(void)
{
    #if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
        a_timer_free(g_volTimer);
    #endif

    #if A_CONFIG_TRAIT_KEYBOARD
        a_button_free(g_muteButton);
    #endif

    a_music_stop();
}

void a_sound__tick(void)
{
    #if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
        int adjust = 0;

        if(a_button_pressGet(g_volumeUpButton)) {
            adjust = A__VOLUME_STEP;
        } else if(a_button_pressGet(g_volumeDownButton)) {
            adjust = -A__VOLUME_STEP;
        }

        if(adjust) {
            adjustSoundVolume(g_volume + adjust);
            a_timer_start(g_volTimer);
        }
    #endif

    #if A_CONFIG_TRAIT_KEYBOARD
        if(a_button_pressGetOnce(g_muteButton)) {
            a_platform__soundMuteFlip();
        }
    #endif
}

void a_sound__draw(void)
{
    #if A_CONFIG_SYSTEM_GP2X || A_CONFIG_SYSTEM_WIZ
        if(!a_timer_isRunning(g_volTimer) || a_timer_expiredGet(g_volTimer)) {
            return;
        }

        a_pixel_blendSet(A_PIXEL_BLEND_PLAIN);

        a_pixel_colorSetHex(A_CONFIG_COLOR_VOLBAR_BACKGROUND);
        a_draw_rectangle(0, 181, g_volumeMax / A__VOLUME_STEP + 5, 16);

        a_pixel_colorSetHex(A_CONFIG_COLOR_VOLBAR_BORDER);
        a_draw_hline(0, g_volumeMax / A__VOLUME_STEP + 4, 180);
        a_draw_hline(0, g_volumeMax / A__VOLUME_STEP + 4, 183 + 14);
        a_draw_vline(g_volumeMax / A__VOLUME_STEP + 4 + 1, 181, 183 + 13);

        a_pixel_colorSetHex(A_CONFIG_COLOR_VOLBAR_FILL);
        a_draw_rectangle(0, 186, g_volume / A__VOLUME_STEP, 6);
    #endif
}

AMusic* a_music_new(const char* Path)
{
    return a_platform__soundMusicNew(Path);
}

void a_music_free(AMusic* Music)
{
    if(Music) {
        a_platform__soundMusicFree(Music);
    }
}

void a_music_play(AMusic* Music)
{
    if(a_platform__soundMuteGet()) {
        return;
    }

    a_platform__soundMusicPlay(Music);
}

void a_music_stop(void)
{
    a_platform__soundMusicStop();
}

ASample* a_sample_new(const char* Path)
{
    APlatformSoundSample* s = NULL;

    if(a_path_exists(Path, A_PATH_FILE | A_PATH_REAL)) {
        s = a_platform__soundSampleNewFromFile(Path);
    } else {
        const AEmbeddedFile* e = a_embed__getFile(Path);

        if(e) {
            s = a_platform__soundSampleNewFromData(e->buffer, (int)e->size);
        }
    }

    return s;
}

void a_sample_free(ASample* Sample)
{
    if(Sample) {
        a_platform__soundSampleFree(Sample);
    }
}

int a_channel_new(void)
{
    return a_platform__soundSampleChannelGet();
}

void a_channel_play(int Channel, ASample* Sample, AChannelFlags Flags)
{
    if(a_platform__soundMuteGet()) {
        return;
    }

    if(A_FLAG_TEST_ANY(Flags, A_CHANNEL_RESTART)) {
        a_platform__soundSampleStop(Channel);
    } else if(A_FLAG_TEST_ANY(Flags, A_CHANNEL_YIELD)
        && a_platform__soundSampleIsPlaying(Channel)) {

        return;
    }

    a_platform__soundSamplePlay(
        Sample, Channel, A_FLAG_TEST_ANY(Flags, A_CHANNEL_LOOP));
}

void a_channel_stop(int Channel)
{
    a_platform__soundSampleStop(Channel);
}

bool a_channel_isPlaying(int Channel)
{
    return a_platform__soundSampleIsPlaying(Channel);
}
