#include "FramelessHelper.h"
#include <QRect>
#include <QRubberBand>
#include <QMouseEvent>
#include <QHoverEvent>
#include <QApplication>
#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QLayout>
#include <QScreen>
#include <windows.h>
#include <QDebug>
#include <qmath.h>
class WidgetData;
/*****
 * FramelessHelperPrivate
 * 存储界面对应的数据集合，以及是否可移动、可缩放属性
*****/
class FramelessHelperPrivate
{
public:
    QHash<QWidget*, WidgetData*> m_widgetDataHash;
    bool m_bWidgetMovable        : true;
    bool m_bWidgetResizable      : true;
    bool m_bRubberBandOnResize   : true;
    bool m_bRubberBandOnMove     : true;
};
/*****
 * CursorPosCalculator
 * 计算鼠标是否位于左、上、右、下、左上角、左下角、右上角、右下角
*****/
class CursorPosCalculator
{
public:
    explicit CursorPosCalculator();
    void reset();
    void recalculate(const QPoint &globalMousePos, const QRect &frameRect);

public:
    bool m_bOnEdges              : true;
    bool m_bOnLeftEdge           : true;
    bool m_bOnRightEdge          : true;
    bool m_bOnTopEdge            : true;
    bool m_bOnBottomEdge         : true;
    bool m_bOnTopLeftEdge        : true;
    bool m_bOnBottomLeftEdge     : true;
    bool m_bOnTopRightEdge       : true;
    bool m_bOnBottomRightEdge    : true;

    static int m_nBorderWidth;
    static int m_nTitleHeight;
};

int CursorPosCalculator::m_nBorderWidth = 6;
int CursorPosCalculator::m_nTitleHeight = 30;

/***** CursorPosCalculator *****/
CursorPosCalculator::CursorPosCalculator()
{
    reset();
}

void CursorPosCalculator::reset()
{
    m_bOnEdges = false;
    m_bOnLeftEdge = false;
    m_bOnRightEdge = false;
    m_bOnTopEdge = false;
    m_bOnBottomEdge = false;
    m_bOnTopLeftEdge = false;
    m_bOnBottomLeftEdge = false;
    m_bOnTopRightEdge  = false;
    m_bOnBottomRightEdge = false;
}

void CursorPosCalculator::recalculate(const QPoint &gMousePos, const QRect &frameRect)
{
    int globalMouseX = gMousePos.x();
    int globalMouseY = gMousePos.y();

    int frameX = frameRect.x();
    int frameY = frameRect.y();

    int frameWidth = frameRect.width();
    int frameHeight = frameRect.height();

    m_bOnLeftEdge = (globalMouseX >= frameX &&
                  globalMouseX <= frameX + m_nBorderWidth );


    m_bOnRightEdge = (globalMouseX >= frameX + frameWidth - m_nBorderWidth &&
                   globalMouseX <= frameX + frameWidth);

    m_bOnTopEdge = (globalMouseY >= frameY &&
                 globalMouseY <= frameY + m_nBorderWidth );

    m_bOnBottomEdge = (globalMouseY >= frameY + frameHeight - m_nBorderWidth &&
                    globalMouseY <= frameY + frameHeight);

    m_bOnTopLeftEdge = m_bOnTopEdge && m_bOnLeftEdge;
    m_bOnBottomLeftEdge = m_bOnBottomEdge && m_bOnLeftEdge;
    m_bOnTopRightEdge = m_bOnTopEdge && m_bOnRightEdge;
    m_bOnBottomRightEdge = m_bOnBottomEdge && m_bOnRightEdge;

    m_bOnEdges = m_bOnLeftEdge || m_bOnRightEdge || m_bOnTopEdge || m_bOnBottomEdge;
}

/*****
 * WidgetData
 * 更新鼠标样式、移动窗体、缩放窗体
*****/
class WidgetData
{
public:
    explicit WidgetData(FramelessHelperPrivate *d, QWidget *pTopLevelWidget);
    ~WidgetData();

    void setButtonGroup(SAWindowButtonGroup *buttonGroup);

    QWidget* widget();
    // 处理鼠标事件-划过、按下、释放、移动
    bool handleWidgetEvent(QEvent *event);
    // 更新橡皮筋状态
    void updateRubberBandStatus();

