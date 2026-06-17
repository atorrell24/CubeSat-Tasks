include(${CMAKE_CURRENT_LIST_DIR}/test_helper.cmake)

require(eil_check_chip_features)

describe("CHIP_HAS_PCNT")
    when("target is esp32c2")
        set(IDF_TARGET esp32c2)
        it("is FALSE")
        expect(TO_BE FALSE)

    when("target is esp32")
        set(IDF_TARGET esp32)
        it("is TRUE")
        expect(TO_BE TRUE)
