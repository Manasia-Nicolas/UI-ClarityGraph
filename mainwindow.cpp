#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "graphwidget.h"

#include <QTimer>
#include <QDebug>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QTimer::singleShot(0, this, [this]() {

        QSplitter *splitter = new QSplitter(Qt::Horizontal, ui->centralwidget);

        QWidget *leftPanel = new QWidget();
        leftPanel->setMinimumWidth(250);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

        // ------------------- NODE LIST -------------------
        QLabel *nodeLabel = new QLabel("Nodes:");
        leftLayout->addWidget(nodeLabel);

        this->nodeList = new QListWidget();
        leftLayout->addWidget(this->nodeList);

        // ------------------- NODE TYPE -------------------
        QLabel *typeLabel = new QLabel("Node Type:");
        leftLayout->addWidget(typeLabel);

        this->typeSelector = new QComboBox();
        this->typeSelector->addItems({"Machine", "Server", "Database", "Firewall", "Router"});
        leftLayout->addWidget(this->typeSelector);

        // ------------------- PRIVILEGE -------------------
        QLabel *privLabel = new QLabel("Privilege:");
        privLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
        leftLayout->addWidget(privLabel);

        QComboBox *privSelector = new QComboBox();
        privSelector->addItems({"Low", "Medium", "High"});
        privSelector->setStyleSheet("font-size: 14px;");
        leftLayout->addWidget(privSelector);

        // store pointer
        this->privSelector = privSelector;


        // ------------------- EDGE EDITOR -------------------
        QLabel *label = new QLabel("Enter edges (u v):");
        leftLayout->addWidget(label);

        QTextEdit *edgeEditor = new QTextEdit();
        edgeEditor->setPlaceholderText("Example:\n0 1\n0 2\n1 3\n2 4");
        leftLayout->addWidget(edgeEditor);

        // ------------------- GRAPH WIDGET -------------------
        graphWidget = new GraphWidget();
        graphWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        splitter->addWidget(leftPanel);
        splitter->addWidget(graphWidget);

        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 1);

        QVBoxLayout *layout = new QVBoxLayout(ui->centralwidget);
        layout->addWidget(splitter);

        // ------------------- INITIAL NODES -------------------
        /*std::vector<NodeInfo> pts(10);
        for (int i = 0; i < 10; i++) {

            pts[i].x = rand() % 600;
            pts[i].y = rand() % 400;
            pts[i].type = "Machine";
            pts[i].privilege = "Low";
            pts[i].color = NodeInfo::colorForType(pts[i].type);

            nodeList->addItem("Node " + QString::number(i) + " (" + pts[i].type + ")");
        }

        graphWidget->setNodes(pts);
*/
        // ------------------------- SELECT NODE -------------------------
        connect(nodeList, &QListWidget::currentRowChanged,
                this, [this, privSelector](int idx) {
                    if (!graphWidget) return;
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    typeSelector->blockSignals(true);
                    typeSelector->setCurrentText(graphWidget->nodes[idx].type);
                    typeSelector->blockSignals(false);

                    privSelector->blockSignals(true);
                    privSelector->setCurrentText(graphWidget->nodes[idx].privilege);
                    privSelector->blockSignals(false);
                });

        // ------------------------- CHANGE NODE TYPE -------------------------
        connect(typeSelector, &QComboBox::currentTextChanged,
                this, [this](const QString &newType) {

                    int idx = nodeList->currentRow();
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    graphWidget->nodes[idx].type = newType;
                    graphWidget->nodes[idx].color = NodeInfo::colorForType(newType);

                    nodeList->item(idx)->setText("Node " + QString::number(idx) +
                                                 " (" + newType + ") (" + graphWidget->nodes[idx].privilege + ")");

                    graphWidget->update();
                });

        connect(privSelector, &QComboBox::currentTextChanged,
                this, [this](const QString &newPriv) {

                int idx = nodeList->currentRow();
                if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                graphWidget->nodes[idx].privilege = newPriv;
                graphWidget->nodes[idx].colorBorder = NodeInfo::colorBorderForPriv(newPriv);

                nodeList->item(idx)->setText("Node " + QString::number(idx) +
                                         " (" + graphWidget->nodes[idx].type + ") (" + newPriv + ")");

                graphWidget->update();
                });

        // ------------------------- EDGE PARSER -------------------------
        connect(edgeEditor, &QTextEdit::textChanged,
                this, [this, edgeEditor]() {

                    QString text = edgeEditor->toPlainText();
                    QStringList lines =
                        text.split("\n", Qt::SkipEmptyParts);

                    // -------------------------------
                    // 1) Find max node index used
                    // -------------------------------
                    int maxNode = -1;

                    for (const QString &line : lines) {
                        auto parts = line.split(" ", Qt::SkipEmptyParts);
                        if (parts.size() != 2) continue;

                        int u = parts[0].toInt();
                        int v = parts[1].toInt();
                        maxNode = std::max({maxNode, u, v});
                    }

                    // No edges → clear everything fully
                    if (maxNode < 0) {
                        graphWidget->nodes.clear();
                        graphWidget->setAdjacency({});
                        nodeList->clear();
                        graphWidget->update();
                        return;
                    }

                    // -------------------------------
                    // 2) Build adjacency list
                    // -------------------------------
                    std::vector<std::vector<int>> G(maxNode + 1);

                    for (const QString &line : lines) {
                        auto parts = line.split(" ", Qt::SkipEmptyParts);
                        if (parts.size() != 2) continue;

                        int u = parts[0].toInt();
                        int v = parts[1].toInt();

                        G[u].push_back(v);
                        G[v].push_back(u);
                    }

                    // -------------------------------
                    // 3) Rebuild nodes EXACTLY
                    // -------------------------------
                    auto &nodes = graphWidget->nodes;

                    // Save old info (types/colors) so we can restore
                    std::vector<NodeInfo> old = nodes;

                    nodes.clear();
                    nodes.resize(maxNode + 1);

                    for (int i = 0; i <= maxNode; i++) {

                        // If old node existed → preserve its metadata and position
                        if (i < old.size()) {
                            nodes[i] = old[i];
                        }
                        else {
                            // Brand new node → initialize defaults
                            nodes[i].x = rand() % graphWidget->width();
                            nodes[i].y = rand() % graphWidget->height();
                            nodes[i].type = "Machine";
                            nodes[i].privilege = "Low";
                            nodes[i].color = NodeInfo::colorForType("Machine");
                            nodes[i].colorBorder = NodeInfo::colorBorderForPriv("Low");
                        }
                    }

                    // -------------------------------
                    // 4) Update node list
                    // -------------------------------
                    nodeList->clear();
                    for (int i = 0; i <= maxNode; i++) {
                        nodeList->addItem("Node " + QString::number(i) +
                                          " (" + nodes[i].type + ") (" + nodes[i].privilege + ")");
                    }

                    // -------------------------------
                    // 5) Apply to graph
                    // -------------------------------
                    graphWidget->setAdjacency(G);
                    graphWidget->update();
                });
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
