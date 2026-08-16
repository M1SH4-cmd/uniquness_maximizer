# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\analysis_contract_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\analysis_contract_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\analysis_engine_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\analysis_engine_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\document_chunker_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\document_chunker_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\docx_reader_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\docx_reader_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\ollama_integration_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ollama_integration_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\ollama_provider_test_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\ollama_provider_test_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\uniquness_maximizer_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\uniquness_maximizer_autogen.dir\\ParseCache.txt"
  "analysis_contract_test_autogen"
  "analysis_engine_test_autogen"
  "document_chunker_test_autogen"
  "docx_reader_test_autogen"
  "ollama_integration_test_autogen"
  "ollama_provider_test_autogen"
  "uniquness_maximizer_autogen"
  )
endif()
