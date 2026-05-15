#pragma once
#include <sqlite3.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <mutex>

struct Vacancy {
    std::string id;
    std::string name;
    std::string employer;
    std::string description;
    std::string published_at;
    std::string url;
};

struct SkillStat {
    std::string date;
    std::string keyword;
    int count;
};

class DbHandler {
public:
    static DbHandler& instance();
    bool open(const std::string& path);
    std::vector<Vacancy> getVacancies(int page, int per_page);
    std::vector<SkillStat> getSkillStats(const std::string& date);
    int getTotalVacancies();
    void to_json(nlohmann::json& j, const Vacancy& v);
    void to_json(nlohmann::json& j, const SkillStat& s);

private:
    DbHandler() = default;
    sqlite3* db = nullptr;
    std::mutex mutex;
};
