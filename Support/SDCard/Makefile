TOOL_DIR := ../../Firmware/Tool
TOOL := $(TOOL_DIR)/Tool
TOOL_SOURCES := $(shell find $(TOOL_DIR) -name '*.go' -type f)

IMAGE := sdcard.img
TEMP_IMAGE := $(IMAGE).tmp
PCM_FILES := $(sort $(wildcard Sounds/Modes/*/*.pcm))

.PHONY: all clean

all: $(IMAGE)

$(TOOL): $(TOOL_SOURCES) $(TOOL_DIR)/go.mod $(TOOL_DIR)/go.sum
	cd $(TOOL_DIR) && go build -o Tool .

$(IMAGE): $(TOOL) $(PCM_FILES) Makefile
	@set -eu; \
		test -n "$(PCM_FILES)" || { echo "no PCM files found" >&2; exit 1; }; \
		rm -f "$(TEMP_IMAGE)"; \
		trap 'rm -f "$(TEMP_IMAGE)"' EXIT HUP INT TERM; \
		"$(TOOL)" sdcard fat format "$(TEMP_IMAGE)" --size 128; \
		for source in $(PCM_FILES); do \
			directory=$${source%/*}; \
			mode_directory=$${directory##*/}; \
			mode=$${mode_directory%%-*}; \
			"$(TOOL)" sdcard fat add "$(TEMP_IMAGE)" "$$source" --mode "$$mode"; \
		done; \
		mv "$(TEMP_IMAGE)" "$(IMAGE)"; \
		trap - EXIT HUP INT TERM

clean:
	rm -f "$(IMAGE)" "$(TEMP_IMAGE)"
