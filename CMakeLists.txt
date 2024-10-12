# CMakeList.txt: Coupler 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.20)

# MSVC编译时，静态链接C++运行时
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project (Coupler VERSION 1.2.0)

if (MSVC)
    set(CMAKE_CXX_STANDARD          20)   # 指定C++标准
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    # 只能捕捉C++异常，且保证异常抛出时调用局部对象析构函数
    set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} /EHsc")   
else ()
    # 指定C++语言版本  
    set(CMAKE_CXX_FLAGS "${CAMKE_CXX_FLAGS} -std=c++20")
    set(CMAKE_CXX_STANDARD_REQUIRED True)
    set(VISIBILITY_INLINES_HIDDEN   1)        # 隐藏内联函数的符号
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)   # 所有符号默认不导出
    set(CMAKE_C_VISIBILITY_PRESET   hidden)   # 所有符号默认不导出  
    add_definitions("-Wl,-rpath=./")          # 库文件查找的首选目录“./”    
endif ()

# 将源代码添加到此项目的可执行文件。
#add_library (${PROJECT_NAME} SHARED
add_executable (${PROJECT_NAME} 
    "src/createdem.cpp" 
    "src/globalfun.cpp"
    "src/couple_1d_2d.cpp"
    "src/couple_1d_1d.cpp"
    "src/couple_1d_1d_2d.cpp"
    "src/coupler.cpp"     
    "test/couple_1d_1d_2d_test.cpp"
   # "test/online_couple_test.cpp"
    )

# 链接LIB库，区分WINDOWS和LINUX两个平台，WINDOWS区分Release和Debug两种情况
if (WIN32)
    target_include_directories(${PROJECT_NAME} PUBLIC "D:/sdk_v1.7.0/include") 
    target_link_directories(${PROJECT_NAME} PUBLIC "D:/sdk_v1.7.0/lib/windows")
	target_link_libraries(${PROJECT_NAME} optimized SWMMKernel GeoProcess rtflood-cpu)
	target_link_libraries(${PROJECT_NAME} debug     SWMMKernel GeoProcess rtflood-cpu)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_include_directories(${PROJECT_NAME} PUBLIC "/mnt/d/sdk_v1.7.0/include") 
    target_link_directories(${PROJECT_NAME} PUBLIC "/mnt/d/sdk_v1.7.0/lib/linux")
	target_link_libraries(${PROJECT_NAME} SWMMKernel GeoProcess rtflood-cpu)
elseif (CMAKE_SYSTEM_PROCESSOR MATCHES "arm|arm64|aarch64")
    target_include_directories(${PROJECT_NAME} PUBLIC "/root/waterbyte/include") 
    target_link_directories(${PROJECT_NAME} PUBLIC "/root/waterbyte/lib") 
    target_link_libraries(${PROJECT_NAME} SWMMKernel GeoProcess rtflood-cpu)
endif()

# TODO: 如有需要，请添加测试并安装目标。