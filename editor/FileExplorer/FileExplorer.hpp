//
// Created by Mohamad on 07/12/2025.
//

#ifndef GL2D_FILEEXPLORER_HPP
#define GL2D_FILEEXPLORER_HPP


#include <QString>

#include "widgets/EditorWidget.hpp"

class QModelIndex;

class QFileSystemModel;
class QSortFilterProxyModel;
class QTreeView;

class FileExplorer : public EditorWidget {
    Q_OBJECT

public:
    explicit FileExplorer(QWidget* parent = nullptr);
    void setRootPath(const QString& path);

signals:
    void fileActivated(const QString& path);
    void folderActivated(const QString& path);

private slots:
    void onEntryActivated(const QModelIndex& proxyIndex);
    void refresh();

private:
    void applyFilters();

    QFileSystemModel* m_model = nullptr;
    QSortFilterProxyModel* m_filterModel = nullptr;
    QTreeView* m_view = nullptr;
    bool m_showHidden = false;
};

#endif //GL2D_FILEEXPLORER_HPP
