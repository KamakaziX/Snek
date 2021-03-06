//
// Created by jordan on 5/10/19.
//


#include <algorithm>
#include <thread>
#include <chrono>
#include "Scenes/TitleScreen.h"
#include "Engine.h"
#include "AudioEngine.h"
#include "Scenes/MainGameScene.h"
#include "Scenes/OptionsScene.h"
/**
 * Default constructor.
 */
Engine::Engine() : numPlayers(0)
{
}


/**
 * Constructor
 * @param gameWindow The game window we will be drawing to.
 * @param players The number of players.
 */
Engine::Engine(int players)
{
    inputRouter = std::make_unique<InputRouter>();
    inputRouter->setWindow(stdscr);

    scenes.emplace_back(std::make_shared<TitleScreen>("titleScreen"));
    scenes.emplace_back(std::make_shared<MainGameScene>("gameScene"));
    scenes.emplace_back(std::make_shared<OptionsScene>("optionsScene"));

    currentScene = 0;
    scenes[currentScene]->LoadScene();
    if (numPlayers == 0)
    {
        return;
    }


}


/**
 * A routine for beginning the game. This could provide
 * a splash screen, menu screens whatever before we get into the main
 * loop.
 */
void Engine::Begin()
{
    // Do anything we need to then start the main loop.
    state = GameState::MAIN;
}


/**
 * The engine's main game loop.
 */
void Engine::MainLoop()
{
    // Do anything we need to, then end the game.
    int input = ERR;
    while(input != 'q' && !shouldQuit)
    {
        // First check if we should change the scene. This prevents any
        // temporaries from becoming destroyed during an update or draw.
        if (shouldChangeScene)
        {
            internalChangeScene();
        }
        scenes[currentScene]->clearWindows();
        inputRouter->checkInput();
        input = inputRouter->getInput();

        for (GameObjectSptr const &go : scenes[currentScene]->getGameObjects())
        {
            // Move the game forward
            go->Update();

            // Draw the game state.
            go->Draw();

        }
        processCollisions();
        processDestroyed();

        scenes[currentScene]->refreshWindows();
        // Sleep to provide a framerate.
        std::this_thread::sleep_for(std::chrono::milliseconds(gameSleep));
    }
    state = GameState::END;
}


/**
 * A routine for when the main loop ends. Display anything needed to display once the game finishs.
 */
void Engine::End()
{
    // TODO: Should we go back to Engine::Begin?
}


int Engine::getInput()
{
    return inputRouter->getInput();
}

/**
 * Determine if there is a collision, and if there is, execute each game object's collider action.
 */
void Engine::processCollisions()
{
    // auto const &go syntax is weird. Essentially I'm telling the compiler
    // That I don't want to make any changes or copies.
    // auto x - Works with copies
    // auto &x - Work with original, but I might make changes
    // auto const &x - Work with original, with no changes.
    std::list<GameObjectSptr> gameObjects = scenes[currentScene]->getGameObjects();
    for (auto const &go : gameObjects)
    {
        for ( auto const &other : gameObjects)
        {
            // We don't want to collide with ourselves.
            if (go == other)
            {
                continue;
            }
            if (go->getx() == other->getx() && go->gety() == other->gety())
            {
                go->Collider(other);
            }
        }
    }
}


/**
 * Remove any destroyed game objects from our list.
 *
 * If game object has its Destroy() function called, then this function will clean them
 * up at the end of a frame.
 *
 * @TODO: Destroy returns a boolean value which we should use to decide if we even want to call this function.
 * That would add to more efficiancy.
 */
void Engine::processDestroyed()
{
    scenes[currentScene]->removeDestroyed();
}


/**
 * Begin the game. Main entrypoin for our game. Manages state.
 *
 * TODO: Not sure how this should work. Can it be removed entierly?
 */
void Engine::RunGame()
{

    switch(state)
    {
        case GameState::BEGIN:
            Begin();
            break;
        case GameState::MAIN:
            MainLoop();
            break;
        case GameState::END:
            End();
            break;
        default:
            break;
    }
}


/**
 * Add a game object to the game.
 * @param go The game object to add.
 */
void Engine::addGameObject(GameObjectSptr go)
{
    if (go == nullptr)
    {
        return;
    }
    std::list<GameObjectSptr> gameObjects = scenes[currentScene]->getGameObjects();
    auto it = std::find(std::begin(gameObjects), std::end(gameObjects), go);
    if (it != std::end(gameObjects))
    {
        // We found our game object already in or list. Don't add it.
        // TODO: Is there any real reason to return a value here?
        return;
    }

    scenes[currentScene]->addGameObject(go);
}


/**
 * Determine the number of game objects.
 * @return The game objects.
 */
int Engine::numGameObjects()
{
    return scenes[currentScene]->getGameObjects().size();
}


