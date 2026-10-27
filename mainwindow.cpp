#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QHash>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
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

    // 核心判重：一个操作数里只允许出现一个小数点。
    // 只要已存在 '.'，后续的小数点输入一律忽略，从根上杜绝 "12..3" 这类非法数字。
    if (m_currentInput.contains(QLatin1Char('.')))
        return;

    m_currentInput += QLatin1Char('.');
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

void MainWindow::applyOperator(const QString &op)
{
    if (m_hasError)
        return;                         // 错误态下运算符无效，必须先按 C 解除

    if (!m_pendingOperator.isEmpty() && !m_waitingForOperand) {
        // 连续运算：如 "2 + 3 +"，先把 2+3 结算为 5，再把 "+" 挂为新的待执行运算符
        if (!computePending())
            return;
    } else {
        m_accumulated = currentValue(); // 记录第一个操作数
    }

    m_pendingOperator = op;
    m_waitingForOperand = true;         // 下一个数字开启第二个操作数
    highlightOperatorButton(op);
    updateDisplay();
}

void MainWindow::calculateResult()
{
    if (m_hasError || m_pendingOperator.isEmpty())
        return;                         // 没有待执行的运算时，等号无效

    if (m_waitingForOperand) {
        // "8 + =" 这类输入：第二个操作数缺省取左操作数（与 Windows 计算器行为一致）
        m_currentInput = formatNumber(m_accumulated);
    }

    if (!computePending())
        return;

    m_pendingOperator.clear();
    highlightOperatorButton(QString());
    // FIXME(C7)：算出结果后未置 m_waitingForOperand，导致继续输入数字会追加到结果后面
    updateDisplay();
}

bool MainWindow::computePending()
{
    const double rhs = currentValue();
    double result = 0.0;

    if      (m_pendingOperator == QStringLiteral("+")) result = m_accumulated + rhs;
    else if (m_pendingOperator == QStringLiteral("-")) result = m_accumulated - rhs;
    else if (m_pendingOperator == QStringLiteral("*")) result = m_accumulated * rhs;
    else if (m_pendingOperator == QStringLiteral("/")) result = m_accumulated / rhs;  // FIXME(C5)：未判断除数为 0

    m_accumulated = result;
    m_currentInput = formatNumber(result);
    return true;
}

double MainWindow::currentValue() const
{
    // m_currentInput 是字符串，"12." 这类输入中间态也能被 toDouble 正确解析
    return m_currentInput.toDouble();
}

void MainWindow::enterError(const QString &message)
{
    m_hasError = true;
    m_errorMessage = message;
    m_pendingOperator.clear();          // 出错后不再保留待执行运算
    highlightOperatorButton(QString());
    updateDisplay();
}

void MainWindow::highlightOperatorButton(const QString &op)
{
    // 用动态属性 + unpolish/polish 切换"当前待执行运算符"的高亮，
    // 避免为 4 个运算符按钮各写一段重复的样式代码。
    static const QHash<QString, QString> buttonOfOperator {
        { QStringLiteral("+"), QStringLiteral("btnAdd")      },
        { QStringLiteral("-"), QStringLiteral("btnSubtract") },
        { QStringLiteral("*"), QStringLiteral("btnMultiply") },
        { QStringLiteral("/"), QStringLiteral("btnDivide")   },
    };
    for (auto it = buttonOfOperator.constBegin(); it != buttonOfOperator.constEnd(); ++it) {
        QPushButton *btn = findChild<QPushButton *>(it.value());
        if (!btn)
            continue;
        btn->setProperty("active", it.key() == op);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void MainWindow::flashButton(const QString &objectName) { Q_UNUSED(objectName); }

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    QMainWindow::keyPressEvent(event);
}
