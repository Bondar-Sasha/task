// analysis_methods.h

#ifndef ANALYSIS_METHODS_H
#define ANALYSIS_METHODS_H

#include <vector>
#include <limits> // Для std::numeric_limits

class AnalysisMethods {
public:
    // -----------------------------------------------------------------
    // СТАНДАРТНЫЕ МЕТОДЫ АНАЛИЗА СМО
    // -----------------------------------------------------------------

    static double GlobalMeanAnalysis(const std::vector<double>& data);
    
    // M/M/c
    static double MMcAnalysis(double arrivalRate, double serviceRate, int servers);
    
    // M/M/1/K (Пропускная способность)
    static double MM1KAnalysis(double arrivalRate, double serviceRate, int bufferSize);
    
    // M/M/1 (Задержка)
    static double MMDelayModel(double arrivalRate, double serviceRate);
    
    // M/M/1/K (Вероятность потери)
    static double MMLossModel(double arrivalRate, double serviceRate, int bufferSize);

    // -----------------------------------------------------------------
    // ТОЧНЫЙ МЕТОД ДЛЯ M/M/1/K (Глобальный Баланс) - 4 параметра
    // -----------------------------------------------------------------

    static double GlobalBalanceMethod(double lambda, double mu, int bufferSize);
    static double GlobalBalanceMethod(double lambda, double mu, int bufferSize, int numNodes);

    // -----------------------------------------------------------------
    // АНАЛИЗ ЗАДЕРЖКИ В СЕТИ ПОСЛЕДОВАТЕЛЬНЫХ УЗЛОВ M/M/1 - 3 параметра
    // -----------------------------------------------------------------

    static double CalculateSeriesMM1Delay(double lambda, double mu, int numNodes);

    // -----------------------------------------------------------------
    // МЕТОДЫ АНАЛИЗА СРЕДНИХ ЗНАЧЕНИЙ (MVA) И ДРУГИЕ МЕТОДЫ
    // -----------------------------------------------------------------

    static double MeanValueAnalysis(double lambda, double mu, int bufferSize, int numNodes);
    static double GordonNewellMethod(double lambda, double mu, int bufferSize, int numNodes);
    static double BuzenMethod(double lambda, double mu, int bufferSize, int numNodes);
    static double CalculateMVADelay_Mode1(double lambda, double mu, int bufferSize, int numNodes);
    static double CalculateMVADelay_Mode2(double lambda, double mu, int bufferSize, int numGroups, int nodesPerGroup);

    // -----------------------------------------------------------------
    // СПЕЦИАЛИЗИРОВАННЫЕ МОДЕЛИ ДЛЯ AD-HOC И ГРУППОВЫХ СЕТЕЙ
    // -----------------------------------------------------------------

    static double AdHocThroughputModel(double load, double dataRateMbps, int numNodes);
    static double GroupThroughputModel(double load, double dataRateMbps, int numGroups);
};

#endif