    void updateMaxIcon();

private:
    // 更新鼠标样式
    void updateCursorShape(const QPoint &gMousePos);
    // 重置窗体大小
    void resizeWidget(const QPoint &gMousePos);
    // 移动窗体
    void moveWidget(const QPoint &gMousePos);
    // 处理鼠标按下
    bool handleMousePressEvent(QMouseEvent *event);
    // 处理鼠标释放
    bool handleMouseReleaseEvent(QMouseEvent *event);
    // 处理鼠标移动
    bool handleMouseMoveEvent(QMouseEvent *event);
    // 处理鼠标离开
    bool handleLeaveEvent(QEvent *event);
    // 处理鼠标进入
    bool handleHoverMoveEvent(QHoverEvent *event);
    //处理鼠标双击事件
    bool handleDoubleClickedMouseEvent(QMouseEvent *event);

private:
    FramelessHelperPrivate *d;
    QRubberBand *m_pRubberBand;
    QWidget *m_pWidget;
    SAWindowButtonGroup *m_pButtonGroup;
    QPoint m_ptDragPos;
    QPoint m_ptTopLeftPos;
    CursorPosCalculator m_pressedMousePos;
    CursorPosCalculator m_moveMousePos;
    bool m_bLeftButtonPressed;
    bool m_bCursorShapeChanged;
    bool m_bLeftButtonTitlePressed;
    Qt::WindowFlags m_windowFlags;
    bool m_bMaximize;
    QWidget *m_pTranslucentDialog;
};

/***** WidgetData *****/
WidgetData::WidgetData(FramelessHelperPrivate *_d, QWidget *pTopLevelWidget)
{
    d = _d;
    m_pWidget = pTopLevelWidget;
    m_bLeftButtonPressed = false;
    m_bCursorShapeChanged = false;
    m_bLeftButtonTitlePressed = false;
    m_bMaximize = false;
    m_pRubberBand = NULL;
    m_pButtonGroup = NULL;

    m_pTranslucentDialog = new QDialog(0, Qt::FramelessWindowHint | Qt::Tool);
    m_pTranslucentDialog->setWindowOpacity(0.1);

    QLayout *shadowLayout = new QGridLayout();

    QWidget *transparentWidget = new QWidget();
    transparentWidget->setStyleSheet("Background-color:rgb(0, 0, 30)");
    transparentWidget->setAutoFillBackground(true);

    shadowLayout->addWidget(transparentWidget);
    //shadowLayout->setMargin(5);

    m_pTranslucentDialog->setLayout(shadowLayout);
    m_pTranslucentDialog->setAttribute(Qt::WA_TranslucentBackground);

    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius(15);
    effect->setColor(QColor(255,255,255,255));
    effect->setOffset(0);
    transparentWidget->setGraphicsEffect(effect);

    m_windowFlags = m_pWidget->windowFlags();
    m_pWidget->setMouseTracking(true);
    m_pWidget->setAttribute(Qt::WA_Hover, true);

    updateRubberBandStatus();
}

WidgetData::~WidgetData()
{
    m_pWidget->setMouseTracking(false);
    m_pWidget->setWindowFlags(m_windowFlags);
    m_pWidget->setAttribute(Qt::WA_Hover, false);

    delete m_pRubberBand;
    m_pRubberBand = NULL;
}

void WidgetData::setButtonGroup(SAWindowButtonGroup *buttonGroup)
{
    m_pButtonGroup = buttonGroup;
}

QWidget* WidgetData::widget()
{
    return m_pWidget;
}

bool WidgetData::handleWidgetEvent(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
        return handleMousePressEvent(static_cast<QMouseEvent*>(event));
    case QEvent::MouseButtonRelease:
        return handleMouseReleaseEvent(static_cast<QMouseEvent*>(event));
    case QEvent::MouseMove:
        return handleMouseMoveEvent(static_cast<QMouseEvent*>(event));
    case QEvent::Leave:
        return handleLeaveEvent(static_cast<QMouseEvent*>(event));
    case QEvent::HoverMove:
        return handleHoverMoveEvent(static_cast<QHoverEvent*>(event));
    case QEvent::MouseButtonDblClick:
        return handleDoubleClickedMouseEvent(static_cast<QMouseEvent*>(event));
    default:
        break;
    }
    return false;
}

