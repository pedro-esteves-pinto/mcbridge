# inspired by https://stackoverflow.com/questions/1027247/specify-source-files-globally-with-glob

# Compare the new contents with the existing file, if it exists and is the 
# same we don't want to trigger a make by changing its timestamp.
function(update_file path content)
    set(old_content "")
    if(EXISTS "${path}")
        file(READ "${path}" old_content)
    endif()
    if(NOT old_content STREQUAL content)
        file(WRITE "${path}" "${content}")
    endif()
endfunction(update_file)

# Creates a file called CMakeDeps.cmake next to your CMakeLists.txt with
# the list of dependencies in it - this file should be treated as part of 
# CMakeLists.txt (source controlled, etc.).
function(update_deps_file deps)
    set(deps_file "CMakeDeps.cmake")
    # Normalize the list so it's the same on every machine
    list(REMOVE_DUPLICATES deps)
    list(SORT deps)
    # Update the deps file
    set(content "set(sources ${deps})")
    update_file(${deps_file} "${content}")
    # Include the file so it's tracked as a generation dependency we don't
    # need the content.
    include(${deps_file})
endfunction(update_deps_file)
