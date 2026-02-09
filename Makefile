BUILD_DIR = build

all:
	@cmake -B $(BUILD_DIR) -S . -DENABLE_COVERAGE=OFF
	@cmake --build $(BUILD_DIR) --parallel $(shell nproc)
	@cp ./build/amazons-c ./amazons-c

clean:
	rm -rf $(BUILD_DIR)/*
	rm ./amazons-c

run: all
	./$(BUILD_DIR)/$(EXE_NAME)

.PHONY: all clean run