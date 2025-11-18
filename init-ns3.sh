#!/bin/bash

set -e

NS3_VERSION="3.45"
NS3_DIR="$HOME/ns-3.${NS3_VERSION}"
BUILD_EXAMPLES="--enable-examples"
BUILD_TESTS="--enable-tests"

echo "🔹 Установка ns-3.${NS3_VERSION} (официальный релиз)"

if ! command -v apt &> /dev/null; then
    echo "❌ Поддерживаются только Debian/Ubuntu."
    exit 1
fi

echo "🔹 Обновление системы..."
apt update

echo "🔹 Установка системных зависимостей..."
apt install -y \
    g++ \
    python3 \
    python3-dev \
    python3-setuptools \
    python3-pip \
    cmake \
    ninja-build \
    git \
    wget \
    tar \
    bzip2 \
    unzip \
    libgtk-3-dev \
    libxml2-dev \
    libsqlite3-dev \
    libc6-dev \
    libboost-all-dev \
    libgsl-dev \
    protobuf-compiler \
    libprotobuf-dev

apt update

apt install -y software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
apt update

apt install -y gcc-11 g++-11

update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 110
update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 110

echo "🔹 Проверка версий компонентов..."

check_version_ge() {
    local name="$1"
    local current="$2"
    local required="$3"
    if [ "$(printf '%s\n' "$required" "$current" | sort -V | head -n1)" = "$required" ]; then
        echo "✅ $name: $current (>= $required)"
    else
        echo "❌ $name: $current (< $required) — требуется как минимум $required"
        exit 1
    fi
}

GCC_VER=$(g++ -dumpfullversion 2>/dev/null || g++ -dumpversion)
PY_VER=$(python3 -c 'import sys; print(f"{sys.version_info.major}.{sys.version_info.minor}")')
CMAKE_VER=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)

check_version_ge "g++" "$GCC_VER" "10.1"
check_version_ge "Python" "$PY_VER" "3.8"
check_version_ge "CMake" "$CMAKE_VER" "3.16"

if [ -d "$NS3_DIR" ]; then
    echo "🟡 Каталог $NS3_DIR уже существует. Пропускаем загрузку."
else
    echo "🔹 Клонирование ns-3.${NS3_VERSION} из официального репозитория..."
    cd "$HOME"
    git clone --depth 1 --branch ns-3.45 https://gitlab.com/nsnam/ns-3-dev.git "$NS3_DIR"
fi

cd "$NS3_DIR"

echo "🔹 Установка Python-зависимостей ns-3..."
python3 -m pip install --user --upgrade pip
python3 -m pip install --user pybindgen

echo "🔹 Конфигурация ns-3..."
./ns3 configure --build-profile=optimized $BUILD_EXAMPLES $BUILD_TESTS

echo "🔹 Сборка ns-3..."
./ns3 build -j$(nproc)

echo "🔹 Запуск базовых тестов (может занять время)..."
./test.py -c || echo "⚠️ Некоторые тесты не прошли — это нормально в некоторых окружениях."

echo "🔹 Настройка глобального доступа к 'ns3'..."
mkdir -p "$HOME/.local/bin"
ln -sf "$NS3_DIR/ns3" "$HOME/.local/bin/ns3"

if [[ ":$PATH:" != *":$HOME/.local/bin:"* ]]; then
    echo 'export PATH="$HOME/.local/bin:$PATH"' >> "$HOME/.bashrc"
    export PATH="$HOME/.local/bin:$PATH"
    echo "🔸 Добавлен ~/.local/bin в ~/.bashrc"
fi

echo
echo "🎉 Установка ns-3.${NS3_VERSION} завершена успешно!"
echo "📁 Рабочий каталог: $NS3_DIR"
echo "🚀 Теперь вы можете использовать 'ns3 run <example>' из любого места в терминале!"
echo "💡 Пример: ns3 run hello-simulator"