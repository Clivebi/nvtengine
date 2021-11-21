
#pragma once
#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace openvas {
class DatabaseObject {
protected:
    sqlite3* mRaw;

public:
    DatabaseObject(const std::string& path) {
        if (SQLITE_OK != sqlite3_open(path.c_str(), &mRaw)) {
            throw std::runtime_error("open " + path + "failed");
        }
    }
    ~DatabaseObject() {
        if (mRaw != NULL) {
            sqlite3_close(mRaw);
        }
        mRaw = NULL;
    }
    bool BeginTransaction() { return ExecuteSQL("begin transaction"); }

    bool CommitTransaction() { return ExecuteSQL("commit transaction"); }

    bool RollbackTransaction() { return ExecuteSQL("rollback transaction"); }
    bool ExecuteSQL(std::string sql) {
        return sqlite3_exec(mRaw, sql.c_str(), NULL, NULL, NULL) == SQLITE_OK;
    }

    int64_t Integer(sqlite3_stmt* stmt, int index) { return sqlite3_column_int64(stmt, index); }
    std::string String(sqlite3_stmt* stmt, int index) {
        const unsigned char* text = sqlite3_column_text(stmt, index);
        int length = sqlite3_column_bytes(stmt, index);
        if (text == NULL) {
            return "";
        }
        return std::string((const char*)text, length);
    }
    std::string Bytes(sqlite3_stmt* stmt, int index) {
        const void* buffer = sqlite3_column_blob(stmt, index);
        int length = sqlite3_column_bytes(stmt, index);
        if (buffer == NULL) {
            return "";
        }
        return std::string((const char*)buffer, length);
    }
};
} // namespace openvas
