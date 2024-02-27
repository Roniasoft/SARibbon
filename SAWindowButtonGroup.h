﻿#ifndef SAWINDOWBUTTONGROUP_H
#define SAWINDOWBUTTONGROUP_H
#include <QWidget>
class SAWindowButtonGroupPrivate;
///
/// \brief 窗口的最大最小化按钮
///
class SAWindowButtonGroup : public QWidget
{
    Q_OBJECT
public:
    SAWindowButtonGroup(QWidget* parent);
    ~SAWindowButtonGroup();
    void setupMinimizeButton(bool on);
    void setupMaximizeButton(bool on);
    void setupCloseButton(bool on);
    void updateWindowFlag();
    void updateMaximizeButtonIcon();
protected:
    QSize sizeHint();
    virtual bool eventFilter(QObject *watched, QEvent *e);
    virtual void parentResize();
protected slots:
    Q_SLOT void closeWindow();
    Q_SLOT void minimizeWindow();
    Q_SLOT void maximizeWindow();
signals:
    void iconUpdated();
private:
    friend class SAWindowButtonGroupPrivate;
    SAWindowButtonGroupPrivate* m_d;
};

#endif // SAWINDOWBUTTONGROUP_H
