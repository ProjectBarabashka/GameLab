// accounts.hpp — AETHORIA: Аккаунты + Персонажи (Приоритет 5 редмапа)
// SQLite: таблицы accounts + characters.
// Поток: login → character_select → enter_world.
//
// Зависимость: sqlite3 (один .c файл, добавь в проект или:
//   Windows: скачай amalgamation с https://sqlite.org/download.html
//   Linux:   apt install libsqlite3-dev
//   Link:    -lsqlite3

#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <functional>

// Если sqlite3.h недоступен — закомментируй #define USE_SQLITE
// и подключи SQLite как amalgamation
#define USE_SQLITE
#ifdef USE_SQLITE
#include <sqlite3.h>
#endif

// ═══════════════════════════════════════════════════════════════
// СТРУКТУРА АККАУНТА
// ═══════════════════════════════════════════════════════════════
struct Account {
    int         id       = 0;
    std::string username;
    std::string email;
    bool        banned   = false;
    bool        verified = false;
    std::string createdAt;
    int         lastLogin = 0;
};

// ═══════════════════════════════════════════════════════════════
// СТРУКТУРА ПЕРСОНАЖА
// ═══════════════════════════════════════════════════════════════
struct CharacterRecord {
    int         id          = 0;
    int         accountId   = 0;
    std::string name;
    std::string className;    // "WARRIOR", "MAGE" и т.д.
    int         level       = 1;
    int         experience  = 0;
    float       posX        = 1920.f;  // 60*32
    float       posY        = 1920.f;
    std::string currentScene = "aethoria_city";
    int         hp          = 100;
    int         maxHp       = 100;
    int         mp          = 50;
    int         maxMp       = 50;
    int         gold        = 100;
    std::string inventoryJson = "[]";
    std::string createdAt;
};

// ═══════════════════════════════════════════════════════════════
// РЕЗУЛЬТАТ АУТЕНТИФИКАЦИИ
// ═══════════════════════════════════════════════════════════════
enum class AuthResult {
    OK,
    WRONG_PASSWORD,
    NOT_FOUND,
    BANNED,
    ALREADY_EXISTS,
    DB_ERROR
};

inline std::string authResultStr(AuthResult r) {
    switch (r) {
        case AuthResult::OK:             return "OK";
        case AuthResult::WRONG_PASSWORD: return "Неверный пароль";
        case AuthResult::NOT_FOUND:      return "Аккаунт не найден";
        case AuthResult::BANNED:         return "Аккаунт заблокирован";
        case AuthResult::ALREADY_EXISTS: return "Логин уже занят";
        case AuthResult::DB_ERROR:       return "Ошибка базы данных";
        default: return "Ошибка";
    }
}

// ═══════════════════════════════════════════════════════════════
// МЕНЕДЖЕР АККАУНТОВ
// ═══════════════════════════════════════════════════════════════
class AccountManager {
public:
    explicit AccountManager(const std::string& dbPath = "accounts.db")
        : dbPath_(dbPath) {}

    ~AccountManager() {
#ifdef USE_SQLITE
        if (db_) sqlite3_close(db_);
#endif
    }

    // ── Открыть / создать БД ─────────────────────────────────
    bool open() {
#ifndef USE_SQLITE
        std::cerr << "[Accounts] SQLite не скомпилирован\n";
        return false;
#else
        if (sqlite3_open(dbPath_.c_str(), &db_) != SQLITE_OK) {
            std::cerr << "[Accounts] Не открыть БД: "
                      << sqlite3_errmsg(db_) << "\n";
            return false;
        }
        _initSchema();
        std::cout << "[Accounts] БД открыта: " << dbPath_ << "\n";
        return true;
#endif
    }

    // ── Регистрация ──────────────────────────────────────────
    AuthResult registerAccount(const std::string& username,
                                const std::string& password,
                                const std::string& email = "") {
#ifndef USE_SQLITE
        return AuthResult::DB_ERROR;
#else
        if (!db_) return AuthResult::DB_ERROR;
        // Проверяем дубликат
        Account existing;
        if (_findByUsername(username, existing)) return AuthResult::ALREADY_EXISTS;

        std::string hash = _hashPassword(password);
        const char* sql =
            "INSERT INTO accounts (username, password_hash, email, created_at) "
            "VALUES (?, ?, ?, datetime('now'))";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return AuthResult::DB_ERROR;
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, hash.c_str(),     -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, email.c_str(),    -1, SQLITE_TRANSIENT);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        if (rc != SQLITE_DONE) return AuthResult::DB_ERROR;
        std::cout << "[Accounts] Зарегистрирован: " << username << "\n";
        return AuthResult::OK;
#endif
    }

    // ── Логин ────────────────────────────────────────────────
    // Возвращает заполненный Account при успехе
    AuthResult login(const std::string& username,
                     const std::string& password,
                     Account& outAccount) {
#ifndef USE_SQLITE
        return AuthResult::DB_ERROR;
#else
        if (!db_) return AuthResult::DB_ERROR;
        if (!_findByUsername(username, outAccount)) return AuthResult::NOT_FOUND;
        if (outAccount.banned) return AuthResult::BANNED;

        std::string hash = _hashPassword(password);
        const char* sql =
            "SELECT password_hash FROM accounts WHERE username=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return AuthResult::DB_ERROR;
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        AuthResult result = AuthResult::WRONG_PASSWORD;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string stored = (const char*)sqlite3_column_text(stmt, 0);
            if (stored == hash) {
                result = AuthResult::OK;
                _updateLastLogin(outAccount.id);
            }
        }
        sqlite3_finalize(stmt);
        if (result == AuthResult::OK)
            std::cout << "[Accounts] Логин: " << username << " id=" << outAccount.id << "\n";
        return result;
