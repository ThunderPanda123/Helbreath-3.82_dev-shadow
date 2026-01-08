#include "GameConfigSqliteStore.h"

#define _WINSOCKAPI_
#include <windows.h>
#include <cstdio>
#include <cstring>

#include "Item.h"
#include "BuildItem.h"
#include "Game.h"
#include "Magic.h"
#include "Npc.h"
#include "Portion.h"
#include "Quest.h"
#include "Skill.h"
#include "sqlite3.h"

extern void PutLogList(char* cMsg);
extern void PutLogListLevel(int level, const char* cMsg);

namespace
{
    bool ExecSql(sqlite3* db, const char* sql)
    {
        char* err = nullptr;
        int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
        if (rc != SQLITE_OK) {
            char logMsg[512] = {};
            std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Exec failed: %s", err ? err : "unknown");
            PutLogList(logMsg);
            sqlite3_free(err);
            return false;
        }
        return true;
    }

    bool BeginTransaction(sqlite3* db)
    {
        return ExecSql(db, "BEGIN;");
    }

    bool CommitTransaction(sqlite3* db)
    {
        return ExecSql(db, "COMMIT;");
    }

    bool RollbackTransaction(sqlite3* db)
    {
        return ExecSql(db, "ROLLBACK;");
    }

    bool ClearTable(sqlite3* db, const char* tableName)
    {
        char sql[256] = {};
        std::snprintf(sql, sizeof(sql), "DELETE FROM %s;", tableName);
        return ExecSql(db, sql);
    }

    bool PrepareAndBindText(sqlite3_stmt* stmt, int idx, const char* value)
    {
        return sqlite3_bind_text(stmt, idx, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
    }

    bool HasColumn(sqlite3* db, const char* tableName, const char* columnName)
    {
        if (db == nullptr || tableName == nullptr || columnName == nullptr) {
            return false;
        }
        char sql[256] = {};
        std::snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", tableName);
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        bool found = false;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* name = sqlite3_column_text(stmt, 1);
            if (name != nullptr && std::strcmp(reinterpret_cast<const char*>(name), columnName) == 0) {
                found = true;
                break;
            }
        }
        sqlite3_finalize(stmt);
        return found;
    }

    void CopyColumnText(sqlite3_stmt* stmt, int col, char* dest, size_t destSize)
    {
        const unsigned char* text = sqlite3_column_text(stmt, col);
        if (text == nullptr) {
            if (destSize > 0) {
                dest[0] = 0;
            }
            return;
        }
        std::snprintf(dest, destSize, "%s", reinterpret_cast<const char*>(text));
    }

    bool InsertKeyValue(sqlite3_stmt* stmt, const char* key, const char* value)
    {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        bool ok = true;
        ok &= PrepareAndBindText(stmt, 1, key);
        ok &= PrepareAndBindText(stmt, 2, value);
        if (!ok) {
            return false;
        }
        return sqlite3_step(stmt) == SQLITE_DONE;
    }

    bool InsertKeyValueInt(sqlite3_stmt* stmt, const char* key, int value)
    {
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        bool ok = true;
        ok &= PrepareAndBindText(stmt, 1, key);
        ok &= (sqlite3_bind_int(stmt, 2, value) == SQLITE_OK);
        if (!ok) {
            return false;
        }
        return sqlite3_step(stmt) == SQLITE_DONE;
    }
}

bool EnsureGameConfigDatabase(sqlite3** outDb, std::string& outPath, bool* outCreated)
{
    if (outDb == nullptr) {
        return false;
    }

    std::string dbPath = "GameConfigs.db";
    DWORD attrs = GetFileAttributes(dbPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        char modulePath[MAX_PATH] = {};
        DWORD len = GetModuleFileNameA(nullptr, modulePath, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            char* lastSlash = strrchr(modulePath, '\\');
            if (lastSlash != nullptr) {
                *(lastSlash + 1) = '\0';
                dbPath.assign(modulePath);
                dbPath.append("GameConfigs.db");
            }
        }
    }
    outPath = dbPath;

    bool created = false;
    attrs = GetFileAttributes(dbPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        created = true;
    }

    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        char logMsg[512] = {};
        std::snprintf(logMsg, sizeof(logMsg), "(SQLITE) Open failed: %s", sqlite3_errmsg(db));
        PutLogList(logMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_busy_timeout(db, 1000);
    if (!ExecSql(db, "PRAGMA foreign_keys = ON;")) {
        sqlite3_close(db);
        return false;
    }

    const char* schemaSql =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS meta ("
        " key TEXT PRIMARY KEY,"
        " value TEXT NOT NULL"
        ");"
        "INSERT OR REPLACE INTO meta(key, value) VALUES('schema_version','3');"
        "CREATE TABLE IF NOT EXISTS items ("
        " item_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " item_type INTEGER NOT NULL,"
        " equip_pos INTEGER NOT NULL,"
        " item_effect_type INTEGER NOT NULL,"
        " item_effect_value1 INTEGER NOT NULL,"
        " item_effect_value2 INTEGER NOT NULL,"
        " item_effect_value3 INTEGER NOT NULL,"
        " item_effect_value4 INTEGER NOT NULL,"
        " item_effect_value5 INTEGER NOT NULL,"
        " item_effect_value6 INTEGER NOT NULL,"
        " max_lifespan INTEGER NOT NULL,"
        " special_effect INTEGER NOT NULL,"
        " sprite INTEGER NOT NULL,"
        " sprite_frame INTEGER NOT NULL,"
        " price INTEGER NOT NULL,"
        " is_for_sale INTEGER NOT NULL,"
        " weight INTEGER NOT NULL,"
        " appr_value INTEGER NOT NULL,"
        " speed INTEGER NOT NULL,"
        " level_limit INTEGER NOT NULL,"
        " gender_limit INTEGER NOT NULL,"
        " special_effect_value1 INTEGER NOT NULL,"
        " special_effect_value2 INTEGER NOT NULL,"
        " related_skill INTEGER NOT NULL,"
        " category INTEGER NOT NULL,"
        " item_color INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS program_config ("
        " key TEXT PRIMARY KEY,"
        " value TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS maps ("
        " map_index INTEGER PRIMARY KEY,"
        " map_name TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS settings ("
        " key TEXT PRIMARY KEY,"
        " value TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS admin_list ("
        " gm_name TEXT PRIMARY KEY"
        ");"
        "CREATE TABLE IF NOT EXISTS banned_list ("
        " ip_address TEXT PRIMARY KEY"
        ");"
        "CREATE TABLE IF NOT EXISTS admin_settings ("
        " key TEXT PRIMARY KEY,"
        " value INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS npc_configs ("
        " npc_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " npc_type INTEGER NOT NULL,"
        " hit_dice INTEGER NOT NULL,"
        " defense_ratio INTEGER NOT NULL,"
        " hit_ratio INTEGER NOT NULL,"
        " min_bravery INTEGER NOT NULL,"
        " exp_min INTEGER NOT NULL,"
        " exp_max INTEGER NOT NULL,"
        " gold_min INTEGER NOT NULL,"
        " gold_max INTEGER NOT NULL,"
        " attack_dice_throw INTEGER NOT NULL,"
        " attack_dice_range INTEGER NOT NULL,"
        " npc_size INTEGER NOT NULL,"
        " side INTEGER NOT NULL,"
        " action_limit INTEGER NOT NULL,"
        " action_time INTEGER NOT NULL,"
        " resist_magic INTEGER NOT NULL,"
        " magic_level INTEGER NOT NULL,"
        " day_of_week_limit INTEGER NOT NULL,"
        " chat_msg_presence INTEGER NOT NULL,"
        " target_search_range INTEGER NOT NULL,"
        " regen_time INTEGER NOT NULL,"
        " attribute INTEGER NOT NULL,"
        " abs_damage INTEGER NOT NULL,"
        " max_mana INTEGER NOT NULL,"
        " magic_hit_ratio INTEGER NOT NULL,"
        " attack_range INTEGER NOT NULL,"
        " drop_table_id INTEGER NOT NULL DEFAULT 0"
        ");"
        "CREATE TABLE IF NOT EXISTS drop_tables ("
        " drop_table_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " description TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS drop_entries ("
        " drop_table_id INTEGER NOT NULL,"
        " tier INTEGER NOT NULL,"
        " item_id INTEGER NOT NULL,"
        " weight INTEGER NOT NULL,"
        " min_count INTEGER NOT NULL,"
        " max_count INTEGER NOT NULL,"
        " PRIMARY KEY (drop_table_id, tier, item_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS magic_configs ("
        " magic_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " magic_type INTEGER NOT NULL,"
        " delay_time INTEGER NOT NULL,"
        " last_time INTEGER NOT NULL,"
        " value1 INTEGER NOT NULL,"
        " value2 INTEGER NOT NULL,"
        " value3 INTEGER NOT NULL,"
        " value4 INTEGER NOT NULL,"
        " value5 INTEGER NOT NULL,"
        " value6 INTEGER NOT NULL,"
        " value7 INTEGER NOT NULL,"
        " value8 INTEGER NOT NULL,"
        " value9 INTEGER NOT NULL,"
        " value10 INTEGER NOT NULL,"
        " value11 INTEGER NOT NULL,"
        " value12 INTEGER NOT NULL,"
        " int_limit INTEGER NOT NULL,"
        " gold_cost INTEGER NOT NULL,"
        " category INTEGER NOT NULL,"
        " attribute INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS skill_configs ("
        " skill_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " skill_type INTEGER NOT NULL,"
        " value1 INTEGER NOT NULL,"
        " value2 INTEGER NOT NULL,"
        " value3 INTEGER NOT NULL,"
        " value4 INTEGER NOT NULL,"
        " value5 INTEGER NOT NULL,"
        " value6 INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS quest_configs ("
        " quest_index INTEGER PRIMARY KEY,"
        " side INTEGER NOT NULL,"
        " quest_type INTEGER NOT NULL,"
        " target_type INTEGER NOT NULL,"
        " max_count INTEGER NOT NULL,"
        " quest_from INTEGER NOT NULL,"
        " min_level INTEGER NOT NULL,"
        " max_level INTEGER NOT NULL,"
        " required_skill_num INTEGER NOT NULL,"
        " required_skill_level INTEGER NOT NULL,"
        " time_limit INTEGER NOT NULL,"
        " assign_type INTEGER NOT NULL,"
        " reward_type1 INTEGER NOT NULL,"
        " reward_amount1 INTEGER NOT NULL,"
        " reward_type2 INTEGER NOT NULL,"
        " reward_amount2 INTEGER NOT NULL,"
        " reward_type3 INTEGER NOT NULL,"
        " reward_amount3 INTEGER NOT NULL,"
        " contribution INTEGER NOT NULL,"
        " contribution_limit INTEGER NOT NULL,"
        " response_mode INTEGER NOT NULL,"
        " target_name TEXT NOT NULL,"
        " target_x INTEGER NOT NULL,"
        " target_y INTEGER NOT NULL,"
        " target_range INTEGER NOT NULL,"
        " quest_id INTEGER NOT NULL,"
        " req_contribution INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS potion_configs ("
        " potion_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " array0 INTEGER NOT NULL,"
        " array1 INTEGER NOT NULL,"
        " array2 INTEGER NOT NULL,"
        " array3 INTEGER NOT NULL,"
        " array4 INTEGER NOT NULL,"
        " array5 INTEGER NOT NULL,"
        " array6 INTEGER NOT NULL,"
        " array7 INTEGER NOT NULL,"
        " array8 INTEGER NOT NULL,"
        " array9 INTEGER NOT NULL,"
        " array10 INTEGER NOT NULL,"
        " array11 INTEGER NOT NULL,"
        " skill_limit INTEGER NOT NULL,"
        " difficulty INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS crafting_configs ("
        " crafting_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " array0 INTEGER NOT NULL,"
        " array1 INTEGER NOT NULL,"
        " array2 INTEGER NOT NULL,"
        " array3 INTEGER NOT NULL,"
        " array4 INTEGER NOT NULL,"
        " array5 INTEGER NOT NULL,"
        " array6 INTEGER NOT NULL,"
        " array7 INTEGER NOT NULL,"
        " array8 INTEGER NOT NULL,"
        " array9 INTEGER NOT NULL,"
        " array10 INTEGER NOT NULL,"
        " array11 INTEGER NOT NULL,"
        " skill_limit INTEGER NOT NULL,"
        " difficulty INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS builditem_configs ("
        " build_id INTEGER PRIMARY KEY,"
        " name TEXT NOT NULL,"
        " skill_limit INTEGER NOT NULL,"
        " material_id1 INTEGER NOT NULL,"
        " material_count1 INTEGER NOT NULL,"
        " material_value1 INTEGER NOT NULL,"
        " material_id2 INTEGER NOT NULL,"
        " material_count2 INTEGER NOT NULL,"
        " material_value2 INTEGER NOT NULL,"
        " material_id3 INTEGER NOT NULL,"
        " material_count3 INTEGER NOT NULL,"
        " material_value3 INTEGER NOT NULL,"
        " material_id4 INTEGER NOT NULL,"
        " material_count4 INTEGER NOT NULL,"
        " material_value4 INTEGER NOT NULL,"
        " material_id5 INTEGER NOT NULL,"
        " material_count5 INTEGER NOT NULL,"
        " material_value5 INTEGER NOT NULL,"
        " material_id6 INTEGER NOT NULL,"
        " material_count6 INTEGER NOT NULL,"
        " material_value6 INTEGER NOT NULL,"
        " average_value INTEGER NOT NULL,"
        " max_skill INTEGER NOT NULL,"
        " attribute INTEGER NOT NULL,"
        " item_id INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS dup_item_ids ("
        " dup_id INTEGER PRIMARY KEY,"
        " touch_effect_type INTEGER NOT NULL,"
        " touch_effect_value1 INTEGER NOT NULL,"
        " touch_effect_value2 INTEGER NOT NULL,"
        " touch_effect_value3 INTEGER NOT NULL,"
        " price INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS crusade_structures ("
        " structure_id INTEGER PRIMARY KEY,"
        " map_name TEXT NOT NULL,"
        " structure_type INTEGER NOT NULL,"
        " pos_x INTEGER NOT NULL,"
        " pos_y INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS schedule_crusade ("
        " schedule_id INTEGER PRIMARY KEY,"
        " day INTEGER NOT NULL,"
        " hour INTEGER NOT NULL,"
        " minute INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS schedule_apocalypse_start ("
        " schedule_id INTEGER PRIMARY KEY,"
        " day INTEGER NOT NULL,"
        " hour INTEGER NOT NULL,"
        " minute INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS schedule_apocalypse_end ("
        " schedule_id INTEGER PRIMARY KEY,"
        " day INTEGER NOT NULL,"
        " hour INTEGER NOT NULL,"
        " minute INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS schedule_heldenian ("
        " schedule_id INTEGER PRIMARY KEY,"
        " day INTEGER NOT NULL,"
        " start_hour INTEGER NOT NULL,"
        " start_minute INTEGER NOT NULL,"
        " end_hour INTEGER NOT NULL,"
        " end_minute INTEGER NOT NULL"
        ");"
        "COMMIT;";

    if (!ExecSql(db, schemaSql)) {
        sqlite3_close(db);
        return false;
    }

    if (!HasColumn(db, "npc_configs", "drop_table_id")) {
        ExecSql(db, "ALTER TABLE npc_configs ADD COLUMN drop_table_id INTEGER NOT NULL DEFAULT 0;");
    }

    *outDb = db;
    if (outCreated != nullptr) {
        *outCreated = created;
    }
    return true;
}

bool SaveItemConfigs(sqlite3* db, CItem* const* itemList, int maxItems)
{
    if (db == nullptr || itemList == nullptr || maxItems <= 0) {
        return false;
    }

    if (!ExecSql(db, "BEGIN;")) {
        return false;
    }
    if (!ExecSql(db, "DELETE FROM items;")) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    const char* sql =
        "INSERT INTO items("
        " item_id, name, item_type, equip_pos, item_effect_type, item_effect_value1,"
        " item_effect_value2, item_effect_value3, item_effect_value4, item_effect_value5,"
        " item_effect_value6, max_lifespan, special_effect, sprite, sprite_frame, price,"
        " is_for_sale, weight, appr_value, speed, level_limit, gender_limit,"
        " special_effect_value1, special_effect_value2, related_skill, category, item_color"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }

    for (int i = 0; i < maxItems; i++) {
        if (itemList[i] == nullptr) {
            continue;
        }

        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sIDnum) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, itemList[i]->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cItemType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cEquipPos) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue4) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue5) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sItemEffectValue6) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_wMaxLifeSpan) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sSpecialEffect) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sSprite) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sSpriteFrame) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(itemList[i]->m_wPrice)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_bIsForSale ? 1 : 0) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_wWeight) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cApprValue) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cSpeed) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sLevelLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cGenderLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sSpecialEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sSpecialEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_sRelatedSkill) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cCategory) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, itemList[i]->m_cItemColor) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            ExecSql(db, "ROLLBACK;");
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!ExecSql(db, "COMMIT;")) {
        ExecSql(db, "ROLLBACK;");
        return false;
    }
    return true;
}

