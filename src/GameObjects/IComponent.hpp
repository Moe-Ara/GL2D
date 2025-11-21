#ifndef GL2D_ICOMPONENT_HPP
#define GL2D_ICOMPONENT_HPP

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
class IRenderableComponent : public IComponent {
public:
    ~IRenderableComponent() override = default;
    virtual void render(Entity& owner) = 0;
};
#endif // GL2D_ICOMPONENT_HPP
