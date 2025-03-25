XBE_TITLE = iconator
GEN_XISO = $(XBE_TITLE).iso
SRCS = $(CURDIR)/xbe_parser.cpp $(CURDIR)/main.cpp
NXDK_DIR ?= $(CURDIR)/../..
NXDK_CXX = y

include $(NXDK_DIR)/Makefile
