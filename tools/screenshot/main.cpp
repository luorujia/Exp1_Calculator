/**
 * @file main.cpp
 * @brief 实验报告截图驱动工具
 *
 * 两种驱动方式都走真实事件链路，不做任何"绕过界面直接改数据"的操作：
 *   - 鼠标：QPushButton::click()  -> clicked() 信号 -> 与真实点击完全相同的槽
 *   - 键盘：QApplication::sendEvent() 投递 QKeyEvent -> MainWindow::keyPressEvent()
 * 抓取：QWidget::grab() 把窗口渲染成 QPixmap 后保存为 PNG。
 *
 * 用法：ScreenshotTool.exe <输出目录> <场景名>
 */

#include "../../mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QKeyEvent>
#include <QPushButton>
#include <QStringList>
#include <QTimer>
#include <QWidget>

namespace {

QString g_outDir;

/** 让事件循环真正跑起来（QTimer 才能触发），用于等待界面刷新 / 按钮闪烁结束 */
void waitMs(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

/** 抓取窗口并保存为 PNG */
void grab(QWidget *w, const QString &name)
{
    waitMs(320);                       // 等待布局、样式和按钮闪烁动画结束
    const QString path = g_outDir + QLatin1Char('/') + name + QStringLiteral(".png");
    if (w->grab().save(path))
        qDebug().noquote() << "[OK]   " << path;
    else
        qWarning().noquote() << "[FAIL] " << path;
}

/** 字符 -> 对应按钮的 objectName */
QString buttonNameOf(QChar c)
{
    if (c.isDigit())            return QStringLiteral("btnDigit") + c;
    if (c == QLatin1Char('.'))  return QStringLiteral("btnPoint");
    if (c == QLatin1Char('+'))  return QStringLiteral("btnAdd");
    if (c == QLatin1Char('-'))  return QStringLiteral("btnSubtract");
    if (c == QLatin1Char('*'))  return QStringLiteral("btnMultiply");
    if (c == QLatin1Char('/'))  return QStringLiteral("btnDivide");
    if (c == QLatin1Char('='))  return QStringLiteral("btnEqual");
    if (c == QLatin1Char('\b')) return QStringLiteral("btnBackspace");
    if (c == QLatin1Char('c') || c == QLatin1Char('C')) return QStringLiteral("btnClear");
    return QString();
}

/** 鼠标：按字符串顺序点击按钮，例如 "12.5+3.75=" */
void clickSeq(QWidget *w, const QString &seq)
{
    for (const QChar c : seq) {
        const QString name = buttonNameOf(c);
        if (name.isEmpty()) {
            qWarning() << "无对应按钮：" << c;
            continue;
        }
        QPushButton *btn = w->findChild<QPushButton *>(name);
        if (!btn) {
            qWarning() << "找不到按钮：" << name;
            continue;
        }
        btn->setDown(true);
        waitMs(60);
        btn->click();                  // 与真实鼠标点击触发同一条 clicked() 链路
        btn->setDown(false);
        waitMs(90);
    }
}

Qt::Key keyOf(QChar c)
{
    if (c.isDigit())            return static_cast<Qt::Key>(Qt::Key_0 + (c.unicode() - QLatin1Char('0').unicode()));
    if (c == QLatin1Char('.'))  return Qt::Key_Period;
    if (c == QLatin1Char('+'))  return Qt::Key_Plus;
    if (c == QLatin1Char('-'))  return Qt::Key_Minus;
    if (c == QLatin1Char('*'))  return Qt::Key_Asterisk;
    if (c == QLatin1Char('/'))  return Qt::Key_Slash;
    if (c == QLatin1Char('='))  return Qt::Key_Equal;
    if (c == QLatin1Char('\r') || c == QLatin1Char('\n')) return Qt::Key_Return;
    if (c == QLatin1Char('\b')) return Qt::Key_Backspace;
    if (c == QLatin1Char('\x1B')) return Qt::Key_Escape;
    return Qt::Key_unknown;
}

/** 键盘：按字符串顺序投递 QKeyEvent，例如 "98/4\r" */
void keySeq(QWidget *w, const QString &seq)
{
    for (const QChar c : seq) {
        QKeyEvent ev(QEvent::KeyPress, keyOf(c), Qt::NoModifier, QString(c));
        QApplication::sendEvent(w, &ev);   // 走真实的事件分发，进入 keyPressEvent
        waitMs(90);
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QStringList args = app.arguments();
    if (args.size() < 3) {
        qWarning() << "用法：ScreenshotTool.exe <输出目录> <场景名>";
        return 1;
    }
    g_outDir = args.at(1);
    const QString scene = args.at(2);

    MainWindow w;
    w.show();
    w.raise();
    waitMs(400);

    // ---- 初版 / 优化后界面 ----
    if (scene == QStringLiteral("initial") || scene == QStringLiteral("final_ui")) {
        grab(&w, scene);
        return 0;
    }

    // ---- 鼠标完成带小数运算：12.5 + 3.75 = 16.25 ----
    if (scene == QStringLiteral("mouse_decimal")) {
        clickSeq(&w, QStringLiteral("12.5+3.75="));
        grab(&w, scene);
        return 0;
    }

    // ---- 键盘完成计算：98 / 4 + Enter = 24.5 ----
    if (scene == QStringLiteral("keyboard_calc")) {
        keySeq(&w, QStringLiteral("98/4\r"));
        grab(&w, scene);
        return 0;
    }

    // ---- 鼠标与键盘混合输入：鼠标 7 + 键盘 "+8" + 鼠标 = ----
    if (scene == QStringLiteral("mixed_input")) {
        clickSeq(&w, QStringLiteral("7"));
        keySeq(&w, QStringLiteral("+8"));
        clickSeq(&w, QStringLiteral("="));
        grab(&w, scene);
        return 0;
    }

    // ---- 退格 ----
    if (scene == QStringLiteral("backspace")) {
        clickSeq(&w, QStringLiteral("1234\b\b"));
        grab(&w, scene);
        return 0;
    }

    // ---- 清除 ----
    if (scene == QStringLiteral("clear")) {
        clickSeq(&w, QStringLiteral("99c"));
        grab(&w, scene);
        return 0;
    }

    // ---- 连续运算：2 + 3 × 4 = 20（本计算器按输入顺序逐次结算）----
    if (scene == QStringLiteral("continuous")) {
        clickSeq(&w, QStringLiteral("2+3*4="));
        grab(&w, scene);
        return 0;
    }

    // ---- 边界：同一操作数重复输入小数点 ----
    if (scene == QStringLiteral("decimal_point")) {
        clickSeq(&w, QStringLiteral("12..3"));
        grab(&w, scene);
        return 0;
    }

    // ---- 边界：除数为 0 ----
    if (scene == QStringLiteral("divzero")) {
        clickSeq(&w, QStringLiteral("5/0="));
        grab(&w, scene);
        return 0;
    }

    // ---- 边界：算出结果后继续输入数字 ----
    if (scene == QStringLiteral("after_equal")) {
        clickSeq(&w, QStringLiteral("2+3=7"));
        grab(&w, scene);
        return 0;
    }

    // ---- 边界：直接按运算符开头（0 + 5 =）----
    if (scene == QStringLiteral("leading_operator")) {
        clickSeq(&w, QStringLiteral("+5="));
        grab(&w, scene);
        return 0;
    }

    qWarning() << "未知场景：" << scene;
    return 2;
}
