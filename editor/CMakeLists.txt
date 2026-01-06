set(CMAKE_AUTOMOC ON)
set(EDITOR_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE EDITOR_SOURCES CONFIGURE_DEPENDS "${EDITOR_SRC_DIR}/*.cpp")
file(GLOB_RECURSE EDITOR_HEADERS CONFIGURE_DEPENDS "${EDITOR_SRC_DIR}/*.hpp" "${EDITOR_SRC_DIR}/*.h")

add_executable(GL2DEditor
        ${EDITOR_SOURCES}
        ${EDITOR_HEADERS}
)

target_include_directories(GL2DEditor
        PRIVATE
        ${EDITOR_SRC_DIR}
)

target_link_libraries(GL2DEditor
        PRIVATE
        Qt5::Widgets
        gl2d_engine
)
