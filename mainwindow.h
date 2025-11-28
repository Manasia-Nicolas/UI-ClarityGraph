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

    // Returns 0..3 depending on the selected heuristic in the dropdown
    int currentHeuristicIndex() const { return heuristicIndex; }

private:
    Ui::MainWindow *ui;
    GraphWidget *graphWidget;
    QListWidget *nodeList;       // List of all nodes
    QComboBox   *typeSelector;
    QComboBox *privSelector;
    QComboBox *heuristicSelector; // Top-right dropdown for heuristic selection

    int heuristicIndex = 0;      // 0..3 maps to the selected heuristic
};
#endif // MAINWINDOW_H