bool LoadItemConfigs(sqlite3* db, CItem** itemList, int maxItems)
{
    if (db == nullptr || itemList == nullptr || maxItems <= 0) {
        return false;
    }

    const char* sql =
        "SELECT item_id, name, item_type, equip_pos, item_effect_type, item_effect_value1,"
        " item_effect_value2, item_effect_value3, item_effect_value4, item_effect_value5,"
        " item_effect_value6, max_lifespan, special_effect, sprite, sprite_frame, price,"
        " is_for_sale, weight, appr_value, speed, level_limit, gender_limit,"
        " special_effect_value1, special_effect_value2, related_skill, category, item_color"
        " FROM items ORDER BY item_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int itemId = sqlite3_column_int(stmt, col++);
        if (itemId < 0 || itemId >= maxItems) {
            continue;
        }

        if (itemList[itemId] != nullptr) {
            delete itemList[itemId];
            itemList[itemId] = nullptr;
        }

        CItem* item = new CItem();
        item->m_sIDnum = (short)itemId;
        CopyColumnText(stmt, col++, item->m_cName, sizeof(item->m_cName));
        item->m_cItemType = (char)sqlite3_column_int(stmt, col++);
        item->m_cEquipPos = (char)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectType = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue1 = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue2 = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue3 = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue4 = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue5 = (short)sqlite3_column_int(stmt, col++);
        item->m_sItemEffectValue6 = (short)sqlite3_column_int(stmt, col++);
        item->m_wMaxLifeSpan = (uint16_t)sqlite3_column_int(stmt, col++);
        item->m_sSpecialEffect = (short)sqlite3_column_int(stmt, col++);
        item->m_sSprite = (short)sqlite3_column_int(stmt, col++);
        item->m_sSpriteFrame = (short)sqlite3_column_int(stmt, col++);
        item->m_wPrice = (uint32_t)sqlite3_column_int(stmt, col++);
        item->m_bIsForSale = (sqlite3_column_int(stmt, col++) != 0);
        item->m_wWeight = (uint16_t)sqlite3_column_int(stmt, col++);
        item->m_cApprValue = (char)sqlite3_column_int(stmt, col++);
        item->m_cSpeed = (char)sqlite3_column_int(stmt, col++);
        item->m_sLevelLimit = (short)sqlite3_column_int(stmt, col++);
        item->m_cGenderLimit = (char)sqlite3_column_int(stmt, col++);
        item->m_sSpecialEffectValue1 = (short)sqlite3_column_int(stmt, col++);
        item->m_sSpecialEffectValue2 = (short)sqlite3_column_int(stmt, col++);
        item->m_sRelatedSkill = (short)sqlite3_column_int(stmt, col++);
        item->m_cCategory = (char)sqlite3_column_int(stmt, col++);
        item->m_cItemColor = (char)sqlite3_column_int(stmt, col++);

        itemList[itemId] = item;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveProgramConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "program_config") || !ClearTable(db, "maps")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmtConfig = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO program_config(key, value) VALUES(?, ?);", -1, &stmtConfig, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    bool ok = true;
    ok &= InsertKeyValue(stmtConfig, "game-server-name", game->m_cServerName);
    ok &= InsertKeyValueInt(stmtConfig, "game-server-port", game->m_iGameServerPort);
    ok &= InsertKeyValue(stmtConfig, "log-server-address", game->m_cLogServerAddr);
    ok &= InsertKeyValueInt(stmtConfig, "internal-log-server-port", game->m_iLogServerPort);
    ok &= InsertKeyValue(stmtConfig, "gate-server-address", game->m_cGateServerAddr);
    ok &= InsertKeyValueInt(stmtConfig, "gate-server-port", game->m_iGateServerPort);
    ok &= InsertKeyValue(stmtConfig, "game-server-internal-address", game->m_cGameServerAddrInternal);
    ok &= InsertKeyValue(stmtConfig, "game-server-external-address", game->m_cGameServerAddrExternal);
    ok &= InsertKeyValue(stmtConfig, "game-server-address", game->m_cGameServerAddr);
    if (game->m_iGameServerMode == 1) {
        ok &= InsertKeyValue(stmtConfig, "game-server-mode", "lan");
    } else if (game->m_iGameServerMode == 2) {
        ok &= InsertKeyValue(stmtConfig, "game-server-mode", "internet");
    } else {
        ok &= InsertKeyValueInt(stmtConfig, "game-server-mode", game->m_iGameServerMode);
    }

    sqlite3_finalize(stmtConfig);
    if (!ok) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmtMap = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO maps(map_index, map_name) VALUES(?, ?);", -1, &stmtMap, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    int mapIndex = 0;
    for (int i = 0; i < DEF_MAXMAPS; i++) {
        if (game->m_pMapList[i] == nullptr) {
            continue;
        }
        sqlite3_reset(stmtMap);
        sqlite3_clear_bindings(stmtMap);
        ok = (sqlite3_bind_int(stmtMap, 1, mapIndex++) == SQLITE_OK);
        ok &= PrepareAndBindText(stmtMap, 2, game->m_pMapList[i]->m_cName);
        if (!ok || sqlite3_step(stmtMap) != SQLITE_DONE) {
            sqlite3_finalize(stmtMap);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmtMap);

    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadProgramConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT key, value FROM program_config;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* keyText = sqlite3_column_text(stmt, 0);
        const unsigned char* valueText = sqlite3_column_text(stmt, 1);
        if (keyText == nullptr || valueText == nullptr) {
            continue;
        }
        const char* key = reinterpret_cast<const char*>(keyText);
        const char* value = reinterpret_cast<const char*>(valueText);

        if (std::strcmp(key, "game-server-name") == 0) {
            std::snprintf(game->m_cServerName, sizeof(game->m_cServerName), "%s", value);
        } else if (std::strcmp(key, "game-server-port") == 0) {
            game->m_iGameServerPort = std::atoi(value);
        } else if (std::strcmp(key, "log-server-address") == 0) {
            std::snprintf(game->m_cLogServerAddr, sizeof(game->m_cLogServerAddr), "%s", value);
        } else if (std::strcmp(key, "internal-log-server-port") == 0) {
            game->m_iLogServerPort = std::atoi(value);
        } else if (std::strcmp(key, "gate-server-address") == 0) {
            std::snprintf(game->m_cGateServerAddr, sizeof(game->m_cGateServerAddr), "%s", value);
        } else if (std::strcmp(key, "gate-server-port") == 0) {
            game->m_iGateServerPort = std::atoi(value);
        } else if (std::strcmp(key, "game-server-internal-address") == 0) {
            std::snprintf(game->m_cGameServerAddrInternal, sizeof(game->m_cGameServerAddrInternal), "%s", value);
        } else if (std::strcmp(key, "game-server-external-address") == 0) {
            std::snprintf(game->m_cGameServerAddrExternal, sizeof(game->m_cGameServerAddrExternal), "%s", value);
        } else if (std::strcmp(key, "game-server-address") == 0) {
            std::snprintf(game->m_cGameServerAddr, sizeof(game->m_cGameServerAddr), "%s", value);
        } else if (std::strcmp(key, "game-server-mode") == 0) {
            if (_stricmp(value, "lan") == 0) {
                game->m_iGameServerMode = 1;
            } else if (_stricmp(value, "internet") == 0) {
                game->m_iGameServerMode = 2;
            } else {
                game->m_iGameServerMode = std::atoi(value);
            }
        }
    }

    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, "SELECT map_name FROM maps ORDER BY map_index;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int mapsLoaded = 0;
    PutLogListLevel(LOG_LEVEL_NOTICE, "Loading maps from GameConfigs.db...");
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* nameText = sqlite3_column_text(stmt, 0);
        if (nameText == nullptr) {
            continue;
        }
        const char* name = reinterpret_cast<const char*>(nameText);
        if (!game->_bRegisterMap(const_cast<char*>(name))) {
            char logMsg[256] = {};
            std::snprintf(logMsg, sizeof(logMsg), "Map load failed: %s", name);
            PutLogListLevel(LOG_LEVEL_ERROR, logMsg);
            sqlite3_finalize(stmt);
            return false;
        }
        mapsLoaded++;
    }

    sqlite3_finalize(stmt);
    {
        char logMsg[128] = {};
        std::snprintf(logMsg, sizeof(logMsg), "Loaded %d maps.", mapsLoaded);
        PutLogListLevel(LOG_LEVEL_NOTICE, logMsg);
    }
    return (game->m_iGameServerMode != 0);
}

