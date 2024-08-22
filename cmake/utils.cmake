
#
# Require the value in variable_name to not be the empty string
#
function(ls_require_not_empty variable_name)
    if ("${${variable_name}}" STREQUAL "")
        message(FATAL_ERROR "variable ${variable_name} cannot be empty")
    endif()

endfunction()


#
# Requires the value in variable_name to be of a subset of true values considered boolean.
#
function(ls_require_bool variable_name)
    string(TOUPPER "${${variable_name}}" value_upper)

    foreach(option ON YES TRUE OFF NO FALSE)
        if ("${value_upper}" STREQUAL "${option}")
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "variable ${variable_name} is not a boolean")
endfunction()


#
# Requires the value in to be of a subset of true values considered boolean.
#
function(ls_require_bool_value value)
    string(TOUPPER "${value}" value_upper)

    foreach(option ON YES TRUE OFF NO FALSE)
        if ("${value_upper}" STREQUAL "${option}")
            return()
        endif()
    endforeach()

    message(FATAL_ERROR "value ${value} is not a boolean")
endfunction()

#
# Throw error if two string values are not equal.
#
function(ls_require_equal_str value_a value_b)
    if (NOT (value_a STREQUAL value_b))
        message(FATAL_ERROR "values '${value_a}' and '${value_b}' need to be equal")
    endif()
endfunction()


#
# Set global variable with name and value.
#
# The variable is set in the cache, which works in nested scopes.
# Then it is made sure that no local or file variable is hiding the cache.
#
function(ls_set_global variable_name value)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE CACHE INTERNAL "")
    # make sure no variable hides the cache
    ls_require_equal_str("${CMAKE_INTERPROCEDURAL_OPTIMIZATION}" "TRUE")
endfunction()

