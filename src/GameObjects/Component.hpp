#ifndef GL2D_COMPONENT_HPP
#define GL2D_COMPONENT_HPP

class Entity;

class IComponent {
public:
    virtual ~IComponent() = default;
};

class IUpdatableComponent : public IComponent {
public:
    ~IUpdatableComponent() override = default;
    virtual void update(Entity &owner, double dt) = 0;
};

#endif // GL2D_COMPONENT_HPP
