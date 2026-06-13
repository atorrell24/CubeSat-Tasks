# Prevent double inclusion
if(COMMAND eil_check_idf_features)
    return()
endif()


#[=[@doc

IDF_HAS_ESP_DRIVERS: Check for esp_driver_* component support

Older esp-idf versions provide a monolithtic `driver` component. This
component includes several headers, such as `driver/gpio.h`. The component was
later succeeded by multiple components, such as `esp_driver_gpio`.

See Migration from 5.2 to 5.3 for more details.

https://docs.espressif.com/projects/esp-idf/en/release-v6.0/esp32/migration-guides/release-5.x/5.3/peripherals.html

When `IDF_HAS_ESP_DRIVERS` is true, add `esp_driver_*` to REQUIRES or
PRIV_REQUIRES when calling idf_component_register().

When false, add `driver` to REQUIRES or PRIV_REQUIRES.

REQUIRES should be set to all components whose header files are
`#included` from the public header files of this component.

PRIV_REQUIRES should be set to all components whose header files are
`#included` from any source files in this component, unless already listed
in REQUIRES.

#]=]

macro(eil_check_idf_features)
    message(DEBUG "eil_check_idf_features")
    message(DEBUG "Running IDF feature checks for version ${IDF_VERSION}")
    if(NOT DEFINED IDF_VERSION)
        message(FATAL_ERROR "IDF_VERSION is not defined. Are you running this inside an ESP-IDF environment?")
    endif()

    # Check for esp_driver_* support (ESP-IDF ≥ 5.3)
    set(IDF_HAS_ESP_DRIVERS FALSE)
    if((IDF_VERSION_MAJOR GREATER 5) OR
       (IDF_VERSION_MAJOR EQUAL 5 AND IDF_VERSION_MINOR GREATER_EQUAL 3))
        set(IDF_HAS_ESP_DRIVERS TRUE)
    endif()

    # CACHE store this entry in the cache file rather than keeping it as a
    #       local, temporary variable.
    set(IDF_HAS_ESP_DRIVERS ${IDF_HAS_ESP_DRIVERS} CACHE INTERNAL "esp-idf supports esp_driver_ components")

    message(DEBUG "esp-idf feature checks (eil_check_idf_features)")
    message(DEBUG " - IDF_HAS_ESP_DRIVERS: ${IDF_HAS_ESP_DRIVERS}")

endmacro()
