#if DEBUG

#include "global.h"
#include "list_menu.h"
#include "main.h"
#include "map_name_popup.h"
#include "menu.h"
#include "new_menu_helpers.h"
#include "script.h"
#include "sound.h"
#include "strings.h"
#include "task.h"
#include "constants/songs.h"
#include "pokemon_storage_system.h"
#include "constants/species.h"
#include "constants/maps.h"
#include "overworld.h"
#include "field_fadetransition.h"
#include "pokedex.h"
#include "constants/flags.h"
#include "event_data.h"

#define DEBUG_MAIN_MENU_HEIGHT 7
#define DEBUG_MAIN_MENU_WIDTH 17

void Debug_ShowMainMenu(void);
static void Debug_DestroyMainMenu(u8);
static void DebugAction_Cancel(u8);
static void DebugTask_HandleMainMenuInput(u8);

static void DebugAction_LivingDex(u8);
static void DebugAction_PruneParty(u8);
static void DebugAction_WarpToPallet(u8);
static void DebugAction_CompletePokedex(u8);
static void DebugAction_UnlockAllMenus(u8);

enum {
    DEBUG_MENU_ITEM_CANCEL,
    DEBUG_MENU_ITEM_LIVINGDEX,
    DEBUG_MENU_ITEM_PRUNEPARTY,
    DEBUG_MENU_ITEM_WARPTOPALLET,
    DEBUG_MENU_ITEM_COMPLETEPOKEDEX,
    DEBUG_MENU_ITEM_UNLOCKALLMENUS,
};

static const u8 gDebugText_Cancel[] = _("Cancel");
static const u8 gDebugText_LivingDex[] = _("Living Dex");
static const u8 gDebugText_PruneParty[] = _("Prune Party");
static const u8 gDebugText_WarpToPallet[] = _("Warp to Pallet");
static const u8 gDebugText_CompletePokedex[] = _("Complete Pokédex");
static const u8 gDebugText_UnlockAllMenus[] = _("Unlock All Menus");

static const struct ListMenuItem sDebugMenuItems[] =
{
    [DEBUG_MENU_ITEM_CANCEL] = {gDebugText_Cancel, DEBUG_MENU_ITEM_CANCEL},
    [DEBUG_MENU_ITEM_LIVINGDEX] = {gDebugText_LivingDex, DEBUG_MENU_ITEM_LIVINGDEX},
    [DEBUG_MENU_ITEM_PRUNEPARTY] = {gDebugText_PruneParty, DEBUG_MENU_ITEM_PRUNEPARTY},
    [DEBUG_MENU_ITEM_WARPTOPALLET] = {gDebugText_WarpToPallet, DEBUG_MENU_ITEM_WARPTOPALLET},
    [DEBUG_MENU_ITEM_COMPLETEPOKEDEX] = {gDebugText_CompletePokedex, DEBUG_MENU_ITEM_COMPLETEPOKEDEX},
    [DEBUG_MENU_ITEM_UNLOCKALLMENUS] = {gDebugText_UnlockAllMenus, DEBUG_MENU_ITEM_UNLOCKALLMENUS},
};

static void (*const sDebugMenuActions[])(u8) =
{
    [DEBUG_MENU_ITEM_CANCEL] = DebugAction_Cancel,
    [DEBUG_MENU_ITEM_LIVINGDEX] = DebugAction_LivingDex,
    [DEBUG_MENU_ITEM_PRUNEPARTY] = DebugAction_PruneParty,
    [DEBUG_MENU_ITEM_WARPTOPALLET] = DebugAction_WarpToPallet,
    [DEBUG_MENU_ITEM_COMPLETEPOKEDEX] = DebugAction_CompletePokedex,
    [DEBUG_MENU_ITEM_UNLOCKALLMENUS] = DebugAction_UnlockAllMenus,
};

static const struct WindowTemplate sDebugMenuWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_MAIN_MENU_WIDTH,
    .height = 2 * DEBUG_MAIN_MENU_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 1,
};

