#pragma once

#pragma once
#include "../../../engine/value.hpp"
#include "sqlitedb.hpp"
using namespace Interpreter;
Value ParseJSON(std::string& str, bool unescape);
namespace support {

class ScanConfig : DatabaseObject {
public:
    ScanConfig(const std::string& path) : DatabaseObject(path) {
        std::string sql = "CREATE TABLE IF NOT EXISTS scanconfig (name TEXT,oid TEXT )";
        if (!ExecuteSQL(sql)) {
            throw std::runtime_error("Prefs db error");
        }
    }

    bool Add(const std::string& name, const std::string& oid) {
        sqlite3_stmt* stmt = NULL;
        std::string sql;
        sql = "INSERT INTO scanconfig(name,oid) VALUES(?,?)";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), (int)sql.size(), &stmt, NULL)) {
            throw std::runtime_error("sqlite3_prepare_v2 sql error");
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), (int)name.size(), NULL);
        sqlite3_bind_blob(stmt, 2, oid.c_str(), (int)oid.size(), NULL);
        if (SQLITE_DONE != sqlite3_step(stmt)) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("sqlite3_step sql error");
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    bool Remove(const std::string& name, const std::string& oid) {
        sqlite3_stmt* stmt = NULL;
        std::string sql;
        sql = "delete from scanconfig WHERE name=? AND oid=?";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), (int)sql.size(), &stmt, NULL)) {
            throw std::runtime_error("sqlite3_prepare_v2 sql error");
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), (int)name.size(), NULL);
        sqlite3_bind_blob(stmt, 2, oid.c_str(), (int)oid.size(), NULL);
        if (SQLITE_DONE != sqlite3_step(stmt)) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("sqlite3_step sql error");
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    void Get(const std::string& name, std::list<std::string>& result) {
        sqlite3_stmt* stmt = NULL;
        std::string sql = "SELECT oid FROM scanconfig WHERE name = ?";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), (int)sql.size(), &stmt, NULL)) {
            throw std::runtime_error("Prefs::Get sql error");
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), (int)name.size(), NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result.push_back(String(stmt, 0));
        }
        sqlite3_finalize(stmt);
    }
};
}; // namespace support