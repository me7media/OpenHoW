/* OpenHoW
 * Copyright (C) 2017-2019 Mark Sowden <markelswo@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <PL/platform_graphics.h>
#include <PL/platform_graphics_camera.h>
#include <PL/platform_filesystem.h>

#include "../engine.h"
#include "../input.h"
#include "../game.h"

#include "frontend.h"
#include "font.h"
#include "display.h"
#include "video.h"

static unsigned int frontend_state = FE_MODE_INIT;
static unsigned int old_frontend_state = (unsigned int) -1;

/* for now we're going to hard-code most of this but eventually
 * we will start freeing most of this up... either through JS
 * or some other way, so y'know. Wheeee.
 */

static const char *papers_teams_paths[MAX_TEAMS]={
        "fe/papers/british.bmp",
        "fe/papers/american.bmp",
        "fe/papers/french.bmp",
        "fe/papers/german.bmp",
        "fe/papers/russian.bmp",
        "fe/papers/japan.bmp",
        "fe/papers/teamlard.bmp"
};

/* texture assets, these are loaded and free'd at runtime */
static PLTexture *fe_background    = NULL;
static PLTexture *fe_press         = NULL;
static PLTexture *fe_any           = NULL;
static PLTexture *fe_key           = NULL;
static PLTexture *fe_papers_teams[MAX_TEAMS] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

enum {
    FE_TEXTURE_ANG,
    FE_TEXTURE_ANGPOINT,

    FE_TEXTURE_CLOCK,
    FE_TEXTURE_TIMER,
    FE_TEXTURE_CLIGHT,

    FE_TEXTURE_CROSSHAIR,
    FE_TEXTURE_TARGET,

    FE_TEXTURE_ARROW,
    FE_TEXTURE_CROSS,

    FE_TEXTURE_PAUSE,

    MAX_FE_GAME_TEXTURES
};
static PLTexture *fe_tx_game_textures[MAX_FE_GAME_TEXTURES];  /* textures that we'll be using in-game */
//static PLTexture *fe_tx_game_icons[MAX_ITEM_TYPES];

/************************************************************/
/* FE Object Implementation
 *  Individual frontend objects that can manipulated
 *  on the fly at runtime - specifically created for
 *  implementing Hogs of War's really awesome animated
 *  menu! */

#define MAX_FE_OBJECTS  1024

typedef struct FEObject {
    int x, y;
    unsigned int w, h;

    PLTexture **frames;
    unsigned int num_frames;
    unsigned int cur_frame;

    bool is_reserved;
} FEObject;
static FEObject fe_objects[MAX_FE_OBJECTS];

void SimulateFEObjects(void) {
    for(unsigned int i = 0; i < MAX_FE_OBJECTS; ++i) {
        if(!fe_objects[i].is_reserved) {
            continue;
        }
    }
}

void SetFEObjectCommand(unsigned int argc, char *argv[]) {
    if(argc < 2) {
        LogWarn("invalid number of arguments, ignoring!\n");
        return;
    }

    for(unsigned int i = 2; i < argc; ++i) {

    }
}

/************************************************************/

void FrontendInputCallback(int key, bool is_pressed) {
    if(frontend_state == FE_MODE_START && is_pressed) {
        /* todo, play 'ting' sound! */

        /* we've hit our key, we can take away this
         * callback now and carry on to whatever */
        Input_SetKeyboardFocusCallback(NULL);
        FE_SetState(FE_MODE_MAIN_MENU);
        return;
    }
}

static void CacheFEGameData(void) {
#if 1
    fe_tx_game_textures[FE_TEXTURE_ANG] = Display_LoadTexture("fe/dash/ang", PL_TEXTURE_FILTER_LINEAR);
    fe_tx_game_textures[FE_TEXTURE_ANGPOINT] = Display_LoadTexture("fe/dash/angpoint", PL_TEXTURE_FILTER_LINEAR);

    fe_tx_game_textures[FE_TEXTURE_CLOCK] = Display_LoadTexture("fe/dash/clock", PL_TEXTURE_FILTER_LINEAR);
    fe_tx_game_textures[FE_TEXTURE_CLIGHT] = Display_LoadTexture("fe/dash/timlit.png", PL_TEXTURE_FILTER_LINEAR);
    fe_tx_game_textures[FE_TEXTURE_TIMER] = Display_LoadTexture("fe/dash/timer", PL_TEXTURE_FILTER_LINEAR);
#endif
}

static void CacheFEMenuData(void) {
    fe_background = Display_LoadTexture("fe/title/titlemon", PL_TEXTURE_FILTER_LINEAR);
    for(unsigned int i = 0; i < MAX_TEAMS; ++i) {
        fe_papers_teams[i] = Display_LoadTexture(papers_teams_paths[i], PL_TEXTURE_FILTER_LINEAR);
    }
}

static void ClearFEGameData(void) {
    for(unsigned int i = 0; i < MAX_FE_GAME_TEXTURES; ++i) {
        if(fe_tx_game_textures[i] == NULL) {
            continue;
        }
        plDeleteTexture(fe_tx_game_textures[i], true);
    }
}

