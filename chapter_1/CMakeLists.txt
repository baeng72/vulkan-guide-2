#
# Add source to this project's executable.
add_executable (chapter_1 
  main.cpp
  vk_engine.cpp
  vk_engine.h
 )

 set_property(TARGET chapter_1 PROPERTY CXX_STANDARD 20)

 target_link_libraries(chapter_1 vkguide_shared  vk-bootstrap)