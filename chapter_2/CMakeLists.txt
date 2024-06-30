#
# Add source to this project's executable.
add_executable (chapter_2 
  main.cpp
  vk_engine.cpp
  vk_engine.h
 )

 set_property(TARGET chapter_2 PROPERTY CXX_STANDARD 20)

 target_link_libraries(chapter_2 vkguide_shared  vk-bootstrap imgui)