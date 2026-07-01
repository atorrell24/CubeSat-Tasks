# CubeSat Firmware — C++ & ESP-IDF Cheat Sheet

Notes for adding new sensors to the `cubesat_sensors` component.
Everything here is something we actually used getting the MLX90614 working.

---

## Part 1 — Vocabulary

**Header file (`.h`)** — the "menu." Lists *declarations*: function names, their inputs,
and their outputs. Other files read it to know what they're allowed to call.

**Source file (`.cpp`)** — the "kitchen." Holds the *definitions*: the actual code that
does the work. Rule: you `#include` the `.h`, **never** the `.cpp`.

**`#include`** — pastes in another file's declarations so you can use them.

**Header guard** (`#ifndef / #define / #endif`) — wraps a header so it can't be pasted
twice by accident.

**`extern "C"`** — a bridge so C++ code and C code link together. Needed because the
sensor drivers are written in C.

**Declaration vs definition** — a declaration says *"this function exists and looks like
this"* (in the `.h`); a definition is the actual body (in the `.cpp`).

**Type** — what kind of value something is: `float` (decimal number), `int` (whole
number), `uint16_t` (unsigned 16-bit whole number), `bool` (true/false), `void` (nothing).

**`esp_err_t`** — ESP-IDF's error-code type. Almost every ESP-IDF function returns one.
`ESP_OK` means success; anything else is a failure code.

**Parameter / argument** — the inputs you hand a function inside its `( )`.

**Pointer (`*`) and address-of (`&`)** — a pointer holds the *location* of a variable.
`&object_temp` means "the address of my box." Used for **output parameters**: instead of
returning the value, the function fills the box whose address you gave it.

**Output-parameter pattern** — ESP-IDF functions usually return an `esp_err_t` (status)
and write the real result into a pointer you pass in. Example:
`mlx90614_get_to(handle, &temp)`.

**`struct`** — a bundle of named fields grouped under one name (e.g. the config structs).
Access fields with a dot: `cfg.scl_speed_hz`.

**Designated initializer** (`.field = value`) — a way to fill a struct by naming fields.
In C++ they must be in *declaration order*, and you can't write `.a.b = c` (use a nested
brace or a plain assignment instead).

**`= {}`** — value-initialization: sets *every* field of a struct to zero at once. We use
this, then assign the fields we care about, to dodge the "missing field initializer" error.

**`static` (at file scope)** — makes a variable private to its file and keeps it alive for
the whole program. That's how our handles survive between `sensors_init()` and the reads.

**Handle** — an opaque token that represents a created thing (the I²C bus, a sensor). You
get it from an init function and pass it to every later call.

**`enum`** — a set of named integer constants. Examples: `GPIO_NUM_8`, `I2C_NUM_0`,
`ESP_OK`. In C++ you must use the named constant, not a bare number, for enum fields.

**`const`** — marks something read-only.

**`if / else`** — run code conditionally. `==` compares; `=` assigns.

**`while (true) { ... }`** — the forever loop your firmware runs in.

**`vTaskDelay(pdMS_TO_TICKS(1000))`** — FreeRTOS sleep. `pdMS_TO_TICKS` converts
milliseconds into the OS's internal "ticks." This one waits 1 second.

**Logging macros** — `ESP_LOGI` (info), `ESP_LOGW` (warning), `ESP_LOGE` (error).
Form: `ESP_LOGI(TAG, "text %.2f", value);`. Each file defines its own `TAG` string.

**`esp_err_to_name(err)`** — converts an error code into a readable name string.

**Component / `CMakeLists.txt` / `REQUIRES`** — ESP-IDF organizes code into components.
Each component's `CMakeLists.txt` lists its source files (`SRCS`), its include folder
(`INCLUDE_DIRS`), and the other components it depends on (`REQUIRES`).

---

## Part 2 — Log / print format placeholders

Inside a log or `printf` string, `%`-codes are placeholders filled by the values after it:

| Code    | Means                          | Example value |
|---------|--------------------------------|---------------|
| `%d`    | whole number (int)             | `42`          |
| `%u`    | unsigned whole number          | `42`          |
| `%f`    | decimal number (float)         | `24.531000`   |
| `%.2f`  | float, 2 decimal places        | `24.53`       |
| `%s`    | text string                    | `ESP_OK`      |
| `%x`    | number in hexadecimal          | `0x5a`        |

The number of values after the string must match the number of `%` codes, in order.

---

## Part 3 — The recipe to add ANY sensor

You proved this pattern with the MLX. Every new sensor repeats it:

1. **Read the driver's header** (`components/<sensor>/<sensor>.h`). Find: its init
   function, its read functions, its config struct, and its handle type.
2. **Add a `static` handle** for it near the top of `cubesat_sensors.cpp`.
3. **Attach it in `sensors_init()`** — fill its config struct (`= {}` then assign),
   then call its init function, reusing the shared `bus_handle`.
4. **Write a wrapper read function** (e.g. `sensors_read_voltage(float *v)`) in the
   `.cpp`, and **declare it** in `cubesat_sensors.h`.
5. **Add the driver to `REQUIRES`** in `components/cubesat_sensors/CMakeLists.txt`.
6. **Call your new function** from `main.cpp`, using the same try-check-log block.

---

## Part 4 — Gotchas checklist (things that bit us)

- Use named enum constants, not bare numbers: `GPIO_NUM_8`, not `8`.
- Fill config structs with `= {}` first, then assign fields (avoids the
  `-Werror=missing-field-initializers` build error and ordering rules).
- `#include` the `.h`, never the `.cpp` (double-include = linker errors).
- Call init **once**, before the loop. Read **inside** the loop.
- Every `.cpp` that logs needs its own `static const char *TAG`.
- Handles must be `static` at file scope so they survive after init returns.
- When you call a new driver, add its component name to `REQUIRES`.
- **Not every driver uses the same I²C layer.** The MLX uses the new `i2c_master`
  driver. Some drivers (e.g. INA219/BME via `i2cdev`) use the older I²C API and set up
  the bus differently — so always read each header first; the *shape* is the same but the
  exact function names and bus setup may differ. We'll adapt per sensor.

---

## Part 5 — Your working MLX example (the template to copy)

```cpp
// in cubesat_sensors.cpp
static mlx90614_handle_t mlx_handle;          // 1. a private handle

// inside sensors_init(), after the bus exists:
mlx90614_config_t mlx_cfg = {};               // 2. zero everything
mlx_cfg.mlx90614_device.device_address = MLX90614_DEFAULT_ADDRESS;
mlx_cfg.mlx90614_device.scl_speed_hz   = 100000;
err = mlx90614_init(bus_handle, &mlx_cfg, &mlx_handle);   // 3. attach on shared bus

// a wrapper read function:
esp_err_t sensors_read_object_temp(float *temp_c) {
    return mlx90614_get_to(mlx_handle, temp_c);
}
```
