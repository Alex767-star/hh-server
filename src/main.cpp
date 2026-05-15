#include <drogon/drogon.h>
#include "db_handler.h"
#include <nlohmann/json.hpp>
#include <json/json.h>

using namespace drogon;

std::string getCurrentDate() {
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", t);
    return std::string(buf);
}

Json::Value nlohmannToJsoncpp(const nlohmann::json& j) {
    Json::Value result;
    if (j.is_object()) {
        for (auto& [key, value] : j.items()) {
            result[key] = nlohmannToJsoncpp(value);
        }
    } else if (j.is_array()) {
        for (size_t i = 0; i < j.size(); i++) {
            result.append(nlohmannToJsoncpp(j[i]));
        }
    } else if (j.is_string()) {
        result = j.get<std::string>();
    } else if (j.is_number_integer()) {
        result = j.get<int64_t>();
    } else if (j.is_number_float()) {
        result = j.get<double>();
    } else if (j.is_boolean()) {
        result = j.get<bool>();
    }
    return result;
}

int main() {
    if (!DbHandler::instance().open("/home/ellilot/hh-scheduler/hh_analytics.db")) {
        LOG_ERROR << "Failed to open database";
        return 1;
    }
    
    LOG_INFO << "Database opened successfully";
    LOG_INFO << "Total vacancies: " << DbHandler::instance().getTotalVacancies();
    
    app().setLogPath("./")
         .setLogLevel(trantor::Logger::kInfo)
         .addListener("0.0.0.0", 8080)
         .setThreadNum(4)
         .setDocumentRoot("./static")
         .registerPostHandlingAdvice([](const HttpRequestPtr& req, const HttpResponsePtr& resp) {
             resp->addHeader("Access-Control-Allow-Origin", "*");
             resp->addHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
             resp->addHeader("Access-Control-Allow-Headers", "Content-Type");
         });
    
    app().registerHandler("/api/vacancies", [](const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) {
        
        int page = 0;
        int per_page = 20;
        
        auto page_param = req->getParameter("page");
        if (!page_param.empty()) page = std::stoi(page_param);
        
        auto per_page_param = req->getParameter("per_page");
        if (!per_page_param.empty()) per_page = std::min(std::stoi(per_page_param), 100);
        
        auto vacancies = DbHandler::instance().getVacancies(page, per_page);
        int total = DbHandler::instance().getTotalVacancies();
        
        nlohmann::json j;
        j["vacancies"] = nlohmann::json::array();
        for (auto& v : vacancies) {
            nlohmann::json item;
            DbHandler::instance().to_json(item, v);
            j["vacancies"].push_back(item);
        }
        j["total"] = total;
        j["page"] = page;
        j["per_page"] = per_page;
        j["total_pages"] = (total + per_page - 1) / per_page;
        
        auto resp = HttpResponse::newHttpJsonResponse(nlohmannToJsoncpp(j));
        callback(resp);
    });
    
    app().registerHandler("/api/skills", [](const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) {
        
        std::string date = getCurrentDate();
        auto date_param = req->getParameter("date");
        if (!date_param.empty()) date = date_param;
        
        auto skills = DbHandler::instance().getSkillStats(date);
        
        nlohmann::json j;
        j["skills"] = nlohmann::json::array();
        for (auto& s : skills) {
            nlohmann::json item;
            DbHandler::instance().to_json(item, s);
            j["skills"].push_back(item);
        }
        j["date"] = date;
        
        auto resp = HttpResponse::newHttpJsonResponse(nlohmannToJsoncpp(j));
        callback(resp);
    });
    
    app().registerHandler("/api/health", [](const HttpRequestPtr&,
        std::function<void(const HttpResponsePtr&)>&& callback) {
        
        nlohmann::json j;
        j["status"] = "ok";
        j["service"] = "hh-server";
        j["vacancies_count"] = DbHandler::instance().getTotalVacancies();
        
        auto resp = HttpResponse::newHttpJsonResponse(nlohmannToJsoncpp(j));
        callback(resp);
    });
    
    app().registerHandler("/api/descriptions", [](const HttpRequestPtr& req,
        std::function<void(const HttpResponsePtr&)>&& callback) {
        
        int page = 0;
        auto page_param = req->getParameter("page");
        if (!page_param.empty()) page = std::stoi(page_param);
        
        auto vacancies = DbHandler::instance().getVacancies(page, 100);
        
        nlohmann::json j;
        j["descriptions"] = nlohmann::json::array();
        for (auto& v : vacancies) {
            j["descriptions"].push_back(v.description);
        }
        
        auto resp = HttpResponse::newHttpJsonResponse(nlohmannToJsoncpp(j));
        callback(resp);
    });
    
    LOG_INFO << "Server starting on http://0.0.0.0:8080";
    app().run();
    return 0;
}
