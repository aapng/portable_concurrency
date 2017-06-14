macro(headeronly_target_headers __target)
  foreach(__item ${ARGN})
    get_filename_component(__header_abs_path ${__item} ABSOLUTE)
    list(APPEND __header_list ${__header_abs_path})
  endforeach()
  target_sources(${__target} INTERFACE  $<BUILD_INTERFACE:${__header_list}> $<INSTALL_INTERFACE:>)
endmacro(headeronly_target_headers)
