LLVM_BIN=/Users/root/ATsgx/build/Release/bin
CLANG_PATH = ${LLVM_BIN}/clang

AR  = ${LLVM_BIN}/llvm-ar
LD = ${LLVM_BIN}/ld.lld
CFLAGS = -fno-asynchronous-unwind-tables -fno-inline -fno-jump-tables -O0 -g3
PHONY_CC=${CLANG_PATH} -flto ${CFLAGS}


PHONY_LD_FLAGS = -flavor gnu -z relro --hash-style=gnu --eh-frame-hdr \
	-m elf_x86_64 -dynamic-linker /lib64/ld-linux-x86-64.so.2 \
	-L/lib -L/lib/x86_64-linux-gnu -L/usr/lib \
	-L/usr/lib/x86_64-linux-gnu \
	-L/usr/lib/gcc/x86_64-linux-gnu/5.4.0 \
	-L/lib64 -L/usr/lib/x86_64-linux-gnu \
	-L/Users/alex/workingSpace/Klotski-defense/build/Release/lib \
	-lgcc --as-needed -lgcc_s --no-as-needed \
	-lc -lgcc --as-needed -lgcc_s \
	--no-as-needed /usr/lib/x86_64-linux-gnu/crt1.o \
	/usr/lib/x86_64-linux-gnu/crti.o \
	/usr/lib/x86_64-linux-gnu/crtn.o \
	/usr/lib/gcc/x86_64-linux-gnu/5.4.0/crtend.o \
	/usr/lib/gcc/x86_64-linux-gnu/5.4.0/crtbegin.o \

PHONY_LD=${LD} ${PHONY_LD_FLAGS}

### ====================lib/springboard.o ===================
LIB_SPRINGBOARD_PATH = ./lib
SPRINGBOARD_OBJ=${LIB_SPRINGBOARD_PATH}/springboard.o
${SPRINGBOARD_OBJ}:
	-${MAKE} -s -C ${LIB_SPRINGBOARD_PATH}

### ======================== libjpeg ========================
LIBJPEG_BUILD_PATH = ./build
CJPEG_PATH = ./cjpeg-pack
DJPEG_PATH = ./djpeg-pack

# JPEG_SDK=${pwd}/libjpeg.a
${LIBJPEG_BUILD_PATH}/Makefile:
	$(shell mkdir -p ${LIBJPEG_BUILD_PATH})
	cd ${LIBJPEG_BUILD_PATH} && ../jpeg-9a/configure

