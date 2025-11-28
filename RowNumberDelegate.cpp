#include "RowNumberDelegate.h"
#include <QPainter>
#include <QLineEdit>
#include <QStyleOptionViewItem>

RowNumberDelegate::RowNumberDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

// ----------------------------------------------------------
// PAINT: Draw row index column + highlight + item text
// ----------------------------------------------------------
void RowNumberDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;

    // -----------------------------------------
    // 1) Draw highlight for selected row
    // -----------------------------------------
    if (option.state & QStyle::State_Selected)
        painter->fillRect(rect, QColor("#c4dafc"));   // light blue
    else
        painter->fillRect(rect, Qt::white);

    // -----------------------------------------
    // 2) Draw index column (left)
    // -----------------------------------------
    QRect indexRect(rect.left(), rect.top(), 32, rect.height());
    QRect textRect(rect.left() + 36, rect.top(),
                   rect.width() - 36, rect.height());

    painter->fillRect(indexRect, QColor("#dce9f7"));  // soft blue
    painter->setPen(Qt::black);
    painter->drawText(indexRect, Qt::AlignCenter,
                      QString::number(index.row()));

    // -----------------------------------------
    // 3) Draw item text (full string)
    // -----------------------------------------
    painter->setPen(Qt::black);
    QString text = index.data().toString();

    painter->drawText(textRect,
                      Qt::AlignVCenter | Qt::AlignLeft,
                      text);

    painter->restore();
}

// ----------------------------------------------------------
// EDITOR: Only edit NAME portion (strip type + privilege)
// ----------------------------------------------------------
QWidget *RowNumberDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &,
                                         const QModelIndex &index) const
{
    QLineEdit *editor = new QLineEdit(parent);

    // Extract name only
    QString full = index.data().toString();
    QString name = full.section("(", 0, 0).trimmed();
    editor->setText(name);

    //
    // ---------- FORCE VISIBLE EDITING (WHITE TEXT) ----------
    //
    editor->setStyleSheet(
        "color: white;"              // text ALWAYS visible
        "background-color: #000000;" // black background
        "selection-color: white;"
        "selection-background-color: #333333;"
        );

    return editor;
}
// ----------------------------------------------------------
// COMMIT EDIT: Send only the new name
// ----------------------------------------------------------
void RowNumberDelegate::setModelData(QWidget *editor,
                                     QAbstractItemModel *model,
                                     const QModelIndex &index) const
{
    QLineEdit *line = qobject_cast<QLineEdit*>(editor);
    if (!line) return;

    QString newName = line->text().trimmed();

    // MUST commit so itemChanged is emitted
    model->setData(index, newName);
}
