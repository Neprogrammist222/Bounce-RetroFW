#!/bin/bash
set -e

echo "=== 0. Доустанавливаем python и mercurial ==="
sudo apt update
sudo apt install -y python-is-python3 mercurial

echo "=== 1. Скачиваем RetroFW Buildroot ==="
cd ~
if [ ! -d "retrofw-buildroot" ]; then
    HOST="github.com"
    REPO="retrofw/buildroot.git"
    git clone "https://${HOST}/${REPO}" retrofw-buildroot
fi

echo "=== 2. Патчим Makefile и качаем недостающие архивы ==="
cd ~/retrofw-buildroot
if ! grep -q "Workaround bug in make-4.3" Makefile; then
    printf '\n# Workaround bug in make-4.3\n$(BASE_DIR)/.br-external.mk:;\n' >> Makefile
fi

if [ -f "output/build/host-m4-1.4.18/lib/c-stack.c" ]; then
    sed -i 's/SIGSTKSZ < 16384/0/g' output/build/host-m4-1.4.18/lib/c-stack.c
fi

if [ -f "output/build/host-cmake-3.8.2/Source/cmServerProtocol.cxx" ]; then
    sed -i '1i #include <limits>' output/build/host-cmake-3.8.2/Source/cmServerProtocol.cxx
fi

mkdir -p dl
if [ ! -f "dl/libpng-1.6.37.tar.xz" ]; then
    SF_HOST="downloads.sourceforge.net"
    SF_PATH="project/libpng/libpng16/older-releases/1.6.37/libpng-1.6.37.tar.xz"
    wget -O dl/libpng-1.6.37.tar.xz "https://${SF_HOST}/${SF_PATH}"
fi

make RetroFW_defconfig BR2_EXTERNAL="$HOME/retrofw-buildroot/retrofw"
make toolchain sdl sdl_image BR2_JLEVEL=0

echo "=== 3. Собираем игру Bounce и упаковываем в bounce.opk ==="
cd ~/bounce-rs97
rm -rf output
sed -i 's/\r$//' main.cpp Makefile default.retrofw.desktop
make opk

echo "=== ГОТОВО! Файл bounce.opk лежит в ~/bounce-rs97/bounce.opk ==="