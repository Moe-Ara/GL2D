#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "FileExplorer/FileExplorer.hpp"

#include <QUrl>

#include <iostream>

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("GL2D Editor");

    auto* explorer = new FileExplorer();

    auto* label = new QLabel("Editor prototype is attached to the GL2D engine.");
    label->setAlignment(Qt::AlignCenter);

    auto* button = new QPushButton("Open Project Folder");
    QObject::connect(button, &QPushButton::clicked, []() {
        auto path = QDir::currentPath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });

    QObject::connect(explorer, &FileExplorer::fileActivated, [label](const QString& path) {
        label->setText(QStringLiteral("Selected file: %1").arg(path));
    });
    QObject::connect(explorer, &FileExplorer::folderActivated, [label](const QString& path) {
        label->setText(QStringLiteral("Selected folder: %1").arg(path));
    });

    auto* infoLayout = new QVBoxLayout();
    infoLayout->addWidget(label);
    infoLayout->addWidget(button);
    infoLayout->addStretch();

    auto* mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(16);
    explorer->attachTo(mainLayout, 2);
    mainLayout->addLayout(infoLayout, 1);

    window.setLayout(mainLayout);
    window.resize(640, 360);

    window.show();
    return app.exec();
}
