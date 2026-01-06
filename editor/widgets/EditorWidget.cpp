#include "EditorWidget.hpp"

#include <QBoxLayout>

EditorWidget::EditorWidget(const QString& title, QWidget* parent)
        : QWidget(parent),
          m_title(title) {
    updateSizePolicy();
}

void EditorWidget::setResizable(bool resizable) {
    if (m_resizable == resizable) {
        return;
    }
    m_resizable = resizable;
    updateSizePolicy();
}

bool EditorWidget::isResizable() const {
    return m_resizable;
}

void EditorWidget::attachTo(QLayout* layout, int stretch) {
    if (!layout) {
        return;
    }
    if (m_attachedLayout == layout) {
        return;
    }
    detach();
    if (auto* boxLayout = qobject_cast<QBoxLayout*>(layout)) {
        boxLayout->addWidget(this, stretch);
    } else {
        layout->addWidget(this);
    }
    m_attachedLayout = layout;
    show();
    emit attached();
}

void EditorWidget::detach() {
    if (!m_attachedLayout) {
        return;
    }
    m_attachedLayout->removeWidget(this);
    m_attachedLayout = nullptr;
    hide();
    emit detached();
}

bool EditorWidget::isAttached() const {
    return m_attachedLayout != nullptr;
}

QSizePolicy::Policy EditorWidget::currentPolicy() const {
    return m_resizable ? QSizePolicy::Expanding : QSizePolicy::Preferred;
}

void EditorWidget::updateSizePolicy() {
    setSizePolicy(currentPolicy(), currentPolicy());
}
