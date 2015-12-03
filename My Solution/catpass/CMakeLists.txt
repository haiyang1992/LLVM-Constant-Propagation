add_library(CatPass MODULE
    CatPass.cpp
)

target_compile_features(CatPass PRIVATE cxx_range_for cxx_auto_type)

set_target_properties(CatPass PROPERTIES
    COMPILE_FLAGS "-fno-rtti"
)

if(APPLE)
    set_target_properties(CatPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)
