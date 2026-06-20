idf_component_register(
    SRCS
        "main.c"
        "compiler.c"
        "vm/vm.c"
        "vm/bytecode.c"
        "hal/gpio.c"
        "hal/sleep.c"
        "repl/repl.c"
    INCLUDE_DIRS
        "."
        "../include"
    REQUIRES spiffs esp_driver_uart esp_driver_gpio
)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu17)
