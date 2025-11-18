# Собираем все исходные файлы из поддиректорий
file(GLOB_RECURSE ALL_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cc
)

# Создаем исполняемый файл
build_exec(
    EXECNAME main
    EXECNAME_PREFIX scratch_src_
    SOURCE_FILES "${ALL_SOURCES}"
    LIBRARIES_TO_LINK "${ns3-libs}" "${ns3-contrib-libs}"
    EXECUTABLE_DIRECTORY_PATH ${CMAKE_OUTPUT_DIRECTORY}/scratch/src/
)

