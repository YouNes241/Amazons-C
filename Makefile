# Raccourcis pour simplifier la vie
BUILD_DIR = build

all:
	@cmake -B $(BUILD_DIR) -S .
	@make -C $(BUILD_DIR)

# Exigence 1.6 : Lancement facile des tests et gcovr
test: all
	@cd $(BUILD_DIR) && ctest --output-on-failure
	@echo "Analyse de la couverture (Objectif 85%)..."
	@gcovr -r . --html --html-details -o $(BUILD_DIR)/coverage.html

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/amazons-c

.PHONY: all test clean run