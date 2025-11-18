// analysis_methods.cc (Только реализация)

#include "analysis_methods.h" // Файл, содержащий объявление класса
#include "queueing_models.h"  // Для использования методов QueueingModels
#include <cmath>
#include <algorithm>
#include <limits> 

double AnalysisMethods::GlobalMeanAnalysis(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    double sum = 0.0;
    for (double value : data) {
        sum += value;
    }
    return sum / data.size();
}

// M/M/c анализ
double AnalysisMethods::MMcAnalysis(double arrivalRate, double serviceRate, int servers) {
    if (arrivalRate < 0.0 || serviceRate <= 0.0 || servers <= 0) return 0.0;
    if (arrivalRate == 0.0) return 0.0;
    
    double rho = arrivalRate / (servers * serviceRate);
    if (rho >= 1.0) {
        return servers * serviceRate; // Максимальная пропускная способность
    }
    return arrivalRate; // В стабильном состоянии
}

// M/M/1/K анализ пропускной способности
double AnalysisMethods::MM1KAnalysis(double arrivalRate, double serviceRate, int bufferSize) {
    return QueueingModels::Calculate_MM1K_Throughput(arrivalRate, serviceRate, bufferSize);
}

// M/M/1 модель задержки
double AnalysisMethods::MMDelayModel(double arrivalRate, double serviceRate) {
    if (arrivalRate < 0.0 || serviceRate <= 0.0) return 0.0;
    if (arrivalRate == 0.0) return 1.0 / serviceRate;
    
    if (arrivalRate >= serviceRate) {
        return std::numeric_limits<double>::infinity();
    }
    
    return 1.0 / (serviceRate - arrivalRate);
}

// M/M/1/K модель потерь
double AnalysisMethods::MMLossModel(double arrivalRate, double serviceRate, int bufferSize) {
    if (arrivalRate < 0.0 || serviceRate <= 0.0) return 0.0;
    if (arrivalRate == 0.0) return 0.0;
    if (bufferSize <= 0) return 0.0; // M/M/1 без потерь
    
    double rho = arrivalRate / serviceRate;
    
    if (std::abs(rho - 1.0) < 1e-9) {
        return 1.0 / (static_cast<double>(bufferSize) + 1.0);
    }
    
    double p0 = (1.0 - rho) / (1.0 - std::pow(rho, bufferSize + 1));
    double pK = p0 * std::pow(rho, bufferSize);
    
    return pK;
}

// ТОЧНЫЙ МЕТОД ДЛЯ M/M/1/K (Глобальный Баланс) - 3 параметра
double AnalysisMethods::GlobalBalanceMethod(double lambda, double mu, int bufferSize) {
    return MM1KAnalysis(lambda, mu, bufferSize);
}

// ТОЧНЫЙ МЕТОД ДЛЯ M/M/1/K (Глобальный Баланс) - 4 параметра
double AnalysisMethods::GlobalBalanceMethod(double lambda, double mu, int bufferSize, int numNodes) {
    return QueueingModels::GlobalBalanceMethod(lambda, mu, bufferSize, numNodes);
}

// АНАЛИЗ ЗАДЕРЖКИ В СЕТИ ПОСЛЕДОВАТЕЛЬНЫХ УЗЛОВ M/M/1
double AnalysisMethods::CalculateSeriesMM1Delay(double lambda, double mu, int numNodes) {
   if (lambda < 0.0 || mu <= 0.0 || numNodes <= 0) return 0.0;
   
   const double serviceTime = 1.0 / mu;

   if (std::abs(lambda) < 1e-9) {
       return static_cast<double>(numNodes) * serviceTime;
   }

   if (lambda >= mu) {
       return std::numeric_limits<double>::infinity();
   }

   double delayPerNode = 1.0 / (mu - lambda);
   double totalDelay = static_cast<double>(numNodes) * delayPerNode;

   return totalDelay;
}

// Метод анализа средних значений (MVA)
double AnalysisMethods::MeanValueAnalysis(double lambda, double mu, int bufferSize, int numNodes) {
    return QueueingModels::MeanValueAnalysis(lambda, mu, bufferSize, numNodes);
}