libjpeg.a:
	-${MAKE} -s -C ${LIBJPEG_BUILD_PATH} CC="${PHONY_CC}" cjpeg
	-${MAKE} -s -C ${LIBJPEG_BUILD_PATH} CC="${PHONY_CC}" djpeg
	cp ${LIBJPEG_BUILD_PATH}/*.o ${LIBJPEG_BUILD_PATH}/.libs/
	rm ${LIBJPEG_BUILD_PATH}/.libs/cjpeg.o ${LIBJPEG_BUILD_PATH}/.libs/djpeg.o
	${AR} rv libjpeg.a ${LIBJPEG_BUILD_PATH}/.libs/*.o
	cp ${LIBJPEG_BUILD_PATH}/cjpeg.o ${CJPEG_PATH}
	cp ${LIBJPEG_BUILD_PATH}/djpeg.o ${DJPEG_PATH}

.PHONY: libjpeg
libjpeg: ${LIBJPEG_BUILD_PATH}/Makefile libjpeg.a
#### ======================== djpeg ========================
#### 1. 生成原生DJPEG
DJPEG_ORG_EXE = ${DJPEG_PATH}/djpeg.org
DJPEG_ATSGX_EXE = ${DJPEG_PATH}/djpeg.atsgx
DJPEG_LEAKAGE_FILE = ${DJPEG_PATH}/leakages_info.code ${DJPEG_PATH}/leakages_info.data

${DJPEG_PATH}/djpeg.o: ${LIBJPEG_BUILD_PATH}/djpeg.o
	cp ${LIBJPEG_BUILD_PATH}/djpeg.o ${DJPEG_PATH}

.PHONY: djpeg.org
djpeg.org: libjpeg ${DJPEG_PATH}/djpeg.o
	-${MAKE} -C ${DJPEG_PATH} LD="${PHONY_LD}" org.exe

${DJPEG_LEAKAGE_FILE}: djpeg.org ${DJPEG_PATH}/leakages_byte.code ${DJPEG_PATH}/leakages_byte.data
	# {0} <executable> <leakage_file> <output_file>
#	python3 dwarf_decode_address.py  ${DJPEG_ORG_EXE} ${DJPEG_PATH}/leakages_byte.code \
#	        ${DJPEG_PATH}/leakages_info.code
#	python3 dwarf_decode_address.py  ${DJPEG_ORG_EXE} ${DJPEG_PATH}/leakages_byte.data \
#	        ${DJPEG_PATH}/leakages_info.data

.PHONY: djpeg.atsgx
djpeg.atsgx: libjpeg ${DJPEG_PATH}/djpeg.o ${DJPEG_LEAKAGE_FILE} ${SPRINGBOARD_OBJ}
	-${MAKE} -C ${DJPEG_PATH} LD="${PHONY_LD}" atsgx.exe

.PHONY: test-djpeg
test-djpeg: djpeg.atsgx djpeg.org
	-${MAKE} -C ${DJPEG_PATH} test.org
	-${MAKE} -C ${DJPEG_PATH} test.atsgx
	

### ======================== testcase ========================
TESTCASE_PATH = ./helloworld
TESTCSSE_ORG_EXE = ${TESTCASE_PATH}/testcase.org

#1. 生成bytecode
${TESTCASE_PATH}/testcase.o: ${TESTCASE_PATH}/testcase.c
	-${MAKE} -s -C ${TESTCASE_PATH} CC="${PHONY_CC}"

#2. 生成原生二进制文件 testcase.org
${TESTCSSE_ORG_EXE}: ${TESTCASE_PATH}/testcase.o
	-${LD} ${PHONY_LD_FLAGS}\
	-o ${TESTCSSE_ORG_EXE} ${TESTCASE_PATH}/testcase.o

#3. 根据分析的结果得到相应的源码泄露点
# {0} <executable> <leakage_file> <output_file>
.PHONY: leakages
leakages: ${TESTCSSE_ORG_EXE} ${TESTCASE_PATH}/leakages_byte.code ${TESTCASE_PATH}/leakages_byte.data
	python3 dwarf_decode_address.py  ${TESTCSSE_ORG_EXE} ${TESTCASE_PATH}/leakages_byte.code \
	        ${TESTCASE_PATH}/leakages_source.code
	python3 dwarf_decode_address.py  ${TESTCSSE_ORG_EXE} ${TESTCASE_PATH}/leakages_byte.data \
	        ${TESTCASE_PATH}/leakages_source.data

#4. 根据结果得到test case
testcase: ${TESTCASE_PATH}/testcase.o leakages ${SPRINGBOARD_OBJ}
	-${LD} ${PHONY_LD_FLAGS}\
	-o ${TESTCASE_PATH}/testcase ${TESTCASE_PATH}/testcase.o ${SPRINGBOARD_OBJ} \
	-ATSGX --ATSGXOpt=2 \
	-ATSGXDebug \
	--leakage_info_file ${TESTCASE_PATH}/leakages_source

### ======================== generate all ========================
.PHONY: all
all: libjpeg djpeg.org djpeg.atsgx

### ======================== clean ========================
.PHONY: clean-libjpeg
clean-libjpeg:
	-${MAKE} -C ${LIBJPEG_BUILD_PATH} clean
	-${RM} libjpeg.a
	-${MAKE} -C ${DJPEG_PATH} clean
	-${MAKE} -C ${CJPEG_PATH} clean

.PHONY: clean-testcase
clean-testcase:
	-${MAKE} -C ${TESTCASE_PATH} clean

.PHONY: clean-springboard
clean-springboard:
	-${MAKE} -C ${LIB_SPRINGBOARD_PATH} clean

.PHONY: clean-all
clean-all:
	${MAKE} clean-libjpeg
	${MAKE} clean-testcase
	${MAKE} clean-springboard
	-${RM} -r ${LIBJPEG_BUILD_PATH}	

