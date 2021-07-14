
.PHONY: test
test: ./cjpeg
	./cjpeg -dct int -outfile testout.jpg testimg.ppm

clean:
	-${RM} cjpeg cjpeg.*
	-${RM} testout.jpg

clean-all:
	${MAKE} clean
	-${RM}  leakages_byte.* leakages_source.*