bool SaveSettingsConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "settings")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO settings(key, value) VALUES(?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    bool ok = true;
    ok &= InsertKeyValueInt(stmt, "primary-drop-rate", game->m_iPrimaryDropRate);
    ok &= InsertKeyValueInt(stmt, "secondary-drop-rate", game->m_iSecondaryDropRate);
    ok &= InsertKeyValue(stmt, "enemy-kill-mode", game->m_bEnemyKillMode ? "deathmatch" : "classic");
    ok &= InsertKeyValueInt(stmt, "enemy-kill-adjust", game->m_iEnemyKillAdjust);
    ok &= InsertKeyValue(stmt, "admin-security", game->m_bAdminSecurity ? "on" : "off");
    ok &= InsertKeyValueInt(stmt, "monday-raid-time", game->m_sRaidTimeMonday);
    ok &= InsertKeyValueInt(stmt, "tuesday-raid-time", game->m_sRaidTimeTuesday);
    ok &= InsertKeyValueInt(stmt, "wednesday-raid-time", game->m_sRaidTimeWednesday);
    ok &= InsertKeyValueInt(stmt, "thursday-raid-time", game->m_sRaidTimeThursday);
    ok &= InsertKeyValueInt(stmt, "friday-raid-time", game->m_sRaidTimeFriday);
    ok &= InsertKeyValueInt(stmt, "saturday-raid-time", game->m_sRaidTimeSaturday);
    ok &= InsertKeyValueInt(stmt, "sunday-raid-time", game->m_sRaidTimeSunday);
    switch (game->m_bLogChatOption) {
    case 1: ok &= InsertKeyValue(stmt, "log-chat-settings", "player"); break;
    case 2: ok &= InsertKeyValue(stmt, "log-chat-settings", "gm"); break;
    case 3: ok &= InsertKeyValue(stmt, "log-chat-settings", "all"); break;
    case 4: ok &= InsertKeyValue(stmt, "log-chat-settings", "none"); break;
    default: ok &= InsertKeyValue(stmt, "log-chat-settings", "player"); break;
    }
    ok &= InsertKeyValueInt(stmt, "summonguild-cost", game->m_iSummonGuildCost);
    ok &= InsertKeyValueInt(stmt, "slate-success-rate", game->m_sSlateSuccessRate);
    ok &= InsertKeyValueInt(stmt, "character-stat-limit", game->m_sCharStatLimit);
    ok &= InsertKeyValueInt(stmt, "character-skill-limit", game->m_sCharSkillLimit);
    ok &= InsertKeyValueInt(stmt, "rep-drop-modifier", game->m_cRepDropModifier);
    ok &= InsertKeyValue(stmt, "admin-security-code", game->m_cSecurityNumber);
    ok &= InsertKeyValueInt(stmt, "max-player-level", game->m_iPlayerMaxLevel);

    sqlite3_finalize(stmt);
    if (!ok) {
        RollbackTransaction(db);
        return false;
    }

    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadSettingsConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT key, value FROM settings;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* keyText = sqlite3_column_text(stmt, 0);
        const unsigned char* valueText = sqlite3_column_text(stmt, 1);
        if (keyText == nullptr || valueText == nullptr) {
            continue;
        }
        const char* key = reinterpret_cast<const char*>(keyText);
        const char* value = reinterpret_cast<const char*>(valueText);

        if (std::strcmp(key, "primary-drop-rate") == 0) {
            game->m_iPrimaryDropRate = std::atoi(value);
        } else if (std::strcmp(key, "secondary-drop-rate") == 0) {
            game->m_iSecondaryDropRate = std::atoi(value);
        } else if (std::strcmp(key, "enemy-kill-mode") == 0) {
            if (_stricmp(value, "deathmatch") == 0) {
                game->m_bEnemyKillMode = true;
            } else if (_stricmp(value, "classic") == 0) {
                game->m_bEnemyKillMode = false;
            } else {
                game->m_bEnemyKillMode = (std::atoi(value) != 0);
            }
        } else if (std::strcmp(key, "enemy-kill-adjust") == 0) {
            game->m_iEnemyKillAdjust = std::atoi(value);
        } else if (std::strcmp(key, "admin-security") == 0) {
            if (_stricmp(value, "on") == 0) {
                game->m_bAdminSecurity = true;
            } else if (_stricmp(value, "off") == 0) {
                game->m_bAdminSecurity = false;
            } else {
                game->m_bAdminSecurity = (std::atoi(value) != 0);
            }
        } else if (std::strcmp(key, "monday-raid-time") == 0) {
            game->m_sRaidTimeMonday = (short)std::atoi(value);
        } else if (std::strcmp(key, "tuesday-raid-time") == 0) {
            game->m_sRaidTimeTuesday = (short)std::atoi(value);
        } else if (std::strcmp(key, "wednesday-raid-time") == 0) {
            game->m_sRaidTimeWednesday = (short)std::atoi(value);
        } else if (std::strcmp(key, "thursday-raid-time") == 0) {
            game->m_sRaidTimeThursday = (short)std::atoi(value);
        } else if (std::strcmp(key, "friday-raid-time") == 0) {
            game->m_sRaidTimeFriday = (short)std::atoi(value);
        } else if (std::strcmp(key, "saturday-raid-time") == 0) {
            game->m_sRaidTimeSaturday = (short)std::atoi(value);
        } else if (std::strcmp(key, "sunday-raid-time") == 0) {
            game->m_sRaidTimeSunday = (short)std::atoi(value);
        } else if (std::strcmp(key, "log-chat-settings") == 0) {
            if (_stricmp(value, "player") == 0) {
                game->m_bLogChatOption = 1;
            } else if (_stricmp(value, "gm") == 0) {
                game->m_bLogChatOption = 2;
            } else if (_stricmp(value, "all") == 0) {
                game->m_bLogChatOption = 3;
            } else if (_stricmp(value, "none") == 0) {
                game->m_bLogChatOption = 4;
            }
        } else if (std::strcmp(key, "summonguild-cost") == 0) {
            game->m_iSummonGuildCost = std::atoi(value);
        } else if (std::strcmp(key, "slate-success-rate") == 0) {
            game->m_sSlateSuccessRate = (short)std::atoi(value);
        } else if (std::strcmp(key, "character-stat-limit") == 0) {
            game->m_sCharStatLimit = (short)std::atoi(value);
        } else if (std::strcmp(key, "character-skill-limit") == 0) {
            game->m_sCharSkillLimit = (short)std::atoi(value);
        } else if (std::strcmp(key, "rep-drop-modifier") == 0) {
            game->m_cRepDropModifier = (char)std::atoi(value);
        } else if (std::strcmp(key, "admin-security-code") == 0) {
            std::snprintf(game->m_cSecurityNumber, sizeof(game->m_cSecurityNumber), "%s", value);
        } else if (std::strcmp(key, "max-player-level") == 0) {
            game->m_iPlayerMaxLevel = std::atoi(value);
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveAdminListConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "admin_list")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO admin_list(gm_name) VALUES(?);", -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXADMINS; i++) {
        if (game->m_stAdminList[i].m_cGMName[0] == 0) {
            continue;
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        if (!PrepareAndBindText(stmt, 1, game->m_stAdminList[i].m_cGMName) ||
            sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadAdminListConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXADMINS; i++) {
        std::memset(game->m_stAdminList[i].m_cGMName, 0, sizeof(game->m_stAdminList[i].m_cGMName));
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT gm_name FROM admin_list ORDER BY gm_name;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (index >= DEF_MAXADMINS) {
            break;
        }
        CopyColumnText(stmt, 0, game->m_stAdminList[index].m_cGMName, sizeof(game->m_stAdminList[index].m_cGMName));
        index++;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveBannedListConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "banned_list")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO banned_list(ip_address) VALUES(?);", -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXBANNED; i++) {
        if (game->m_stBannedList[i].m_cBannedIPaddress[0] == 0) {
            continue;
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        if (!PrepareAndBindText(stmt, 1, game->m_stBannedList[i].m_cBannedIPaddress) ||
            sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadBannedListConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXBANNED; i++) {
        std::memset(game->m_stBannedList[i].m_cBannedIPaddress, 0, sizeof(game->m_stBannedList[i].m_cBannedIPaddress));
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT ip_address FROM banned_list ORDER BY ip_address;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    int index = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (index >= DEF_MAXBANNED) {
            break;
        }
        CopyColumnText(stmt, 0, game->m_stBannedList[index].m_cBannedIPaddress, sizeof(game->m_stBannedList[index].m_cBannedIPaddress));
        index++;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveAdminSettingsConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "admin_settings")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO admin_settings(key, value) VALUES(?, ?);", -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    struct SettingMap {
        const char* key;
        int CGame::*member;
    };

    const SettingMap settings[] = {
        {"Admin-Level-/kill", &CGame::m_iAdminLevelGMKill},
        {"Admin-Level-/revive", &CGame::m_iAdminLevelGMRevive},
        {"Admin-Level-/closecon", &CGame::m_iAdminLevelGMCloseconn},
        {"Admin-Level-/checkrep", &CGame::m_iAdminLevelGMCheckRep},
        {"Admin-Level-/who", &CGame::m_iAdminLevelWho},
        {"Admin-Level-/energysphere", &CGame::m_iAdminLevelEnergySphere},
        {"Admin-Level-/shutdownthisserverrightnow", &CGame::m_iAdminLevelShutdown},
        {"Admin-Level-/setobservermode", &CGame::m_iAdminLevelObserver},
        {"Admin-Level-/shutup", &CGame::m_iAdminLevelShutup},
        {"Admin-Level-/attack", &CGame::m_iAdminLevelCallGaurd},
        {"Admin-Level-/summondemon", &CGame::m_iAdminLevelSummonDemon},
        {"Admin-Level-/summondeath", &CGame::m_iAdminLevelSummonDeath},
        {"Admin-Level-/reservefightzone", &CGame::m_iAdminLevelReserveFightzone},
        {"Admin-Level-/createfish", &CGame::m_iAdminLevelCreateFish},
        {"Admin-Level-/teleport", &CGame::m_iAdminLevelTeleport},
        {"Admin-Level-/checkip", &CGame::m_iAdminLevelCheckIP},
        {"Admin-Level-/polymorph", &CGame::m_iAdminLevelPolymorph},
        {"Admin-Level-/setinvi", &CGame::m_iAdminLevelSetInvis},
        {"Admin-Level-/setzerk", &CGame::m_iAdminLevelSetZerk},
        {"Admin-Level-/setfreeze", &CGame::m_iAdminLevelSetIce},
        {"Admin-Level-/gns", &CGame::m_iAdminLevelGetNpcStatus},
        {"Admin-Level-/setattackmode", &CGame::m_iAdminLevelSetAttackMode},
        {"Admin-Level-/unsummonall", &CGame::m_iAdminLevelUnsummonAll},
        {"Admin-Level-/unsummondemon", &CGame::m_iAdminLevelUnsummonDemon},
        {"Admin-Level-/summonnpc", &CGame::m_iAdminLevelSummon},
        {"Admin-Level-/summonall", &CGame::m_iAdminLevelSummonAll},
        {"Admin-Level-/summonplayer", &CGame::m_iAdminLevelSummonPlayer},
        {"Admin-Level-/disconnectall", &CGame::m_iAdminLevelDisconnectAll},
        {"Admin-Level-/enableadmincreateitem", &CGame::m_iAdminLevelEnableCreateItem},
        {"Admin-Level-/createitem", &CGame::m_iAdminLevelCreateItem},
        {"Admin-Level-/storm", &CGame::m_iAdminLevelStorm},
        {"Admin-Level-/weather", &CGame::m_iAdminLevelWeather},
        {"Admin-Level-/setstatus", &CGame::m_iAdminLevelSetStatus},
        {"Admin-Level-/goto", &CGame::m_iAdminLevelGoto},
        {"Admin-Level-/monstercount", &CGame::m_iAdminLevelMonsterCount},
        {"Admin-Level-/setforcerecalltime", &CGame::m_iAdminLevelSetRecallTime},
        {"Admin-Level-/unsummonboss", &CGame::m_iAdminLevelUnsummonBoss},
        {"Admin-Level-/clearnpc", &CGame::m_iAdminLevelClearNpc},
        {"Admin-Level-/time", &CGame::m_iAdminLevelTime},
        {"Admin-Level-/send", &CGame::m_iAdminLevelPushPlayer},
        {"Admin-Level-/summonguild", &CGame::m_iAdminLevelSummonGuild},
        {"Admin-Level-/checkstatus", &CGame::m_iAdminLevelCheckStatus},
        {"Admin-Level-/clearmap", &CGame::m_iAdminLevelCleanMap},
    };

    bool ok = true;
    for (const auto& entry : settings) {
        ok &= InsertKeyValueInt(stmt, entry.key, game->*(entry.member));
        if (!ok) {
            break;
        }
    }

    sqlite3_finalize(stmt);
    if (!ok) {
        RollbackTransaction(db);
        return false;
    }

    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadAdminSettingsConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT key, value FROM admin_settings;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* keyText = sqlite3_column_text(stmt, 0);
        if (keyText == nullptr) {
            continue;
        }
        const char* key = reinterpret_cast<const char*>(keyText);
        int value = sqlite3_column_int(stmt, 1);

        if (std::strcmp(key, "Admin-Level-/kill") == 0) {
            game->m_iAdminLevelGMKill = value;
        } else if (std::strcmp(key, "Admin-Level-/revive") == 0) {
            game->m_iAdminLevelGMRevive = value;
        } else if (std::strcmp(key, "Admin-Level-/closecon") == 0) {
            game->m_iAdminLevelGMCloseconn = value;
        } else if (std::strcmp(key, "Admin-Level-/checkrep") == 0) {
            game->m_iAdminLevelGMCheckRep = value;
        } else if (std::strcmp(key, "Admin-Level-/who") == 0) {
            game->m_iAdminLevelWho = value;
        } else if (std::strcmp(key, "Admin-Level-/energysphere") == 0) {
            game->m_iAdminLevelEnergySphere = value;
        } else if (std::strcmp(key, "Admin-Level-/shutdownthisserverrightnow") == 0) {
            game->m_iAdminLevelShutdown = value;
        } else if (std::strcmp(key, "Admin-Level-/setobservermode") == 0) {
            game->m_iAdminLevelObserver = value;
        } else if (std::strcmp(key, "Admin-Level-/shutup") == 0) {
            game->m_iAdminLevelShutup = value;
        } else if (std::strcmp(key, "Admin-Level-/attack") == 0) {
            game->m_iAdminLevelCallGaurd = value;
        } else if (std::strcmp(key, "Admin-Level-/summondemon") == 0) {
            game->m_iAdminLevelSummonDemon = value;
        } else if (std::strcmp(key, "Admin-Level-/summondeath") == 0) {
            game->m_iAdminLevelSummonDeath = value;
        } else if (std::strcmp(key, "Admin-Level-/reservefightzone") == 0) {
            game->m_iAdminLevelReserveFightzone = value;
        } else if (std::strcmp(key, "Admin-Level-/createfish") == 0) {
            game->m_iAdminLevelCreateFish = value;
        } else if (std::strcmp(key, "Admin-Level-/teleport") == 0) {
            game->m_iAdminLevelTeleport = value;
        } else if (std::strcmp(key, "Admin-Level-/checkip") == 0) {
            game->m_iAdminLevelCheckIP = value;
        } else if (std::strcmp(key, "Admin-Level-/polymorph") == 0) {
            game->m_iAdminLevelPolymorph = value;
        } else if (std::strcmp(key, "Admin-Level-/setinvi") == 0) {
            game->m_iAdminLevelSetInvis = value;
        } else if (std::strcmp(key, "Admin-Level-/setzerk") == 0) {
            game->m_iAdminLevelSetZerk = value;
        } else if (std::strcmp(key, "Admin-Level-/setfreeze") == 0) {
            game->m_iAdminLevelSetIce = value;
        } else if (std::strcmp(key, "Admin-Level-/gns") == 0) {
            game->m_iAdminLevelGetNpcStatus = value;
        } else if (std::strcmp(key, "Admin-Level-/setattackmode") == 0) {
            game->m_iAdminLevelSetAttackMode = value;
        } else if (std::strcmp(key, "Admin-Level-/unsummonall") == 0) {
            game->m_iAdminLevelUnsummonAll = value;
        } else if (std::strcmp(key, "Admin-Level-/unsummondemon") == 0) {
            game->m_iAdminLevelUnsummonDemon = value;
        } else if (std::strcmp(key, "Admin-Level-/summonnpc") == 0) {
            game->m_iAdminLevelSummon = value;
        } else if (std::strcmp(key, "Admin-Level-/summonall") == 0) {
            game->m_iAdminLevelSummonAll = value;
        } else if (std::strcmp(key, "Admin-Level-/summonplayer") == 0) {
            game->m_iAdminLevelSummonPlayer = value;
        } else if (std::strcmp(key, "Admin-Level-/disconnectall") == 0) {
            game->m_iAdminLevelDisconnectAll = value;
        } else if (std::strcmp(key, "Admin-Level-/enableadmincreateitem") == 0) {
            game->m_iAdminLevelEnableCreateItem = value;
        } else if (std::strcmp(key, "Admin-Level-/createitem") == 0) {
            game->m_iAdminLevelCreateItem = value;
        } else if (std::strcmp(key, "Admin-Level-/storm") == 0) {
            game->m_iAdminLevelStorm = value;
        } else if (std::strcmp(key, "Admin-Level-/weather") == 0) {
            game->m_iAdminLevelWeather = value;
        } else if (std::strcmp(key, "Admin-Level-/setstatus") == 0) {
            game->m_iAdminLevelSetStatus = value;
        } else if (std::strcmp(key, "Admin-Level-/goto") == 0) {
            game->m_iAdminLevelGoto = value;
        } else if (std::strcmp(key, "Admin-Level-/monstercount") == 0) {
            game->m_iAdminLevelMonsterCount = value;
        } else if (std::strcmp(key, "Admin-Level-/setforcerecalltime") == 0) {
            game->m_iAdminLevelSetRecallTime = value;
        } else if (std::strcmp(key, "Admin-Level-/unsummonboss") == 0) {
            game->m_iAdminLevelUnsummonBoss = value;
        } else if (std::strcmp(key, "Admin-Level-/clearnpc") == 0) {
            game->m_iAdminLevelClearNpc = value;
        } else if (std::strcmp(key, "Admin-Level-/time") == 0) {
            game->m_iAdminLevelTime = value;
        } else if (std::strcmp(key, "Admin-Level-/send") == 0) {
            game->m_iAdminLevelPushPlayer = value;
        } else if (std::strcmp(key, "Admin-Level-/summonguild") == 0) {
            game->m_iAdminLevelSummonGuild = value;
        } else if (std::strcmp(key, "Admin-Level-/checkstatus") == 0) {
            game->m_iAdminLevelCheckStatus = value;
        } else if (std::strcmp(key, "Admin-Level-/clearmap") == 0) {
            game->m_iAdminLevelCleanMap = value;
        }
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveNpcConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "npc_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO npc_configs("
        " npc_id, name, npc_type, hit_dice, defense_ratio, hit_ratio, min_bravery,"
        " exp_min, exp_max, gold_min, gold_max, attack_dice_throw, attack_dice_range,"
        " npc_size, side, action_limit, action_time, resist_magic, magic_level,"
        " day_of_week_limit, chat_msg_presence, target_search_range, regen_time,"
        " attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXNPCTYPES; i++) {
        if (game->m_pNpcConfigList[i] == nullptr) {
            continue;
        }

        const CNpc* npc = game->m_pNpcConfigList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, npc->m_cNpcName);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_sType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iHitDice) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iDefenseRatio) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iHitRatio) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iMinBravery) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_iExpDiceMin)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_iExpDiceMax)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_iGoldDiceMin)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_iGoldDiceMax)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cAttackDiceThrow) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cAttackDiceRange) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cSize) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cSide) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cActionLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_dwActionTime)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cResistMagic) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cMagicLevel) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cDayOfWeekLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cChatMsgPresence) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cTargetSearchRange) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(npc->m_dwRegenTime)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_cAttribute) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iAbsDamage) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iMaxMana) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iMagicHitRatio) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iAttackRange) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, npc->m_iDropTableId) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadNpcConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXNPCTYPES; i++) {
        delete game->m_pNpcConfigList[i];
        game->m_pNpcConfigList[i] = nullptr;
    }

    const char* sql =
        "SELECT npc_id, name, npc_type, hit_dice, defense_ratio, hit_ratio, min_bravery,"
        " exp_min, exp_max, gold_min, gold_max, attack_dice_throw, attack_dice_range,"
        " npc_size, side, action_limit, action_time, resist_magic, magic_level,"
        " day_of_week_limit, chat_msg_presence, target_search_range, regen_time,"
        " attribute, abs_damage, max_mana, magic_hit_ratio, attack_range, drop_table_id"
        " FROM npc_configs ORDER BY npc_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int npcId = sqlite3_column_int(stmt, col++);
        if (npcId < 0 || npcId >= DEF_MAXNPCTYPES) {
            continue;
        }

        CNpc* npc = new CNpc(" ");
        std::memset(npc->m_cNpcName, 0, sizeof(npc->m_cNpcName));
        CopyColumnText(stmt, col++, npc->m_cNpcName, sizeof(npc->m_cNpcName));
        npc->m_sType = (short)sqlite3_column_int(stmt, col++);
        npc->m_iHitDice = sqlite3_column_int(stmt, col++);
        npc->m_iDefenseRatio = sqlite3_column_int(stmt, col++);
        npc->m_iHitRatio = sqlite3_column_int(stmt, col++);
        npc->m_iMinBravery = sqlite3_column_int(stmt, col++);
        npc->m_iExpDiceMin = sqlite3_column_int(stmt, col++);
        npc->m_iExpDiceMax = sqlite3_column_int(stmt, col++);
        npc->m_iGoldDiceMin = sqlite3_column_int(stmt, col++);
        npc->m_iGoldDiceMax = sqlite3_column_int(stmt, col++);
        npc->m_cAttackDiceThrow = (char)sqlite3_column_int(stmt, col++);
        npc->m_cAttackDiceRange = (char)sqlite3_column_int(stmt, col++);
        npc->m_cSize = (char)sqlite3_column_int(stmt, col++);
        npc->m_cSide = (char)sqlite3_column_int(stmt, col++);
        npc->m_cActionLimit = (char)sqlite3_column_int(stmt, col++);
        npc->m_dwActionTime = (uint32_t)sqlite3_column_int(stmt, col++);
        npc->m_cResistMagic = (char)sqlite3_column_int(stmt, col++);
        npc->m_cMagicLevel = (char)sqlite3_column_int(stmt, col++);
        npc->m_cDayOfWeekLimit = (char)sqlite3_column_int(stmt, col++);
        npc->m_cChatMsgPresence = (char)sqlite3_column_int(stmt, col++);
        npc->m_cTargetSearchRange = (char)sqlite3_column_int(stmt, col++);
        npc->m_dwRegenTime = (uint32_t)sqlite3_column_int(stmt, col++);
        npc->m_cAttribute = (char)sqlite3_column_int(stmt, col++);
        npc->m_iAbsDamage = sqlite3_column_int(stmt, col++);
        npc->m_iMaxMana = sqlite3_column_int(stmt, col++);
        npc->m_iMagicHitRatio = sqlite3_column_int(stmt, col++);
        npc->m_iAttackRange = sqlite3_column_int(stmt, col++);
        npc->m_iDropTableId = sqlite3_column_int(stmt, col++);

        game->m_pNpcConfigList[npcId] = npc;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool LoadDropTables(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    game->m_DropTables.clear();

    const char* tableSql = "SELECT drop_table_id, name, description FROM drop_tables ORDER BY drop_table_id;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, tableSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DropTable table = {};
        table.id = sqlite3_column_int(stmt, 0);
        std::memset(table.name, 0, sizeof(table.name));
        std::memset(table.description, 0, sizeof(table.description));
        CopyColumnText(stmt, 1, table.name, sizeof(table.name));
        CopyColumnText(stmt, 2, table.description, sizeof(table.description));
        std::memset(table.totalWeight, 0, sizeof(table.totalWeight));
        game->m_DropTables[table.id] = table;
    }
    sqlite3_finalize(stmt);

    const char* entrySql =
        "SELECT drop_table_id, tier, item_id, weight, min_count, max_count"
        " FROM drop_entries ORDER BY drop_table_id, tier;";
    if (sqlite3_prepare_v2(db, entrySql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int tableId = sqlite3_column_int(stmt, 0);
        int tier = sqlite3_column_int(stmt, 1);
        int itemId = sqlite3_column_int(stmt, 2);
        int weight = sqlite3_column_int(stmt, 3);
        int minCount = sqlite3_column_int(stmt, 4);
        int maxCount = sqlite3_column_int(stmt, 5);

        auto it = game->m_DropTables.find(tableId);
        if (it == game->m_DropTables.end()) {
            continue;
        }
        if (tier < 1 || tier > 2) {
            continue;
        }
        if (weight <= 0) {
            continue;
        }

        DropEntry entry = {};
        entry.itemId = itemId;
        entry.weight = weight;
        entry.minCount = minCount;
        entry.maxCount = maxCount;
        it->second.tierEntries[tier].push_back(entry);
        it->second.totalWeight[tier] += weight;
    }
    sqlite3_finalize(stmt);

    return !game->m_DropTables.empty();
}

