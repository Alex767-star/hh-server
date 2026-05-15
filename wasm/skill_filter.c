#include <string.h>
#include <ctype.h>

static const char* keywords[] = {
    "rust", "tokio", "actix", "axum", "docker", "kubernetes",
    "postgresql", "mysql", "mongodb", "redis", "grpc", "graphql",
    "rest", "aws", "gcp", "azure", "ci/cd", "gitlab",
    "python", "go", "java", "kotlin", "typescript", "javascript",
    "react", "vue", "angular", "kafka", "rabbitmq",
    "linux", "terraform", "ansible", "microservices",
    "ddd", "tdd", "agile", "scrum"
};

static const int keyword_count = sizeof(keywords) / sizeof(keywords[0]);

void to_lower(char* str) {
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

int contains_keyword(const char* description, const char* keyword) {
    char* pos = strstr(description, keyword);
    return pos != NULL ? 1 : 0;
}

int filter_description(const char* description, int* results, int max_results) {
    char buffer[8192];
    strncpy(buffer, description, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    to_lower(buffer);
    
    int count = 0;
    for (int i = 0; i < keyword_count && count < max_results; i++) {
        if (contains_keyword(buffer, keywords[i])) {
            results[count] = i;
            count++;
        }
    }
    return count;
}

int get_keyword_count() {
    return keyword_count;
}

const char* get_keyword(int index) {
    if (index >= 0 && index < keyword_count) {
        return keywords[index];
    }
    return "";
}
