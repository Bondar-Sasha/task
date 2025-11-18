# NS-3 Network Simulation Project

## Структура проекта

```
scratch/
├── .env                          # Конфигурация симуляции
├── requirements.txt              # Python зависимости
├── README.md                     # Документация проекта
├── public/                       # Публичные данные
│   └── result/                   # Результаты симуляций
│       ├── simulation_results.csv
│       ├── analysis_results.csv
│       ├── node_statistics.csv
│       └── simulation_plots.png
├── src/                          # Исходный код
│   ├── analysis/                 # Аналитические методы
│   │   ├── analysis_methods.h
│   │   └── analysis_methods.cc
│   ├── env/                      # Загрузка конфигурации
│   │   ├── environment_config.h
│   │   └── environment_config.cc
│   ├── metrics/                  # Расчёт метрик
│   │   ├── simulation_metrics.h
│   │   ├── simulation_metrics.cc
│   │   ├── metrics_calculator.h
│   │   └── metrics_calculator.cc
│   ├── simulation/               # Симуляторы
│   │   ├── adhoc_simulator.h
│   │   ├── adhoc_simulator.cc
│   │   ├── group_simulator.h
│   │   └── group_simulator.cc
│   ├── writers/                  # Запись результатов
│   │   ├── csv_writer.h
│   │   └── csv_writer.cc
│   ├── plot_results.py           # Визуализация результатов
│   └── main.cc                   # Точка входа (оркестрация)
└── venv/                         # Python виртуальное окружение

init-ns3.sh - скрипт для установки ns3

```

## Описание модулей

### `src/analysis/`
Аналитические методы для расчёта теоретических моделей:
- Global Mean Analysis
- M/M/1 Queue Analysis
- M/M/1/K Queue Analysis (с ограниченным буфером)
- Ad-Hoc Throughput Model
- Group Throughput Model
- Модели задержки и потерь

### `src/env/`
Загрузка и валидация конфигурации из `.env`:
- Парсинг всех параметров симуляции
- Валидация обязательных ключей
- Проверка корректности NODE_*_LOAD и NODE_*_BUFFER

### `src/metrics/`
Расчёт метрик из результатов симуляции:
- Структура `SimulationMetrics` с полными данными
- `MetricsCalculator` для обработки FlowMonitor
- Пропускная способность, задержка, джиттер, потери пакетов
- Статистика по узлам

### `src/simulation/`
Запуск сетевых симуляций:
- `AdHocSimulator` - WiFi Ad-Hoc сеть
- `GroupSimulator` - CSMA групповая сеть
- Настройка топологии, мобильности, приложений
- Сбор метрик через FlowMonitor

### `src/writers/`
Запись результатов в CSV файлы:
- `WriteResults()` - основные результаты симуляции
- `WriteAnalysis()` - аналитические модели
- `WriteNodeStatistics()` - детальная статистика по узлам

### `src/main.cc`
Оркестрация всего процесса:
1. Загрузка конфигурации
2. Запуск симуляций для диапазона Lambda
3. Сравнительный анализ
4. Запись результатов в CSV
5. Вывод итоговой статистики

## Запуск симуляции

```bash
# Из корневой директории ns-3
./waf --run scratch/src/main
```

## Визуализация результатов

```bash
cd scratch
source venv/bin/activate
python3 src/plot_results.py
```

## Конфигурация

Все параметры задаются в файле `.env`:
- Параметры симуляции (длительность, размер пакетов, буферы)
- Диапазон нагрузки Lambda
- Нагрузка и буферы для каждого узла
- Параметры сети (скорость, задержка)
- Параметры WiFi и мобильности
- Параметры графиков

