#pragma once

#include <QLayout>
#include <QSizePolicy>
#include <QWidget>
#include <QString>

#include "IAttachable.hpp"
#include "IResizable.hpp"

class EditorWidget : public QWidget, public IResizable, public IAttachable {
    Q_OBJECT

public:
    explicit EditorWidget(const QString& title = {}, QWidget* parent = nullptr);

    void setResizable(bool resizable) override;
    [[nodiscard]] bool isResizable() const override;

    void attachTo(QLayout* layout, int stretch = 0) override;
    void detach() override;
    [[nodiscard]] bool isAttached() const override;

signals:
    void attached();
    void detached();

protected:
    QSizePolicy::Policy currentPolicy() const;

private:
    void updateSizePolicy();

    QLayout* m_attachedLayout = nullptr;
    bool m_resizable = true;
    QString m_title;
};
