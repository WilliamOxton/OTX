DEBUG=OFF
ifeq ($(DEBUG), OFF)
	SPRINGBOARD_FILE = springboard.s
else
	SPRINGBOARD_FILE = springboard_notgx.s
endif

springboard.o: ${SPRINGBOARD_FILE}
	@$(CC) -c $< -o $@
	@echo "CC  <=  $<"

clean:
	rm springboard.o
