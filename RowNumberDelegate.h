#pragma once
#include <QStyledItemDelegate>

class RowNumberDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit RowNumberDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &,
                          const QModelIndex &index) const override;

    void setModelData(QWidget *editor,
                      QAbstractItemModel *model,
                      const QModelIndex &index) const override;

signals:
    void nameEdited(int row, const QString &newName) const;
};