void WidgetData::updateRubberBandStatus()
{
    if (d->m_bRubberBandOnMove || d->m_bRubberBandOnResize)
    {
        if (NULL == m_pRubberBand)
            m_pRubberBand = new QRubberBand(QRubberBand::Rectangle);
    }
    else
    {
        delete m_pRubberBand;
        m_pRubberBand = NULL;
    }
}

void WidgetData::updateCursorShape(const QPoint &gMousePos)
{
    if (m_pWidget->isFullScreen() || m_pWidget->isMaximized())
    {
        if (m_bCursorShapeChanged)
        {
            m_pWidget->unsetCursor();
        }
        return;
    }

    m_moveMousePos.recalculate(gMousePos, m_pWidget->frameGeometry());

    if(m_moveMousePos.m_bOnTopLeftEdge || m_moveMousePos.m_bOnBottomRightEdge)
    {
        m_pWidget->setCursor( Qt::SizeFDiagCursor );
        m_bCursorShapeChanged = true;
    }
    else if(m_moveMousePos.m_bOnTopRightEdge || m_moveMousePos.m_bOnBottomLeftEdge)
    {
        m_pWidget->setCursor( Qt::SizeBDiagCursor );
        m_bCursorShapeChanged = true;
    }
    else if(m_moveMousePos.m_bOnLeftEdge || m_moveMousePos.m_bOnRightEdge)
    {
        m_pWidget->setCursor( Qt::SizeHorCursor );
        m_bCursorShapeChanged = true;
    }
    else if(m_moveMousePos.m_bOnTopEdge || m_moveMousePos.m_bOnBottomEdge)
    {
        m_pWidget->setCursor( Qt::SizeVerCursor );
        m_bCursorShapeChanged = true;
    }
    else
    {
        if (m_bCursorShapeChanged)
        {
            m_pWidget->unsetCursor();
            m_bCursorShapeChanged = false;
        }
    }
}

void WidgetData::resizeWidget(const QPoint &gMousePos)
{
    QRect origRect;

    if (d->m_bRubberBandOnResize)
        origRect = m_pRubberBand->frameGeometry();
    else
        origRect = m_pWidget->frameGeometry();

    int left = origRect.left();
    int top = origRect.top();
    int right = origRect.right();
    int bottom = origRect.bottom();
    origRect.getCoords(&left, &top, &right, &bottom);

    int minWidth = m_pWidget->minimumWidth();
    int minHeight = m_pWidget->minimumHeight();

    if (m_pressedMousePos.m_bOnTopLeftEdge)
    {
        left = gMousePos.x();
        top = gMousePos.y();
    }
    else if (m_pressedMousePos.m_bOnBottomLeftEdge)
    {
        left = gMousePos.x();
        bottom = gMousePos.y();
    }
    else if (m_pressedMousePos.m_bOnTopRightEdge)
    {
        right = gMousePos.x();
        top = gMousePos.y();
    }
    else if (m_pressedMousePos.m_bOnBottomRightEdge)
    {
        right = gMousePos.x();
        bottom = gMousePos.y();
    }
    else if (m_pressedMousePos.m_bOnLeftEdge)
    {
        left = gMousePos.x();
    }
    else if (m_pressedMousePos.m_bOnRightEdge)
    {
        right = gMousePos.x();
    }
    else if (m_pressedMousePos.m_bOnTopEdge)
    {
        top = gMousePos.y();
    }
    else if (m_pressedMousePos.m_bOnBottomEdge)
    {
        bottom = gMousePos.y();
    }

    QRect newRect(QPoint(left, top), QPoint(right, bottom));

    if (newRect.isValid())
    {
        if (minWidth > newRect.width())
        {
            if (left != origRect.left())
                newRect.setLeft(origRect.left());
            else
                newRect.setRight(origRect.right());
        }
        if (minHeight > newRect.height())
        {
            if (top != origRect.top())
                newRect.setTop(origRect.top());
            else
                newRect.setBottom(origRect.bottom());
        }

        if (d->m_bRubberBandOnResize)
        {
            m_pRubberBand->setGeometry(newRect);
        }
        else
        {
            m_pWidget->setGeometry(newRect);
        }
    }
}

void WidgetData::moveWidget(const QPoint& gMousePos)
{
    if (d->m_bRubberBandOnMove)
    {
        m_pRubberBand->move(gMousePos - m_ptDragPos);
    }
    else
    {
        m_pWidget->move(gMousePos - m_ptDragPos);
    }
}