static void ClearFEMenuData(void) {
    plDeleteTexture(fe_background, true);
    for(unsigned int i = 0; i < MAX_TEAMS; ++i) {
        plDeleteTexture(fe_papers_teams[i], true);
    }
}

/************************************************************/

void FE_Initialize(void) {
    memset(fe_objects, 0, sizeof(FEObject) * MAX_FE_OBJECTS);

    CacheFontData();
    CacheFEMenuData();
}

void FE_Shutdown(void) {
    ClearFontData();
}

void FE_ProcessInput(void) {
    switch(frontend_state) {
        default:break;

        case FE_MODE_START: {
            /* this is... kind of a hack... but ensures that
             * nothing will take away our check for a key during
             * the 'start' screen, e.g. bringing the console up */
            Input_SetKeyboardFocusCallback(FrontendInputCallback);
        } break;

        case FE_MODE_VIDEO: {
            if(Input_GetKeyState(PORK_KEY_SPACE) || Input_GetKeyState(PORK_KEY_ESCAPE)) {
                Video_SkipCurrent();
            }
        } break;
    }
}

void FE_Simulate(void) {
    SimulateFEObjects();

    switch(frontend_state) {
        default:break;

        case FE_MODE_INIT: {
            /* by this point we'll make an assumption that we're
             * done initializing, bump up the progress, draw that
             * frame and then switch over to our 'start' state */
            if(FE_GetLoadingProgress() < 100) {
                FE_SetLoadingDescription("LOADING FRONTEND RESOURCES");
                FE_SetLoadingProgress(100);

                /* load in some of the assets we'll be using on the
                 * next screen before proceeding... */
                fe_press = Display_LoadTexture("fe/title/press", PL_TEXTURE_FILTER_LINEAR);
                fe_any = Display_LoadTexture("fe/title/any", PL_TEXTURE_FILTER_LINEAR);
                fe_key = Display_LoadTexture("fe/title/key", PL_TEXTURE_FILTER_LINEAR);
                fe_background = Display_LoadTexture("fe/title/title", PL_TEXTURE_FILTER_LINEAR);
                break;
            }

            FE_SetState(FE_MODE_START);
        } break;
    }
}

/************************************************************/

char loading_description[256];
uint8_t loading_progress = 0;

#define Redraw()   FE_Draw();

void FE_SetLoadingBackground(const char *name) {
    if(fe_background != NULL) {
        plDeleteTexture(fe_background, true);
    }

    char screen_path[PL_SYSTEM_MAX_PATH];
    snprintf(screen_path, sizeof(screen_path), "fe/briefing/%s", name);
    if(!plFileExists(screen_path)) {
        snprintf(screen_path, sizeof(screen_path), "fe/briefing/loadmult");
    }

    fe_background = Display_LoadTexture(screen_path, PL_TEXTURE_FILTER_LINEAR);
    Redraw();
}

void FE_SetLoadingDescription(const char *description) {
    snprintf(loading_description, sizeof(loading_description), "%s ...", description);
    Redraw();
}

void FE_SetLoadingProgress(uint8_t progress) {
    if(progress > 100) progress = 100;
    loading_progress = progress;
    Redraw();
}

uint8_t FE_GetLoadingProgress(void) {
    return loading_progress;
}

/************************************************************/

/* Hogs of War's menu was designed
 * with a fixed resolution in mind
 * and scales poorly with anything
 * else. For now we'll just keep
 * things fixed and worry about this
 * later. */
#define FRONTEND_MENU_WIDTH     640
#define FRONTEND_MENU_HEIGHT    480

static void DrawLoadingScreen(void) {
    //int c_x = (GetUIViewportWidth() / 2) - FRONTEND_MENU_WIDTH / 2;
    //int c_y = (GetUIViewportHeight() / 2) - FRONTEND_MENU_HEIGHT / 2;
    plDrawTexturedRectangle(0, 0, FRONTEND_MENU_WIDTH, FRONTEND_MENU_HEIGHT, fe_background);

    /* originally I wrote some code ensuring the menu bar
     * was centered... that was until I found out that on
     * the background, the slot for the bar ISN'T centered
     * at all. JOY... */
    static const int bar_w = 330;
    int bar_x = 151; //c_x + (FRONTEND_MENU_WIDTH / 2) - bar_w / 2;
    int bar_y = 450;
    if(loading_progress > 0) {
        plDrawFilledRectangle(plCreateRectangle(
                PLVector2(bar_x, bar_y),
                PLVector2(((float) (bar_w) / 100) * loading_progress, 18),
                PL_COLOUR_INDIAN_RED,
                PL_COLOUR_INDIAN_RED,
                PL_COLOUR_RED,
                PL_COLOUR_RED
        ));
    }

    if(loading_description[0] != ' ' && loading_description[0] != '\0') {
        Font_DrawBitmapString(g_fonts[FONT_CHARS2], bar_x + 2, bar_y + 1, 4, 1.f, PL_COLOUR_WHITE, loading_description);
    }
}

