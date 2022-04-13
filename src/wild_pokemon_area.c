#include "global.h"
#include "field_specials.h"
#include "event_data.h"
#include "wild_encounter.h"
#include "roamer.h"
#include "overworld.h"
#include "pokedex_area_markers.h"
#include "constants/region_map_sections.h"

struct SeviiDexArea
{
    const u16 (*lut)[2];
    s32 count;
};

struct RoamerPair
{
    u16 roamer;
    u16 starter;
};

static s32 GetRoamerIndex(u16 species);
static s32 CountRoamerNests(u16 species, struct Subsprite * subsprites);
static bool32 PokemonInAnyEncounterTableInMap(const struct WildPokemonHeader * data, s32 species);
static bool32 PokemonInEncounterTable(const struct WildPokemonInfo * pokemon, s32 species, s32 count);
static u16 GetMapSecIdFromWildMonHeader(const struct WildPokemonHeader * header);
static bool32 TryGetMapSecPokedexAreaEntry(u16 mapSecId, const u16 (*lut)[2], s32 count, s32 * lutIdx_p, u16 * tableIdx_p);

static const u16 sDexAreas_Kanto[][2] = {
    {}
};

static const u16 sDexAreas_Sevii1[][2] = {
	{}    
};

static const u16 sDexAreas_Sevii2[][2] = {
	{}
};

static const u16 sDexAreas_Sevii3[][2] = {
	{}   
};

static const u16 sDexAreas_Sevii4[][2] = {
	{}
};

static const u16 sDexAreas_Sevii5[][2] = {
	{}
};

static const u16 sDexAreas_Sevii6[][2] = {
	{}
};

static const u16 sDexAreas_Sevii7[][2] = {
	{}   
};

static const struct SeviiDexArea sSeviiDexAreas[] = {
    {}
};

static const struct RoamerPair sRoamerPairs[] = {
    { SPECIES_ENTEI,   SPECIES_BULBASAUR  },
    { SPECIES_SUICUNE, SPECIES_CHARMANDER },
    { SPECIES_RAIKOU,  SPECIES_SQUIRTLE   }
};

s32 BuildPokedexAreaSubspriteBuffer(u16 species, struct Subsprite * subsprites)
{
    s32 areaCount;
    s32 j;
    s32 mapSecId;
    u16 dexAreaSubspriteIdx;
    s32 dexAreaEntryLUTidx;
    s32 seviiAreas;
    s32 alteringCaveCount;
    s32 alteringCaveNum;
    s32 i;

    if (GetRoamerIndex(species) >= SPECIES_NONE)
    {
        return CountRoamerNests(species, subsprites);
    }

    seviiAreas = GetUnlockedSeviiAreas();
    alteringCaveCount = 0;
    alteringCaveNum = VarGet(VAR_ALTERING_CAVE_WILD_SET);
    if (alteringCaveNum > 8)
        alteringCaveNum = 0;
    for (i = 0, areaCount = 0; gWildMonHeaders[i].mapGroup != 0xFF; i++)
    {
        mapSecId = GetMapSecIdFromWildMonHeader(&gWildMonHeaders[i]);
        if (mapSecId == MAPSEC_PLACEHOLDER)
        {
            alteringCaveCount++;
            if (alteringCaveNum != alteringCaveCount - 1)
                continue;
        }
        if (PokemonInAnyEncounterTableInMap(&gWildMonHeaders[i], species))
        {
            dexAreaEntryLUTidx = 0;
            while (TryGetMapSecPokedexAreaEntry(mapSecId, sDexAreas_Kanto, 55, &dexAreaEntryLUTidx, &dexAreaSubspriteIdx))
            {
                if (dexAreaSubspriteIdx != 0)
                {
                    SetAreaSubsprite(areaCount++, dexAreaSubspriteIdx, subsprites);
                }
            }
            for (j = 0; j < NELEMS(sSeviiDexAreas); j++)
            {
                if ((seviiAreas >> j) & 1)
                {
                    dexAreaEntryLUTidx = 0;
                    while (TryGetMapSecPokedexAreaEntry(mapSecId, sSeviiDexAreas[j].lut, sSeviiDexAreas[j].count, &dexAreaEntryLUTidx, &dexAreaSubspriteIdx))
                    {
                        if (dexAreaSubspriteIdx != 0)
                        {
                            SetAreaSubsprite(areaCount++, dexAreaSubspriteIdx, subsprites);
                        }
                    }
                }
            }
        }
    }

    return areaCount;
}

static s32 GetRoamerIndex(u16 species)
{
    s32 i;
    for (i = 0; i < NELEMS(sRoamerPairs); i++)
    {
        if (sRoamerPairs[i].roamer == species)
            return i;
    }

    return -1;
}

static s32 CountRoamerNests(u16 species, struct Subsprite * subsprites)
{
    u16 roamerLocation;
    s32 roamerIdx;
    u16 dexAreaSubspriteIdx;
    s32 dexAreaEntryLUTidx;

    roamerIdx = GetRoamerIndex(species);
    if (roamerIdx < 0)
        return 0;
    if (sRoamerPairs[roamerIdx].starter != GetStarterSpecies())
        return 0;
    roamerLocation = GetRoamerLocationMapSectionId();
    dexAreaEntryLUTidx = 0;
    if (TryGetMapSecPokedexAreaEntry(roamerLocation, sDexAreas_Kanto, 55, &dexAreaEntryLUTidx, &dexAreaSubspriteIdx))
    {
        if (dexAreaSubspriteIdx != 0)
        {
            SetAreaSubsprite(0, dexAreaSubspriteIdx, subsprites);
            return 1;
        }
    }
    return 0;
}

static bool32 PokemonInAnyEncounterTableInMap(const struct WildPokemonHeader * data, s32 species)
{
    if (PokemonInEncounterTable(data->landMonsInfo, species, 12))
        return TRUE;
    if (PokemonInEncounterTable(data->waterMonsInfo, species, 5))
        return TRUE;
    if (PokemonInEncounterTable(data->fishingMonsInfo, species, 12)) // 10
        return TRUE;
    if (PokemonInEncounterTable(data->rockSmashMonsInfo, species, 5))
        return TRUE;

    return FALSE;
}

static bool32 PokemonInEncounterTable(const struct WildPokemonInfo * info, s32 species, s32 count)
{
    s32 i;
    if (info != NULL)
    {
        for (i = 0; i < count; i++)
        {
            if (info->wildPokemon[i].species == species)
                return TRUE;
        }
    }
    return FALSE;
}

static u16 GetMapSecIdFromWildMonHeader(const struct WildPokemonHeader * header)
{
    return Overworld_GetMapHeaderByGroupAndId(header->mapGroup, header->mapNum)->regionMapSectionId;
}

static bool32 TryGetMapSecPokedexAreaEntry(u16 mapSecId, const u16 (*lut)[2], s32 count, s32 * lutIdx_p, u16 * tableIdx_p)
{
    s32 i;
    for (i = *lutIdx_p; i < count; i++)
    {
        if (lut[i][0] == mapSecId)
        {
            *tableIdx_p = lut[i][1];
            *lutIdx_p = i + 1;
            return TRUE;
        }
    }
    return FALSE;
}
