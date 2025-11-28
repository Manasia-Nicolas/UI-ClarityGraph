#ifndef TOPBAR_H
#define TOPBAR_H

#include <QWidget>      // base class
#include <QLabel>       // for QLabel
#include <QHBoxLayout>  // for QHBoxLayout

class TopBar : public QWidget
{
    Q_OBJECT  // required for Qt classes that might use signals/slots

public:
    TopBar(QWidget *parent = nullptr); // constructor

private:
    QLabel* titleLabel;    // label to display "Clarity Graph"
    QHBoxLayout* layout;   // layout to arrange widgets horizontally
};

#endif // TOPBAR_H
