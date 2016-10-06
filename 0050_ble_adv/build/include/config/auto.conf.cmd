deps_config := \
	/home/esp32/esp/esp-idf/components/bt/Kconfig \
	/home/esp32/esp/esp-idf/components/esp32/Kconfig \
	/home/esp32/esp/esp-idf/components/freertos/Kconfig \
	/home/esp32/esp/esp-idf/components/log/Kconfig \
	/home/esp32/esp/esp-idf/components/lwip/Kconfig \
	/home/esp32/esp/esp-idf/components/mbedtls/Kconfig \
	/home/esp32/esp/esp-idf/components/spi_flash/Kconfig \
	/home/esp32/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/esp32/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/esp32/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/esp32/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
