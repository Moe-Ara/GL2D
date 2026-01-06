#include "FileExplorer.hpp"

#include <QAbstractItemView>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

FileExplorer::FileExplorer(QWidget* parent)
        : EditorWidget(QStringLiteral("File Explorer"), parent),
          m_model(new QFileSystemModel(this)),
          m_filterModel(new QSortFilterProxyModel(this)) {
    m_model->setRootPath(QDir::currentPath());
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setReadOnly(true);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setFilterKeyColumn(0);

    auto* search = new QLineEdit(this);
    search->setPlaceholderText("Filter files...");
    connect(search, &QLineEdit::textChanged, m_filterModel, &QSortFilterProxyModel::setFilterWildcard);

    auto* refreshButton = new QToolButton(this);
    refreshButton->setText("Refresh");
    connect(refreshButton, &QToolButton::clicked, this, &FileExplorer::refresh);

    auto* hiddenButton = new QToolButton(this);
    hiddenButton->setText("Show Hidden");
    hiddenButton->setCheckable(true);
    connect(hiddenButton, &QToolButton::toggled, this, [this, hiddenButton](bool enabled) {
        m_showHidden = enabled;
        hiddenButton->setText(enabled ? "Hide Hidden" : "Show Hidden");
        applyFilters();
    });

    auto* toolbar = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(6);
    toolbarLayout->addWidget(search, 1);
    toolbarLayout->addWidget(refreshButton);
    toolbarLayout->addWidget(hiddenButton);

    auto* tree = new QTreeView(this);
    tree->setModel(m_filterModel);
    tree->setRootIndex(m_filterModel->mapFromSource(m_model->index(QDir::currentPath())));
    tree->setHeaderHidden(true);
    tree->setAnimated(true);
    tree->setUniformRowHeights(true);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setContextMenuPolicy(Qt::CustomContextMenu);
    tree->setExpandsOnDoubleClick(true);

    connect(tree, &QTreeView::doubleClicked, this, &FileExplorer::onEntryActivated);
    m_view = tree;

    applyFilters();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);
    layout->addWidget(toolbar);
    layout->addWidget(tree);
}

void FileExplorer::setRootPath(const QString& path) {
    const auto sourceIndex = m_model->index(path);
    if (!sourceIndex.isValid()) {
        return;
    }
    const auto proxyIndex = m_filterModel->mapFromSource(sourceIndex);
    if (proxyIndex.isValid() && m_view) {
        m_view->setRootIndex(proxyIndex);
    }
}

void FileExplorer::onEntryActivated(const QModelIndex& proxyIndex) {
    const auto sourceIndex = m_filterModel->mapToSource(proxyIndex);
    const QString path = m_model->filePath(sourceIndex);
    const QFileInfo info(path);
    if (info.isDir()) {
        emit folderActivated(path);
    } else {
        emit fileActivated(path);
    }
}

void FileExplorer::refresh() {
    const QString currentRoot = m_model->rootPath();
    m_model->setRootPath(QString());
    m_model->setRootPath(currentRoot);
}

void FileExplorer::applyFilters() {
    QDir::Filters filters = QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot;
    if (m_showHidden) {
        filters |= QDir::Hidden | QDir::System;
    }
    m_model->setFilter(filters);
}
