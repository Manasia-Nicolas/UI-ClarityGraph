#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "graphwidget.h"
#include <QListWidget>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>

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
    int countCrossings();

    // Returns 0..3 depending on the selected heuristic in the dropdown
    int currentHeuristicIndex() const { return heuristicIndex; }
    std::vector<std::pair<double,double>> runMultipleLayouts(int V, int E, const std::vector<std::vector<int>> &G);
private:
    // Recompute layout and refresh UI using current graphWidget state and heuristic
    void recomputeLayoutFromGraphState();
    // Randomize positions for current nodes and refresh UI (no solver)
    void randomizeCurrentNodePositions();
    // Randomize positions only for nodes in [startIdx, endExclusive)
    void randomizeNodePositionsInRange(int startIdx, int endExclusive);

    Ui::MainWindow *ui;
    GraphWidget *graphWidget;
    QListWidget *nodeList;       // List of all nodes
    QComboBox   *typeSelector;
    QComboBox *privSelector;
    QLabel *kLabel;
    int k = -1;
    int crossings = 0;
    QLabel *crossLabel;
    QComboBox *heuristicSelector; // Top-right dropdown for heuristic selection
    QCheckBox *autoUpdateCheck;   // Checkbox to toggle auto layout

    int heuristicIndex = 0;      // 0..3 maps to the selected heuristic
};
#endif // MAINWINDOW_H
