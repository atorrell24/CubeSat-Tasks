# Prevent double inclusion
if(COMMAND eil_check_chip_features)
    return()
endif()

cmake_policy(SET CMP0057 NEW)  # Enable modern IN_LIST behavior
cmake_policy(SET CMP0011 NEW)  # Enable automatic ``cmake_policy()`` PUSH and POP.
#[=[@doc

TBW

#]=]

macro(eil_check_chip_features)
    message(DEBUG "eil_check_chip_features")

    # CHIP_HAS_PCNT
    set(CHIP_HAS_PCNT TRUE)
    set(TARGETS_WITHOUT_PCNT esp32c2 esp32c3 esp32c61)
    if(IDF_TARGET IN_LIST TARGETS_WITHOUT_PCNT)
        set(CHIP_HAS_PCNT FALSE)
    endif()
    set(CHIP_HAS_PCNT ${CHIP_HAS_PCNT} CACHE INTERNAL "The chip supports PCNT (Pulse Counter)")

    message(DEBUG "chip feature checks (eil_check_chip_features)")
    message(DEBUG " - CHIP_HAS_PCNT: ${CHIP_HAS_PCNT}")
endmacro()
