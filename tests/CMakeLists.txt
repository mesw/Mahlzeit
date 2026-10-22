find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Network)

qt_add_executable(testMalzeit
    test_malzeit.cpp
    ../source/models/Recipe.h
    ../source/models/Recipe.cpp
    ../source/models/PantryModel.h
    ../source/models/PantryModel.cpp
    ../source/models/ShoppingListModel.h
    ../source/models/ShoppingListModel.cpp
    ../source/models/RecipeModel.h
    ../source/models/RecipeModel.cpp
    ../source/services/SettingsManager.h
    ../source/services/SettingsManager.cpp
    ../source/services/RecipeRepository.h
    ../source/services/RecipeRepository.cpp
    ../source/services/SuggestionEngine.h
    ../source/services/SuggestionEngine.cpp
    ../source/services/SyncManager.h
    ../source/services/SyncManager.cpp
)

target_include_directories(testMalzeit PRIVATE
    ../source
    ../source/models
    ../source/services
)

qt_add_resources(testMalzeit "test_resources"
    PREFIX "/"
    FILES
        ../resources/recipes.json
        ../resources/recipies.json
)

target_link_libraries(testMalzeit PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
    Qt6::Network
)

add_test(NAME testMalzeit COMMAND testMalzeit)
