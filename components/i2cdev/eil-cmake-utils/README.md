# eil-cmake-utils

Provides helper `cmake` utilities for `esp-idf-lib`, designed to simplify
cross-target and cross-version library development for the ESP-IDF framework.

<!-- vim-markdown-toc GFM -->

* [Key Features:](#key-features)
* [Usage](#usage)
* [Testing](#testing)

<!-- vim-markdown-toc -->

## Key Features:

- **Target-Specific Logic**: Conditionally implement features based on
  hardware availability (e.g., fallback to custom pulse-counting logic when
  the PCNT peripheral is unavailable).
- **Version-Aware Dependencies**: Automatically resolve component dependencies
  based on the `esp-idf` version in use:
  - Modern versions: Requires `esp_driver_gpio`
  - Legacy versions: Falls back to the `driver` component
- **Multi-Target Support**: Streamlines building libraries compatible with
  diverse ESP32/ESP8266 variants and ESP-IDF releases.

> *Note*: Reduces boilerplate for conditional compilation and dependency management in ESP-IDF projects.

## Usage

```yaml
---
# main/idf_component.yml
version: 1.0.0
description: default
dependencies:
  esp-idf-lib/eil-cmake-utils:
    version: "*"
  idf:
    version: ">=5.2.0"
```

```cmake
# main/CMakeLists.txt
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

# The cmake files are already available to include. Include one to use in your
# project.
include(eil_check_idf_features)
include(eil_check_chip_features)

if(IDF_HAS_ESP_DRIVERS)
    # do something
endif()
```

## Testing

The project uses a custom test runner implemented in `tests/runner.cmake`.
This script automatically discovers and executes all tests located in the
`tests/` directory.

To run all tests:

```console
cmake -P tests/runner.cmake
```

To run a test:
```console
cmake -P tests/foo_test.cmake
```
