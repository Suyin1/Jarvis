//
// Created on 2025/12/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef OPENQTCEF_TestDialog_H
#define OPENQTCEF_TestDialog_H

#include "testcef/TCSimpleHandler.h"
#include <QMainWindow>
#include <QObject>

#include "newwindow.h"

class TestDialog : public QMainWindow {
    Q_OBJECT

public:
    TestDialog(CefRefPtr<TCSimpleHandler> handler, QWidget *parent = nullptr);
    ~TestDialog();

    void initControls();
    void activeChildWindow();
    
public slots:
    void onChildWindowClosed(NewWindow* window);
    
signals:
    void windowClosed(TestDialog* dialog);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;
    
private:
    QWidget *parent_ = nullptr;
    CefRefPtr<TCSimpleHandler> handler_ = nullptr;
    std::vector<NewWindow*> testWindows_;
    bool isClosing_ = false;
};

#endif //OPENQTCEF_TestDialog_H