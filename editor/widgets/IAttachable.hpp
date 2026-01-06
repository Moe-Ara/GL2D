#pragma once

#include <QtGlobal>

class QLayout;

class IAttachable {
public:
    virtual ~IAttachable() = default;
    virtual void attachTo(QLayout* layout, int stretch = 0) = 0;
    virtual void detach() = 0;
    [[nodiscard]] virtual bool isAttached() const = 0;
};
