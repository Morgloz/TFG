set(LLVM_LINK_COMPONENTS support)

add_clang_executable(injectorV2
  InjectorV2.cpp
  )
target_link_libraries(injectorV2 PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
