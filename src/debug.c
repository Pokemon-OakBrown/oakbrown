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
#include "script.h"
#include "event_scripts.h"
#include "string_util.h"
#include "librfu.h"
#include "printf.h"
#include "mgba.h"

#define DEBUG_MAIN_MENU_HEIGHT 8
#define DEBUG_MAIN_MENU_WIDTH 17
#define DEBUG_NUMBER_DISPLAY_WIDTH 10
#define DEBUG_NUMBER_DISPLAY_HEIGHT 4
#define DEBUG_NUMBER_DIGITS_FLAGS 4

void Debug_ShowMainMenu(void);
static void Debug_DestroyMainMenu(u8);
static void DebugAction_Cancel(u8);
static void DebugTask_HandleMainMenuInput(u8);

static void DebugAction_LivingDex(u8);
static void DebugAction_PruneParty(u8);
static void DebugAction_WarpZone(u8);
static void DebugAction_CompletePokedex(u8);
static void DebugAction_UnlockAllQuests(u8);
static void DebugAction_AccessPC(u8);
static void DebugAction_Flags_Flags(u8 taskId);
static void DebugAction_Flags_FlagsSelect(u8 taskId);

static const u8 gDebugText_Flags[] = _("Flags");
static const u8 gDebugText_Flags_Flag[] =                   _("Flag: {STR_VAR_1}   \n{STR_VAR_2}                   \n{STR_VAR_3}");
static const u8 gDebugText_Flags_FlagHex[] =                _("{STR_VAR_1}           \n0x{STR_VAR_2}             ");
static const u8 gDebugText_Flags_FlagSet[] =                _("TRUE");
static const u8 gDebugText_Flags_FlagUnset[] =              _("FALSE");
static const u8 digitInidicator_1[] =               _("{LEFT_ARROW}+0x1{RIGHT_ARROW}        ");
static const u8 digitInidicator_10[] =              _("{LEFT_ARROW}+0x10{RIGHT_ARROW}       ");
static const u8 digitInidicator_100[] =             _("{LEFT_ARROW}+0x100{RIGHT_ARROW}      ");
static const u8 digitInidicator_1000[] =            _("{LEFT_ARROW}+0x1000{RIGHT_ARROW}     ");
const u8 * const gText_DigitIndicator[] =
{
    digitInidicator_1,
    digitInidicator_10,
    digitInidicator_100,
    digitInidicator_1000,
};

enum {
    DEBUG_MENU_ITEM_CANCEL,
    DEBUG_MENU_ITEM_LIVINGDEX,
    DEBUG_MENU_ITEM_PRUNEPARTY,
    DEBUG_MENU_ITEM_WARPZONE,
    DEBUG_MENU_ITEM_COMPLETEPOKEDEX,
    DEBUG_MENU_ITEM_ACCESSPC,
    DEBUG_MENU_ITEM_FLAGS,
};

static const u8 gDebugText_Cancel[] = _("Cancel");
static const u8 gDebugText_LivingDex[] = _("Living Dex");
static const u8 gDebugText_PruneParty[] = _("Prune Party");
static const u8 gDebugText_WarpZone[] = _("Warp Zone");
static const u8 gDebugText_CompletePokedex[] = _("Complete Pokédex");
static const u8 gDebugText_AccessPC[] = _("Access PC");

static const struct ListMenuItem sDebugMenuItems[] =
{
    [DEBUG_MENU_ITEM_CANCEL] = {gDebugText_Cancel, DEBUG_MENU_ITEM_CANCEL},
    [DEBUG_MENU_ITEM_LIVINGDEX] = {gDebugText_LivingDex, DEBUG_MENU_ITEM_LIVINGDEX},
    [DEBUG_MENU_ITEM_PRUNEPARTY] = {gDebugText_PruneParty, DEBUG_MENU_ITEM_PRUNEPARTY},
    [DEBUG_MENU_ITEM_WARPZONE] = {gDebugText_WarpZone, DEBUG_MENU_ITEM_WARPZONE},
    [DEBUG_MENU_ITEM_COMPLETEPOKEDEX] = {gDebugText_CompletePokedex, DEBUG_MENU_ITEM_COMPLETEPOKEDEX},
    [DEBUG_MENU_ITEM_ACCESSPC] = {gDebugText_AccessPC, DEBUG_MENU_ITEM_ACCESSPC},
	[DEBUG_MENU_ITEM_FLAGS] = {gDebugText_Flags, DEBUG_MENU_ITEM_FLAGS},
};

static void (*const sDebugMenuActions[])(u8) =
{
    [DEBUG_MENU_ITEM_CANCEL] = DebugAction_Cancel,
    [DEBUG_MENU_ITEM_LIVINGDEX] = DebugAction_LivingDex,
    [DEBUG_MENU_ITEM_PRUNEPARTY] = DebugAction_PruneParty,
    [DEBUG_MENU_ITEM_WARPZONE] = DebugAction_WarpZone,
    [DEBUG_MENU_ITEM_COMPLETEPOKEDEX] = DebugAction_CompletePokedex,
    [DEBUG_MENU_ITEM_ACCESSPC] = DebugAction_AccessPC,
	[DEBUG_MENU_ITEM_FLAGS] = DebugAction_Flags_Flags,
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
    .totalItems = NELEMS(sDebugMenuItems),
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
    FlagSet(FLAG_SYS_POKEMON_GET);
    
    level = GetMonData(&gPlayerParty[0], MON_DATA_LEVEL, NULL);
    if (level < 1)
        level = 1;
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

static void DebugAction_WarpZone(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);

    SetWarpDestination(MAP_GROUP(DEBUG_WARP_ZONE), MAP_NUM(DEBUG_WARP_ZONE), -1, 4, 12);
    DoTeleportWarp();
}

