#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>

class QKeyEvent;
class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 带键盘事件的计算器主窗口
 *
 * 设计要点：
 * 1. 所有输入（鼠标点击按钮 / 键盘按键）都归口到 handleInput(token) 这一个入口，
 *    两种输入方式共享同一套处理逻辑、同一份状态，因此行为严格一致。
 * 2. 用 5 个成员变量完整描述计算器的状态（见下方"计算状态机"注释）。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    /** 键盘事件入口：把物理按键翻译成 token 后交给 handleInput，与鼠标按钮走同一条路。 */
    void keyPressEvent(QKeyEvent *event) override;

private:
    // ---------- 统一的输入处理入口 ----------
    // token 取值："0"~"9"、"."、"+" "-" "*" "/"、"="、"BS"（退格）、"C"（清除）
    void handleInput(const QString &token);

    void inputDigit(const QString &digit);   // 输入一个数字字符
    void inputDecimalPoint();                // 输入小数点
    void applyOperator(const QString &op);   // 按下四则运算符
    void calculateResult();                  // 按下等号
    void backspace();                        // 退格
    void clearAll();                         // 清除（归零）

    // ---------- 内部辅助 ----------
    bool   computePending();                     // 执行 m_pendingOperator；出错返回 false
    double currentValue() const;                 // 把 m_currentInput 解析成 double
    void   enterError(const QString &message);   // 进入错误态
    void   updateDisplay();                      // 刷新表达式行与结果显示
    void   highlightOperatorButton(const QString &op); // 高亮当前待执行的运算符
    void   flashButton(const QString &objectName);     // 键盘按下时给对应按钮视觉反馈
    void   connectButtons();                     // 批量关联按钮信号与处理逻辑

    static QString formatNumber(double value);          // 统一数字格式化，避免浮点尾巴
    static QString displayOperator(const QString &op);  // 内部运算符 -> 界面显示符号

    Ui::MainWindow *ui;

    // ---------- 计算状态机 ----------
    QString m_currentInput;      // 当前正在输入的操作数（字符串形态，可保留 "12." 这类中间态）
    double  m_accumulated;       // 左操作数；连续运算时保存累积结果
    QString m_pendingOperator;   // 待执行运算符；为空表示当前没有待执行的运算
    bool    m_waitingForOperand; // true：下一个数字要开启新操作数（刚按运算符或刚算出结果）
    bool    m_hasError;          // true：错误态（除零 / 溢出），只能由 C 或新数字解除
    QString m_errorMessage;      // 错误提示文本

    static constexpr int kMaxInputLength = 15;  // 单个操作数允许输入的最大字符数
};

#endif // MAINWINDOW_H