void FE_Draw(void) {
    /* render and handle the main menu */
    if(frontend_state != FE_MODE_GAME) { // todo: what's going on here... ?
        //int c_x = (GetUIViewportWidth() / 2) - FRONTEND_MENU_WIDTH / 2;
        //int c_y = (GetUIViewportHeight() / 2) - FRONTEND_MENU_HEIGHT / 2;
        switch(frontend_state) {
            default:break;

            case FE_MODE_INIT: {
                /* here we display the background for the init once,
                 * taking advantage of the fact that we're not going
                 * to clear the buffer initially - this gives us an
                 * opportunity to unload the texture and load in the
                 * final one */
                static bool is_background_drawn = false;
                if(!is_background_drawn) {
                    plDrawTexturedRectangle(0, 0, FRONTEND_MENU_WIDTH, FRONTEND_MENU_HEIGHT, fe_background);
                    plDeleteTexture(fe_background, true);
                    is_background_drawn = true;
                }

                /* first we'll draw in a little rectangle representing
                 * the incomplete portion of the load */
                static bool is_load_drawn = false;
                if(!is_load_drawn) {
                    plSetBlendMode(PL_BLEND_DEFAULT);
                    plDrawFilledRectangle(plCreateRectangle(
                            PLVector2(0, 464),
                            PLVector2(FRONTEND_MENU_WIDTH, 16),
                            PLColour(0, 0, 0, 150),
                            PLColour(0, 0, 0, 150),
                            PLColour(0, 0, 0, 150),
                            PLColour(0, 0, 0, 150)
                    ));
                    plSetBlendMode(PL_BLEND_DISABLE);
                    is_load_drawn = true;
                }

                /* and now we're going to draw the loading bar. */
                plDrawFilledRectangle(plCreateRectangle(
                        PLVector2(0, 464),
                        PLVector2(loading_progress, 16),
                        PL_COLOUR_INDIAN_RED,
                        PL_COLOUR_INDIAN_RED,
                        PL_COLOUR_RED,
                        PL_COLOUR_RED
                ));
            } break;

            case FE_MODE_START: {
                plDrawTexturedRectangle(0, 0, FRONTEND_MENU_WIDTH, FRONTEND_MENU_HEIGHT, fe_background);

                /* this will probably be rewritten later on, but below we're setting up
                 * where each of the little blocks of text will be and then moving them
                 * up and down until the player presses an action (which is handled via
                 * our crappy callback) */

                int press_x = (FRONTEND_MENU_WIDTH / 2) - (180 / 2);
                int press_y = 435;
                /* todo, these should be bouncing... */
                plDrawTexturedRectangle(press_x, press_y, 62, 16, fe_press);
                plDrawTexturedRectangle(press_x += 70, press_y, 40, 16, fe_any);
                plDrawTexturedRectangle(press_x += 48, press_y - 4, 39, 20, fe_key);
            } break;

            case FE_MODE_MAIN_MENU: {
                plDrawTexturedRectangle(0, 0, FRONTEND_MENU_WIDTH, FRONTEND_MENU_HEIGHT, fe_background);
            } break;

            case FE_MODE_LOADING: {
                DrawLoadingScreen();
            } break;

            case FE_MODE_VIDEO: {
                Video_Draw();
            } break;
        }
    }
}

/* * * * * * * * * * * * * * * * * * * * * * */

void FE_RestoreLastState(void) {
    FE_SetState(old_frontend_state);
}

unsigned int FE_GetState(void) {
    return frontend_state;
}

void FE_SetState(unsigned int state) {
    if(state == frontend_state) {
        LogDebug("attempted to set debug state to an already existing state!\n");
        return;
    }

    if(frontend_state == FE_MODE_GAME && state != FE_MODE_GAME) {
        ClearFEGameData();
        CacheFEMenuData();
    } else if(frontend_state != FE_MODE_GAME && state == FE_MODE_GAME) {
        ClearFEMenuData();
        CacheFEGameData();
    }

    LogDebug("changing frontend state to %u...\n", state);
    switch(state) {
        default: {
            LogWarn("invalid frontend state, %u, aborting\n", state);
            return;
        }

        case FE_MODE_MAIN_MENU: {
            /* remove the textures we loaded in
             * for the start screen - we won't
             * be needing them again... */
            plDeleteTexture(fe_press, true);
            plDeleteTexture(fe_any, true);
            plDeleteTexture(fe_key, true);
            plDeleteTexture(fe_background, true);

            fe_background = Display_LoadTexture("fe/pigbkpc1", PL_TEXTURE_FILTER_LINEAR);
        } break;

        case FE_MODE_START: {

        } break;

        case FE_MODE_GAME: {

        } break;

        case FE_MODE_LOADING: {
            loading_description[0] = '\0';
            loading_progress = 0;
        } break;

        case FE_MODE_EDITOR: {

        } break;
    }
    old_frontend_state = frontend_state;
    frontend_state = state;
}

