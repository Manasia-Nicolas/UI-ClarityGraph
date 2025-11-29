#pragma once
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QEasingCurve>
#include <vector>

struct NodeInfo {
    double x = 0, y = 0;
    QString name = "Node";
    QString type = "Machine";
    QString privilege = "Low";

    QColor color;
    QColor colorBorder;

    NodeInfo() {}

    void updateColors() {
        color = colorForType(type);
        colorBorder = colorBorderForPriv(privilege);
    }

    static QColor colorForType(const QString &type)
    {
        if (type == "Machine")  return QColor("#1f77b4");
        if (type == "Server")   return QColor("#ff7f0e");
        if (type == "Database") return QColor("#2ca02c");
        if (type == "Firewall") return QColor("#d62728");
        if (type == "Router")   return QColor("#9467bd");
        return Qt::black;
    }

    static QColor colorBorderForPriv(const QString &priv)
    {
        if (priv == "Low")    return QColor("#1f77b4");
        if (priv == "Medium") return QColor("#ff7f0e");
        if (priv == "High")   return QColor("#d62728");
        return Qt::black;
    }
};

class GraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    void setNodes(const std::vector<NodeInfo> &pts);
    void setAdjacency(const std::vector<std::vector<int>> &g);

    // Start smooth animation to target positions (graph coordinates)
    void animateTo(const std::vector<QPointF> &targets, int durationMs = 400);

protected:
    void paintEvent(QPaintEvent *event) override;

    // Zoom & pan
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

public:
    std::vector<NodeInfo> nodes;
    std::vector<std::vector<int>> adj;

    double zoom = 1.0;
    double offsetX = 0;
    double offsetY = 0;

    int selectedNode = -1;

    bool dragging = false;
    QPoint lastMousePos;

    bool draggingNode = false;
    int draggedNodeIndex = -1;
    QPointF dragOffsetGraph;

    // Animation state
    QTimer animTimer;
    QElapsedTimer animClock;
    std::vector<QPointF> animStartPos;
    std::vector<QPointF> animTargetPos;
    int animDurationMs = 0;
    bool animating = false;

    QPointF screenToGraph(const QPointF &p, double zoom, double offsetX, double offsetY);

signals:
    void nodeClicked(int index);   // used to sync with node list
    void nodeMoved();
    void nodeReleased();
};
