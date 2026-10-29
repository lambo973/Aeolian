# Aeolian v1.0-a - Eliane su Daisy Pod
TARGET = aeolian_v1_0_a

CPP_SOURCES = main.cpp

# Percorso di libDaisy (clonata con --recurse-submodules e compilata con `make` nella sua cartella)
LIBDAISY_DIR ?= ../libDaisy

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
