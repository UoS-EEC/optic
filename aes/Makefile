PROJECT = aes

MSP430_GCC_DIR = /opt/msp430-gcc/bin
MSP430_GCC_INC = /opt/msp430-gcc
# IDIR = $(MSP430_GCC_INC)/include
SUPPORT_FILE_DIRECTORY = /opt/msp430-gcc/include

CC = msp430-elf-gcc
DB = mspdebug
DEVICE = msp430fr5994
CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -O0 -g 
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -T $(DEVICE).ld

# implicit
# %.o: %.c  
#     $(CC) $(CPPFLAGS) $(CFLAGS) -c

# %: %.o    
#     $(CC) $(LDFLAGS) n.o $(LOADLIBES) $(LDLIBS)

OBJ = main.o

all: $(OBJ)
	$(CC) $? $(CFLAGS) $(LFLAGS) -o $(PROJECT).out

main.o: main.c
	$(CC) -c $(CFLAGS) $< -o $@

debug: $(PROJECT).out
	$(DB) tilib -d usbmodem144301 -v 3300
	
clean: 
	rm *.out *.o