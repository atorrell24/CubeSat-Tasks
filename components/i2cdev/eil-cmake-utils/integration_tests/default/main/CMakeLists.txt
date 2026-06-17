idf_component_register(SRCS "default.c"
                    INCLUDE_DIRS ".")

# The cmake files are already available to include. Include one to use in your
# project.
include(eil_check_idf_features)
include(eil_check_chip_features)

if(IDF_HAS_ESP_DRIVERS)
    # do something
endif()
