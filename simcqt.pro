TEMPLATE = app
TARGET = SimulationCraft
QT += core gui network webkit
#CONFIG += paperdoll
#CONFIG += openssl

exists( build.conf ) {
  include( build.conf )
}

contains ( QT_MAJOR_VERSION , 5 ) {
QT += widgets webkitwidgets
DEFINES += QT_VERSION_5
}

QMAKE_CXXFLAGS += $$OPTS

QMAKE_CXXFLAGS_RELEASE += -DNDEBUG

win32 {
  LIBS += -lwininet
  RC_FILE += simcqt.rc

  # OpenSSL stuff:
  OPENSSL_INCLUDES = C:/OpenSSL-Win32/include
  OPENSSL_LIBS = C:/OpenSSL-Win32/lib
}

macx {
  QMAKE_INFO_PLIST = qt/Simulationcraft.plist
  ICON = qt/icon/Simcraft2.icns
  OBJECTIVE_SOURCES += qt/sc_mac_update.mm
  LIBS += -framework CoreFoundation -framework Sparkle -framework AppKit
}

COMPILER_CHECK_CXX = $$replace(QMAKE_CXX,'.*g\\+\\+'.*,'g++')

contains(COMPILER_CHECK_CXX,'g++') {
  QMAKE_CXXFLAGS += -std=c++0x -ffast-math
}

INCLUDEPATH += engine
DEPENDPATH += engine

HEADERS += engine/simulationcraft.hpp
HEADERS += engine/class_modules/agent_smug.hpp
HEADERS += engine/class_modules/cons_inq.hpp
HEADERS += engine/class_modules/bount_troop.hpp
HEADERS += engine/class_modules/warr_knight.hpp
HEADERS += engine/utf8.h
HEADERS += engine/utf8/core.h
HEADERS += engine/utf8/checked.h
HEADERS += engine/utf8/unchecked.h
HEADERS += qt/sc_autoupdate.h
HEADERS += qt/simulationcraftqt.hpp

SOURCES += engine/sc_action.cpp
SOURCES += engine/sc_buff.cpp
SOURCES += engine/sc_chart.cpp
SOURCES += engine/sc_consumable.cpp
SOURCES += engine/sc_dot.cpp
SOURCES += engine/sc_enchant.cpp
SOURCES += engine/sc_event.cpp
SOURCES += engine/sc_expressions.cpp
SOURCES += engine/sc_gear_stats.cpp
SOURCES += engine/sc_heal.cpp
SOURCES += engine/sc_http.cpp
SOURCES += engine/sc_item.cpp
SOURCES += engine/sc_js.cpp
SOURCES += engine/sc_knotor.cpp
SOURCES += engine/sc_log.cpp
SOURCES += engine/sc_mrrobot.cpp
SOURCES += engine/sc_option.cpp
SOURCES += engine/sc_pet.cpp
SOURCES += engine/sc_player.cpp
SOURCES += engine/sc_plot.cpp
SOURCES += engine/sc_raid_event.cpp
SOURCES += engine/sc_rating.cpp
SOURCES += engine/sc_reforge_plot.cpp
SOURCES += engine/sc_report_html_player.cpp
SOURCES += engine/sc_report_html_sim.cpp
SOURCES += engine/sc_report_text.cpp
SOURCES += engine/sc_report_xml.cpp
SOURCES += engine/sc_report.cpp
SOURCES += engine/sc_rng.cpp
SOURCES += engine/sc_sample_data.cpp
SOURCES += engine/sc_scaling.cpp
SOURCES += engine/sc_sequence.cpp
SOURCES += engine/sc_set_bonus.cpp
SOURCES += engine/sc_sim.cpp
SOURCES += engine/sc_stats.cpp
SOURCES += engine/sc_talent.cpp
SOURCES += engine/sc_target.cpp
SOURCES += engine/sc_thread.cpp
SOURCES += engine/sc_torhead.cpp
SOURCES += engine/sc_unique_gear.cpp
SOURCES += engine/sc_util.cpp
SOURCES += engine/sc_weapon.cpp
SOURCES += engine/sc_xml.cpp
SOURCES += engine/class_modules/sc_agent_smug.cpp
SOURCES += engine/class_modules/sc_warr_knight.cpp
SOURCES += engine/class_modules/sc_commando_mercenary.cpp
SOURCES += engine/class_modules/sc_gunslinger_sniper.cpp
SOURCES += engine/class_modules/sc_juggernaut_guardian.cpp
SOURCES += engine/class_modules/sc_sage_sorcerer.cpp
SOURCES += engine/class_modules/sc_scoundrel_operative.cpp
SOURCES += engine/class_modules/sc_sentinel_marauder.cpp
SOURCES += engine/class_modules/sc_shadow_assassin.cpp
SOURCES += engine/class_modules/sc_vanguard_powertech.cpp

SOURCES += qt/main.cpp
SOURCES += qt/sc_window.cpp

CONFIG(paperdoll) {
  DEFINES += SC_PAPERDOLL
  HEADERS += qt/simcpaperdoll.h
  SOURCES += qt/simcpaperdoll.cc
}

CONFIG(openssl) {
  DEFINES += SC_USE_OPENSSL
  INCLUDEPATH += $$OPENSSL_INCLUDES
  LIBS += -L$$OPENSSL_LIBS -lssleay32
}
