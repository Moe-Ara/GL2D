//
// Created by Mohamad on 21/11/2025.
//

#ifndef GL2D_PREFABCATALOUGE_HPP
#define GL2D_PREFABCATALOUGE_HPP


#include "Prefab.hpp"

class PrefabCatalouge {
public:
    PrefabCatalouge()=delete;

    virtual ~PrefabCatalouge()=delete;

    PrefabCatalouge(const PrefabCatalouge &other) = delete;

    PrefabCatalouge &operator=(const PrefabCatalouge &other) = delete;

    PrefabCatalouge(PrefabCatalouge &&other) = delete;

    PrefabCatalouge &operator=(PrefabCatalouge &&other) = delete;
    static void registerPrefab(const Prefab& prefab);
    static const Prefab& get(const std::string& id);
    static bool contains(const std::string& id);
private:
    static std::unordered_map<std::string, Prefab>& prefabs();
};


#endif //GL2D_PREFABCATALOUGE_HPP
