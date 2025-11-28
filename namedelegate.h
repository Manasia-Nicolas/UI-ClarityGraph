#pragma once
#include <QStyledItemDelegate>
#include <QLineEdit>

class NameDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit NameDelegate(QObject *parent = nullptr);

signals:
    void nameEdited(int row, const QString &newName) const;

public:
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;
};
