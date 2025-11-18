#!/usr/bin/env python3
"""
NS-3 Network Simulation Results Visualization
Визуализация результатов симуляции (Пропускная способность и Задержка)
"""

import pandas as pd
import matplotlib
matplotlib.use('Agg')  # Используем non-interactive backend для headless режима
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path
from scipy.interpolate import interp1d

# Настройка стиля
plt.style.use('seaborn-v0_8-darkgrid')
plt.rcParams['font.size'] = 10
plt.rcParams['axes.titlesize'] = 12
plt.rcParams['axes.labelsize'] = 11
plt.rcParams['legend.fontsize'] = 10
plt.rcParams['lines.linewidth'] = 2.8
plt.rcParams['lines.solid_capstyle'] = 'round'  # Скругляем концы линий

def prepare_series(df, value_column):
    """
    Подготавливает упорядоченные и усреднённые ряды по значениям lambda.
    Возвращает массивы lambda и значений.
    """
    if df.empty:
        return np.array([]), np.array([])

    grouped = (
        df.groupby('Lambda', as_index=False)[value_column]
        .mean()
        .sort_values('Lambda')
    )
    lambdas = grouped['Lambda'].to_numpy()
    values = grouped[value_column].to_numpy()
    
    # Фильтруем бесконечные и NaN значения
    valid_mask = np.isfinite(values) & np.isfinite(lambdas)
    lambdas = lambdas[valid_mask]
    values = values[valid_mask]
    
    return lambdas, values


def smooth_series(lambdas, values, resolution=300, window=3):
    """
    Строит сглаженную кривую через кубическую интерполяцию.
    Использует сглаживание для более плавных кривых.
    """
    if len(lambdas) == 0:
        return lambdas, values
    if len(lambdas) == 1:
        return lambdas, values

    sorted_idx = np.argsort(lambdas)
    lambdas = lambdas[sorted_idx]
    values = values[sorted_idx]

    # Фильтруем бесконечные и NaN значения
    valid_mask = np.isfinite(values) & np.isfinite(lambdas)
    lambdas = lambdas[valid_mask]
    values = values[valid_mask]
    
    if len(lambdas) == 0:
        return np.array([]), np.array([])
    if len(lambdas) == 1:
        return lambdas, values

    # Удаляем дубликаты по lambda для корректной интерполяции
    unique_lambdas = []
    unique_values = []
    for i in range(len(lambdas)):
        if i == 0 or lambdas[i] != lambdas[i-1]:
            unique_lambdas.append(lambdas[i])
            unique_values.append(values[i])
        else:
            # Если есть дубликат, берем среднее значение
            unique_values[-1] = (unique_values[-1] + values[i]) / 2.0
    
    unique_lambdas = np.array(unique_lambdas)
    unique_values = np.array(unique_values)
    
    if len(unique_lambdas) < 2:
        return unique_lambdas, unique_values

    # Используем кубическую интерполяцию для более плавных кривых
    # Создаем плотную сетку для интерполяции
    dense_x = np.linspace(unique_lambdas[0], unique_lambdas[-1], 
                         max(resolution, len(unique_lambdas) * 10))
    
    try:
        # Пробуем кубическую интерполяцию
        if len(unique_lambdas) >= 4:
            f = interp1d(unique_lambdas, unique_values, kind='cubic', 
                         bounds_error=False, fill_value='extrapolate')
            dense_y = f(dense_x)
        else:
            # Если точек мало, используем линейную
            f = interp1d(unique_lambdas, unique_values, kind='linear',
                         bounds_error=False, fill_value='extrapolate')
            dense_y = f(dense_x)
    except:
        # В случае ошибки используем простую линейную интерполяцию
        dense_y = np.interp(dense_x, unique_lambdas, unique_values)
    
    # Фильтруем бесконечные значения в результате
    valid_mask = np.isfinite(dense_y)
    dense_x = dense_x[valid_mask]
    dense_y = dense_y[valid_mask]

    return dense_x, dense_y


def plot_smoothed(ax, lambdas, values, color, label, linestyle='-', zorder=5, marker='o', show_points=False):
    """
    Рисует сглаженную линию (и опционально точки).
    """
    if len(lambdas) == 0:
        return

    smooth_x, smooth_y = smooth_series(lambdas, values)
    ax.plot(smooth_x, smooth_y, color=color, linestyle=linestyle, label=label, 
            zorder=zorder, linewidth=2.8, alpha=0.95, solid_capstyle='round')
    
    if show_points:
        ax.scatter(lambdas, values, color=color, edgecolor='white', 
                   linewidth=0.8, s=40, zorder=zorder + 1, alpha=0.8)

