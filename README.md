## Build Manual

step1:
    cd build
    mkdir Release
    cd Release

step 2:
    ../cmake/bin/cmake -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_TARGETS_TO_BUILD=host \
    -DLLVM_TARGET_ARCH=host  -DLLVM_ENABLE_ASSERTIONS=On \
    -DLLVM_ENABLE_PROJECTS="lld;clang" \
    -DLLVM_EXTERNAL_LLD_SOURCE_DIR="/Users/root/ATsgx/src/llvm-project/lld" \
    -DLLVM_EXTERNAL_CLANG_SOURCE_DIR="/Users/root/ATsgx/src/llvm-project/clang" \
    -G "CodeBlocks - Unix Makefiles" \
    -S "/Users/root/ATsgx/src/llvm-project/llvm/"

step 3:
    ../cmake/bin/cmake --build . -j 16 \
    --target lld clang opt llvm-config llvm-ar