bool WidgetData::handleMousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_bLeftButtonPressed = true;
        m_bLeftButtonTitlePressed = event->pos().y() < m_moveMousePos.m_nTitleHeight;

        QRect frameRect = m_pWidget->frameGeometry();
        m_pressedMousePos.recalculate(event->globalPos(), frameRect);

        QPoint topLeft = frameRect.topLeft();
        m_ptDragPos = event->globalPos() - topLeft;

        if (!m_pWidget->isMaximized())
            m_ptTopLeftPos = topLeft;

        if (m_pressedMousePos.m_bOnEdges)
        {
            if(m_pWidget->isMaximized())
            {
                //窗口在最大化状态时，点击边界不做任何处理
                return false;
            }
            if (d->m_bRubberBandOnResize)
            {
                m_pRubberBand->setGeometry(frameRect);
                m_pRubberBand->show();
                return true;
            }
        }
        else if (d->m_bRubberBandOnMove && m_bLeftButtonTitlePressed)
        {
            if(m_pWidget->isMaximized())
            {
                //窗口在最大化状态时，点击标题栏不做任何处理
                return false;
            }
            m_pRubberBand->setGeometry(frameRect);
            m_pRubberBand->show();
            return true;
        }
    }
    return false;
}

bool WidgetData::handleMouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_bLeftButtonPressed = false;
        m_bLeftButtonTitlePressed = false;
        m_pressedMousePos.reset();

        if (m_bMaximize)
        {
            m_bMaximize = false;

            m_pTranslucentDialog->hide();

//            QRect sourceScreenGeometry = qApp->desktop()->screenGeometry(m_ptTopLeftPos +
//                                                                         QPoint(m_pWidget->frameGeometry().width() / 2, 0));
//            QRect destScreenGeometry = qApp->desktop()->screenGeometry(event->globalPos());
            QScreen *sourceScreen = qApp->screenAt(m_ptTopLeftPos +
                                                   QPoint(m_pWidget->frameGeometry().width() / 2, 0));
            QRect sourceScreenGeometry = sourceScreen ? sourceScreen->geometry() : QRect();

            QScreen *destScreen = qApp->screenAt(event->globalPos());
            QRect destScreenGeometry = destScreen ? destScreen->geometry() : QRect();

            QPoint movePoint = m_ptTopLeftPos;
            if (!destScreenGeometry.contains(movePoint + QPoint(m_pWidget->frameGeometry().width() / 2, 0)))
            {
                movePoint = destScreenGeometry.topLeft() + (movePoint - sourceScreenGeometry.topLeft());
            }

            m_pWidget->move(movePoint);
            m_pWidget->showMaximized();

//            m_pWidget->setWindowState(Qt::WindowMaximized); // bug with multimonitor in second monitor
            updateMaxIcon();
//            m_pWidget->setGeometry(qApp->desktop()->availableGeometry(event->globalPos()));
        }

        if (m_pRubberBand && m_pRubberBand->isVisible())
        {
            m_pRubberBand->hide();
            m_pWidget->setGeometry(m_pRubberBand->geometry());
            return true;
        }
    }
    return false;
}

