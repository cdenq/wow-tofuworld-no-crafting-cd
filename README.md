# wow-tofuworld-no-crafting-cd

AzerothCore module for the Tofu Worldview server (mod-playerbots fork of azerothcore-wotlk, WotLK 3.3.5a). It removes profession craft cooldowns and keeps the client's cooldown display in step.

## What it does

- SQL (`data/sql/db-world/updates/tofu_2026_11_18_00_no_crafting_cd.sql`, applied by the worldserver's updater as a module file): sets `RecoveryTime` and `CategoryRecoveryTime` to 0 in `spell_cooldown_overrides` for 56 craft spells (transmutes, Mooncloth and the TBC specialty cloths, alchemy and inscription research, Brilliant Glass, Icy Prism, Prismatic and Void Sphere, Glacial Bag) plus the Salt Shaker spell, and zeros the Salt Shaker item cooldown. 57 override rows in total.
- C++ (`src/no_crafting_cd.cpp`): the 3.3.5a client starts each craft's cooldown from its own Spell.dbc, so without help the button stays grey until relog. After such a cast, the module sends `SMSG_CLEAR_COOLDOWN` for the spell and for every known spell in the same spell category whose override is also 0.
- Login message, switchable in the conf.

The spell list comes from VenomekPL/mod-profession-craft-cd at commit d9dc8a1 (MIT), which this module replaces. That module's newer client fix calls a core function that the mod-playerbots fork does not have.

## Install

1. Copy this repo folder into the deploy set `WoW Projects\modules` and run `Update Source.bat` (answer Y to Rebuild anyway).
2. Copy `no_crafting_cd.conf.dist` to `no_crafting_cd.conf` in `C:\Build\bin\RelWithDebInfo\configs\modules`.
3. Start the servers.

## Configuration

| Key | Default | Meaning |
|---|---|---|
| `NoCraftingCd.Enable` | 1 | Clear the client cooldown display after a craft |
| `NoCraftingCd.Announce` | 1 | Login message |

## Check

- SQL (acore_world), expect 57: `SELECT COUNT(*) FROM spell_cooldown_overrides WHERE Comment LIKE 'Profession craft CD removed%';`
- In game: cast a transmute twice in a row without relogging; the second cast works and the button never greys out.
