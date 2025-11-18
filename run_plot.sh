#!/bin/bash
# Скрипт для запуска визуализации результатов симуляции

echo "=================================================="
echo "Запуск визуализации результатов NS-3 симуляции"
echo "=================================================="
echo ""

cd "$(dirname "$0")"

if [ ! -d "venv" ]; then
    echo "✗ Виртуальное окружение не найдено!"
    echo "  Создайте его командой: python3 -m venv venv"
    exit 1
fi

source venv/bin/activate

if [ ! -f "public/simulation_results.csv" ]; then
    echo "✗ Файл результатов не найден: public/simulation_results.csv"
    echo "  Сначала запустите симуляцию!"
    exit 1
fi

echo "✓ Запуск скрипта визуализации..."
echo ""

python3 ./src/plot_results.py

deactivate

echo ""
echo "=================================================="
echo "Визуализация завершена"
echo "=================================================="

