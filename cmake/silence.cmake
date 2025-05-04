# Use this function to temporarily silence warnings for a target. This is useful
# for third-party libraries that we don't want to modify.
function(silence target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0 /WX-)
  else()
    target_compile_options(${target} PRIVATE -w -Wno-error)
  endif()
  set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "")
  set_target_properties(${target} PROPERTIES CXX_CPPCHECK "")
  set_target_properties(${target} PROPERTIES CXX_CPPLINT "")
endfunction()