bool WidgetData::handleMouseMoveEvent(QMouseEvent *event)
{
    if (m_bLeftButtonPressed)
    {
        if (d->m_bWidgetResizable && m_pressedMousePos.m_bOnEdges)
        {
            if(m_pWidget->isMaximized())
            {
                //窗口在最大化状态时，点击边界不做任何处理
                return false;
            }
            resizeWidget(event->globalPos());
            return true;
        }
        else if (d->m_bWidgetMovable && m_bLeftButtonTitlePressed)
        {
            if(m_pWidget->isMaximized())
            {
                //窗口在最大化状态时，点击标题栏不做任何处理
                QRect frameRect = m_pWidget->frameGeometry();
                QPoint localPos = event->globalPos() - frameRect.topLeft();

                double ratio = (double)localPos.x() / (double)frameRect.width();

                if (localPos.y() - m_ptDragPos.y() > 3)
                {
                    m_pWidget->showNormal();
                    updateMaxIcon();
                    qApp->processEvents();

                    m_ptDragPos.setX(m_pWidget->frameGeometry().width() * ratio);
                    m_ptDragPos.setY(localPos.y());
                }
                else
                {
                    return false;
                }

            }

            moveWidget(event->globalPos());

            //int screenNumber = qApp->desktop()->screenNumber(event->globalPos());
            //QRect desktopGeometry = qApp->desktop()->screen(screenNumber)->geometry();
            QScreen *sourceScreen = qApp->screenAt(m_ptTopLeftPos +
                                                   QPoint(m_pWidget->frameGeometry().width() / 2, 0));
            QRect sourceScreenGeometry = sourceScreen ? sourceScreen->geometry() : QRect();

            QScreen *destScreen = qApp->screenAt(event->globalPos());
            QRect destScreenGeometry = destScreen ? destScreen->geometry() : QRect();

            QRect desktopGeometry = QGuiApplication::primaryScreen()->geometry();

            if (event->screenPos().y() - desktopGeometry.top() < 5)
            {
                if (!m_bMaximize) {
                    m_bMaximize = true;

                    QScreen *sourceScreen = qApp->screenAt(m_ptTopLeftPos +
                                                           QPoint(m_pWidget->frameGeometry().width() / 2, 0));
                    QRect source = sourceScreen ? sourceScreen->geometry() : QRect();

                    QScreen *destScreen = qApp->screenAt(event->globalPos());
                    QRect dest = destScreen ? destScreen->geometry() : QRect();


//                    QRect source = m_pWidget->frameGeometry();
//                    QRect dest = qApp->desktop()->availableGeometry(screenNumber);

                    if (source.top() < dest.top())
                        source.setTop(dest.top());

                    if (source.bottom() > dest.bottom())
                        source.setBottom(dest.bottom());

                    if (source.right() > dest.right())
                        source.setRight(dest.right());

                    if (source.left() < dest.left())
                        source.setLeft(dest.left());

                    m_pTranslucentDialog->showNormal();
                    m_pWidget->raise();

                    int dist = qMax(qAbs(source.left() - dest.left()), qAbs(source.right() - dest.right()));
                    double count = floor((double)dist / 60.0);
                    double rate = 1.0 / count;
                    int sleep = 7 - floor((double)dist / 120.0);

                    for (int i = 0; i < count + 1; ++i) {
                        QPoint topLeft = QPoint(source.topLeft().x() - rate * i * (source.topLeft().x() - dest.topLeft().x()),
                                                source.topLeft().y() - rate * i * (source.topLeft().y() - dest.topLeft().y()));

                        QPoint bottomRight = QPoint(source.bottomRight().x() + rate * i * (dest.bottomRight().x() - source.bottomRight().x()),
                                                source.bottomRight().y() + rate * i * (dest.bottomRight().y() - source.bottomRight().y()));

                        QRect rect = QRect(topLeft, bottomRight);
                        m_pTranslucentDialog->setGeometry(rect);
                        m_pTranslucentDialog->repaint();
                        m_pTranslucentDialog->update();
                        Sleep(sleep + i);
                    }
                }
            }
            else
            {
                m_bMaximize = false;
                m_pTranslucentDialog->hide();
            }

            return true;
        }
        return false;
    }
    else if (d->m_bWidgetResizable)
    {
        updateCursorShape(event->globalPos());
    }
    return false;
}

bool WidgetData::handleLeaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    if (!m_bLeftButtonPressed)
    {
        m_pWidget->unsetCursor();
        return true;
    }
    return false;
}

bool WidgetData::handleHoverMoveEvent(QHoverEvent *event)
{
    if (d->m_bWidgetResizable)
    {
        updateCursorShape(m_pWidget->mapToGlobal(event->pos()));
    }
    return false;
}

bool WidgetData::handleDoubleClickedMouseEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        if(m_pWidget)
        {
            bool titlePressed = event->pos().y() < m_moveMousePos.m_nTitleHeight;
            if(titlePressed)
            {
                if(m_pWidget->isMaximized())
                {
                    m_pWidget->showNormal();
                }
                else
                {
                    m_pWidget->showMaximized();
                }
                updateMaxIcon();
                return true;
            }
        }
    }
    return false;
}

void WidgetData::updateMaxIcon()
{
    if (NULL != m_pButtonGroup)
    {
        m_pButtonGroup->updateMaximizeButtonIcon();
    }
}

