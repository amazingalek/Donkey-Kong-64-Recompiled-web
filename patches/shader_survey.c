#include "common_structs.h"

// The shader survey: lets a script take the game to any map by itself, to record the shaders each world draws for
// the page's prebuilt list (src/rt64_shaders.txt, made by tools/record_shaders.sh) without anyone playing. It does
// nothing unless the script asks.
//
// The script finds this struct in RDRAM by its two magic words and writes a map into `map`, with its exit in `exit`
// and a Kong in `kong` (0 DK, 1 Diddy, 2 Lanky, 3 Tiny, 4 Chunky; -1 keeps the one playing) first; `loaded` counts
// the requests taken, and `current_map` and `game_mode` are the game's, copied here every frame, so the script can
// tell when the map has loaded. The game's main loop (func_global_asm_805FBFF4, boot_logos_patches.c) calls
// shader_survey_frame once a frame, between frames, so a request is taken there:
// A request waits for the DK Rap, DK TV, the main menu or adventure mode, or 3 seconds into the logos.
// - MAP_MAIN_MENU goes to the title screen and main menu, as the game does after the DK Rap or Quit Game.
// - MAP_DK_RAP plays the DK Rap, as Start does in the logos.
// - In adventure mode, a map is warped to as a loading zone does (func_global_asm_805FF378, the debug warp menu's).
// - Anywhere else (the DK Rap, DK TV, the main menu) the game is started on file 3 as the file select's Start does
//   (func_global_asm_807144B8), but into the map asked for, so whatever plays on a first visit plays. File 1 is left
//   to the plan's new game, which goes through the menus as a player does.

void func_global_asm_806C9AE0(void);
void func_global_asm_80731030(void); // clearTemporaryFlags()
void func_global_asm_8060DC3C(u8 fileIndex, s32 arg1);
void func_global_asm_805FF4D8(Maps map, s32 exit);
void func_global_asm_805FF378(Maps nextMap, s32 nextExit);
void func_global_asm_80712EDC(void);
void func_global_asm_807124B8(Maps newMap, s32 cutsceneIndex, u8 newGameMode);
extern s32 D_global_asm_80755334; // frames since the game mode started, up to 0x5A
extern Maps current_map;
extern u8 current_character_index[];
extern u8 game_mode;
extern u8 game_mode_copy;

struct ShaderSurvey {
    u32 magic[2];
    volatile u32 loaded;
    volatile s32 map;
    volatile s32 exit;
    volatile s32 kong;
    volatile s32 current_map;
    volatile s32 game_mode;
};

struct ShaderSurvey shader_survey = { { 0x53484452, 0x53525659 }, 0, -1, 0, -1, -1, -1 }; // "SHDR" "SRVY"

void shader_survey_frame(void) {
    s32 map, exit, kong;

    shader_survey.current_map = current_map;
    shader_survey.game_mode = game_mode_copy;
    // Not while the game is changing modes, and in the logos only the main menu and the DK Rap, once the logos have
    // run 3 seconds, when Start would skip them (before that, the main menu faults).
    if ((shader_survey.map < 0) || (game_mode != game_mode_copy)) {
        return;
    }
    if ((game_mode_copy == GAME_MODE_NINTENDO_LOGO) || (game_mode_copy == GAME_MODE_OPENING_CUTSCENE)) {
        if ((D_global_asm_80755334 < 0x5A) || ((shader_survey.map != MAP_MAIN_MENU) && (shader_survey.map != MAP_DK_RAP))) {
            return;
        }
    } else if ((game_mode_copy != GAME_MODE_DK_RAP) && (game_mode_copy != GAME_MODE_DK_TV) &&
               (game_mode_copy != GAME_MODE_MAIN_MENU) && (game_mode_copy != GAME_MODE_ADVENTURE)) {
        return;
    }
    map = shader_survey.map;
    exit = shader_survey.exit;
    kong = shader_survey.kong;
    shader_survey.map = -1;

    if (map == MAP_MAIN_MENU) {
        func_global_asm_80712EDC();
    } else if (map == MAP_DK_RAP) {
        func_global_asm_807124B8(MAP_DK_RAP, 0, GAME_MODE_DK_RAP);
    } else if (game_mode_copy == GAME_MODE_ADVENTURE) {
        if (kong >= 0) {
            current_character_index[0] = kong;
        }
        func_global_asm_805FF378(map, exit);
    } else {
        func_global_asm_806C9AE0();
        func_global_asm_80731030();
        func_global_asm_8060DC3C(2, 1);
        current_character_index[0] = (kong >= 0) ? kong : 0;
        func_global_asm_805FF4D8(map, exit);
        game_mode = GAME_MODE_ADVENTURE;
    }
    shader_survey.loaded++;
}