// Метод Гордона-Ньюэлла
double AnalysisMethods::GordonNewellMethod(double lambda, double mu, int bufferSize, int numNodes) {
    return QueueingModels::GordonNewellMethod(lambda, mu, bufferSize, numNodes);
}

// Метод Бузена
double AnalysisMethods::BuzenMethod(double lambda, double mu, int bufferSize, int numNodes) {
    return QueueingModels::BuzenMethod(lambda, mu, bufferSize, numNodes);
}

// MVA задержка - режим 1 (Ad-hoc)
// Используем чистую формулу M/M/1 для последовательной сети
double AnalysisMethods::CalculateMVADelay_Mode1(double lambda, double mu, int bufferSize, int numNodes) {
    // Используем существующую функцию для последовательной сети M/M/1
    return CalculateSeriesMM1Delay(lambda, mu, numNodes);
}

// MVA задержка - режим 2 (Group)
// Используем чистую формулу M/M/1 для иерархической сети
double AnalysisMethods::CalculateMVADelay_Mode2(double lambda, double mu, int bufferSize, int numGroups, int nodesPerGroup) {
    if (lambda < 0.0 || mu <= 0.0 || numGroups <= 0 || nodesPerGroup <= 0) return 0.0;
    
    // Нагрузка распределяется между группами
    double lambdaPerGroup = lambda / static_cast<double>(numGroups);
    
    // Задержка внутри одной группы (последовательная сеть из nodesPerGroup узлов)
    double delayPerGroup = CalculateSeriesMM1Delay(lambdaPerGroup, mu, nodesPerGroup);
    
    if (std::isinf(delayPerGroup)) {
        return std::numeric_limits<double>::infinity();
    }
    
    // Эффективная скорость обслуживания группы
    double effectiveServiceRate = 1.0 / delayPerGroup;
    
    // Задержка между группами (numGroups - 1 связей между группами)
    if (lambdaPerGroup >= effectiveServiceRate) {
        return std::numeric_limits<double>::infinity();
    }
    
    // Задержка на одной связи между группами
    double delayPerLink = 1.0 / (effectiveServiceRate - lambdaPerGroup);
    // Всего (numGroups - 1) связей между numGroups группами
    double delayBetweenGroups = static_cast<double>(numGroups - 1) * delayPerLink;
    
    // Общая задержка = задержка внутри группы + задержка между группами
    double totalDelay = delayPerGroup + delayBetweenGroups;
    
    return totalDelay;
}

// Специализированная модель для Ad-Hoc сетей
double AnalysisMethods::AdHocThroughputModel(double load, double dataRateMbps, int numNodes) {
    if (load < 0.0 || dataRateMbps <= 0.0 || numNodes <= 0) return 0.0;
    
    // Модель учитывает конкуренцию между узлами в Ad-Hoc сети
    // Каждый узел конкурирует за доступ к каналу
    double totalLoad = load * dataRateMbps;
    
    // Эвристика: пропускная способность уменьшается из-за конкуренции
    // Коэффициент эффективности зависит от числа узлов
    double efficiency = 1.0 / (1.0 + 0.1 * (numNodes - 1));
    
    // Ограничиваем пропускную способность максимальной скоростью канала
    double throughput = totalLoad * efficiency;
    return std::min(throughput, dataRateMbps);
}

// Специализированная модель для групповых сетей
double AnalysisMethods::GroupThroughputModel(double load, double dataRateMbps, int numGroups) {
    if (load < 0.0 || dataRateMbps <= 0.0 || numGroups <= 0) return 0.0;
    
    // Модель учитывает иерархическую структуру групповой сети
    // Группы работают более эффективно благодаря агрегации трафика
    double totalLoad = load * dataRateMbps;
    
    // Эвристика: группирование улучшает эффективность
    // Коэффициент эффективности выше для меньшего числа групп
    double efficiency = 1.0 - 0.05 * (numGroups - 1);
    efficiency = std::max(0.5, efficiency); // Минимум 50% эффективности
    
    // Ограничиваем пропускную способность максимальной скоростью канала
    double throughput = totalLoad * efficiency;
    return std::min(throughput, dataRateMbps);
}