static void DebugAction_CompletePokedex(u8 taskId)
{
    u16 i;

    Debug_DestroyMainMenu(taskId);
    FlagSet(FLAG_SYS_POKEDEX_GET);

    for (i = 0; i < NATIONAL_DEX_COUNT + 1; i++)
    {
        GetSetPokedexFlag(i, FLAG_SET_SEEN);
        GetSetPokedexFlag(i, FLAG_SET_CAUGHT);
    }
    PlaySE(SE_SAVE);
}

static void DebugAction_AccessPC(u8 taskId)
{
    Debug_DestroyMainMenu(taskId);

    ScriptContext1_SetupScript(EventScript_PCDebug);
    ScriptContext2_Enable();
}

static const s32 sPowerOfSixteen[] =
{
             1,
            0x10,
           0x100,
          0x1000,
};

static const struct WindowTemplate sDebugNumberDisplayWindowTemplate =
{
    .bg = 0,
    .tilemapLeft = 1,
    .tilemapTop = 1,
    .width = DEBUG_NUMBER_DISPLAY_WIDTH,
    .height = 2 * DEBUG_NUMBER_DISPLAY_HEIGHT,
    .paletteNum = 15,
    .baseBlock = 1,
};

static void DebugAction_DestroyExtraWindow(u8 taskId)
{
    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    ClearStdWindowAndFrame(gTasks[taskId].data[2], TRUE);
    RemoveWindow(gTasks[taskId].data[2]);

    DestroyTask(taskId);
    EnableBothScriptContexts();
}

static void DebugAction_Flags_Flags(u8 taskId)
{
    u8 windowId;

    ClearStdWindowAndFrame(gTasks[taskId].data[1], TRUE);
    RemoveWindow(gTasks[taskId].data[1]);

    DismissMapNamePopup();
    LoadStdWindowFrameGfx();
    windowId = AddWindow(&sDebugNumberDisplayWindowTemplate);
    DrawStdWindowFrame(windowId, FALSE);

    CopyWindowToVram(windowId, 3);

    //Display initial Flag
    ConvertIntToDecimalStringN(gStringVar1, 0, STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_FLAGS);
    ConvertIntToHexStringN(gStringVar2, 0, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringExpandPlaceholders(gStringVar1, gDebugText_Flags_FlagHex);
    if(FlagGet(0) == TRUE)
        StringCopyPadded(gStringVar2, gDebugText_Flags_FlagSet, CHAR_SPACE, 15);
    else
        StringCopyPadded(gStringVar2, gDebugText_Flags_FlagUnset, CHAR_SPACE, 15);
    StringCopy(gStringVar3, gText_DigitIndicator[0]);
    StringExpandPlaceholders(gStringVar4, gDebugText_Flags_Flag);
    AddTextPrinterParameterized(windowId, 1, gStringVar4, 1, 1, 0, NULL);

    gTasks[taskId].func = DebugAction_Flags_FlagsSelect;
    gTasks[taskId].data[2] = windowId;
    gTasks[taskId].data[3] = 0;            //Current Flag
    gTasks[taskId].data[4] = 0;            //Digit Selected
}

static void DebugAction_Flags_FlagsSelect(u8 taskId)
{
    if (gMain.newKeys & A_BUTTON)
        FlagToggle(gTasks[taskId].data[3]);
    else if (gMain.newKeys & B_BUTTON)
    {
        PlaySE(SE_SELECT);
        DebugAction_DestroyExtraWindow(taskId);
        return;
    }

    if(gMain.newAndRepeatedKeys & DPAD_UP)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[3] += sPowerOfSixteen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] >= FLAGS_COUNT){
            gTasks[taskId].data[3] = FLAGS_COUNT - 1;
        }
    }
    if(gMain.newAndRepeatedKeys & DPAD_DOWN)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[3] -= sPowerOfSixteen[gTasks[taskId].data[4]];
        if(gTasks[taskId].data[3] < 0){
            gTasks[taskId].data[3] = 0;
        }
    }
    if(gMain.newKeys & DPAD_LEFT)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[4] -= 1;
        if(gTasks[taskId].data[4] < 0)
        {
            gTasks[taskId].data[4] = 0;
        }
    }
    if(gMain.newKeys & DPAD_RIGHT)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId].data[4] += 1;
        if(gTasks[taskId].data[4] > DEBUG_NUMBER_DIGITS_FLAGS-1)
        {
            gTasks[taskId].data[4] = DEBUG_NUMBER_DIGITS_FLAGS-1;
        }
    }

    if (gMain.newKeys & DPAD_ANY || gMain.newKeys & A_BUTTON)
    {
        ConvertIntToDecimalStringN(gStringVar1, gTasks[taskId].data[3], STR_CONV_MODE_LEADING_ZEROS, DEBUG_NUMBER_DIGITS_FLAGS);
        ConvertIntToHexStringN(gStringVar2, gTasks[taskId].data[3], STR_CONV_MODE_LEFT_ALIGN, 3);
        StringExpandPlaceholders(gStringVar1, gDebugText_Flags_FlagHex);
        if(FlagGet(gTasks[taskId].data[3]) == TRUE)
            StringCopyPadded(gStringVar2, gDebugText_Flags_FlagSet, CHAR_SPACE, 15);
        else
            StringCopyPadded(gStringVar2, gDebugText_Flags_FlagUnset, CHAR_SPACE, 15);
        StringCopy(gStringVar3, gText_DigitIndicator[gTasks[taskId].data[4]]);
        StringExpandPlaceholders(gStringVar4, gDebugText_Flags_Flag);
        AddTextPrinterParameterized(gTasks[taskId].data[2], 1, gStringVar4, 1, 1, 0, NULL);
    }
}

#endif // DEBUG
