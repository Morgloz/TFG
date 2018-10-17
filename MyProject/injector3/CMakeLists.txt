set(LLVM_LINK_COMPONENTS support)

add_clang_executable(injectorV3
  InjectorV3.cpp
  )
target_link_libraries(injectorV3 PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
