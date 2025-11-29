#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "graphwidget.h"
#include "namedelegate.h"
#include "RowNumberDelegate.h"
#include "solver.h"

#include <QTimer>
#include <QDebug>
#include <QSplitter>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QJsonArray>
#include <QJsonObject>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QToolBar>

static bool segmentsIntersect(
    QPointF A, QPointF B,
    QPointF C, QPointF D)
{
    auto ccw = [](QPointF A, QPointF B, QPointF C) {
        return (C.y() - A.y()) * (B.x() - A.x()) >
               (B.y() - A.y()) * (C.x() - A.x());
    };

    return ccw(A,C,D) != ccw(B,C,D) &&
           ccw(A,B,C) != ccw(A,B,D);
}

int MainWindow::countCrossings()
{
    int count = 0;

    auto &nodes = graphWidget->nodes;
    auto &adj   = graphWidget->adj;

    // For every pair of edges
    for (int u = 0; u < adj.size(); u++) {
        for (int v : adj[u]) {
            if (u >= v) continue; // avoid duplicates

            QPointF A(nodes[u].x, nodes[u].y);
            QPointF B(nodes[v].x, nodes[v].y);

            for (int x = u + 1; x < adj.size(); x++) {
                for (int y : adj[x]) {

                    if (x >= y) continue;
                    if (u == x || u == y || v == x || v == y)
                        continue; // share vertex → not a crossing

                    QPointF C(nodes[x].x, nodes[x].y);
                    QPointF D(nodes[y].x, nodes[y].y);

                    if (segmentsIntersect(A, B, C, D))
                        count++;
                }
            }
        }
    }

    return count;
}