FramelessHelper::FramelessHelper(QObject *parent)
    : QObject(parent),
      d(new FramelessHelperPrivate())
{
    d->m_bWidgetMovable = true;
    d->m_bWidgetResizable = true;
    d->m_bRubberBandOnResize = false;
    d->m_bRubberBandOnMove = false;
    if(parent)
    {
        QWidget* w = qobject_cast<QWidget*>(parent);
        if(w)
        {
            w->setWindowFlags(w->windowFlags() | Qt::FramelessWindowHint);
            setWidgetMovable(true);  //设置窗体可移动
            setWidgetResizable(true);  //设置窗体可缩放
            setRubberBandOnMove(true);  //设置橡皮筋效果-可移动
            setRubberBandOnResize(true);  //设置橡皮筋效果-可缩放
            activateOn(w);  //激活当前窗体
        }
    }
}

FramelessHelper::~FramelessHelper()
{
    QList<QWidget*> keys = d->m_widgetDataHash.keys();
    int size = keys.size();
    for (int i = 0; i < size; ++i)
    {
        delete d->m_widgetDataHash.take(keys[i]);
    }

    delete d;
}

bool FramelessHelper::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
    case QEvent::MouseMove:
    case QEvent::HoverMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Leave:
    {
        WidgetData *data = d->m_widgetDataHash.value(static_cast<QWidget*>(obj));
        if (data)
        {
            return data->handleWidgetEvent(event);
        }
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void FramelessHelper::activateOn(QWidget *topLevelWidget)
{
    if (!d->m_widgetDataHash.contains(topLevelWidget))
    {
        WidgetData *data = new WidgetData(d, topLevelWidget);
        d->m_widgetDataHash.insert(topLevelWidget, data);

        topLevelWidget->installEventFilter(this);
    }
}

void FramelessHelper::removeFrom(QWidget *topLevelWidget)
{
    WidgetData *data = d->m_widgetDataHash.take(topLevelWidget);
    if (data)
    {
        topLevelWidget->removeEventFilter(this);
        delete data;
    }
}

void FramelessHelper::setRubberBandOnMove(bool movable)
{
    d->m_bRubberBandOnMove = movable;
    QList<WidgetData*> list = d->m_widgetDataHash.values();
    foreach (WidgetData *data, list)
    {
        data->updateRubberBandStatus();
    }
}

void FramelessHelper::setWidgetMovable(bool movable)
{
    d->m_bWidgetMovable = movable;
}

void FramelessHelper::setWidgetResizable(bool resizable)
{
    d->m_bWidgetResizable = resizable;
}

void FramelessHelper::setRubberBandOnResize(bool resizable)
{
    d->m_bRubberBandOnResize = resizable;
    QList<WidgetData*> list = d->m_widgetDataHash.values();
    foreach (WidgetData *data, list)
    {
        data->updateRubberBandStatus();
    }
}

void FramelessHelper::setBorderWidth(uint width)
{
    if (width > 0)
    {
        CursorPosCalculator::m_nBorderWidth = width;
    }
}

void FramelessHelper::setTitleHeight(uint height)
{
    if (height > 0)
    {
        CursorPosCalculator::m_nTitleHeight = height;
    }
}

bool FramelessHelper::widgetMovable()
{
    return d->m_bWidgetMovable;
}

bool FramelessHelper::widgetResizable()
{
    return d->m_bWidgetResizable;
}

bool FramelessHelper::rubberBandOnMove()
{
    return d->m_bRubberBandOnMove;
}

bool FramelessHelper::rubberBandOnResisze()
{
    return d->m_bRubberBandOnResize;
}

uint FramelessHelper::borderWidth()
{
    return CursorPosCalculator::m_nBorderWidth;
}

uint FramelessHelper::titleHeight()
{
    return CursorPosCalculator::m_nTitleHeight;
}

void FramelessHelper::setWindowButtonGroup(SAWindowButtonGroup *buttonGroup)
{
    QList<WidgetData*> list = d->m_widgetDataHash.values();
    foreach (WidgetData *data, list)
    {
        data->setButtonGroup(buttonGroup);
    }
}

void FramelessHelper::updateMaximumIcon()
{
    QList<WidgetData*> list = d->m_widgetDataHash.values();
    foreach (WidgetData *data, list)
    {
        data->updateMaxIcon();
    }
}
