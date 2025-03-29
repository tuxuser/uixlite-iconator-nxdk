XBE_TITLE = iconator
ISO_DEPS = $(OUTPUT_DIR)/Icons
GEN_XISO = $(XBE_TITLE).iso


$(GEN_XISO): $(ISO_DEPS)
$(OUTPUT_DIR)/Icons:
	cp -R external/UIX-Lite/Icons $(OUTPUT_DIR)/

SRCS = $(CURDIR)/xbe_parser.cpp $(CURDIR)/main.cpp
NXDK_DIR ?= $(CURDIR)/../..
NXDK_CXX = y

include $(NXDK_DIR)/Makefile

clean: clean_iso_deps
.PHONY: clean_iso_deps
clean_iso_deps:
	rm -rf $(ISO_DEPS)