void MainWindow::autoSave()
{
    QString basePath = QCoreApplication::applicationDirPath();
    QString filePath = basePath + "/Projects/autosave.json";

    // ---- Create JSON ----
    QJsonArray nodeArray;
    for (int i = 0; i < graphWidget->nodes.size(); i++) {
        const auto &N = graphWidget->nodes[i];

        QJsonObject obj;
        obj["name"] = N.name;
        obj["type"] = N.type;
        obj["privilege"] = N.privilege;
        obj["x"] = N.x;
        obj["y"] = N.y;

        nodeArray.append(obj);
    }

    QJsonArray edgeArray;
    for (int u = 0; u < graphWidget->adj.size(); u++) {
        for (int v : graphWidget->adj[u]) {
            if (u < v) {
                QJsonArray pair;
                pair.append(u);
                pair.append(v);
                edgeArray.append(pair);
            }
        }
    }

    QJsonObject root;
    root["nodes"] = nodeArray;
    root["edges"] = edgeArray;

    QJsonDocument doc(root);

    // ---- Write file ----
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qDebug() << "Auto-saved to" << filePath;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Clarity Graph");

    QTimer::singleShot(0, this, [this]() {
        QString basePath = QCoreApplication::applicationDirPath();
        QString projectsFolder = basePath + "/Projects";

        QDir dir;
        if (!dir.exists(projectsFolder)) {
            if (!dir.mkpath(projectsFolder)) {
                qDebug() << "ERROR: Cannot create folder:" << projectsFolder;
            } else {
                qDebug() << "Created folder:" << projectsFolder;
            }
        } else {
            qDebug() << "Projects folder exists:" << projectsFolder;
        }



        QToolBar *toolbar = addToolBar("Main Toolbar");
        ///toolbar->setMovable(false);   // optional

        QAction *buttonExport = new QAction("Export JSON", this);
        QAction *buttonImport = new QAction("Import JSON", this);

        QMenu *fileMenu = menuBar()->addMenu("File");

        fileMenu->addAction(buttonExport);
        fileMenu->addAction(buttonImport);

        // Top-right heuristic selector combo box placed on the main toolbar
        heuristicSelector = new QComboBox(toolbar);
        heuristicSelector->addItems({
            "Spiral heuristic",
            "Degree greedy heuristic",
            "Barycentric heuristic",
            "distance refined barycentric heuristic",
            "Best heuristic"
        });
        heuristicSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        heuristicSelector->setToolTip("Choose layout heuristic");

        // Place it at the far right of the main toolbar (more reliable than menubar corner widgets across platforms)
        QWidget *toolbarSpacer = new QWidget(this);
        toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        toolbar->addWidget(toolbarSpacer);
        toolbar->addWidget(heuristicSelector);

        // Initialize and track heuristic index (0..3)
        heuristicSelector->setCurrentIndex(0);
        heuristicIndex = 0;
        connect(heuristicSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this](int idx){
                    // clamp to 0..3 to be safe if items change
                    if (idx < 0) idx = 0;
                    if (idx > 4) idx = 4;
                    heuristicIndex = idx;
                });

        // --------------------------------------------------------
        // SPLITTER
        // --------------------------------------------------------
        QSplitter *splitter = new QSplitter(Qt::Horizontal, ui->centralwidget);

        // --------------------------------------------------------
        // LEFT PANEL
        // --------------------------------------------------------
        QWidget *leftPanel = new QWidget();
        leftPanel->setMinimumWidth(250);
        QVBoxLayout *leftLayout = new QVBoxLayout(leftPanel);

        //K
        kLabel = new QLabel("k = ?");
        kLabel->setStyleSheet("font-weight: bold; font-size: 16px; padding: 4px;");
        leftLayout->addWidget(kLabel);

        //cross counter
        crossLabel = new QLabel("Crossings = 0");
        crossLabel->setStyleSheet("font-weight: bold; font-size: 16px; padding: 4px;");
        leftLayout->addWidget(crossLabel);

        // Node list
        QLabel *nodeLabel = new QLabel("Nodes:");
        leftLayout->addWidget(nodeLabel);

        nodeList = new QListWidget();
        nodeList->setEditTriggers(QAbstractItemView::DoubleClicked);
        nodeList->setStyleSheet("background: white;");

        auto *nameDelegate = new NameDelegate(this);
        nodeList->setItemDelegate(new RowNumberDelegate(this));
        leftLayout->addWidget(nodeList);

        // when user finishes editing the name in the list
        connect(nameDelegate, &NameDelegate::nameEdited,
                this, [this](int row, const QString &newName) {

                    if (row < 0 || row >= graphWidget->nodes.size())
                        return;

                    auto &N = graphWidget->nodes[row];
                    N.name = newName;

                    QListWidgetItem *item = nodeList->item(row);
                    if (!item) return;

                    nodeList->blockSignals(true);
                    item->setText(
                        N.name + " (" + N.type + ") (" + N.privilege + ")"
                        );
                    nodeList->blockSignals(false);

                    graphWidget->update();
                });

        // Type selector
        QLabel *typeLabel = new QLabel("Node Type:");
        leftLayout->addWidget(typeLabel);

        typeSelector = new QComboBox();
        typeSelector->addItems({"Machine", "Server", "Database", "Firewall", "Router"});
        leftLayout->addWidget(typeSelector);

        // Privilege
        QLabel *privLabel = new QLabel("Privilege:");
        leftLayout->addWidget(privLabel);

        privSelector = new QComboBox();
        privSelector->addItems({"Low", "Medium", "High"});
        leftLayout->addWidget(privSelector);

        // Edges
        QLabel *edgeLabel = new QLabel("Enter edges (u v):");
        leftLayout->addWidget(edgeLabel);

        QTextEdit *edgeEditor = new QTextEdit();
        ///edgeEditor->setPlaceholderText("Example:\n0 1\n0 2\n1 3\n2 4");
        edgeEditor->setStyleSheet(
            "background: white; "
            "color: black; "
            "selection-background-color: #cce4ff; "
            "selection-color: black;"
            );
        leftLayout->addWidget(edgeEditor);

        // --------------------------------------------------------
        // GRAPH WIDGET
        // --------------------------------------------------------
        graphWidget = new GraphWidget();
        graphWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        splitter->addWidget(leftPanel);
        splitter->addWidget(graphWidget);

        splitter->setStretchFactor(0, 1);
        splitter->setStretchFactor(1, 2);

        QVBoxLayout *mainLayout = new QVBoxLayout(ui->centralwidget);
        mainLayout->addWidget(splitter);

        ///QPushButton *buttonExport = new QPushButton("Export JSON");
        ///leftLayout->addWidget(buttonExport);

        ///QPushButton *buttonImport = new QPushButton("Import JSON");
        ///leftLayout->addWidget(buttonImport);

        // --------------------------------------------------------
        // CONNECT: Clicking node → select in list
        // --------------------------------------------------------
        connect(graphWidget, &GraphWidget::nodeClicked,
                this, [this](int idx) {
                    if (idx >= 0 && idx < nodeList->count())
                        nodeList->setCurrentRow(idx);
                });

        // --------------------------------------------------------
        // CONNECT: Selecting in list → highlight node
        // --------------------------------------------------------
        connect(nodeList, &QListWidget::currentRowChanged,
                this, [this](int idx) {

                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    graphWidget->selectedNode = idx;
                    graphWidget->update();

                    typeSelector->blockSignals(true);
                    typeSelector->setCurrentText(graphWidget->nodes[idx].type);
                    typeSelector->blockSignals(false);

                    privSelector->blockSignals(true);
                    privSelector->setCurrentText(graphWidget->nodes[idx].privilege);
                    privSelector->blockSignals(false);

                    autoSave();

                });
        connect(buttonImport, &QAction::triggered, this, [this, projectsFolder, edgeEditor](){
            QFileDialog::Options opts = QFileDialog::DontUseNativeDialog;
            QString filePath = QFileDialog::getOpenFileName(
                this,
                "Open Project JSON",
                projectsFolder,
                "JSON Files (*.json)",
                nullptr,
                opts
                );

            if (filePath.isEmpty())
                return;


            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "Error", "Cannot open JSON file.");
                return;
            }

            QByteArray data = file.readAll();
            file.close();

            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) {
                QMessageBox::warning(this, "Error", "Invalid JSON format.");
                return;
            }

            QJsonObject root = doc.object();

            if (!root.contains("nodes") || !root["nodes"].isArray()) {
                QMessageBox::warning(this, "Error", "JSON missing 'nodes'");
                return;
            }

            QJsonArray nodeArr = root["nodes"].toArray();
            graphWidget->nodes.clear();
            graphWidget->nodes.resize(nodeArr.size());

            nodeList->clear();

            for (int i = 0; i < nodeArr.size(); i++) {

                QJsonObject obj = nodeArr[i].toObject();

                NodeInfo &N = graphWidget->nodes[i];

                N.name = obj["name"].toString("Node " + QString::number(i));
                N.x = obj["x"].toDouble();
                N.y = obj["y"].toDouble();
                N.type = obj["type"].toString("Machine");
                N.privilege = obj["privilege"].toString("Low");

                N.updateColors();

                nodeList->addItem(
                    N.name + " (" + N.type + ") (" + N.privilege + ")"
                    );
            }

            // --------------------
            // IMPORT EDGES
            // --------------------
            std::vector<std::vector<int>> G(graphWidget->nodes.size());

            if (root.contains("edges") && root["edges"].isArray()) {

                QJsonArray edgeArr = root["edges"].toArray();
                for (auto eRef : edgeArr) {
                    QJsonArray e = eRef.toArray();
                    if (e.size() == 2) {
                        int u = e[0].toInt();
                        int v = e[1].toInt();
                        if (u < G.size() && v < G.size()) {
                            G[u].push_back(v);
                            G[v].push_back(u);
                        }
                    }
                }
            }

            graphWidget->setAdjacency(G);
            graphWidget->update();

            QString edgeText;
            for (int i = 0; i < G.size(); i++) {
                for (int v : G[i]) {
                    if (i < v)   // avoid duplicates
                        edgeText += QString::number(i) + " " + QString::number(v) + "\n";
                }
            }

            edgeEditor->setPlainText(edgeText);

            QMessageBox::information(this, "Imported", "Project loaded successfully.");
        });


        connect(buttonExport, &QAction::triggered, this, [this]() {

            QJsonArray nodeArray;
            for (int i = 0; i < graphWidget->nodes.size(); i++) {
                const auto &N = graphWidget->nodes[i];

                QJsonObject obj;
                obj["name"]      = N.name;
                obj["type"]      = N.type;
                obj["privilege"] = N.privilege;
                obj["x"]         = N.x;
                obj["y"]         = N.y;

                nodeArray.append(obj);
            }

            QJsonArray edgeArray;
            for (int u = 0; u < graphWidget->adj.size(); u++) {
                for (int v : graphWidget->adj[u]) {
                    if (u < v) {   // avoid duplicates
                        QJsonArray pair;
                        pair.append(u);
                        pair.append(v);
                        edgeArray.append(pair);
                    }
                }
            }

            QJsonObject root;
            root["nodes"] = nodeArray;
            root["edges"] = edgeArray;

            QJsonDocument doc(root);
            ///QString jsonPretty = doc.toJson(QJsonDocument::Indented);

            QString filename = "graph_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".json";

            QString fullPath = "Projects/" + filename;

            QFile file(fullPath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qDebug() << "ERROR: Cannot write file" << fullPath;
                return;
            }
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();

            qDebug() << "JSON exported to:" << fullPath;


            /*QDialog *dlg = new QDialog(this);
            dlg->setWindowTitle("Exported JSON");
            dlg->resize(600, 700);

            QVBoxLayout *lay = new QVBoxLayout(dlg);
            QTextEdit *edit = new QTextEdit();
            edit->setPlainText(jsonPretty);
            lay->addWidget(edit);

            QPushButton *closeBtn = new QPushButton("Close");
            lay->addWidget(closeBtn);
            connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::close);

            dlg->exec();*/

        });

        // ------------------------- RENAME NODE (FULL FIX) -------------------------
        connect(nodeList, &QListWidget::itemDoubleClicked,
                this, [this](QListWidgetItem *item) {

                    int idx = nodeList->row(item);
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    // Show ONLY the name while editing
                    QString name = graphWidget->nodes[idx].name;

                    nodeList->blockSignals(true);
                    item->setText(name);
                    nodeList->blockSignals(false);
                    autoSave();

                });

        connect(nodeList, &QListWidget::itemChanged,
                this, [this](QListWidgetItem *item) {

                    int idx = nodeList->row(item);
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    // 1. Read the edited name
                    QString newName = item->text().trimmed();
                    graphWidget->nodes[idx].name = newName;

                    // 2. Rebuild full label
                    const auto &N = graphWidget->nodes[idx];
                    ///QString full =
                        ///N.name + " (" + N.type + ") (" + N.privilege + ")";

                    // 3. Write full label back
                    nodeList->blockSignals(true);
                    item->setText(N.name);
                    nodeList->blockSignals(false);

                    graphWidget->update();
                    autoSave();

                });
        connect(nodeList->itemDelegate(), &QAbstractItemDelegate::closeEditor,
                this, [this](QWidget *, QAbstractItemDelegate::EndEditHint) {

                    int idx = nodeList->currentRow();
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    // Current edited item
                    QListWidgetItem *item = nodeList->item(idx);
                    if (!item) return;

                    // Extract final name (editor may not have triggered itemChanged)
                    QString finalName = item->text().trimmed();
                    graphWidget->nodes[idx].name = finalName;

                    // Rebuild full label with type + privilege
                    const auto &N = graphWidget->nodes[idx];
                    QString fullLabel =
                        N.name + " (" + N.type + ") (" + N.privilege + ")";

                    nodeList->blockSignals(true);
                    item->setText(fullLabel);
                    nodeList->blockSignals(false);

                    graphWidget->update();
                    autoSave();

                });
        // --------------------------------------------------------
        // CONNECT: Type change
        // --------------------------------------------------------
        connect(typeSelector, &QComboBox::currentTextChanged,
                this, [this](const QString &newType) {

                    int idx = nodeList->currentRow();
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    auto &N = graphWidget->nodes[idx];
                    N.type = newType;
                    N.updateColors();

                    QListWidgetItem *item = nodeList->item(idx);

                    nodeList->blockSignals(true);
                    item->setText(
                        N.name + " (" + N.type + ") (" + N.privilege + ")"
                        );
                    nodeList->blockSignals(false);

                    graphWidget->update();
                    autoSave();

                });

        // --------------------------------------------------------
        // CONNECT: Privilege change
        // --------------------------------------------------------
        connect(privSelector, &QComboBox::currentTextChanged,
                this, [this](const QString &newPriv) {

                    int idx = nodeList->currentRow();
                    if (idx < 0 || idx >= graphWidget->nodes.size()) return;

                    auto &N = graphWidget->nodes[idx];
                    N.privilege = newPriv;
                    N.updateColors();

                    QListWidgetItem *item = nodeList->item(idx);

                    nodeList->blockSignals(true);
                    item->setText(
                        N.name + " (" + N.type + ") (" + N.privilege + ")"
                        );
                    nodeList->blockSignals(false);

                    graphWidget->update();
                    autoSave();

                });

        // --------------------------------------------------------
        // CONNECT: Edge parser (FULLY FIXED)
        // --------------------------------------------------------
        connect(edgeEditor, &QTextEdit::textChanged,
                this, [this, edgeEditor]() {

                    // -------------------------
                    // Parse edges
                    // -------------------------
                    QStringList lines =
                        edgeEditor->toPlainText().split("\n", Qt::SkipEmptyParts);

                    int maxNode = -1;
                    for (const QString &line : lines) {
                        auto s = line.split(" ", Qt::SkipEmptyParts);
                        if (s.size() == 2)
                            maxNode = std::max({maxNode, s[0].toInt(), s[1].toInt()});
                    }

                    if (maxNode < 0) {
                        graphWidget->nodes.clear();
                        graphWidget->adj.clear();
                        nodeList->clear();
                        graphWidget->update();
                        return;
                    }

                    // -------------------------
                    // Build NEW adjacency G
                    // -------------------------
                    std::vector<std::vector<int>> G(maxNode + 1);

                    for (const QString &line : lines) {
                        auto s = line.split(" ", Qt::SkipEmptyParts);
                        if (s.size() != 2) continue;
                        int u = s[0].toInt(), v = s[1].toInt();
                        G[u].push_back(v);
                        G[v].push_back(u);
                    }

                    // -------------------------
                    // Resize or preserve nodes
                    // -------------------------
                    std::vector<NodeInfo> old = graphWidget->nodes;
                    auto &nodes = graphWidget->nodes;
                    nodes.resize(maxNode + 1);

                    for (int i = 0; i <= maxNode; i++) {
                        if (i < old.size()) {
                            nodes[i] = old[i];
                        } else {
                            nodes[i].name = "Node " + QString::number(i);
                            nodes[i].type = "Machine";
                            nodes[i].privilege = "Low";
                            nodes[i].updateColors();
                        }
                    }

                    // -------------------------
                    // Compute E from NEW G
                    // -------------------------
                    int V = maxNode + 1;
                    int E = 0;
                    for (const auto &lst : G)
                        E += lst.size();
                    E /= 2;

                    // -------------------------
                    // Call layout solver
                    // -------------------------
                    std::pair<int, std::vector<std::pair<double,double>>> layout = Solver::computeLayout(V, E, G, currentHeuristicIndex());

                    k = layout.first;
                    kLabel->setText("k = " + QString::number(k));   // <---- ADD THIS

                    ///qDebug() << layout;

                    // -------------------------
                    // Apply solver positions
                    // (scaled to screen coordinates)
                    // -------------------------
                    double scale   = 60.0;
                    double offsetX = 80.0;
                    double offsetY = 80.0;

                    qDebug() << "!";

                    for (int i = 0; i < V; ++i) {
                        nodes[i].x = layout.second[i].first  * scale + offsetX;
                        nodes[i].y = layout.second[i].second * scale + offsetY;
                    }

                    // -------------------------
                    // Rebuild node list labels
                    // -------------------------
                    nodeList->clear();
                    for (int i = 0; i <= maxNode; i++) {
                        QListWidgetItem *item = new QListWidgetItem(
                            nodes[i].name +
                            " (" + nodes[i].type + ") (" + nodes[i].privilege + ")"
                            );
                        item->setFlags(item->flags() | Qt::ItemIsEditable);
                        nodeList->addItem(item);
                    }

                    // -------------------------
                    // Update graph
                    // -------------------------
                    graphWidget->setAdjacency(G);

                    crossings = countCrossings();
                    crossLabel->setText("Crossings = " + QString::number(crossings));

                    graphWidget->update();
                    autoSave();
                });

        connect(graphWidget, &GraphWidget::nodeMoved, this, [this]() {
            // Recalculate crossings live during dragging
            crossings = countCrossings();
            crossLabel->setText("Crossings = " + QString::number(crossings));
        });

        connect(graphWidget, &GraphWidget::nodeReleased, this, [this]() {
            crossings = countCrossings();
            crossLabel->setText("Crossings = " + QString::number(crossings));
        });
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
