add_executable(badwa src/main.cpp)# Add your Source Files here
target_link_libraries(badwa PRIVATE $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static> )