/**
 * Determine if any of the snakes on the board are dead.
 * @return True if one of the snakes is dead.
 */
bool Engine::areAnyDead()
{

    std::list<std::shared_ptr<Snake>> snakes = FindGOByType<Snake>();
    for (auto const &snake : snakes)
    {
        if (snake->isDead())
        {
            return true;
        }
    }
    // If there are no snakes, i guess they're all dead.
    return true;

}


/**
 * Get all game objects of  specific type.
 *
 * This works similar to Unity's Find() function.
 *
 * @note This function is likely pretty bad performance wise.
 * This is due to the fact that the application has to check the RTTI table
 * each time, and determine if the cast is valid. Then we have to check
 * the return alue of the dynamic cast. All in all, it's probably better
 * to find the game object by the tag, but this can be useful.
 *
 * @tparam T The type of object to find.
 * @return A list of all of the objects matching the type.
 */
template<typename T>
std::list<std::shared_ptr<T>> Engine::FindGOByType()
{
    std::list<std::shared_ptr<T>> l;
    std::list<GameObjectSptr> gameObjects = scenes[currentScene]->getGameObjects();
    // I could use auto here. However, I don't like doing that unless the type is
    // difficult to predict (such as with iterators) or if the actual type isn't somewhere
    // nearby.
    for (GameObjectSptr const &go : gameObjects)
    {
        // Downcast the pointer. If the downcast fails, then nullptr will be returned, and
        // the object isn't the one we want.
        std::shared_ptr<T> tmp = std::dynamic_pointer_cast<T>(go);
        if (tmp != nullptr)
        {
            l.emplace_back(tmp);
        }
    }
    return l;
}


/**
 * Find all functions by their tag. This is likely way more efficiant than finding them by their type, however
 * the user can make the tag of an object anything they want.
 *
 * @param gameObjects The game objects to search through
 * @return A list of matching game objects. This list will be empty if none are found.
 */
std::list<GameObjectSptr> Engine::FindAllByTag(Tag tag)
{
    std::list<GameObjectSptr> matches;
    std::list<GameObjectSptr> gameObjects = scenes[currentScene]->getGameObjects();
    for (auto const &go : gameObjects)
    {
        if (go->getTag() == tag)
        {
            matches.emplace_back(go);
        }
    }

    return matches;
}

/**
 * Determine if there is any game object at the current location.
 *
 * @TODO: Need to figure out how I can deal with the fact that in the case of a snake, this will not take the tail into
 * account.
 *
 * @param y The y location to look at
 * @param x The x location to look at
 * @return True if there is a game object at this location, false otherwise.
 */
bool Engine::gameObjectAtLocation(int y, int x)
{
    std::list<GameObjectSptr> gameObjects = scenes[currentScene]->getGameObjects();
    for (auto const &go : gameObjects)
    {
        if (go->getx() == x && go->gety() == y)
        {
            return true;
        }
    }

    return false;
}

void Engine::changeScene(int newScene)
{
    if (newScene > static_cast<int>(scenes.size()) - 1)
    {
        return;
    }
    shouldChangeScene = true;
    requestedSceneChange = newScene;

}

void Engine::internalChangeScene()
{
    int maxScenes = static_cast<int>(scenes.size()) - 1;
    int minScenes = 0;


    if (requestedSceneChange >= minScenes && requestedSceneChange <= maxScenes)
    {
        scenes[currentScene]->UnloadScene();
        currentScene = requestedSceneChange;
        scenes[currentScene]->LoadScene();

        // Now for each object in the scene, call the start function so they can set up.
        for(GameObjectSptr gameObject : scenes[currentScene]->getGameObjects())
        {
            gameObject->Start();
        }

    }

    shouldChangeScene = false;
    requestedSceneChange = -1;
}

void Engine::changeScene(std::string sceneName)
{

    for (unsigned int i = 0; i < scenes.size(); i++)
    {
        if (scenes[i]->getName() == sceneName)
        {
            changeScene(i);
        }
    }
}

void Engine::quit()
{
    shouldQuit = true;
}

/**
 * A global engine variable for our game.
 *
 * @note: Yeah, globals, they suck right? However, since the engine keeps track of game objects,
 * but game objects need to affect the engine, or at least interact with it and its data, this interface
 * allows that to happen.
 */
std::shared_ptr<Engine> gameEngine;

template class std::list<GameObjectSptr> Engine::FindGOByType<GameObject>();
template class std::list<std::shared_ptr<Snake>> Engine::FindGOByType<Snake>();
template class std::list<std::shared_ptr<Food>> Engine::FindGOByType<Food>();
template class std::list<std::shared_ptr<GameRunner>> Engine::FindGOByType<GameRunner>();
template class std::list<std::shared_ptr<ScoreBoard>> Engine::FindGOByType<ScoreBoard>();

