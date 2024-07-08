#
# Add source to this project's executable.
add_executable (chapter_5 
  main.cpp
  vk_engine.cpp
  vk_engine.h
  vk_loader.h
  vk_loader.cpp
 )

 set_property(TARGET chapter_5 PROPERTY CXX_STANDARD 20)

 target_link_libraries(chapter_5 vkguide_shared  vk-bootstrap imgui tinygltf)