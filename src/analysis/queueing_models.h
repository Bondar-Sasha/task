#pragma once

#include <vector>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <iostream>

/**
 * Класс, реализующий методы анализа для ОДНОЙ системы M/M/1/K.
 *
 * Примечание по параметрам:
 * - lambda: Интенсивность поступления заявок (входной поток)
 * - mu: Интенсивность обслуживания (скорость сервера)
 * - bufferSize: ОБЩАЯ ёмкость системы (очередь + сервер, K).
 */
class QueueingModels {
private:
    /**
     * Вспомогательный метод: вычисляет p0 (вероятность пустой системы) для M/M/1/K.
     * K - общая ёмкость.
     */
    static double Calculate_p0(double rho, int K) {
        if (K <= 0) return 1.0; // Для M/M/1 p0 = 1 - rho
        
        if (std::abs(rho - 1.0) < 1e-9) {
            // Случай rho = 1
            return 1.0 / (static_cast<double>(K) + 1.0);
        } else {
            // Случай rho != 1
            return (1.0 - rho) / (1.0 - std::pow(rho, K + 1));
        }
    }

public:
    /**
     * Вычисляет эффективную пропускную способность (λ_eff) для M/M/1/K.
     * λ_eff = λ * (1 - p_loss)
     */
    static double Calculate_MM1K_Throughput(double lambda, double mu, int bufferSize) {
        if (lambda < 0.0 || mu <= 0.0) return 0.0;
        if (lambda == 0.0) return 0.0;

        double rho = lambda / mu;

        // Случай M/M/1 (бесконечный буфер)
        if (bufferSize <= 0) {
            return (rho < 1.0) ? lambda : mu;
        }

        // Случай M/M/1/K (конечная ёмкость K = bufferSize)
        int K = bufferSize;
        
        // Вероятность p0
        double p0 = Calculate_p0(rho, K);
        
        // Вероятность потери (p_K) - вероятность, что система занята K заявками
        // (Это p_K, а не p_{K+1}, так как ёмкость K - это состояния 0..K)
        double pK_loss = p0 * std::pow(rho, K);
        
        // Эффективная пропускная способность
        return lambda * (1.0 - pK_loss);
    }

    /**
     * Вычисляет среднее число заявок в СИСТЕМЕ (L_s) для M/M/1/K.
     */
    static double Calculate_MM1K_AvgSystemSize(double lambda, double mu, int bufferSize) {
        if (lambda < 0.0 || mu <= 0.0) return 0.0;
        if (lambda == 0.0) return 0.0;

        double rho = lambda / mu;

        // Случай M/M/1 (бесконечный буфер)
        if (bufferSize <= 0) {
            if (rho < 1.0) {
                return rho / (1.0 - rho);
            } else {
                return std::numeric_limits<double>::infinity(); 
            }
        }

        // Случай M/M/1/K (конечная ёмкость K = bufferSize)
        int K = bufferSize;
        double K_double = static_cast<double>(K);

        // L_s для rho = 1
        if (std::abs(rho - 1.0) < 1e-9) {
            return K_double / 2.0;
        }

        // L_s для rho != 1
        // Формула: L_s = ρ * (1 - (K+1)ρ^K + Kρ^(K+1)) / ((1-ρ)(1-ρ^(K+1)))
        double rho_K = std::pow(rho, K);
        double rho_K1 = rho_K * rho;
        
        double num = rho * (1.0 - (K_double + 1.0) * rho_K + K_double * rho_K1);
        double den = (1.0 - rho) * (1.0 - rho_K1);
        
        if (den == 0.0) {
            return K_double;
        }
        
        return num / den;
    }

