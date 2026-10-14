# The page's curl (CMakeLists.txt puts this folder first on CMAKE_MODULE_PATH with Emscripten): RecompFrontend's mod
# store fetches its mod list with libcurl, which a browser hasn't got. The store is off on the page (main.cpp leaves
# the discovery URL out, and the page has no Mods tab), so curl/curl.h here only lets that code compile: nothing in it
# reaches the network, and every call fails.
if (NOT TARGET CURL::libcurl)
    add_library(CURL::libcurl INTERFACE IMPORTED)
    set_target_properties(CURL::libcurl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}")
endif()
set(CURL_FOUND TRUE)
