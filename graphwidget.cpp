#include "graphwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);

    setStyleSheet("background-color: white;");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);   // force white background
    setPalette(pal);

    qDebug() << "GraphWidget constructed:" << this;
    qDebug() << "GraphWidget initial size:" << size();
}

void GraphWidget::setNodes(const std::vector<NodeInfo> &nd) {
    nodes = nd;
    update();
}

void GraphWidget::setAdjacency(const std::vector<std::vector<int>> &g) {
    adj = g;
    update();
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);


    p.fillRect(rect(), Qt::white);  // paint background white

    p.translate(offsetX, offsetY);
    p.scale(zoom, zoom);

    p.setPen(QPen(Qt::darkGray, 2));

    for(int i = 0; i < adj.size(); ++i)
        for(int neigh : adj[i]){

            if(i < neigh){
                const auto &a = nodes[i];
                const auto &b = nodes[neigh];

                p.drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
            }
        }

    int index = 0;
    for (const auto &pt : nodes) {

        if (index == selectedNode) {

            // --- Outer blue glow ---
            p.setPen(QPen(QColor(0, 120, 255, 180), 6));   // semi-transparent blue
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(pt.x, pt.y), 12, 12);

            // --- Inner white separation ring ---
            p.setPen(QPen(Qt::white, 3));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(pt.x, pt.y), 8, 8);

            // --- Original node circle ---
            p.setPen(pt.colorBorder);
            p.setBrush(pt.color);
            p.drawEllipse(QPointF(pt.x, pt.y), 6, 6);
        }
        else {
            // normal node
            p.setPen(pt.colorBorder);
            p.setBrush(pt.color);
            p.drawEllipse(QPointF(pt.x, pt.y), 5, 5);
        }

        ++index;
    }
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
    // Position of cursor in widget coordinates
    QPointF cursorPos = event->position();

    // Convert cursor to graph coordinates BEFORE zoom
    QPointF graphBefore = (cursorPos - QPointF(offsetX, offsetY)) / zoom;

    // Compute zoom factor
    double degrees = event->angleDelta().y() / 8.0;    // wheel degrees
    double steps   = degrees / 15.0;                   // wheel “steps”
    double factor  = std::pow(1.05, steps);

    zoom *= factor;
    if (zoom < 0.1) zoom = 0.1;
    if (zoom > 10.0) zoom = 10.0;

    // Convert graph point to screen after zoom
    QPointF graphAfter = graphBefore * zoom + QPointF(offsetX, offsetY);

    // Adjust offset so cursor stays on same graph point
    offsetX += cursorPos.x() - graphAfter.x();
    offsetY += cursorPos.y() - graphAfter.y();

    update();
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
    dragging = true;
    lastMousePos = event->pos();

    // Convert screen → graph coords
    QPointF g = screenToGraph(event->pos(), zoom, offsetX, offsetY);

    // find nearest node within radius
    selectedNode = -1;
    const double hitRadius = 10.0; // pixels in graph coords (will be scaled)

    for (int i = 0; i < nodes.size(); i++) {
        double dx = g.x() - nodes[i].x;
        double dy = g.y() - nodes[i].y;
        double dist = std::sqrt(dx*dx + dy*dy);

        if (dist <= hitRadius) {
            selectedNode = i;
            break;
        }
    }

    update();
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (dragging) {
        QPoint delta = event->pos() - lastMousePos;
        lastMousePos = event->pos();

        offsetX += delta.x();
        offsetY += delta.y();

        update();
    }
}
void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
    dragging = false;
}

QPointF GraphWidget::screenToGraph(const QPointF &p, double zoom, double offsetX, double offsetY)
{
    return QPointF(
        (p.x() - offsetX) / zoom,
        (p.y() - offsetY) / zoom
        );
}
