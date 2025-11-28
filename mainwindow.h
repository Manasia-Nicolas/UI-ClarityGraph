#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "graphwidget.h"
#include <QListWidget>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void autoSave();

private:
    Ui::MainWindow *ui;
    GraphWidget *graphWidget;
    QListWidget *nodeList;       // List of all nodes
    QComboBox   *typeSelector;
    QComboBox *privSelector;
};
#endif // MAINWINDOW_H
