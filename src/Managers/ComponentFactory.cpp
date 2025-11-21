//
// Created by Mohamad on 21/11/2025.
//

#include "ComponentFactory.hpp"
#include "GameObjects/Sprite.hpp"
#include "Graphics/Animation/AnimationStateMachine.hpp"
#include "Graphics/Animation/Animator.hpp"
#include "SpriteManager.hpp"
#include "AnimatorManager.hpp"
#include "AnimationStateMachineManager.hpp"
#include "TilemapManager.hpp"

std::unique_ptr<IComponent> ComponentFactory::create(const ComponentSpec &specs) {
    if (specs.type == "Transform") {
        auto comp = std::make_unique<TransformComponent>();
        return comp;
    }

    if (specs.type == "Sprite") {
        auto it = specs.strings.find("spriteId");
        GameObjects::Sprite *sprite = nullptr;
        if (it != specs.strings.end()) {
            sprite = SpriteManager::get(it->second);
        }
        return std::make_unique<SpriteComponent>(sprite);
    }

    if (specs.type == "Animator") {
        auto it = specs.strings.find("animatorId");
        Graphics::Animator *anim = nullptr;
        if (it != specs.strings.end()) {
            anim = AnimatorManager::get(it->second);
        }
        return std::make_unique<AnimatorComponent>(anim);
    }

    if (specs.type == "AnimStateMachine") {
        auto it = specs.strings.find("stateMachineId");
        Graphics::AnimationStateMachine *sm = nullptr;
        if (it != specs.strings.end()) {
            sm = AnimationStateMachineManager::get(it->second);
        }
        return std::make_unique<AnimationStateMachineComponent>(sm);
    }

    if (specs.type == "Tilemap") {
        std::shared_ptr<TilemapData> data;
        auto it = specs.strings.find("tilemapId");
        if (it != specs.strings.end()) {
            data = TilemapManager::get(it->second);
        }

        // Allow inline definitions via numbers if provided.
        if (!data) {
            auto wIt = specs.numbers.find("width");
            auto hIt = specs.numbers.find("height");
            auto tsXIt = specs.numbers.find("tileSizeX");
            auto tsYIt = specs.numbers.find("tileSizeY");
            if (wIt != specs.numbers.end() && hIt != specs.numbers.end()) {
                data = std::make_shared<TilemapData>();
                data->width = static_cast<int>(wIt->second);
                data->height = static_cast<int>(hIt->second);
                if (tsXIt != specs.numbers.end() && tsYIt != specs.numbers.end()) {
                    data->tileSize = {tsXIt->second, tsYIt->second};
                }
                data->tiles.resize(data->width * data->height, 0);
            }
        }

        return std::make_unique<TilemapComponent>(data);
    }

    if (specs.type == "Trigger") {
        auto comp = std::make_unique<TriggerComponent>();
        auto posX = specs.numbers.find("posX");
        auto posY = specs.numbers.find("posY");
        auto sizeX = specs.numbers.find("sizeX");
        auto sizeY = specs.numbers.find("sizeY");
        if (posX != specs.numbers.end() && posY != specs.numbers.end()) {
            comp->position = {posX->second, posY->second};
        }
        if (sizeX != specs.numbers.end() && sizeY != specs.numbers.end()) {
            comp->size = {sizeX->second, sizeY->second};
        }
        auto evIt = specs.strings.find("eventId");
        if (evIt != specs.strings.end()) {
            comp->eventId = evIt->second;
        }
        return comp;
    }

    // Unknown type -> return nullptr for now (or throw/assert)
    return nullptr;
}
