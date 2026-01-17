#!/usr/bin/env bash
set -euo pipefail

WITH_PLATFORMS=0
DO_CLEAR=0
SHOW_HELP=0
ENGINE_OPTS=()

print_help() {
  cat <<'EOF'
framedot 빌드/테스트/설치 스크립트

사용법:
  ./run.sh [옵션...]

옵션:
  --help
      이 도움말을 출력합니다.

  --with-platforms
      platforms(예: ncurses 터미널) 타겟을 함께 빌드합니다.
      빌드된 플랫폼 데모 실행파일은 tests/out/ 으로 복사됩니다(자동 실행 안 함).

  --engine-op "<CMake 옵션 문자열>"
      CMake 구성 단계에 추가 옵션을 전달합니다.
      예) ./run.sh --engine-op "-DFRAMEDOT_MAX_INPUT_EVENTS=512 -DFRAMEDOT_INPUT_OVERFLOW_POLICY=1"

  --clear
      build/_install/build_consumer 디렉터리를 전부 삭제하고 종료합니다.
      (삭제만 수행하며, 이후 빌드는 진행하지 않습니다.)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --with-platforms) WITH_PLATFORMS=1; shift ;;
    --clear) DO_CLEAR=1; shift ;;
    --help) SHOW_HELP=1; shift ;;
    --engine-op)
      shift
      [[ $# -gt 0 ]] || { echo "[오류] --engine-op 뒤에는 문자열 인자가 필요합니다."; exit 1; }
      ENGINE_OPTS+=("$1")
      shift
      ;;
    *)
      echo "[오류] 알 수 없는 옵션: $1"
      echo "도움말: ./run.sh --help"
      exit 1
      ;;
  esac
done

if [[ "${SHOW_HELP}" -eq 1 ]]; then
  print_help
  exit 0
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
INSTALL_DIR="${ROOT_DIR}/_install"
CONSUMER_DIR="${ROOT_DIR}/build_consumer"
OUT_DIR="${ROOT_DIR}/tests/out"

if [[ "${DO_CLEAR}" -eq 1 ]]; then
  echo "[0/0] 빌드 산출물 삭제..."
  rm -rf "${BUILD_DIR}" "${INSTALL_DIR}" "${CONSUMER_DIR}" "${OUT_DIR}"
  echo "완료."
  exit 0
fi

PLAT_OPT="OFF"
if [[ "${WITH_PLATFORMS}" -eq 1 ]]; then
  PLAT_OPT="ON"
fi

echo "[1/5] Configure engine..."
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_SHARED_LIBS=OFF \
  -DFRAMEDOT_BUILD_TESTS=ON \
  -DCMAKE_TOOLCHAIN_FILE=${ROOT_DIR}/cmake/toolchains/clang.cmake \
  -DFRAMEDOT_BUILD_EXAMPLES=ON \
  -DFRAMEDOT_BUILD_PLATFORMS="${PLAT_OPT}" \
  -DFRAMEDOT_USE_SYSTEM_DEPS=OFF \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}" \
  ${ENGINE_OPTS[@]+"${ENGINE_OPTS[@]}"}

echo "[2/5] Build engine..."
cmake --build "${BUILD_DIR}" -j16

echo "[3/5] Run tests..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "[4/5] Run smoke example..."
"${BUILD_DIR}/examples/smoke/framedot_smoke"

if [[ "${WITH_PLATFORMS}" -eq 1 ]]; then
  echo "[4.5/5] Collect platform demos into tests/out (manual run)..."
  rm -rf "${OUT_DIR}"
  mkdir -p "${OUT_DIR}"

  # ncurses terminal demo
  NCURSES_DEMO="${BUILD_DIR}/platforms/terminal_ncurses/framedot_term_ncurses_demo"
  if [[ -f "${NCURSES_DEMO}" ]]; then
    cp -f "${NCURSES_DEMO}" "${OUT_DIR}/"
    echo "  copied: framedot_term_ncurses_demo -> tests/out/"
  else
    echo "  [warn] not found: ${NCURSES_DEMO}"
  fi

  echo "Platform demos are ready. Try:"
  echo "  ${OUT_DIR}/framedot_term_ncurses_demo"
  echo "  (press q to quit)"
fi

echo "[5/5] Install + consumer find_package test..."
cmake --install "${BUILD_DIR}"

rm -rf "${CONSUMER_DIR}"
mkdir -p "${CONSUMER_DIR}/src"

cat > "${CONSUMER_DIR}/CMakeLists.txt" <<'EOF'
cmake_minimum_required(VERSION 3.20)
project(framedot_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(framedot CONFIG REQUIRED)

add_executable(consumer src/main.cpp)
target_link_libraries(consumer PRIVATE framedot::framedot)
EOF

cat > "${CONSUMER_DIR}/src/main.cpp" <<'EOF'
#include <iostream>
#include "framedot/framedot.hpp"

int main() {
    auto v = framedot::core::version();
    std::cout << "Consumer sees framedot version: "
              << v.major << "." << v.minor << "." << v.patch << "\n";
    return 0;
}
EOF

cmake -S "${CONSUMER_DIR}" -B "${CONSUMER_DIR}/build" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="${INSTALL_DIR}"

cmake --build "${CONSUMER_DIR}/build" -j16
"${CONSUMER_DIR}/build/consumer"

echo "OK: build/test/smoke/install/consumer all passed."