
LIB_JPEG_PATH = ..
ORG_EXE = ./djpeg.org
ATSGX_EXE = ./djpeg
LEAKAGE_SOURCE_FILE_PREFIX = ./leakages_info
LEAKAGE_SOURCE_FILE = ${LEAKAGE_SOURCE_FILE_PREFIX}.code ${LEAKAGE_SOURCE_FILE_PREFIX}.data

#生成原始exe
.PHONY: org.exe
org.exe: ${LIB_JPEG_PATH}/libjpeg.a djpeg.o
	-${LD} ${PHONY_LD_FLAGS} \
	-L${LIB_JPEG_PATH} -ljpeg \
	djpeg.o \
	-o ${ORG_EXE}

#生成atsgx exe
.PHONY: atsgx.exe
atsgx.exe: ${LIB_JPEG_PATH}/libjpeg.a ${LEAKAGE_SOURCE_FILE} djpeg.o ${LIB_JPEG_PATH}/lib/springboard.o
	-${LD} ${PHONY_LD_FLAGS} \
	-L${LIB_JPEG_PATH} -ljpeg \
	-ATSGX --ATSGXOpt=2 \
	-ATSGXDebug \
	--leakage_info_file ${LEAKAGE_SOURCE_FILE_PREFIX} \
	${LIB_JPEG_PATH}/lib/springboard.o \
	djpeg.o \
	-o ${ATSGX_EXE}

.PHONY: test.atsgx
test.atsgx: ./djpeg.atsgx
	./djpeg.atsgx -dct int -ppm -outfile test_atsgx.ppm testimg.jpg

.PHONY: test.org
test.org: ./djpeg.org
	./djpeg.org -dct int -ppm -outfile test_org.ppm testimg.jpg

clean:
	-${RM} djpeg.o djpeg.atsgx djpeg.org
	-${RM} test_atsgx.ppm
	

clean-all:
	${MAKE} clean
	-${RM}  djpeg.org djpeg.atsgx
	-${RM}  leakages_byte.* leakages_info.*