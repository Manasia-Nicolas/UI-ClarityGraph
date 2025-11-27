#pragma once
#include <QWidget>
#include <vector>

struct NodeInfo {
    double x, y;
    QString type;
    QString privilege = "Low";
    QColor color;
    QColor colorBorder;

    NodeInfo() {}

    NodeInfo(double x, double y, const QString &type, const QString &priv)
        : x(x), y(y), type(type), privilege(priv)
    {
        color = colorForType(type);
        colorBorder = colorBorderForPriv(priv);
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
        if(priv == "Low") return QColor("#1f77b4");
        if(priv == "Medium") return QColor("#ff7f0e");
        if(priv == "High") return QColor("#d62728");
        return Qt::black;
    }
};



class GraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit GraphWidget(QWidget *parent = nullptr);

    void setNodes(const std::vector<NodeInfo> &pts);
    void setAdjacency(const std::vector<std::vector<int>> &g);

protected:
    void paintEvent(QPaintEvent *event) override;

    // NEW for zoom & pan:
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
    QPointF screenToGraph(const QPointF &p, double zoom, double offsetX, double offsetY);
};
