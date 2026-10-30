# Explicit optional entry point. The ordinary compiler makefile never includes this.
.DEFAULT_GOAL := help
.PHONY: help preflight source-kit check report
help:
	@echo 'Optional: preflight; source-kit COMPILER_ROOT=... OUT=FRESH_ABSOLUTE_DIRECTORY'
	@echo 'make -f tools/matrix_flow/Makefile check COMPILER_ROOT=... CORE_ROOT=REVIEWED_CORE_STRUCTURAL OUT=FRESH_ABSOLUTE_DIRECTORY'
preflight:
	python3 -B $(dir $(abspath $(lastword $(MAKEFILE_LIST))))preflight.py
source-kit:
	@test -n "$(COMPILER_ROOT)" -a -n "$(OUT)"
	python3 -B $(dir $(abspath $(lastword $(MAKEFILE_LIST))))developer_kit.py --compiler-root "$(COMPILER_ROOT)" --out "$(OUT)"
check report:
	@test -n "$(COMPILER_ROOT)" -a -n "$(CORE_ROOT)" -a -n "$(OUT)"
	python3 -B $(dir $(abspath $(lastword $(MAKEFILE_LIST))))workflow.py --compiler-root "$(COMPILER_ROOT)" --core-root "$(CORE_ROOT)" --out "$(OUT)" $(if $(filter check,$@),--check,)
