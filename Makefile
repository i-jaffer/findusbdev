DIR_INC := ./inc
DIR_SRC := ./src
DIR_OBJ := ./obj
DIR_LIB := ./lib

TARGET := example.out
TARGET_OBJ := $(DIR_OBJ)/$(TARGET)
CC := gcc
CFLAGS := -I $(DIR_INC)

SRC := $(wildcard $(DIR_SRC)/*.c)
OBJ := $(patsubst %.c, $(DIR_OBJ)/%.o, $(notdir $(SRC)))

$(TARGET_OBJ) : $(OBJ)
	$(CC) $^ -o $@

$(DIR_OBJ)/%.o : $(DIR_SRC)/%.c
	$(CC) $< -c $(CFLAGS) -o $@

.PHONY : lib
lib :
	gcc $(DIR_SRC)/find_usbdevice.c -fPIC -shared $(CFLAGS) -o $(DIR_LIB)/libfind_usbdev.so
	arm-linux-gnueabihf-gcc $(DIR_SRC)/find_usbdevice.c -fPIC -shared $(CFLAGS) -o $(DIR_LIB)/libfind_usbdev_arm.so
	aarch64-linux-gnu-gcc $(DIR_SRC)/find_usbdevice.c -fPIC -shared $(CFLAGS) -o $(DIR_LIB)/libfind_usbdev_arm64.so
	ar crv $(DIR_LIB)/libfind_usbdev.a $(DIR_OBJ)/find_usbdevice.o

.PHONY : debug
debug :
	@echo $(SRC)
	@echo $(OBJ)
	@echo $(TARGET)
	@echo $(TARGET_OBJ)

.PHONY : clean
clean :
	-rm ./lib/*
	-rm ./obj/*