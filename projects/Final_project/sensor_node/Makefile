CONTIKI_PROJECT = sensor_node
all: $(CONTIKI_PROJECT)

# Add library of sensor
CONTIKI_TARGET_SOURCEFILES += dht22.c
MODULES += os/services/simple-energest

MAKE_MAC = MAKE_MAC_TSCH

CONTIKI = ../../..
include $(CONTIKI)/Makefile.include


