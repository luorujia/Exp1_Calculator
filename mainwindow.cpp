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
    // 统一的输入分发入口：鼠标点击按钮与键盘按键都从这里进入，保证两者行为完全一致
    if (token.size() == 1 && token.at(0).isDigit()) {
        inputDigit(token);
    } else if (token == kTokenPoint) {
        inputDecimalPoint();
    } else if (token == QStringLiteral("+") || token == QStringLiteral("-")
            || token == QStringLiteral("*") || token == QStringLiteral("/")) {
        applyOperator(token);
    } else if (token == kTokenEqual) {
        calculateResult();
    } else if (token == kTokenBackspace) {
        backspace();
    } else if (token == kTokenClear) {
        clearAll();
    }
}

void MainWindow::inputDigit(const QString &digit)
{
    if (m_hasError)
        clearAll();                 // 错误态下输入数字 => 自动重新开始一轮计算
    if (m_waitingForOperand) {      // 刚按下运算符或刚得出结果 => 开启一个全新的操作数
        m_currentInput.clear();
        m_waitingForOperand = false;
    }
    if (m_currentInput == QStringLiteral("0"))
        m_currentInput.clear();     // 去掉前导 0，避免 "0" -> "05"
    if (m_currentInput.size() >= kMaxInputLength)
        return;                     // 限制单个操作数的长度，防止显示溢出
    m_currentInput += digit;
    updateDisplay();
}

void MainWindow::inputDecimalPoint()
{
    if (m_hasError)
        clearAll();
    if (m_waitingForOperand) {
        m_currentInput.clear();
        m_waitingForOperand = false;
    }
    if (m_currentInput.isEmpty())
        m_currentInput = QStringLiteral("0");   // 直接按 "." 时自动补出 "0."
    m_currentInput += QLatin1Char('.');         // FIXME(C4)：未判断是否已存在小数点
    updateDisplay();
}

void MainWindow::backspace()
{
    if (m_hasError) {
        clearAll();
        return;
    }
    if (m_waitingForOperand)
        return;                     // 等待新操作数 / 刚出结果时退格无效，避免破坏已得结果
    m_currentInput.chop(1);
    if (m_currentInput.isEmpty() || m_currentInput == QStringLiteral("-"))
        m_currentInput = QStringLiteral("0");   // 删空后回落到 "0"，而不是空白
    updateDisplay();
}

void MainWindow::clearAll()
{
    m_currentInput.clear();
    m_accumulated = 0.0;
    m_pendingOperator.clear();
    m_waitingForOperand = false;
    m_hasError = false;
    m_errorMessage.clear();
    highlightOperatorButton(QString());
    updateDisplay();
}

void MainWindow::updateDisplay()
{
    if (m_hasError) {
        ui->displayEdit->setText(m_errorMessage);
        ui->expressionLabel->clear();
        return;
    }
    ui->displayEdit->setText(m_currentInput.isEmpty() ? QStringLiteral("0") : m_currentInput);

    // 表达式行：显示"左操作数 运算符"，例如 "12.5 +"
    if (m_pendingOperator.isEmpty())
        ui->expressionLabel->clear();
    else
        ui->expressionLabel->setText(formatNumber(m_accumulated) + QLatin1Char(' ')
                                     + displayOperator(m_pendingOperator));
}

QString MainWindow::formatNumber(double value)
{
    // 'g' 格式 + 12 位有效数字：既保留足够精度，又避免 0.1+0.2=0.30000000000000004 这类浮点尾巴
    return QString::number(value, 'g', 12);
}

QString MainWindow::displayOperator(const QString &op)
{
    // 内部统一使用 ASCII 运算符（便于键盘映射），仅在界面显示时换成常规数学符号
    if (op == QStringLiteral("*")) return QStringLiteral("×");
    if (op == QStringLiteral("/")) return QStringLiteral("÷");
    return op;
}

void MainWindow::applyOperator(const QString &op)       { Q_UNUSED(op); }
void MainWindow::calculateResult()                      {}
bool MainWindow::computePending()                       { return false; }
double MainWindow::currentValue() const                 { return 0.0; }
void MainWindow::enterError(const QString &message)     { Q_UNUSED(message); }
void MainWindow::highlightOperatorButton(const QString &op) { Q_UNUSED(op); }
void MainWindow::flashButton(const QString &objectName) { Q_UNUSED(objectName); }

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    QMainWindow::keyPressEvent(event);
}
