execute_process(
    COMMAND "${GENERATOR_EXE}" "${OUTPUT_C}"
    RESULT_VARIABLE gen_result
    OUTPUT_VARIABLE gen_stdout
    ERROR_VARIABLE gen_stderr
)
if(NOT gen_result EQUAL 0)
    message(FATAL_ERROR "codegen smoke generation failed:\n${gen_stdout}\n${gen_stderr}")
endif()

execute_process(
    COMMAND "${CC}" -std=c11 "-I${REPO_SRC}" -c "${OUTPUT_C}" -o "${OUTPUT_OBJ}"
    RESULT_VARIABLE cc_result
    OUTPUT_VARIABLE cc_stdout
    ERROR_VARIABLE cc_stderr
)
if(NOT cc_result EQUAL 0)
    message(FATAL_ERROR "generated code did not compile:\n${cc_stdout}\n${cc_stderr}")
endif()