bool SaveMagicConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "magic_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO magic_configs("
        " magic_id, name, magic_type, delay_time, last_time, value1, value2, value3,"
        " value4, value5, value6, value7, value8, value9, value10, value11, value12,"
        " int_limit, gold_cost, category, attribute"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXMAGICTYPE; i++) {
        if (game->m_pMagicConfigList[i] == nullptr) {
            continue;
        }

        const CMagic* magic = game->m_pMagicConfigList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, magic->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(magic->m_dwDelayTime)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, static_cast<int>(magic->m_dwLastTime)) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue4) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue5) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue6) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue7) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue8) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue9) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue10) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue11) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sValue12) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_sIntLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_iGoldCost) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_cCategory) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, magic->m_iAttribute) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadMagicConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXMAGICTYPE; i++) {
        delete game->m_pMagicConfigList[i];
        game->m_pMagicConfigList[i] = nullptr;
    }

    const char* sql =
        "SELECT magic_id, name, magic_type, delay_time, last_time, value1, value2, value3,"
        " value4, value5, value6, value7, value8, value9, value10, value11, value12,"
        " int_limit, gold_cost, category, attribute"
        " FROM magic_configs ORDER BY magic_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int magicId = sqlite3_column_int(stmt, col++);
        if (magicId < 0 || magicId >= DEF_MAXMAGICTYPE) {
            continue;
        }

        CMagic* magic = new CMagic();
        CopyColumnText(stmt, col++, magic->m_cName, sizeof(magic->m_cName));
        magic->m_sType = (short)sqlite3_column_int(stmt, col++);
        magic->m_dwDelayTime = (uint32_t)sqlite3_column_int(stmt, col++);
        magic->m_dwLastTime = (uint32_t)sqlite3_column_int(stmt, col++);
        magic->m_sValue1 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue2 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue3 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue4 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue5 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue6 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue7 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue8 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue9 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue10 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue11 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sValue12 = (short)sqlite3_column_int(stmt, col++);
        magic->m_sIntLimit = (short)sqlite3_column_int(stmt, col++);
        magic->m_iGoldCost = sqlite3_column_int(stmt, col++);
        magic->m_cCategory = (char)sqlite3_column_int(stmt, col++);
        magic->m_iAttribute = sqlite3_column_int(stmt, col++);

        game->m_pMagicConfigList[magicId] = magic;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveSkillConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "skill_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO skill_configs("
        " skill_id, name, skill_type, value1, value2, value3, value4, value5, value6"
        ") VALUES(?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXSKILLTYPE; i++) {
        if (game->m_pSkillConfigList[i] == nullptr) {
            continue;
        }

        const CSkill* skill = game->m_pSkillConfigList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, skill->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue4) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue5) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, skill->m_sValue6) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadSkillConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXSKILLTYPE; i++) {
        delete game->m_pSkillConfigList[i];
        game->m_pSkillConfigList[i] = nullptr;
    }

    const char* sql =
        "SELECT skill_id, name, skill_type, value1, value2, value3, value4, value5, value6"
        " FROM skill_configs ORDER BY skill_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int skillId = sqlite3_column_int(stmt, col++);
        if (skillId < 0 || skillId >= DEF_MAXSKILLTYPE) {
            continue;
        }

        CSkill* skill = new CSkill();
        CopyColumnText(stmt, col++, skill->m_cName, sizeof(skill->m_cName));
        skill->m_sType = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue1 = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue2 = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue3 = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue4 = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue5 = (short)sqlite3_column_int(stmt, col++);
        skill->m_sValue6 = (short)sqlite3_column_int(stmt, col++);

        game->m_pSkillConfigList[skillId] = skill;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveQuestConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "quest_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO quest_configs("
        " quest_index, side, quest_type, target_type, max_count, quest_from, min_level, max_level,"
        " required_skill_num, required_skill_level, time_limit, assign_type,"
        " reward_type1, reward_amount1, reward_type2, reward_amount2, reward_type3, reward_amount3,"
        " contribution, contribution_limit, response_mode, target_name, target_x, target_y, target_range,"
        " quest_id, req_contribution"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXQUESTTYPE; i++) {
        if (game->m_pQuestConfigList[i] == nullptr) {
            continue;
        }

        const CQuest* quest = game->m_pQuestConfigList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_cSide) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iTargetType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iMaxCount) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iFrom) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iMinLevel) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iMaxLevel) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRequiredSkillNum) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRequiredSkillLevel) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iTimeLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iAssignType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardType[1]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardAmount[1]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardType[2]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardAmount[2]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardType[3]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRewardAmount[3]) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iContribution) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iContributionLimit) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iResponseMode) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, quest->m_cTargetName);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_sX) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_sY) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iRange) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iQuestID) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, quest->m_iReqContribution) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadQuestConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXQUESTTYPE; i++) {
        delete game->m_pQuestConfigList[i];
        game->m_pQuestConfigList[i] = nullptr;
    }

    const char* sql =
        "SELECT quest_index, side, quest_type, target_type, max_count, quest_from, min_level, max_level,"
        " required_skill_num, required_skill_level, time_limit, assign_type,"
        " reward_type1, reward_amount1, reward_type2, reward_amount2, reward_type3, reward_amount3,"
        " contribution, contribution_limit, response_mode, target_name, target_x, target_y, target_range,"
        " quest_id, req_contribution"
        " FROM quest_configs ORDER BY quest_index;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int questIndex = sqlite3_column_int(stmt, col++);
        if (questIndex < 0 || questIndex >= DEF_MAXQUESTTYPE) {
            continue;
        }

        CQuest* quest = new CQuest();
        quest->m_cSide = (char)sqlite3_column_int(stmt, col++);
        quest->m_iType = sqlite3_column_int(stmt, col++);
        quest->m_iTargetType = sqlite3_column_int(stmt, col++);
        quest->m_iMaxCount = sqlite3_column_int(stmt, col++);
        quest->m_iFrom = sqlite3_column_int(stmt, col++);
        quest->m_iMinLevel = sqlite3_column_int(stmt, col++);
        quest->m_iMaxLevel = sqlite3_column_int(stmt, col++);
        quest->m_iRequiredSkillNum = sqlite3_column_int(stmt, col++);
        quest->m_iRequiredSkillLevel = sqlite3_column_int(stmt, col++);
        quest->m_iTimeLimit = sqlite3_column_int(stmt, col++);
        quest->m_iAssignType = sqlite3_column_int(stmt, col++);
        quest->m_iRewardType[1] = sqlite3_column_int(stmt, col++);
        quest->m_iRewardAmount[1] = sqlite3_column_int(stmt, col++);
        quest->m_iRewardType[2] = sqlite3_column_int(stmt, col++);
        quest->m_iRewardAmount[2] = sqlite3_column_int(stmt, col++);
        quest->m_iRewardType[3] = sqlite3_column_int(stmt, col++);
        quest->m_iRewardAmount[3] = sqlite3_column_int(stmt, col++);
        quest->m_iContribution = sqlite3_column_int(stmt, col++);
        quest->m_iContributionLimit = sqlite3_column_int(stmt, col++);
        quest->m_iResponseMode = sqlite3_column_int(stmt, col++);
        CopyColumnText(stmt, col++, quest->m_cTargetName, sizeof(quest->m_cTargetName));
        quest->m_sX = sqlite3_column_int(stmt, col++);
        quest->m_sY = sqlite3_column_int(stmt, col++);
        quest->m_iRange = sqlite3_column_int(stmt, col++);
        quest->m_iQuestID = sqlite3_column_int(stmt, col++);
        quest->m_iReqContribution = sqlite3_column_int(stmt, col++);

        game->m_pQuestConfigList[questIndex] = quest;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SavePortionConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "potion_configs") || !ClearTable(db, "crafting_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* potionSql =
        "INSERT INTO potion_configs("
        " potion_id, name, array0, array1, array2, array3, array4, array5, array6,"
        " array7, array8, array9, array10, array11, skill_limit, difficulty"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    const char* craftingSql =
        "INSERT INTO crafting_configs("
        " crafting_id, name, array0, array1, array2, array3, array4, array5, array6,"
        " array7, array8, array9, array10, array11, skill_limit, difficulty"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* potionStmt = nullptr;
    sqlite3_stmt* craftingStmt = nullptr;
    if (sqlite3_prepare_v2(db, potionSql, -1, &potionStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, craftingSql, -1, &craftingStmt, nullptr) != SQLITE_OK) {
        if (potionStmt) sqlite3_finalize(potionStmt);
        if (craftingStmt) sqlite3_finalize(craftingStmt);
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXPORTIONTYPES; i++) {
        if (game->m_pPortionConfigList[i] != nullptr) {
            const CPortion* potion = game->m_pPortionConfigList[i];
            sqlite3_reset(potionStmt);
            sqlite3_clear_bindings(potionStmt);
            int col = 1;
            bool ok = true;
            ok &= (sqlite3_bind_int(potionStmt, col++, i) == SQLITE_OK);
            ok &= PrepareAndBindText(potionStmt, col++, potion->m_cName);
            for (int a = 0; a < 12; a++) {
                ok &= (sqlite3_bind_int(potionStmt, col++, potion->m_sArray[a]) == SQLITE_OK);
            }
            ok &= (sqlite3_bind_int(potionStmt, col++, potion->m_iSkillLimit) == SQLITE_OK);
            ok &= (sqlite3_bind_int(potionStmt, col++, potion->m_iDifficulty) == SQLITE_OK);
            if (!ok || sqlite3_step(potionStmt) != SQLITE_DONE) {
                sqlite3_finalize(potionStmt);
                sqlite3_finalize(craftingStmt);
                RollbackTransaction(db);
                return false;
            }
        }

        if (game->m_pCraftingConfigList[i] != nullptr) {
            const CPortion* crafting = game->m_pCraftingConfigList[i];
            sqlite3_reset(craftingStmt);
            sqlite3_clear_bindings(craftingStmt);
            int col = 1;
            bool ok = true;
            ok &= (sqlite3_bind_int(craftingStmt, col++, i) == SQLITE_OK);
            ok &= PrepareAndBindText(craftingStmt, col++, crafting->m_cName);
            for (int a = 0; a < 12; a++) {
                ok &= (sqlite3_bind_int(craftingStmt, col++, crafting->m_sArray[a]) == SQLITE_OK);
            }
            ok &= (sqlite3_bind_int(craftingStmt, col++, crafting->m_iSkillLimit) == SQLITE_OK);
            ok &= (sqlite3_bind_int(craftingStmt, col++, crafting->m_iDifficulty) == SQLITE_OK);
            if (!ok || sqlite3_step(craftingStmt) != SQLITE_DONE) {
                sqlite3_finalize(potionStmt);
                sqlite3_finalize(craftingStmt);
                RollbackTransaction(db);
                return false;
            }
        }
    }

    sqlite3_finalize(potionStmt);
    sqlite3_finalize(craftingStmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadPortionConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXPORTIONTYPES; i++) {
        delete game->m_pPortionConfigList[i];
        game->m_pPortionConfigList[i] = nullptr;
        delete game->m_pCraftingConfigList[i];
        game->m_pCraftingConfigList[i] = nullptr;
    }

    const char* potionSql =
        "SELECT potion_id, name, array0, array1, array2, array3, array4, array5, array6,"
        " array7, array8, array9, array10, array11, skill_limit, difficulty"
        " FROM potion_configs ORDER BY potion_id;";
    const char* craftingSql =
        "SELECT crafting_id, name, array0, array1, array2, array3, array4, array5, array6,"
        " array7, array8, array9, array10, array11, skill_limit, difficulty"
        " FROM crafting_configs ORDER BY crafting_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, potionSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int potionId = sqlite3_column_int(stmt, col++);
        if (potionId < 0 || potionId >= DEF_MAXPORTIONTYPES) {
            continue;
        }
        CPortion* potion = new CPortion();
        CopyColumnText(stmt, col++, potion->m_cName, sizeof(potion->m_cName));
        for (int a = 0; a < 12; a++) {
            potion->m_sArray[a] = (short)sqlite3_column_int(stmt, col++);
        }
        potion->m_iSkillLimit = sqlite3_column_int(stmt, col++);
        potion->m_iDifficulty = sqlite3_column_int(stmt, col++);
        game->m_pPortionConfigList[potionId] = potion;
    }

    sqlite3_finalize(stmt);
    if (sqlite3_prepare_v2(db, craftingSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int craftingId = sqlite3_column_int(stmt, col++);
        if (craftingId < 0 || craftingId >= DEF_MAXPORTIONTYPES) {
            continue;
        }
        CPortion* crafting = new CPortion();
        CopyColumnText(stmt, col++, crafting->m_cName, sizeof(crafting->m_cName));
        for (int a = 0; a < 12; a++) {
            crafting->m_sArray[a] = (short)sqlite3_column_int(stmt, col++);
        }
        crafting->m_iSkillLimit = sqlite3_column_int(stmt, col++);
        crafting->m_iDifficulty = sqlite3_column_int(stmt, col++);
        game->m_pCraftingConfigList[craftingId] = crafting;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveBuildItemConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "builditem_configs")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO builditem_configs("
        " build_id, name, skill_limit, material_id1, material_count1, material_value1,"
        " material_id2, material_count2, material_value2, material_id3, material_count3, material_value3,"
        " material_id4, material_count4, material_value4, material_id5, material_count5, material_value5,"
        " material_id6, material_count6, material_value6, average_value, max_skill, attribute, item_id"
        ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXBUILDITEMS; i++) {
        if (game->m_pBuildItemList[i] == nullptr) {
            continue;
        }

        const CBuildItem* build = game->m_pBuildItemList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, col++, build->m_cName);
        ok &= (sqlite3_bind_int(stmt, col++, build->m_iSkillLimit) == SQLITE_OK);
        for (int a = 0; a < 6; a++) {
            ok &= (sqlite3_bind_int(stmt, col++, build->m_iMaterialItemID[a]) == SQLITE_OK);
            ok &= (sqlite3_bind_int(stmt, col++, build->m_iMaterialItemCount[a]) == SQLITE_OK);
            ok &= (sqlite3_bind_int(stmt, col++, build->m_iMaterialItemValue[a]) == SQLITE_OK);
        }
        ok &= (sqlite3_bind_int(stmt, col++, build->m_iAverageValue) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, build->m_iMaxSkill) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, build->m_wAttribute) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, build->m_sItemID) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadBuildItemConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXBUILDITEMS; i++) {
        delete game->m_pBuildItemList[i];
        game->m_pBuildItemList[i] = nullptr;
    }

    const char* sql =
        "SELECT build_id, name, skill_limit, material_id1, material_count1, material_value1,"
        " material_id2, material_count2, material_value2, material_id3, material_count3, material_value3,"
        " material_id4, material_count4, material_value4, material_id5, material_count5, material_value5,"
        " material_id6, material_count6, material_value6, average_value, max_skill, attribute, item_id"
        " FROM builditem_configs ORDER BY build_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int buildId = sqlite3_column_int(stmt, col++);
        if (buildId < 0 || buildId >= DEF_MAXBUILDITEMS) {
            continue;
        }

        CBuildItem* build = new CBuildItem();
        CopyColumnText(stmt, col++, build->m_cName, sizeof(build->m_cName));
        build->m_iSkillLimit = sqlite3_column_int(stmt, col++);
        for (int a = 0; a < 6; a++) {
            build->m_iMaterialItemID[a] = sqlite3_column_int(stmt, col++);
            build->m_iMaterialItemCount[a] = sqlite3_column_int(stmt, col++);
            build->m_iMaterialItemValue[a] = sqlite3_column_int(stmt, col++);
        }
        build->m_iAverageValue = sqlite3_column_int(stmt, col++);
        build->m_iMaxSkill = sqlite3_column_int(stmt, col++);
        build->m_wAttribute = (uint16_t)sqlite3_column_int(stmt, col++);
        int storedItemId = sqlite3_column_int(stmt, col++);
        (void)storedItemId;

        CItem tempItem;
        if (game->_bInitItemAttr(&tempItem, build->m_cName)) {
            build->m_sItemID = tempItem.m_sIDnum;
        } else {
            delete build;
            sqlite3_finalize(stmt);
            return false;
        }

        build->m_iMaxValue = 0;
        for (int a = 0; a < 6; a++) {
            build->m_iMaxValue += (build->m_iMaterialItemValue[a] * 100);
        }

        game->m_pBuildItemList[buildId] = build;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveDupItemIdConfigs(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "dup_item_ids")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO dup_item_ids("
        " dup_id, touch_effect_type, touch_effect_value1, touch_effect_value2, touch_effect_value3, price"
        ") VALUES(?,?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXDUPITEMID; i++) {
        if (game->m_pDupItemIDList[i] == nullptr) {
            continue;
        }

        const CItem* item = game->m_pDupItemIDList[i];
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        int col = 1;
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, col++, i) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, item->m_sTouchEffectType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, item->m_sTouchEffectValue1) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, item->m_sTouchEffectValue2) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, item->m_sTouchEffectValue3) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, col++, item->m_wPrice) == SQLITE_OK);

        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadDupItemIdConfigs(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXDUPITEMID; i++) {
        delete game->m_pDupItemIDList[i];
        game->m_pDupItemIDList[i] = nullptr;
    }

    const char* sql =
        "SELECT dup_id, touch_effect_type, touch_effect_value1, touch_effect_value2, touch_effect_value3, price"
        " FROM dup_item_ids ORDER BY dup_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int dupId = sqlite3_column_int(stmt, col++);
        if (dupId < 0 || dupId >= DEF_MAXDUPITEMID) {
            continue;
        }

        CItem* item = new CItem();
        item->m_sTouchEffectType = (short)sqlite3_column_int(stmt, col++);
        item->m_sTouchEffectValue1 = (short)sqlite3_column_int(stmt, col++);
        item->m_sTouchEffectValue2 = (short)sqlite3_column_int(stmt, col++);
        item->m_sTouchEffectValue3 = (short)sqlite3_column_int(stmt, col++);
        item->m_wPrice = (uint16_t)sqlite3_column_int(stmt, col++);

        game->m_pDupItemIDList[dupId] = item;
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveCrusadeConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "crusade_structures")) {
        RollbackTransaction(db);
        return false;
    }

    const char* sql =
        "INSERT INTO crusade_structures("
        " structure_id, map_name, structure_type, pos_x, pos_y"
        ") VALUES(?,?,?,?,?);";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXCRUSADESTRUCTURES; i++) {
        const auto& entry = game->m_stCrusadeStructures[i];
        if (entry.cType == 0 || entry.cMapName[0] == 0) {
            continue;
        }
        sqlite3_reset(stmt);
        sqlite3_clear_bindings(stmt);
        bool ok = true;
        ok &= (sqlite3_bind_int(stmt, 1, i) == SQLITE_OK);
        ok &= PrepareAndBindText(stmt, 2, entry.cMapName);
        ok &= (sqlite3_bind_int(stmt, 3, entry.cType) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 4, entry.dX) == SQLITE_OK);
        ok &= (sqlite3_bind_int(stmt, 5, entry.dY) == SQLITE_OK);
        if (!ok || sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            RollbackTransaction(db);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadCrusadeConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXCRUSADESTRUCTURES; i++) {
        std::memset(game->m_stCrusadeStructures[i].cMapName, 0, sizeof(game->m_stCrusadeStructures[i].cMapName));
        game->m_stCrusadeStructures[i].cType = 0;
        game->m_stCrusadeStructures[i].dX = 0;
        game->m_stCrusadeStructures[i].dY = 0;
    }

    const char* sql =
        "SELECT structure_id, map_name, structure_type, pos_x, pos_y"
        " FROM crusade_structures ORDER BY structure_id;";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int col = 0;
        int structureId = sqlite3_column_int(stmt, col++);
        if (structureId < 0 || structureId >= DEF_MAXCRUSADESTRUCTURES) {
            continue;
        }
        CopyColumnText(stmt, col++, game->m_stCrusadeStructures[structureId].cMapName,
            sizeof(game->m_stCrusadeStructures[structureId].cMapName));
        game->m_stCrusadeStructures[structureId].cType = (char)sqlite3_column_int(stmt, col++);
        game->m_stCrusadeStructures[structureId].dX = sqlite3_column_int(stmt, col++);
        game->m_stCrusadeStructures[structureId].dY = sqlite3_column_int(stmt, col++);
    }

    sqlite3_finalize(stmt);
    return true;
}

