# HH Analyzer — Система анализа вакансий с hh.ru

Распределённая система из трёх компонентов для сбора, хранения и визуализации данных о вакансиях с автоматическим анализом ключевых навыков.

## Архитектура
hh-analyzer/     # Storage Layer (Rust)
hh-scheduler/    # Планировщик сбора данных (Rust)
hh-server/       # Веб-сервер и дашборд (C++/Drogon)


## Компоненты

### hh-analyzer — Storage Layer
Слой хранения данных, реализованный через трейты с возможностью подмены реализации.

Стек: Rust, sqlx 0.7, SQLite/PostgreSQL, async-trait, chrono, thiserror.

Таблицы: vacancies (id TEXT PK, UPSERT), skill_stats (date + keyword UNIQUE, UPSERT).

Особенности: трейт Storage для лёгкой подмены БД, миграции в SQL файлах, идемпотентный UPSERT, поддержка SQLite и PostgreSQL.

### hh-scheduler — Планировщик
Внутрипроцессный планировщик на tokio. Собирает вакансии, сохраняет в БД, анализирует ключевые навыки.

Стек: tokio, tokio-cron-scheduler, reqwest, sqlx 0.8, tracing, prometheus, tiny_http, chrono, clap.

Возможности:
- Пагинированный обход HH API с retry и exponential backoff
- Обработка 429 с парсингом Retry-After
- Подсчёт 45+ ключевых слов (Rust, Docker, Kubernetes, PostgreSQL, AWS, gRPC, Kafka, Redis, DDD, TDD, Agile, Scrum и другие)
- Graceful shutdown с таймаутом
- Health check на порту 3000
- Prometheus метрики: pipeline_runs_total, pipeline_errors_total, vacancies_saved_total, vacancies_updated_total, keywords_found, pipeline_duration_seconds, zero_updates_streak, missed_ticks
- Очистка старых вакансий (retention_days)
- Отслеживание пропущенных тиков
- Тестовые данные при недоступности API
- CLI: query, area, cron, retention_days, pipeline_timeout_secs

### hh-server — Веб-сервер и дашборд
Высокопроизводительный веб-сервер на C++17 с фреймворком Drogon и фронтендом на чистом JavaScript.

Стек: C++17, Drogon, sqlite3, nlohmann/json, jsoncpp, HTML/CSS/JavaScript.

API эндпоинты:
- GET /api/vacancies?page=0&per_page=20 — пагинированные вакансии с метаданными
- GET /api/skills?date=2026-05-15 — статистика навыков за день
- GET /api/health — health check с количеством вакансий
- GET /api/descriptions?page=0 — описания для wasm фильтрации

Фронтенд:
- Две вкладки: Vacancies и Skills
- Карточки вакансий с извлечёнными навыками
- Фильтрация по навыкам (множественный выбор)
- Статистика: всего вакансий, на странице, всего страниц, активных фильтров
- Пагинация
- Тёмная тема в стиле GitHub

WebAssembly модуль (опционально): фильтрация навыков на C, компиляция через Emscripten, выполняется в браузере без запросов к серверу.

## Схема данных

### vacancies
| Поле | Тип | Описание |
|------|-----|----------|
| id | TEXT PK | ID с hh.ru |
| name | TEXT | Название вакансии |
| employer | TEXT | Компания |
| description | TEXT | Полный текст для анализа |
| published_at | TIMESTAMP | Дата публикации |
| url | TEXT | Ссылка |

### skill_stats
| Поле | Тип | Описание |
|------|-----|----------|
| id | INTEGER PK | Автоинкремент |
| date | DATE | День сбора |
| keyword | TEXT | Ключевое слово |
| count | INTEGER | Частота |

UNIQUE(date, keyword)

### pipeline_state
| Поле | Тип | Описание |
|------|-----|----------|
| id | INTEGER PK | Всегда 1 |
| last_run_at | TIMESTAMP | Время последнего тика |

## Идемпотентность
Все операции вставки используют UPSERT (ON CONFLICT DO UPDATE). Повторный запуск пайплайна не создаёт дубликатов. Статистика навыков перезаписывается за текущий день. Время последнего тика обновляется атомарно.

## Запуск

### hh-analyzer
cd hh-analyzer
cargo build
cargo run


### hh-scheduler
cd hh-scheduler
RUST_LOG=info cargo run -- --query "Rust+developer" --area "113" --cron "0 0 1/6 * * *"


### hh-server
cd hh-server
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake ..
make -j$(nproc)
ln -sf ../static static
ln -sf ~/hh-scheduler/hh_analytics.db hh_analytics.db
./hh-server

Открыть http://localhost:8080

## Мониторинг

- hh-scheduler: http://localhost:3000/health и http://localhost:3000/metrics (Prometheus)
- hh-server: http://localhost:8080/api/health

Метрики для Grafana: скорость обработки, ошибки API, количество новых/обновлённых вакансий, тренды по навыкам, пропущенные тики.

## Тестовые данные
Если HH API недоступен, hh-scheduler автоматически создаёт 5 тестовых вакансий с полным набором ключевых слов для проверки всего пайплайна.

## Безопасность
- User-Agent с контактной информацией (требование HH API)
- Rate limiting между запросами (100ms)
- Экспоненциальный backoff с jitter при ошибках
- Таймаут пайплайна при graceful shutdown
- Только чтение из БД в веб-сервере (SQLITE_OPEN_READONLY)
- CORS заголовки для API