def load_data(csv_path):
    """Загрузка данных из CSV"""
    try:
        df = pd.read_csv(csv_path)
        print(f"✓ Загружен: {csv_path.name}")
        print(f"  Строк: {len(df)}, Столбцов: {len(df.columns)}")
        return df
    except Exception as e:
        print(f"✗ Ошибка: {e}")
        return None

def plot_graphs(df):
    """Построение 4 графиков с методами анализа"""
    
    # Разделяем данные по типу сети
    adhoc = df[df['NetworkType'] == 'AdHoc'].copy()
    group = df[df['NetworkType'] == 'Group'].copy()
    
    # Создаем фигуру с 4 графиками (2x2)
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(18, 14))
    
    # Общий заголовок
    fig.suptitle('NS-3 Сетевая Симуляция: Сравнение Методов Анализа', 
                 fontsize=18, fontweight='bold', y=0.995)
    
    # Цветовая схема для методов
    colors = {
        'Simulation': '#E63946',  # Красный
        'MeanValue': '#1D6996',   # Синий
        'GlobalBalance': '#38A800', # Зеленый
        'GordonNewell': '#F28E2B', # Оранжевый
        'Buzen': '#9467BD',        # Фиолетовый
    }
    
    # === График 1: Пропускная способность Ad-Hoc ===
    if not adhoc.empty:
        # Методы анализа
        lambdas, mva = prepare_series(adhoc, 'MeanValueAnalysis')
        plot_smoothed(ax1, lambdas, mva, colors['MeanValue'], 
                     'Mean Value Analysis', linestyle='-', zorder=5)
        
        lambdas, gb = prepare_series(adhoc, 'GlobalBalanceMethod')
        plot_smoothed(ax1, lambdas, gb, colors['GlobalBalance'], 
                     'Global Balance', linestyle='-', zorder=4)
        
        lambdas, gn = prepare_series(adhoc, 'GordonNewellMethod')
        plot_smoothed(ax1, lambdas, gn, colors['GordonNewell'], 
                     'Gordon-Newell', linestyle='-', zorder=3)
        
        lambdas, bz = prepare_series(adhoc, 'BuzenMethod')
        plot_smoothed(ax1, lambdas, bz, colors['Buzen'], 
                     'Buzen', linestyle='-', zorder=2)
    
    ax1.set_xlabel('Нагрузка λ (пакетов/сек)', fontweight='bold', fontsize=12)
    ax1.set_ylabel('Пропускная способность (Мбит/с)', fontweight='bold', fontsize=12)
    ax1.set_title('Пропускная способность Ad-Hoc сети', fontweight='bold', pad=15, fontsize=14)
    ax1.grid(True, alpha=0.25, linestyle='--', linewidth=0.8)
    ax1.legend(loc='best', framealpha=0.95, fontsize=10, ncol=2)
    
    # === График 2: Пропускная способность Group ===
    if not group.empty:
        # Методы анализа
        lambdas, mva = prepare_series(group, 'MeanValueAnalysis')
        plot_smoothed(ax2, lambdas, mva, colors['MeanValue'], 
                     'Mean Value Analysis', linestyle='-', zorder=5)
        
        lambdas, gb = prepare_series(group, 'GlobalBalanceMethod')
        plot_smoothed(ax2, lambdas, gb, colors['GlobalBalance'], 
                     'Global Balance', linestyle='-', zorder=4)
        
        lambdas, gn = prepare_series(group, 'GordonNewellMethod')
        plot_smoothed(ax2, lambdas, gn, colors['GordonNewell'], 
                     'Gordon-Newell', linestyle='-', zorder=3)
        
        lambdas, bz = prepare_series(group, 'BuzenMethod')
        plot_smoothed(ax2, lambdas, bz, colors['Buzen'], 
                     'Buzen', linestyle='-', zorder=2)

    ax2.set_xlabel('Нагрузка λ (пакетов/сек)', fontweight='bold', fontsize=12)
    ax2.set_ylabel('Пропускная способность (Мбит/с)', fontweight='bold', fontsize=12)
    ax2.set_title('Пропускная способность Групповой сети', fontweight='bold', pad=15, fontsize=14)
    ax2.grid(True, alpha=0.25, linestyle='--', linewidth=0.8)
    ax2.legend(loc='best', framealpha=0.95, fontsize=10, ncol=2)
    
    # === График 3: Задержка Ad-Hoc ===
    if not adhoc.empty:
        # Метод задержки
        lambdas, delay = prepare_series(adhoc, 'MeanValueDelay')
        plot_smoothed(ax3, lambdas, delay * 1000, colors['MeanValue'], 
                     'Mean Value Delay', linestyle='-', zorder=5)
    
    ax3.set_xlabel('Нагрузка λ (пакетов/сек)', fontweight='bold', fontsize=12)
    ax3.set_ylabel('Задержка (мс)', fontweight='bold', fontsize=12)
    ax3.set_title('Задержка Ad-Hoc сети', fontweight='bold', pad=15, fontsize=14)
    ax3.grid(True, alpha=0.25, linestyle='--', linewidth=0.8)
    ax3.legend(loc='best', framealpha=0.95, fontsize=11)
    
    # === График 4: Задержка Group ===
    if not group.empty:
        # Метод задержки
        lambdas, delay = prepare_series(group, 'MeanValueDelay')
        plot_smoothed(ax4, lambdas, delay * 1000, colors['MeanValue'], 
                     'Mean Value Delay', linestyle='-', zorder=5)
    
    ax4.set_xlabel('Нагрузка λ (пакетов/сек)', fontweight='bold', fontsize=12)
    ax4.set_ylabel('Задержка (мс)', fontweight='bold', fontsize=12)
    ax4.set_title('Задержка Групповой сети', fontweight='bold', pad=15, fontsize=14)
    ax4.grid(True, alpha=0.25, linestyle='--', linewidth=0.8)
    ax4.legend(loc='best', framealpha=0.95, fontsize=11)
    
    # Улучшаем расположение
    plt.tight_layout(rect=[0, 0.01, 1, 0.98])
    
    return fig

