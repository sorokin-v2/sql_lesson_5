cmake_minimum_required(VERSION 3.20)
project(lesson_5)
add_executable(lesson_5 main.cpp)
add_subdirectory(C:/libpqxx/libpqxx-7.7.4 libpqxx-build) # Укажите путь к libpq++
target_compile_features(lesson_5 PRIVATE cxx_std_17) # Включите C++17
target_link_libraries(lesson_5 pqxx) # Слинкуйте libpq++ к проекту
