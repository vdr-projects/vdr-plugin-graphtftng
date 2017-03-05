project(plasma-gtft)

find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroOptionalAddSubdirectory)
# find_package(Plasma REQUIRED)

add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})
include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${PLASMA_INCLUDE_DIR})

set(gtft_SRCS 
    configdialog.cc
    gtft.cpp
    comthread.cc
    common.cc
    tcpchannel.cc)

kde4_add_ui_files(gtft_SRCS config.ui)
macro_bool_to_01(KEXIV2_FOUND HAVE_KEXIV2)
kde4_add_plugin(plasma_applet_gtft ${gtft_SRCS})

target_link_libraries(plasma_applet_gtft ${KDE4_PLASMA_LIBS} ${KDE4_KDEUI_LIBS} )

install(TARGETS plasma_applet_gtft DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES plasma-applet-gtft.desktop DESTINATION ${SERVICES_INSTALL_DIR})

