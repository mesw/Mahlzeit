cmake_minimum_required(VERSION 3.16)

project(Malzeit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMalzeit
    source/main.cpp
    source/models/Recipe.h
    source/models/Recipe.cpp
    source/models/PantryModel.h
    source/models/PantryModel.cpp
    source/models/ShoppingListModel.h
    source/models/ShoppingListModel.cpp
    source/models/RecipeModel.h
    source/models/RecipeModel.cpp
    source/services/SettingsManager.h
    source/services/SettingsManager.cpp
    source/services/RecipeRepository.h
    source/services/RecipeRepository.cpp
    source/services/SuggestionEngine.h
    source/services/SuggestionEngine.cpp
    source/services/SyncManager.h
    source/services/SyncManager.cpp
)

target_include_directories(appMalzeit PRIVATE
    source
    source/models
    source/services
)

qt_add_qml_module(appMalzeit
    URI Malzeit
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        source/qml/Main.qml
        source/qml/PantryView.qml
        source/qml/ShoppingListView.qml
        source/qml/RecipesView.qml
        source/qml/RecipeDetailView.qml
        source/qml/SettingsView.qml
    RESOURCES
        resources/recipes.json
        resources/recipies.json
)

qt_add_resources(appMalzeit "recipes_bundle"
    PREFIX "/"
    FILES
        resources/recipes.json
        resources/recipies.json
)

set_target_properties(appMalzeit PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMalzeit
    PRIVATE
        Qt6::Quick
        Qt6::Network
)

include(GNUInstallDirs)
install(TARGETS appMalzeit
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
