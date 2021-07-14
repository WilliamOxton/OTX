testcase.o: testcase.c
	@$(CC) -c -g $< -o $@
	@echo "CC <= $<" 


clean:
	-${RM} testcase.o testcase leakages_source.* 
	-${RM} testcase.org.asm
	-${RM} testcase.org testcase.atsgx