#pragma once

#include <QtGlobal>

class IResizable {
public:
    virtual ~IResizable() = default;
    virtual void setResizable(bool resizable) = 0;
    [[nodiscard]] virtual bool isResizable() const = 0;
};