def print_statistics(df):
    """Вывод базовой статистики"""
    print("\n" + "="*70)
    print("СТАТИСТИКА МЕТОДОВ АНАЛИЗА")
    print("="*70)
    
    for net_type in ['AdHoc', 'Group']:
        data = df[df['NetworkType'] == net_type]
        if len(data) == 0:
            print(f"\n{net_type} сеть: НЕТ ДАННЫХ")
            continue
            
        print(f"\n{net_type} сеть:")
        print(f"  Mean Value Analysis:")
        print(f"    Пропускная способность: {data['MeanValueAnalysis'].min():.4f} - {data['MeanValueAnalysis'].max():.4f} Мбит/с")
        
        print(f"  Global Balance Method:")
        print(f"    Пропускная способность: {data['GlobalBalanceMethod'].min():.4f} - {data['GlobalBalanceMethod'].max():.4f} Мбит/с")
        
        print(f"  Gordon-Newell Method:")
        print(f"    Пропускная способность: {data['GordonNewellMethod'].min():.4f} - {data['GordonNewellMethod'].max():.4f} Мбит/с")
        
        print(f"  Buzen Method:")
        print(f"    Пропускная способность: {data['BuzenMethod'].min():.4f} - {data['BuzenMethod'].max():.4f} Мбит/с")
        
        print(f"  Задержка (Mean Value):")
        print(f"    Мин:  {data['MeanValueDelay'].min()*1000:.4f} мс")
        print(f"    Макс: {data['MeanValueDelay'].max()*1000:.4f} мс")
    
    print("="*70)

def main():
    """Основная функция"""
    print("\n" + "="*70)
    print("NS-3 ВИЗУАЛИЗАЦИЯ РЕЗУЛЬТАТОВ")
    print("="*70 + "\n")
    
    # Пути к файлам
    project_root = Path(__file__).resolve().parent.parent
    data_dir = project_root / 'public'
    csv_path = data_dir / 'analysis_with_methods.csv'
    
    # Загрузка данных
    df = load_data(csv_path)
    if df is None:
        print("\n✗ Не удалось загрузить данные")
        print(f"  Проверьте путь: {csv_path}")
        return
    
    # Проверка столбцов
    required = ['NetworkType', 'Lambda', 'MeanValueAnalysis', 'GlobalBalanceMethod', 
                'GordonNewellMethod', 'BuzenMethod', 'MeanValueDelay']
    if not all(col in df.columns for col in required):
        print(f"\n✗ Отсутствуют требуемые столбцы.")
        print(f"  Нужны: {required}")
        print(f"  Найдены: {list(df.columns)}")
        return
    
    print(f"\nТипы сетей: {df['NetworkType'].unique()}")
    print(f"Диапазон Lambda: {df['Lambda'].min()} - {df['Lambda'].max()}")
    
    # Построение графиков
    print("\nПостроение графиков...")
    fig = plot_graphs(df)
    
    # Статистика
    print_statistics(df)
    
    # Сохранение
    output_path = data_dir / 'simulation_plots.png'
    fig.savefig(output_path, dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='none')
    print(f"\n✓ Сохранено: {output_path}")
    
    # Закрываем фигуру для освобождения памяти
    plt.close(fig)
    
    print("\n" + "="*70)
    print("ЗАВЕРШЕНО")
    print("="*70 + "\n")

if __name__ == "__main__":
    main()