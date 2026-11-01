# Release-owner snapshot dispatch only; ordinary compilation uses the normal graph.
PREPARATION_ALLOWED := release-contract integration-cli-metadata dev-smoke os-policy-contract release-root-contract-test preparation-producer-contract-test release-artifact-disposable release-notarize
ifneq ($(filter-out $(PREPARATION_ALLOWED),$(MAKECMDGOALS)),)
$(error Unsupported contained preparation goal: $(filter-out $(PREPARATION_ALLOWED),$(MAKECMDGOALS)))
endif
ifeq ($(strip $(MAKECMDGOALS)),)
$(error Contained preparation requires an explicit fixed goal)
endif
ifneq ($(filter release-notarize,$(MAKECMDGOALS)),)
ifneq ($(strip $(MAKECMDGOALS)),release-notarize)
$(error Authentication cannot mix with preparation goals)
endif
endif
.DEFAULT_GOAL := preparation-dispatch
.PHONY: preparation-dispatch $(MAKECMDGOALS)
$(MAKECMDGOALS): preparation-dispatch
preparation-dispatch:
	@python3 -B scripts/prepare_release_producer.py --root "$(PREPARATION_ROOT)" --make "$(MAKE)" --compiler "$(CC)" --release-root "$(RELEASE_ROOT)" -- $(MAKECMDGOALS)
