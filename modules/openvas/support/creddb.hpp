#pragma once

#include "../../../engine/value.hpp"
#include "sqlitedb.hpp"
using namespace Interpreter;
Value ParseJSON(std::string& str, bool unescape);
namespace support {

//TODO encrypt cred value
class CredDB : DatabaseObject {
public:
    CredDB(const std::string& path) : DatabaseObject(path) {
        std::string sql = "CREATE TABLE IF NOT EXISTS cred (host TEXT PRIMARY KEY,value BLOB)";
        if (!ExecuteSQL(sql)) {
            throw std::runtime_error("init CredDB db error: " + GetLastError());
        }
    }

    bool Update(const std::string& host, const Value& value) {
        if (value.Type != ValueType::kArray) {
            return false;
        }
        sqlite3_stmt* stmt = NULL;
        std::string sql, blob;
        blob = value.ToJSONString(false);
        sql = "INSERT OR REPLACE INTO cred(host,value) VALUES(?,?)";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), (int)sql.size(), &stmt, NULL)) {
            throw std::runtime_error("sqlite3_prepare_v2 sql error");
        }
        sqlite3_bind_text(stmt, 1, host.c_str(), (int)host.size(), NULL);
        sqlite3_bind_blob(stmt, 2, blob.c_str(), (int)blob.size(), NULL);
        if (SQLITE_DONE != sqlite3_step(stmt)) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("sqlite3_step sql error");
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }

    Value Get(const std::string& host) {
        sqlite3_stmt* stmt = NULL;
        std::string sql = "SELECT value FROM cred WHERE host = ?";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), (int)sql.size(), &stmt, NULL)) {
            throw std::runtime_error("Prefs::Get sql error");
        }
        sqlite3_bind_text(stmt, 1, host.c_str(), (int)host.size(), NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string text = Bytes(stmt, 0);
            Value ret = ParseJSON(text, false);
            sqlite3_finalize(stmt);
            return ret;
        }
        sqlite3_finalize(stmt);
        return Value();
    }
};
}; // namespace support