#endif
    }

    // ── Персонажи аккаунта ──────────────────────────────────
    std::vector<CharacterRecord> getCharacters(int accountId) {
        std::vector<CharacterRecord> result;
#ifdef USE_SQLITE
        if (!db_) return result;
        const char* sql =
            "SELECT id,account_id,name,class,level,xp,pos_x,pos_y,scene,"
            "hp,max_hp,mp,max_mp,gold,inventory,created_at "
            "FROM characters WHERE account_id=? ORDER BY id";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return result;
        sqlite3_bind_int(stmt, 1, accountId);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            CharacterRecord c;
            c.id           = sqlite3_column_int(stmt, 0);
            c.accountId    = sqlite3_column_int(stmt, 1);
            c.name         = _col(stmt, 2);
            c.className    = _col(stmt, 3);
            c.level        = sqlite3_column_int(stmt, 4);
            c.experience   = sqlite3_column_int(stmt, 5);
            c.posX         = (float)sqlite3_column_double(stmt, 6);
            c.posY         = (float)sqlite3_column_double(stmt, 7);
            c.currentScene = _col(stmt, 8);
            c.hp           = sqlite3_column_int(stmt, 9);
            c.maxHp        = sqlite3_column_int(stmt, 10);
            c.mp           = sqlite3_column_int(stmt, 11);
            c.maxMp        = sqlite3_column_int(stmt, 12);
            c.gold         = sqlite3_column_int(stmt, 13);
            c.inventoryJson= _col(stmt, 14);
            c.createdAt    = _col(stmt, 15);
            result.push_back(c);
        }
        sqlite3_finalize(stmt);
