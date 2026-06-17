# the component only provides cmake files. an empty idf_component_register is
# sufficient.
idf_component_register()

# Append cmake directory to CMAKE_MODULE_PATH
list(APPEND CMAKE_MODULE_PATH "${COMPONENT_DIR}/cmake")

# Export CMAKE_MODULE_PATH to the parent scope
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)
