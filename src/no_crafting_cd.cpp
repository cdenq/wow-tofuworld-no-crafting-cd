/*
 * wow-tofuworld-no-crafting-cd
 *
 * Profession craft cooldowns are removed on the server by this module's SQL (data/sql/db-world/updates), which sets RecoveryTime and CategoryRecoveryTime to 0 in spell_cooldown_overrides.
 * The 3.3.5a client still starts the cooldown from its own Spell.dbc when it sees the cast (SMSG_SPELL_GO), so the button greys out until relog even though the server allows the craft.
 * This script fixes the display: after a player casts a spell whose override is 0, it sends SMSG_CLEAR_COOLDOWN for that spell and for every spell the player knows in the same spell category whose override is also 0 (the transmutes share one category).
 */

#include "AllSpellScript.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

namespace
{
    bool sEnabled = true;
    bool sAnnounce = true;

    // True when the spell has a spell_cooldown_overrides row with no cooldown and no category cooldown.
    bool HasZeroCooldownOverride(uint32 spellId)
    {
        if (!sSpellMgr->HasSpellCooldownOverride(spellId))
            return false;

        SpellCooldownOverride const cooldown = sSpellMgr->GetSpellCooldownOverride(spellId);
        return cooldown.RecoveryTime == 0 && cooldown.CategoryRecoveryTime == 0;
    }

    void ClearClientCooldowns(Player* player, uint32 spellId)
    {
        player->SendClearCooldown(spellId, player);

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            return;

        uint32 const category = spellInfo->GetCategory();
        if (!category)
            return;

        auto const itr = sSpellsByCategoryStore.find(category);
        if (itr == sSpellsByCategoryStore.end())
            return;

        // Each entry is (isItemSpell, spellId). Only the player's own known spells are cleared.
        for (auto const& entry : itr->second)
        {
            if (entry.first)
                continue;

            uint32 const siblingId = entry.second;
            if (siblingId == spellId)
                continue;

            if (!player->HasSpell(siblingId))
                continue;

            if (!HasZeroCooldownOverride(siblingId))
                continue;

            player->SendClearCooldown(siblingId, player);
        }
    }
}

class NoCraftingCd_World : public WorldScript
{
public:
    NoCraftingCd_World() : WorldScript("NoCraftingCd_World", { WORLDHOOK_ON_AFTER_CONFIG_LOAD }) { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        sEnabled = sConfigMgr->GetOption<bool>("NoCraftingCd.Enable", true);
        sAnnounce = sConfigMgr->GetOption<bool>("NoCraftingCd.Announce", true);
        LOG_INFO("module", "wow-tofuworld-no-crafting-cd: Enable = {}, Announce = {}", sEnabled, sAnnounce);
    }
};

class NoCraftingCd_Player : public PlayerScript
{
public:
    NoCraftingCd_Player() : PlayerScript("NoCraftingCd_Player", { PLAYERHOOK_ON_LOGIN }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!sEnabled || !sAnnounce)
            return;

        ChatHandler(player->GetSession()).PSendSysMessage("|cff00ff00[No Crafting CD]|r Module active. Profession crafts such as transmutes have no cooldown.");
    }
};

class NoCraftingCd_AllSpell : public AllSpellScript
{
public:
    NoCraftingCd_AllSpell() : AllSpellScript("NoCraftingCd_AllSpell", { ALLSPELLHOOK_ON_CAST }) { }

    // Runs after the core has sent SMSG_SPELL_GO for the cast.
    void OnSpellCast(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        if (!sEnabled || !caster || !spellInfo)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        uint32 const spellId = spellInfo->Id;
        if (!HasZeroCooldownOverride(spellId))
            return;

        // Clear a moment later, after the client has applied its own cooldown. Look the player up again by GUID, since they could log out in between.
        ObjectGuid const guid = player->GetGUID();
        player->m_Events.AddEventAtOffset([guid, spellId]()
        {
            if (Player* target = ObjectAccessor::FindConnectedPlayer(guid))
                ClearClientCooldowns(target, spellId);
        }, 150ms);
    }
};

void AddSC_no_crafting_cd()
{
    new NoCraftingCd_World();
    new NoCraftingCd_Player();
    new NoCraftingCd_AllSpell();
}