static const struct ListMenuTemplate sDebugMenuListTemplate =
{
    .items = sDebugMenuItems,
    .moveCursorFunc = ListMenuDefaultCursorMoveFunc,
    .totalItems = ARRAY_COUNT(sDebugMenuItems),
    .maxShowed = DEBUG_MAIN_MENU_HEIGHT,
    .windowId = 0,
    .header_X = 0,
    .item_X = 8,
    .cursor_X = 0,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .itemVerticalPadding = 0,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = 1,
    .cursorKind = 0
};

void Debug_ShowMainMenu(void) {
    struct ListMenuTemplate menuTemplate;
    u8 windowId;
    u8 menuTaskId;
    u8 inputTaskId;

    // create window
    DismissMapNamePopup();
    LoadStdWindowFrameGfx();
    windowId = AddWindow(&sDebugMenuWindowTemplate);
    DrawStdWindowFrame(windowId, FALSE);

    // create list menu
    menuTemplate = sDebugMenuListTemplate;
    menuTemplate.windowId = windowId;
    menuTaskId = ListMenuInit(&menuTemplate, 0, 0);

    // draw everything
    CopyWindowToVram(windowId, 3);

    // create input handler task
    inputTaskId = CreateTask(DebugTask_HandleMainMenuInput, 3);
    gTasks[inputTaskId].data[0] = menuTaskId;
    gTasks[inputTaskId].data[1] = windowId;
}

static void Debug_DestroyMainMenu(u8 taskId)
{
    DestroyListMenuTask(gTasks[taskId].data[0], NULL, NULL);
    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);
    DestroyTask(taskId);
    EnableBothScriptContexts();
}

static void DebugTask_HandleMainMenuInput(u8 taskId)
{
    void (*func)(u8);
    u32 input = ListMenu_ProcessInput(gTasks[taskId].data[0]);

    if (gMain.newKeys & A_BUTTON)
    {
        PlaySE(SE_SELECT);
        if ((func = sDebugMenuActions[input]) != NULL)
            func(taskId);
    }
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        Debug_DestroyMainMenu(taskId);
    }
}

static void DebugAction_Cancel(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);
}

// Populates boxes with a living dex based on the level of your first party mon.
static void DebugAction_LivingDex(u8 taskId)
{
    u8 level = 0;
    u16 species = 1;
    u8 obedience = TRUE;
    u8 box = 0;
    u8 pos = 0;
    
    Debug_DestroyMainMenu(taskId);
    
    level = GetMonData(&gPlayerParty[0], MON_DATA_LEVEL, NULL);
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            if (species < SPECIES_EGG)
            {
                CreateBoxMonAt(box, pos, species, level, 30, FALSE, 0, 
                    OT_ID_PLAYER_ID, 0);
                SetBoxMonDataAt(box, pos, MON_DATA_EVENT_LEGAL, &obedience);
            }
            else
                ZeroBoxMonAt(box, pos);
            species++;
            if (species == SPECIES_CELEBI + 1)
                species = SPECIES_TREECKO;
        }
    }
    PlaySE(SE_SAVE);
}

// Deletes party mons except the first
static void DebugAction_PruneParty(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);
    
    ZeroMonData(&gPlayerParty[1]);
    ZeroMonData(&gPlayerParty[2]);
    ZeroMonData(&gPlayerParty[3]);
    ZeroMonData(&gPlayerParty[4]);
    ZeroMonData(&gPlayerParty[5]);
    gSaveBlock1Ptr->playerPartyCount = 1;
    PlaySE(SE_PC_OFF);
}

static void DebugAction_WarpToPallet(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);

    SetWarpDestination(MAP_GROUP(PALLET_TOWN), MAP_NUM(PALLET_TOWN), -1, 3, 6);
    DoTeleportWarp();
}

static void DebugAction_CompletePokedex(u8 taskId)
{
    u16 i;

    Debug_DestroyMainMenu(taskId);

    for (i = 0; i < NATIONAL_DEX_COUNT + 1; i++)
    {
        GetSetPokedexFlag(i, FLAG_SET_SEEN);
        GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
    }
    gSaveBlock2Ptr->pokedex.unownPersonality = 0xDEADBEEF;
    gSaveBlock2Ptr->pokedex.spindaPersonality = 0xDEADBEEF;
    PlaySE(SE_SAVE);
}

static void DebugAction_UnlockAllMenus(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);

    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKEDEX_GET);
}

#endif
