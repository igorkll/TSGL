file(GLOB_RECURSE FILES "*.c")
file(GLOB_RECURSE LIBFILES "../../../TSGL/*.c") #this method of connecting the library is not completely correct, but it is suitable for an example

idf_component_register(
    SRCS
        ${FILES}
        ${LIBFILES}
        
    INCLUDE_DIRS
        "."
        "../../../TSGL"
)