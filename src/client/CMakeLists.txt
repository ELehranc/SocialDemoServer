# 定义了一个SRC_LIST变量,包含了该目录下得所有源文件

aux_source_directory(. SRC_LIST)

add_executable(PlantClient ${SRC_LIST})

# 指定可执行文件连接时需要依赖得库文件
target_link_libraries(PlantClient pthread)