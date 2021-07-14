add_llvm_library(LLVMATSGX
        ATSGXIRTag.cpp
        ADDITIONAL_HEADER_DIRS
        ${LLVM_MAIN_INCLUDE_DIR}/llvm/LTO
        DEPENDS
        intrinsics_gen
        )