#endif
        return result;
    }

    // ── Создать персонажа ────────────────────────────────────
    bool createCharacter(int accountId,
                         const std::string& name,
                         const std::string& className,
                         CharacterRecord& outChar) {
#ifndef USE_SQLITE
        return false;
#else
        if (!db_) return false;
        // Лимит 3 персонажа на аккаунт
        auto chars = getCharacters(accountId);
        if ((int)chars.size() >= 3) {
            std::cerr << "[Accounts] Лимит персонажей (3)\n"; return false;
        }
        const char* sql =
            "INSERT INTO characters (account_id,name,class,level,xp,"
            "pos_x,pos_y,scene,hp,max_hp,mp,max_mp,gold,inventory,created_at) "
            "VALUES (?,?,?,1,0,1920,1920,'aethoria_city',100,100,50,50,100,'[]',datetime('now'))";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int (stmt, 1, accountId);
        sqlite3_bind_text(stmt, 2, name.c_str(),      -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, className.c_str(), -1, SQLITE_TRANSIENT);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        if (ok) {
            outChar.id        = (int)sqlite3_last_insert_rowid(db_);
            outChar.accountId = accountId;
            outChar.name      = name;
            outChar.className = className;
            outChar.level     = 1;
            std::cout << "[Accounts] Персонаж создан: " << name << " (" << className << ")\n";
        }
        return ok;
#endif
    }

    // ── Сохранить прогресс персонажа ────────────────────────
    bool saveCharacter(const CharacterRecord& c) {
#ifndef USE_SQLITE
        return false;
#else
        if (!db_) return false;
        const char* sql =
            "UPDATE characters SET level=?,xp=?,pos_x=?,pos_y=?,scene=?,"
            "hp=?,max_hp=?,mp=?,max_mp=?,gold=?,inventory=? WHERE id=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int   (stmt, 1, c.level);
        sqlite3_bind_int   (stmt, 2, c.experience);
        sqlite3_bind_double(stmt, 3, c.posX);
        sqlite3_bind_double(stmt, 4, c.posY);
        sqlite3_bind_text  (stmt, 5, c.currentScene.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int   (stmt, 6, c.hp);
        sqlite3_bind_int   (stmt, 7, c.maxHp);
        sqlite3_bind_int   (stmt, 8, c.mp);
        sqlite3_bind_int   (stmt, 9, c.maxMp);
        sqlite3_bind_int   (stmt, 10, c.gold);
        sqlite3_bind_text  (stmt, 11, c.inventoryJson.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int   (stmt, 12, c.id);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
#endif
    }

    // ── Удалить персонажа ────────────────────────────────────
    bool deleteCharacter(int charId, int accountId) {
#ifndef USE_SQLITE
        return false;
#else
        if (!db_) return false;
        const char* sql = "DELETE FROM characters WHERE id=? AND account_id=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, charId);
        sqlite3_bind_int(stmt, 2, accountId);
        bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return ok;
#endif
    }

private:
#ifdef USE_SQLITE
    sqlite3*    db_     = nullptr;
#endif
    std::string dbPath_;

    void _initSchema() {
#ifdef USE_SQLITE
        const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS accounts (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    email         TEXT DEFAULT '',
    banned        INTEGER DEFAULT 0,
    verified      INTEGER DEFAULT 0,
    last_login    INTEGER DEFAULT 0,
    created_at    TEXT DEFAULT (datetime('now'))
);
CREATE TABLE IF NOT EXISTS characters (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    account_id    INTEGER NOT NULL,
    name          TEXT NOT NULL,
    class         TEXT NOT NULL DEFAULT 'WARRIOR',
    level         INTEGER DEFAULT 1,
    xp            INTEGER DEFAULT 0,
    pos_x         REAL DEFAULT 1920,
    pos_y         REAL DEFAULT 1920,
    scene         TEXT DEFAULT 'aethoria_city',
    hp            INTEGER DEFAULT 100,
    max_hp        INTEGER DEFAULT 100,
    mp            INTEGER DEFAULT 50,
    max_mp        INTEGER DEFAULT 50,
    gold          INTEGER DEFAULT 100,
    inventory     TEXT DEFAULT '[]',
    created_at    TEXT DEFAULT (datetime('now')),
    FOREIGN KEY(account_id) REFERENCES accounts(id)
);
CREATE INDEX IF NOT EXISTS idx_chars_account ON characters(account_id);
)SQL";
        char* errMsg = nullptr;
        sqlite3_exec(db_, schema, nullptr, nullptr, &errMsg);
        if (errMsg) {
            std::cerr << "[Accounts] Схема: " << errMsg << "\n";
            sqlite3_free(errMsg);
        }
#endif
    }

    bool _findByUsername(const std::string& username, Account& out) {
#ifdef USE_SQLITE
        const char* sql =
            "SELECT id,username,email,banned,verified,created_at,last_login "
            "FROM accounts WHERE username=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
        bool found = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            out.id        = sqlite3_column_int(stmt, 0);
            out.username  = _col(stmt, 1);
            out.email     = _col(stmt, 2);
            out.banned    = sqlite3_column_int(stmt, 3) != 0;
            out.verified  = sqlite3_column_int(stmt, 4) != 0;
            out.createdAt = _col(stmt, 5);
            out.lastLogin = sqlite3_column_int(stmt, 6);
            found = true;
        }
        sqlite3_finalize(stmt);
        return found;
#else
        return false;
#endif
    }

    void _updateLastLogin(int id) {
#ifdef USE_SQLITE
        const char* sql = "UPDATE accounts SET last_login=strftime('%s','now') WHERE id=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
#endif
    }

    // Простой хеш (в продакшене замени на bcrypt / argon2)
    static std::string _hashPassword(const std::string& pw) {
        // XOR-хеш + FNV-1a (минимум для прототипа)
        uint64_t h = 14695981039346656037ULL;
        for (char c : pw) {
            h ^= (uint8_t)c;
            h *= 1099511628211ULL;
        }
        // Конвертируем в hex
        char buf[17];
        snprintf(buf, sizeof(buf), "%016llx", (unsigned long long)h);
        return std::string(buf);
    }

#ifdef USE_SQLITE
    static std::string _col(sqlite3_stmt* stmt, int col) {
        const unsigned char* s = sqlite3_column_text(stmt, col);
        return s ? (const char*)s : "";
    }
#endif
};

// ═══════════════════════════════════════════════════════════════
// КАК ИСПОЛЬЗОВАТЬ:
// ═══════════════════════════════════════════════════════════════
//
// AccountManager accounts("accounts.db");
// accounts.open();
//
// // Регистрация
// accounts.registerAccount("papaz", "secret123", "papaz@mail.ru");
//
// // Логин
// Account acc;
// if (accounts.login("papaz", "secret123", acc) == AuthResult::OK) {
//     auto chars = accounts.getCharacters(acc.id);
//     // Показываем экран выбора персонажа
// }
//
// // Создание персонажа
// CharacterRecord ch;
// accounts.createCharacter(acc.id, "Папаз", "WARRIOR", ch);
//
// // Сохранение прогресса (раз в 30 сек или при выходе)
// ch.level = player.level;
// ch.xp    = player.xp;
// ch.posX  = player.pos.x;
// ch.posY  = player.pos.y;
// accounts.saveCharacter(ch);
