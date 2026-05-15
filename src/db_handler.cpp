#include "db_handler.h"
#include <stdexcept>

DbHandler& DbHandler::instance() {
    static DbHandler inst;
    return inst;
}

bool DbHandler::open(const std::string& path) {
    int rc = sqlite3_open_v2(path.c_str(), &db,
        SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
    return rc == SQLITE_OK;
}

int DbHandler::getTotalVacancies() {
    std::lock_guard<std::mutex> lock(mutex);
    sqlite3_stmt* stmt;
    int count = 0;
    
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM vacancies", -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }
        sqlite3_finalize(stmt);
    }
    return count;
}

std::vector<Vacancy> DbHandler::getVacancies(int page, int per_page) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<Vacancy> result;
    sqlite3_stmt* stmt;
    
    int offset = page * per_page;
    const char* sql = R"(
        SELECT id, name, employer, description, published_at, url
        FROM vacancies
        ORDER BY published_at DESC
        LIMIT ? OFFSET ?
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, per_page);
        sqlite3_bind_int(stmt, 2, offset);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Vacancy v;
            v.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            v.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            v.employer = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            v.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            v.published_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            v.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            result.push_back(v);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

std::vector<SkillStat> DbHandler::getSkillStats(const std::string& date) {
    std::lock_guard<std::mutex> lock(mutex);
    std::vector<SkillStat> result;
    sqlite3_stmt* stmt;
    
    const char* sql = R"(
        SELECT date, keyword, count
        FROM skill_stats
        WHERE date = ?
        ORDER BY count DESC
    )";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, date.c_str(), -1, SQLITE_TRANSIENT);
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            SkillStat s;
            s.date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            s.keyword = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            s.count = sqlite3_column_int(stmt, 2);
            result.push_back(s);
        }
        sqlite3_finalize(stmt);
    }
    return result;
}

void DbHandler::to_json(nlohmann::json& j, const Vacancy& v) {
    j = nlohmann::json{
        {"id", v.id},
        {"name", v.name},
        {"employer", v.employer},
        {"description", v.description},
        {"published_at", v.published_at},
        {"url", v.url}
    };
}

void DbHandler::to_json(nlohmann::json& j, const SkillStat& s) {
    j = nlohmann::json{
        {"date", s.date},
        {"keyword", s.keyword},
        {"count", s.count}
    };
}
