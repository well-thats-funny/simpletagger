project(${PROJECT_NAME}-test)

add_executable(${PROJECT_NAME}
    main.cpp
    ../src/TagProcessor.hpp
    ../src/TagProcessor.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE gsl::gsl-lite-v1 Qt6::Test TracyClient)

target_precompile_headers(${PROJECT_NAME} PRIVATE
        <QTest>

        <tracy/Tracy.hpp>
)
