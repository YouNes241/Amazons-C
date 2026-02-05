BUILD_DIR = build

all:
	@cmake -B $(BUILD_DIR) -S . -DENABLE_COVERAGE=OFF
	@cmake --build $(BUILD_DIR) --parallel $(shell nproc)

all_with_gpu:
	@cmake -B $(BUILD_DIR) -S . -DENABLE_COVERAGE=OFF -DENABLE_GPU=ON
	@cmake --build $(BUILD_DIR) --parallel $(shell nproc)

test_build:
	@cmake -B $(BUILD_DIR) -S . -DENABLE_COVERAGE=ON
	@cmake --build $(BUILD_DIR) --parallel $(shell nproc)

test: all
	@cd $(BUILD_DIR) && ctest --output-on-failure
	@echo "Analyse de la couverture (Objectif 85%)..."
	@gcovr -r . --html --html-details -o $(BUILD_DIR)/coverage.html

clean:
	rm -rf $(BUILD_DIR)/*

run: all
	./$(BUILD_DIR)/$(EXE_NAME)

.PHONY: all test test_build clean run