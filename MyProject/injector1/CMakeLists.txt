set(LLVM_LINK_COMPONENTS support)

add_clang_executable(injectorV1
  InjectorV1.cpp
  )
target_link_libraries(injectorV1 PRIVATE
  clangTooling
  clangBasic
  clangASTMatchers
  )