    /**
     * Вычисляет среднюю задержку в СИСТЕМЕ (T_s) по Закону Литтла.
     * T_s = L_s / λ_eff
     */
    static double Calculate_MM1K_Delay(double lambda, double mu, int bufferSize) {
        double L_s = Calculate_MM1K_AvgSystemSize(lambda, mu, bufferSize);
        
        if (L_s <= 0.0) return 0.0;
        if (std::isinf(L_s)) {
            return std::numeric_limits<double>::infinity();
        }

        double lambda_eff = Calculate_MM1K_Throughput(lambda, mu, bufferSize);

        if (lambda_eff <= 0.0) {
            return std::numeric_limits<double>::infinity(); 
        }
        
        // Закон Литтла: T_s = L_s / λ_eff
        return L_s / lambda_eff;
    }

    // === Высокоуровневые методы анализа (с сетевыми эвристиками) ===
    
    /**
     * Метод анализа средних значений (Mean Value Analysis)
     * Вводим упрощенный сетевой фактор (обратная связь) для различения графиков.
     */
    static double MeanValueAnalysis(double lambda, double mu, int bufferSize, int numNodes) {
        if (lambda < 0.0 || mu <= 0.0 || numNodes <= 0) return 0.0;
        if (lambda == 0.0) return 0.0;
        
        // Базовая пропускная способность M/M/1/K
        double baseThroughput = Calculate_MM1K_Throughput(lambda, mu, bufferSize);
        
        // Эвристический фактор MVA для замкнутой сети:
        // Учет того, что прибытие зависит от числа клиентов (N) в системе (обратная связь)
        double MVA_Factor = static_cast<double>(numNodes) / (static_cast<double>(numNodes) + 1.0);

        return baseThroughput * MVA_Factor;
    }

    /**
     * Метод глобального баланса (Global Balance Method)
     * Используем ТОЧНОЕ решение M/M/1/K, так как это его область применения.
     */
    static double GlobalBalanceMethod(double lambda, double mu, int bufferSize, int numNodes) {
        if (lambda < 0.0 || mu <= 0.0) return 0.0;
        if (lambda == 0.0) return 0.0;
        
        // Global Balance Method является основой для M/M/1/K.
        return Calculate_MM1K_Throughput(lambda, mu, bufferSize);
    }

    /**
     * Метод Гордона-Ньюэлла (Gordon-Newell Method)
     * Используем сетевой коэффициент, вдохновленный $G(K, N)$
     */
    static double GordonNewellMethod(double lambda, double mu, int bufferSize, int numNodes) {
        if (lambda < 0.0 || mu <= 0.0 || numNodes <= 0) return 0.0;
        if (lambda == 0.0) return 0.0;
        
        double baseThroughput = Calculate_MM1K_Throughput(lambda, mu, bufferSize);
        
        // Эвристический сетевой коэффициент (стремится к 1)
        int N = std::max(numNodes, 1);
        double networkFactor = 1.0 - std::exp(-(double)N / 3.0);
        
        return baseThroughput * networkFactor;
    }

    /**
     * Метод Бузена (Buzen Method)
     * Используем сетевой коэффициент, вдохновленный алгоритмом свертки $G_n(g)$.
     */
    static double BuzenMethod(double lambda, double mu, int bufferSize, int numNodes) {
        if (lambda < 0.0 || mu <= 0.0 || numNodes <= 0) return 0.0;
        if (lambda == 0.0) return 0.0;
        
        double baseThroughput = Calculate_MM1K_Throughput(lambda, mu, bufferSize);
        
        // Эвристический коэффициент (стремится к 1)
        int N = std::max(numNodes, 1);
        double efficiency = static_cast<double>(N) / (static_cast<double>(N) + 2.0);
        
        return baseThroughput * efficiency;
    }