bool SaveScheduleConfig(sqlite3* db, const CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    if (!BeginTransaction(db)) {
        return false;
    }

    if (!ClearTable(db, "schedule_crusade") ||
        !ClearTable(db, "schedule_apocalypse_start") ||
        !ClearTable(db, "schedule_apocalypse_end") ||
        !ClearTable(db, "schedule_heldenian")) {
        RollbackTransaction(db);
        return false;
    }

    sqlite3_stmt* crusadeStmt = nullptr;
    sqlite3_stmt* apocStartStmt = nullptr;
    sqlite3_stmt* apocEndStmt = nullptr;
    sqlite3_stmt* heldenStmt = nullptr;
    if (sqlite3_prepare_v2(db, "INSERT INTO schedule_crusade(schedule_id, day, hour, minute) VALUES(?,?,?,?);", -1, &crusadeStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "INSERT INTO schedule_apocalypse_start(schedule_id, day, hour, minute) VALUES(?,?,?,?);", -1, &apocStartStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "INSERT INTO schedule_apocalypse_end(schedule_id, day, hour, minute) VALUES(?,?,?,?);", -1, &apocEndStmt, nullptr) != SQLITE_OK ||
        sqlite3_prepare_v2(db, "INSERT INTO schedule_heldenian(schedule_id, day, start_hour, start_minute, end_hour, end_minute) VALUES(?,?,?,?,?,?);", -1, &heldenStmt, nullptr) != SQLITE_OK) {
        if (crusadeStmt) sqlite3_finalize(crusadeStmt);
        if (apocStartStmt) sqlite3_finalize(apocStartStmt);
        if (apocEndStmt) sqlite3_finalize(apocEndStmt);
        if (heldenStmt) sqlite3_finalize(heldenStmt);
        RollbackTransaction(db);
        return false;
    }

    for (int i = 0; i < DEF_MAXSCHEDULE; i++) {
        if (game->m_stCrusadeWarSchedule[i].iDay >= 0) {
            sqlite3_reset(crusadeStmt);
            sqlite3_clear_bindings(crusadeStmt);
            bool ok = true;
            ok &= (sqlite3_bind_int(crusadeStmt, 1, i) == SQLITE_OK);
            ok &= (sqlite3_bind_int(crusadeStmt, 2, game->m_stCrusadeWarSchedule[i].iDay) == SQLITE_OK);
            ok &= (sqlite3_bind_int(crusadeStmt, 3, game->m_stCrusadeWarSchedule[i].iHour) == SQLITE_OK);
            ok &= (sqlite3_bind_int(crusadeStmt, 4, game->m_stCrusadeWarSchedule[i].iMinute) == SQLITE_OK);
            if (!ok || sqlite3_step(crusadeStmt) != SQLITE_DONE) {
                sqlite3_finalize(crusadeStmt);
                sqlite3_finalize(apocStartStmt);
                sqlite3_finalize(apocEndStmt);
                sqlite3_finalize(heldenStmt);
                RollbackTransaction(db);
                return false;
            }
        }
    }

    for (int i = 0; i < DEF_MAXAPOCALYPSE; i++) {
        if (game->m_stApocalypseScheduleStart[i].iDay >= 0) {
            sqlite3_reset(apocStartStmt);
            sqlite3_clear_bindings(apocStartStmt);
            bool ok = true;
            ok &= (sqlite3_bind_int(apocStartStmt, 1, i) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocStartStmt, 2, game->m_stApocalypseScheduleStart[i].iDay) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocStartStmt, 3, game->m_stApocalypseScheduleStart[i].iHour) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocStartStmt, 4, game->m_stApocalypseScheduleStart[i].iMinute) == SQLITE_OK);
            if (!ok || sqlite3_step(apocStartStmt) != SQLITE_DONE) {
                sqlite3_finalize(crusadeStmt);
                sqlite3_finalize(apocStartStmt);
                sqlite3_finalize(apocEndStmt);
                sqlite3_finalize(heldenStmt);
                RollbackTransaction(db);
                return false;
            }
        }

        if (game->m_stApocalypseScheduleEnd[i].iDay >= 0) {
            sqlite3_reset(apocEndStmt);
            sqlite3_clear_bindings(apocEndStmt);
            bool ok = true;
            ok &= (sqlite3_bind_int(apocEndStmt, 1, i) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocEndStmt, 2, game->m_stApocalypseScheduleEnd[i].iDay) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocEndStmt, 3, game->m_stApocalypseScheduleEnd[i].iHour) == SQLITE_OK);
            ok &= (sqlite3_bind_int(apocEndStmt, 4, game->m_stApocalypseScheduleEnd[i].iMinute) == SQLITE_OK);
            if (!ok || sqlite3_step(apocEndStmt) != SQLITE_DONE) {
                sqlite3_finalize(crusadeStmt);
                sqlite3_finalize(apocStartStmt);
                sqlite3_finalize(apocEndStmt);
                sqlite3_finalize(heldenStmt);
                RollbackTransaction(db);
                return false;
            }
        }
    }

    for (int i = 0; i < DEF_MAXHELDENIAN; i++) {
        if (game->m_stHeldenianSchedule[i].iDay >= 0) {
            sqlite3_reset(heldenStmt);
            sqlite3_clear_bindings(heldenStmt);
            bool ok = true;
            ok &= (sqlite3_bind_int(heldenStmt, 1, i) == SQLITE_OK);
            ok &= (sqlite3_bind_int(heldenStmt, 2, game->m_stHeldenianSchedule[i].iDay) == SQLITE_OK);
            ok &= (sqlite3_bind_int(heldenStmt, 3, game->m_stHeldenianSchedule[i].StartiHour) == SQLITE_OK);
            ok &= (sqlite3_bind_int(heldenStmt, 4, game->m_stHeldenianSchedule[i].StartiMinute) == SQLITE_OK);
            ok &= (sqlite3_bind_int(heldenStmt, 5, game->m_stHeldenianSchedule[i].EndiHour) == SQLITE_OK);
            ok &= (sqlite3_bind_int(heldenStmt, 6, game->m_stHeldenianSchedule[i].EndiMinute) == SQLITE_OK);
            if (!ok || sqlite3_step(heldenStmt) != SQLITE_DONE) {
                sqlite3_finalize(crusadeStmt);
                sqlite3_finalize(apocStartStmt);
                sqlite3_finalize(apocEndStmt);
                sqlite3_finalize(heldenStmt);
                RollbackTransaction(db);
                return false;
            }
        }
    }

    sqlite3_finalize(crusadeStmt);
    sqlite3_finalize(apocStartStmt);
    sqlite3_finalize(apocEndStmt);
    sqlite3_finalize(heldenStmt);

    if (!CommitTransaction(db)) {
        RollbackTransaction(db);
        return false;
    }
    return true;
}

