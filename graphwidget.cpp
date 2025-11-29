#include "graphwidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QtMath>

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);

    setStyleSheet("background-color: white;");
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // Animation timer ~60 FPS
    connect(&animTimer, &QTimer::timeout, this, [this]() {
        if (!animating) { animTimer.stop(); return; }
        qint64 elapsed = animClock.elapsed();
        double t = animDurationMs > 0 ? std::min(1.0, elapsed / static_cast<double>(animDurationMs)) : 1.0;
        QEasingCurve curve(QEasingCurve::InOutCubic);
        double e = curve.valueForProgress(t);

        // Ensure vectors are sized
        if (animStartPos.size() != nodes.size()) animStartPos.resize(nodes.size(), QPointF());
        if (animTargetPos.size() != nodes.size()) animTargetPos.resize(nodes.size(), QPointF());

        for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
            if (draggingNode && i == draggedNodeIndex) continue; // don't fight user drag
            const QPointF &s = animStartPos[i];
            const QPointF &d = animTargetPos[i];
            double nx = s.x() + (d.x() - s.x()) * e;
            double ny = s.y() + (d.y() - s.y()) * e;
            nodes[i].x = nx;
            nodes[i].y = ny;
        }
        update();
        emit nodeMoved(); // allow live crossing updates

        if (t >= 1.0 - 1e-6) {
            animating = false;
            animTimer.stop();
        }
    });

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

void GraphWidget::animateTo(const std::vector<QPointF> &targets, int durationMs)
{
    if (targets.size() != nodes.size()) {
        // Resize or ignore extra
        // We'll create a targets array same size as nodes
    }

    animStartPos.resize(nodes.size());
    animTargetPos.resize(nodes.size());

    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        animStartPos[i] = QPointF(nodes[i].x, nodes[i].y);
        if (i < static_cast<int>(targets.size()))
            animTargetPos[i] = targets[i];
        else
            animTargetPos[i] = animStartPos[i];
    }

    animDurationMs = durationMs;
    animClock.restart();
    animating = true;
    if (!animTimer.isActive()) animTimer.start(16);
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), Qt::white);

    p.translate(offsetX, offsetY);
    p.scale(zoom, zoom);

    // ------------------------
    // Draw edges
    // ------------------------
    p.setPen(QPen(Qt::darkGray, 2));

    for (int i = 0; i < adj.size(); ++i)
        for (int neigh : adj[i]) {
            if (i < neigh) {
                const auto &a = nodes[i];
                const auto &b = nodes[neigh];
                p.drawLine(QPointF(a.x, a.y), QPointF(b.x, b.y));
            }
        }

    // ------------------------
    // Draw nodes (with halo)
    // ------------------------
    for (int i = 0; i < nodes.size(); i++) {
        const auto &pt = nodes[i];

        if (i == selectedNode) {

            // Outer blue glow
            p.setPen(QPen(QColor(0, 120, 255, 180), 6));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(pt.x, pt.y), 12, 12);

            // inner white ring
            p.setPen(QPen(Qt::white, 3));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(QPointF(pt.x, pt.y), 8, 8);

            // actual node
            p.setPen(pt.colorBorder);
            p.setBrush(pt.color);
            p.drawEllipse(QPointF(pt.x, pt.y), 6, 6);
        }
        else {
            p.setPen(pt.colorBorder);
            p.setBrush(pt.color);
            p.drawEllipse(QPointF(pt.x, pt.y), 5, 5);
        }
        p.setPen(Qt::black);
        p.setBrush(Qt::NoBrush);
        p.setFont(QFont("Arial", 10));

        QString label = (pt.name.isEmpty()
                            ? QString("Node ") + QString::number(i)
                            : pt.name);

        p.drawText(QPointF(pt.x + 8, pt.y - 8), label);


    }
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
    QPointF cursorPos = event->position();

    QPointF graphBefore = (cursorPos - QPointF(offsetX, offsetY)) / zoom;

    double degrees = event->angleDelta().y() / 8.0;
    double steps   = degrees / 15.0;
    double factor  = std::pow(1.05, steps);

    zoom *= factor;
    zoom = std::clamp(zoom, 0.1, 10.0);

    QPointF graphAfter = graphBefore * zoom + QPointF(offsetX, offsetY);

    offsetX += cursorPos.x() - graphAfter.x();
    offsetY += cursorPos.y() - graphAfter.y();

    update();
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
    dragging = true;
    lastMousePos = event->pos();

    QPointF g = screenToGraph(event->pos(), zoom, offsetX, offsetY);

    selectedNode = -1;

    const double hitRadius = 10.0 / zoom;  // scale click radius with zoom

    for (int i = 0; i < nodes.size(); i++) {
        double dx = g.x() - nodes[i].x;
        double dy = g.y() - nodes[i].y;
        double dist = std::sqrt(dx*dx + dy*dy);

        if (dist <= hitRadius) {
            selectedNode = i;
            draggedNodeIndex = i;
            draggingNode = true;

            dragOffsetGraph = QPointF(nodes[i].x - g.x(), nodes[i].y - g.y());

            emit nodeClicked(i);
            break;
        }
    }

    update();
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{

    QPointF g = screenToGraph(event->pos(), zoom, offsetX, offsetY);

    if (draggingNode && draggedNodeIndex >= 0) {
        // Move selected node
        nodes[draggedNodeIndex].x = g.x() + dragOffsetGraph.x();
        nodes[draggedNodeIndex].y = g.y() + dragOffsetGraph.y();

        update();

        emit nodeMoved();   // <---- ADD THIS

        return;
    }

    // Otherwise: pan
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
    draggingNode = false;
    draggedNodeIndex = -1;

    emit nodeReleased();
}

QPointF GraphWidget::screenToGraph(const QPointF &p, double zoom, double offsetX, double offsetY)
{
    return QPointF(
        (p.x() - offsetX) / zoom,
        (p.y() - offsetY) / zoom
        );
}
