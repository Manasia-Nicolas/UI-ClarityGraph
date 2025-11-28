#include "namedelegate.h"

NameDelegate::NameDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* NameDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &,
                                    const QModelIndex &index) const
{
    auto *line = new QLineEdit(parent);

    QString full = index.data().toString();
    QString name = full.section("(", 0, 0).trimmed();

    line->setText(name);
    return line;
}

void NameDelegate::setModelData(QWidget *editor,
                                QAbstractItemModel *,
                                const QModelIndex &index) const
{
    auto *line = qobject_cast<QLineEdit *>(editor);
    if (!line) return;

    emit nameEdited(index.row(), line->text());
}