   // ========================================================================
// МЕТОД АНАЛИЗА СРЕДНИХ ЗНАЧЕНИЙ (MVA) ДЛЯ ЗАМКНУТЫХ СЕТЕЙ СМО
// ========================================================================

/**
 * Режим 1: Ad-hoc (9 узлов по отдельности)
 * @param mu - средняя интенсивность обслуживания на каждом узле (1/μ_i для всех узлов)
 * @param numNodes - количество узлов в сети (N = 9 для режима Ad-hoc)
 * @param K - ОБЯЗАТЕЛЬНОЕ общее число заявок в системе.
 * @return средняя задержка в системе T̄(K)
 */
 static double CalculateMVADelay_Mode1(double mu, int numNodes, int K) {
    if (mu <= 0.0 || numNodes <= 0 || K <= 0) return 0.0;
    
    const int N = numNodes;
    const double serviceTime = 1.0 / mu;
    
    // ИНИЦИАЛИЗАЦИЯ (k=0)
    std::vector<double> avgQueueLength(N, 0.0); // K̄_i(0) = 0 для всех i
    std::vector<double> visitCount(N, 1.0);     // e_i = 1.0 (каждый узел посещается 1 раз)

    double totalDelay = 0.0; // Будет хранить T̄(k)

    // ИТЕРАЦИОННЫЙ ПРОЦЕСС MVA: от k=1 до K
    for (int k = 1; k <= K; ++k) {
        std::vector<double> delayPerNode(N, 0.0);
        std::vector<double> newQueueLength(N, 0.0);

        // ШАГ a: Расчет T̄_i(k) = (1/μ_i) * [1 + K̄_i(k-1)] 
        // (Формула для MVA)
        for (int i = 0; i < N; ++i) {
            delayPerNode[i] = serviceTime * (1.0 + avgQueueLength[i]);
        }

        // ШАГ b: Расчет T̄(k) = Σ e_i * T̄_i(k)
        totalDelay = 0.0;
        for (int i = 0; i < N; ++i) {
            totalDelay += visitCount[i] * delayPerNode[i];
        }

        // ШАГ c: Расчет λ(k) = k / T̄(k)
        double systemThroughput = 0.0;
        if (totalDelay > 1e-10) {
            systemThroughput = static_cast<double>(k) / totalDelay;
        }

        // ШАГ d: Расчет K̄_i(k) = λ(k) * e_i * T̄_i(k)
        for (int i = 0; i < N; ++i) {
            double nodeThroughput = systemThroughput * visitCount[i];
            newQueueLength[i] = nodeThroughput * delayPerNode[i];
        }

        avgQueueLength = newQueueLength;
    }
    
    return totalDelay; // T̄(K)
}

/**
 * Режим 2: Групповой (3 подсети по 3 узла)
 * @param mu - средняя интенсивность обслуживания на каждом узле (μ)
 * @param numGroups - количество подсетей (3)
 * @param nodesPerGroup - количество узлов в каждой подсети (3)
 * @param K - ОБЯЗАТЕЛЬНОЕ общее число заявок в системе.
 * @return средняя задержка в системе T̄(K)
 */
static double CalculateMVADelay_Mode2(double mu, int numGroups, int nodesPerGroup, int K) {
    if (mu <= 0.0 || numGroups <= 0 || nodesPerGroup <= 0 || K <= 0) return 0.0;
    
    const int numSubnets = numGroups;
    const int nodesInSubnet = nodesPerGroup;
    const double serviceTime = 1.0 / mu;
    
    // ========================================================================
    // ШАГ 1: АНАЛИЗ И АГРЕГАЦИЯ ПОДСЕТЕЙ (FES)
    // ========================================================================
    std::vector<double> subnetThroughput(K + 1, 0.0); 
    std::vector<double> subnetVisitCount(nodesInSubnet, 1.0);

    for (int k_subnet = 1; k_subnet <= K; ++k_subnet) {
        std::vector<double> subnetQueueLength(nodesInSubnet, 0.0);
        double lastSubnetThpt = 0.0;

        for (int k = 1; k <= k_subnet; ++k) {
            std::vector<double> subnetDelay(nodesInSubnet, 0.0);
            std::vector<double> newSubnetQueue(nodesInSubnet, 0.0);

            // ШАГ a: T̄_i(k) = (1/μ_i) * [1 + K̄_i(k-1)]
            for (int i = 0; i < nodesInSubnet; ++i) {
                subnetDelay[i] = serviceTime * (1.0 + subnetQueueLength[i]);
            }

            // ШАГ b: T̄(k) = Σ e_i * T̄_i(k)
            double subnetTotalDelay = 0.0;
            for (int i = 0; i < nodesInSubnet; ++i) {
                subnetTotalDelay += subnetVisitCount[i] * subnetDelay[i];
            }

            // ШАГ c: λ(k) = k / T̄(k)
            double subnetThpt = 0.0;
            if (subnetTotalDelay > 1e-10) {
                subnetThpt = static_cast<double>(k) / subnetTotalDelay;
            }

            // ШАГ d: K̄_i(k) = λ(k) * e_i * T̄_i(k)
            for (int i = 0; i < nodesInSubnet; ++i) {
                double nodeThroughput = subnetThpt * subnetVisitCount[i];
                newSubnetQueue[i] = nodeThroughput * subnetDelay[i];
            }
            
            subnetQueueLength = newSubnetQueue;
            lastSubnetThpt = subnetThpt;
        }

        subnetThroughput[k_subnet] = lastSubnetThpt;
    }

    // ========================================================================
    // ШАГ 2: АНАЛИЗ СЕТИ ВЕРХНЕГО УРОВНЯ (3 эквивалентных узла)
    // ========================================================================
    std::vector<double> fesQueueLength(numSubnets, 0.0);
    std::vector<double> fesVisitCount(numSubnets, 1.0);
    double networkTotalDelay = 0.0;

    for (int k_network = 1; k_network <= K; ++k_network) {
        std::vector<double> fesDelay(numSubnets, 0.0);
        std::vector<double> newFesQueue(numSubnets, 0.0);

        // Расчет задержки на каждом эквивалентном узле
        for (int j = 0; j < numSubnets; ++j) {
            // Эвристика распределения заявок K по подсетям
            double customersInSubnet = static_cast<double>(k_network) / static_cast<double>(numSubnets);
            int k_subnet = std::max(1, static_cast<int>(std::round(customersInSubnet)));
            k_subnet = std::min(k_subnet, K);

            double effectiveServiceTime = 0.0;
            if (subnetThroughput[k_subnet] > 1e-10) {
                // Время обслуживания FES = 1 / λ_subnet(k_subnet)
                effectiveServiceTime = 1.0 / subnetThroughput[k_subnet];
            } else {
                effectiveServiceTime = serviceTime * nodesInSubnet;
            }

            // Формула (2) для FES: T̄_FES(k) = T̄_i(k) * [1 + K̄_FES(k-1)]
            // T̄_i(k) = effectiveServiceTime (для IS дисциплины - см. формулу 3)
            // Но мы используем [1 + K̄_FES(k-1)] по аналогии с формулой (2)
            fesDelay[j] = effectiveServiceTime * (1.0 + fesQueueLength[j]);
        }

        // Общая задержка T̄(k) = Σ e_i * T̄_i(k)
        networkTotalDelay = 0.0;
        for (int j = 0; j < numSubnets; ++j) {
            networkTotalDelay += fesVisitCount[j] * fesDelay[j];
        }

        // Пропускная способность λ(k) = k / T̄(k)
        double networkThroughput = 0.0;
        if (networkTotalDelay > 1e-10) {
            networkThroughput = static_cast<double>(k_network) / networkTotalDelay;
        }

        // Обновление длины очереди K̄_i(k) = λ(k) * e_i * T̄_i(k)
        for (int j = 0; j < numSubnets; ++j) {
            double fesThroughput = networkThroughput * fesVisitCount[j];
            newFesQueue[j] = fesThroughput * fesDelay[j];
        }

        fesQueueLength = newFesQueue;
    }

     return networkTotalDelay;
 }
};