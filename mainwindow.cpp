#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>

#include <cmath>
#include <utility>

namespace {
// 非数字类的输入 token（数字用 "0"~"9" 表示）
const QString kTokenPoint     = QStringLiteral(".");
const QString kTokenEqual     = QStringLiteral("=");
const QString kTokenBackspace = QStringLiteral("BS");
const QString kTokenClear     = QStringLiteral("C");
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_accumulated(0.0)
    , m_waitingForOperand(false)
    , m_hasError(false)
{
    ui->setupUi(this);

    // 主窗口自身接收键盘焦点；所有按钮的 focusPolicy 已在 .ui 中设为 NoFocus，
    // 因此按键不会被子控件"吃掉"，键盘事件一定能到达 keyPressEvent。
    setFocusPolicy(Qt::StrongFocus);

    connectButtons();
    clearAll();
    setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::connectButtons()
{
    // 数字键 0~9：用循环 + lambda 统一归口，避免 10 段重复代码
    for (int i = 0; i <= 9; ++i) {
        QPushButton *btn = findChild<QPushButton *>(QStringLiteral("btnDigit%1").arg(i));
        Q_ASSERT_X(btn, "connectButtons", "找不到数字按钮");
        connect(btn, &QPushButton::clicked, this, [this, i]() { handleInput(QString::number(i)); });
    }

    // 其余功能键：objectName -> token 的映射表，同样循环关联
    const QList<std::pair<QString, QString>> functionKeys {
        { QStringLiteral("btnPoint"),     kTokenPoint     },
        { QStringLiteral("btnAdd"),       QStringLiteral("+") },
        { QStringLiteral("btnSubtract"),  QStringLiteral("-") },
        { QStringLiteral("btnMultiply"),  QStringLiteral("*") },
        { QStringLiteral("btnDivide"),    QStringLiteral("/") },
        { QStringLiteral("btnEqual"),     kTokenEqual     },
        { QStringLiteral("btnBackspace"), kTokenBackspace },
        { QStringLiteral("btnClear"),     kTokenClear     },
    };
    for (const auto &entry : functionKeys) {
        QPushButton *btn = findChild<QPushButton *>(entry.first);
        Q_ASSERT_X(btn, "connectButtons", "找不到功能按钮");
        connect(btn, &QPushButton::clicked, this, [this, token = entry.second]() { handleInput(token); });
    }
}

void MainWindow::handleInput(const QString &token)
{
    // TODO：后续提交中逐个实现
    qDebug() << "[handleInput]" << token;
}

void MainWindow::inputDigit(const QString &digit)       { Q_UNUSED(digit); }
void MainWindow::inputDecimalPoint()                    {}
void MainWindow::applyOperator(const QString &op)       { Q_UNUSED(op); }
void MainWindow::calculateResult()                      {}
void MainWindow::backspace()                            {}
void MainWindow::clearAll()                             {}
bool MainWindow::computePending()                       { return false; }
double MainWindow::currentValue() const                 { return 0.0; }
void MainWindow::enterError(const QString &message)     { Q_UNUSED(message); }
void MainWindow::updateDisplay()                        {}
void MainWindow::highlightOperatorButton(const QString &op) { Q_UNUSED(op); }
void MainWindow::flashButton(const QString &objectName) { Q_UNUSED(objectName); }

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    QMainWindow::keyPressEvent(event);
}

QString MainWindow::formatNumber(double value)          { return QString::number(value); }
QString MainWindow::displayOperator(const QString &op)  { return op; }
