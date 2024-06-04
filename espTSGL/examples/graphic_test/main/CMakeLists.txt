file(GLOB_RECURSE FILES "*.c")

idf_component_register(SRCS
    ${FILES}
    
    INCLUDE_DIRS ".")