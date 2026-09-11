# Release compiler outputs are private to the selected release root.
RELEASE_BUILD_DIR := $(RELEASE_ROOT)/compiler-build
RELEASE_BUILD_BIN := $(RELEASE_BUILD_DIR)/fisics

release-build:
	@$(MAKE) BUILD_PROFILE=unsanitized SHIM_MODE=off BUILD_DIR="$(RELEASE_BUILD_DIR)/objects" BIN="$(RELEASE_BUILD_BIN)" LIB_FRONTEND="$(RELEASE_BUILD_DIR)/libfisics_frontend.a" LLVM_LIBS='$(RELEASE_LLVM_LIBS)' PROFILE_LDFLAGS='$(RELEASE_LINK_FLAGS)' "$(RELEASE_BUILD_BIN)"

release-artifact-disposable:
	@python3 scripts/create_release_root.py "$(RELEASE_ROOT)"
	@$(MAKE) release-contract
	@$(MAKE) release-stage
	@$(MAKE) release-archive-zip-from-stage
	@$(MAKE) release-archive-tgz-from-stage
	@$(MAKE) release-verify
	@$(MAKE) release-disposable-manifests

release-disposable-manifests:
	@set -eu; for fmt in zip tar.gz; do \
		artifact="$(RELEASE_ROOT)/$(RELEASE_BASENAME).$$fmt"; \
		sha=$$(shasum -a 256 "$$artifact" | cut -d ' ' -f 1); \
		{ printf 'product=fisiCs\nprogram=fisiCs\nversion=%s\nplatform=%s\narch=%s\nformat=%s\nchannel=%s\nartifact=%s\nsha256=%s\nsigned=0\nnotarized=0\n' "$(RELEASE_VERSION)" "$(RELEASE_PLATFORM)" "$(RELEASE_ARCH)" "$$fmt" "$(RELEASE_CHANNEL)" "$(RELEASE_BASENAME).$$fmt" "$$sha"; \
		printf 'source_commit=%s\n' "$$(git rev-parse HEAD)"; } > "$$artifact.manifest.txt"; \
	done
	@cp "$(RELEASE_ZIP_MANIFEST)" "$(RELEASE_MANIFEST)"

.PHONY: release-artifact-disposable release-disposable-manifests

release-root-contract-test:
	@python3 tests/integration/test_release_roots.py

.PHONY: release-root-contract-test
