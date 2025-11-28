#include "TopBar.h"
#include <QFont>

TopBar::TopBar(QWidget *parent) : QWidget(parent)
{

    // -------------------------------
    // 1. Create the label
    // -------------------------------
    titleLabel = new QLabel("Clarity Graph");

    // -------------------------------
    // 2. Customize font and color
    // -------------------------------
    QFont font = titleLabel->font(); // get the default font

    font.setPointSize(16);
    font.setBold(true);
    titleLabel->setFont(font);
    titleLabel->setStyleSheet("color: black;");
    //titleLabel->setContentsMargins(20, 0, 3, 0);


    // -------------------------------
    // 3. Create layout
    // -------------------------------
    layout = new QHBoxLayout(this);
    layout->setContentsMargins(2, 0, 0, 10);
    layout->addWidget(titleLabel, 0, Qt::AlignLeft);
    layout->addStretch();


    // Style and size

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    this->setMinimumHeight(60);
    this->setStyleSheet("background-color: white; border: 1px solid red;");

    // show in console
    qDebug() << "TopBar size:" << size();


}
