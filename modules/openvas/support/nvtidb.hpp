
#pragma once
#include "../../../engine/value.hpp"
#include "sqlitedb.hpp"
Value ParseJSON(std::string& str);
namespace openvas {

class NVTIDataBase : DatabaseObject {
public:
    NVTIDataBase(const std::string& path) : DatabaseObject(path) {
        std::string sql =
                "CREATE TABLE IF NOT EXISTS nvti(oid TEXT PRIMARY KEY,file TEXT UNIQUE ,value "
                "BLOB)";
        if (!ExecuteSQL(sql)) {
            throw std::runtime_error("NVTIDataBase db error");
        }
    }

    bool Clean() { return ExecuteSQL("delete from nvti"); }

    bool UpdateAll(const Value& newVal) {
        if (newVal.Type != ValueType::kArray) {
            return false;
        }
        bool bSuccess = true;
        if (!BeginTransaction()) {
            return false;
        }
        sqlite3_stmt* stmt = NULL;
        std::string sql, blob, oid, filename;
        sql = "INSERT OR REPLACE INTO nvti(oid,file,value) VALUES(?,?,?)";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), sql.size(), &stmt, NULL)) {
            throw std::runtime_error("NVTIDataBase::UpdateOne sql error");
        }
        auto iter = newVal.Array()->_array.begin();
        while (iter != newVal.Array()->_array.end()) {
            sqlite3_reset(stmt);
            const Value& val = *iter;
            oid = val["oid"].ToString();
            filename = val["filename"].ToString();
            blob = val.ToJSONString(false);

            sqlite3_bind_text(stmt, 1, oid.c_str(), oid.size(), NULL);
            sqlite3_bind_text(stmt, 2, filename.c_str(), filename.size(), NULL);
            sqlite3_bind_blob(stmt, 3, blob.c_str(), blob.size(), NULL);
            if (SQLITE_DONE != sqlite3_step(stmt)) {
                bSuccess = false;
                break;
            }
            iter++;
        }
        if (bSuccess) {
            CommitTransaction();
        } else {
            RollbackTransaction();
        }
        sqlite3_finalize(stmt);
        return bSuccess;
    }

    Value GetAll() {
        sqlite3_stmt* stmt = NULL;
        std::string sql = "SELECT value FROM nvti";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), sql.size(), &stmt, NULL)) {
            throw std::runtime_error("NVTIDataBase::GetAll sql error");
        }
        Value ret = Value::make_array();
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string text = Bytes(stmt, 0);
            ret._array().push_back(ParseJSON(text));
        }
        sqlite3_finalize(stmt);
        return ret;
    }

    Value Get(std::string oid) {
        sqlite3_stmt* stmt = NULL;
        std::string sql = "SELECT value FROM nvti WHERE oid = ?";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), sql.size(), &stmt, NULL)) {
            throw std::runtime_error("NVTIDataBase::Get sql error");
        }
        sqlite3_bind_text(stmt, 1, oid.c_str(), oid.size(), NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string text = Bytes(stmt, 0);
            Value ret = ParseJSON(text);
            sqlite3_finalize(stmt);
            return ret;
        }
        sqlite3_finalize(stmt);
        return Value();
    }

    Value GetFromFileName(std::string filename) {
        sqlite3_stmt* stmt = NULL;
        std::string sql = "SELECT value FROM nvti WHERE file = ?";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), sql.size(), &stmt, NULL)) {
            throw std::runtime_error("NVTIDataBase::GetFromFileName sql error");
        }
        sqlite3_bind_text(stmt, 1, filename.c_str(), filename.size(), NULL);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string text = Bytes(stmt, 0);
            Value ret = ParseJSON(text);
            sqlite3_finalize(stmt);
            return ret;
        }
        sqlite3_finalize(stmt);
        return Value();
    }

    bool UpdateOne(const Value& val) {
        if (val.Type != ValueType::kMap) {
            return false;
        }
        sqlite3_stmt* stmt = NULL;
        std::string sql, blob, oid, filename;
        oid = val["oid"].ToString();
        filename = val["filename"].ToString();
        sql = "INSERT OR REPLACE INTO nvti(oid,file,value) VALUES(?,?,?)";
        if (SQLITE_OK != sqlite3_prepare_v2(mRaw, sql.c_str(), sql.size(), &stmt, NULL)) {
            throw std::runtime_error("NVTIDataBase::UpdateOne sql error");
        }
        blob = val.ToJSONString(false);
        sqlite3_bind_text(stmt, 1, oid.c_str(), oid.size(), NULL);
        sqlite3_bind_text(stmt, 2, filename.c_str(), filename.size(), NULL);
        sqlite3_bind_blob(stmt, 3, blob.c_str(), blob.size(), NULL);
        if (SQLITE_DONE != sqlite3_step(stmt)) {
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }
};
}; // namespace openvas