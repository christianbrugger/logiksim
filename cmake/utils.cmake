
# Require the value in variable_name to not be the empty string
function(ls_require_not_empty variable_name)
    if ("${${variable_name}}" STREQUAL "")
        message(FATAL_ERROR "variable ${variable_name} cannot be empty")
    endif()

endfunction()

# Requires the value in variable_name to be of a subset of true values considered boolean.
function(ls_require_bool variable_name)
    string(TOUPPER "${${variable_name}}" value_upper)

    foreach(option ON YES TRUE OFF NO FALSE)
        if ("${value_upper}" STREQUAL "${option}")
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "variable ${variable_name} is not a boolean")
endfunction()