bool LoadScheduleConfig(sqlite3* db, CGame* game)
{
    if (db == nullptr || game == nullptr) {
        return false;
    }

    for (int i = 0; i < DEF_MAXSCHEDULE; i++) {
        game->m_stCrusadeWarSchedule[i].iDay = -1;
        game->m_stCrusadeWarSchedule[i].iHour = -1;
        game->m_stCrusadeWarSchedule[i].iMinute = -1;
    }
    for (int i = 0; i < DEF_MAXAPOCALYPSE; i++) {
        game->m_stApocalypseScheduleStart[i].iDay = -1;
        game->m_stApocalypseScheduleStart[i].iHour = -1;
        game->m_stApocalypseScheduleStart[i].iMinute = -1;
        game->m_stApocalypseScheduleEnd[i].iDay = -1;
        game->m_stApocalypseScheduleEnd[i].iHour = -1;
        game->m_stApocalypseScheduleEnd[i].iMinute = -1;
    }
    for (int i = 0; i < DEF_MAXHELDENIAN; i++) {
        game->m_stHeldenianSchedule[i].iDay = -1;
        game->m_stHeldenianSchedule[i].StartiHour = -1;
        game->m_stHeldenianSchedule[i].StartiMinute = -1;
        game->m_stHeldenianSchedule[i].EndiHour = -1;
        game->m_stHeldenianSchedule[i].EndiMinute = -1;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT schedule_id, day, hour, minute FROM schedule_crusade ORDER BY schedule_id;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        if (idx < 0 || idx >= DEF_MAXSCHEDULE) {
            continue;
        }
        game->m_stCrusadeWarSchedule[idx].iDay = sqlite3_column_int(stmt, 1);
        game->m_stCrusadeWarSchedule[idx].iHour = sqlite3_column_int(stmt, 2);
        game->m_stCrusadeWarSchedule[idx].iMinute = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, "SELECT schedule_id, day, hour, minute FROM schedule_apocalypse_start ORDER BY schedule_id;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        if (idx < 0 || idx >= DEF_MAXAPOCALYPSE) {
            continue;
        }
        game->m_stApocalypseScheduleStart[idx].iDay = sqlite3_column_int(stmt, 1);
        game->m_stApocalypseScheduleStart[idx].iHour = sqlite3_column_int(stmt, 2);
        game->m_stApocalypseScheduleStart[idx].iMinute = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, "SELECT schedule_id, day, hour, minute FROM schedule_apocalypse_end ORDER BY schedule_id;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        if (idx < 0 || idx >= DEF_MAXAPOCALYPSE) {
            continue;
        }
        game->m_stApocalypseScheduleEnd[idx].iDay = sqlite3_column_int(stmt, 1);
        game->m_stApocalypseScheduleEnd[idx].iHour = sqlite3_column_int(stmt, 2);
        game->m_stApocalypseScheduleEnd[idx].iMinute = sqlite3_column_int(stmt, 3);
    }
    sqlite3_finalize(stmt);

    if (sqlite3_prepare_v2(db, "SELECT schedule_id, day, start_hour, start_minute, end_hour, end_minute FROM schedule_heldenian ORDER BY schedule_id;", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        if (idx < 0 || idx >= DEF_MAXHELDENIAN) {
            continue;
        }
        game->m_stHeldenianSchedule[idx].iDay = sqlite3_column_int(stmt, 1);
        game->m_stHeldenianSchedule[idx].StartiHour = sqlite3_column_int(stmt, 2);
        game->m_stHeldenianSchedule[idx].StartiMinute = sqlite3_column_int(stmt, 3);
        game->m_stHeldenianSchedule[idx].EndiHour = sqlite3_column_int(stmt, 4);
        game->m_stHeldenianSchedule[idx].EndiMinute = sqlite3_column_int(stmt, 5);
    }
    sqlite3_finalize(stmt);

    return true;
}

bool HasGameConfigRows(sqlite3* db, const char* tableName)
{
    if (db == nullptr || tableName == nullptr || tableName[0] == 0) {
        return false;
    }

    char sql[256] = {};
    std::snprintf(sql, sizeof(sql), "SELECT 1 FROM %s LIMIT 1;", tableName);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool hasRow = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return hasRow;
}

void CloseGameConfigDatabase(sqlite3* db)
{
    if (db != nullptr) {
        sqlite3_close(db);
    }
}
