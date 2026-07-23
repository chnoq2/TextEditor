#ifndef EXTENDEDTEXTEDIT_H
#define EXTENDEDTEXTEDIT_H

#include <QTextEdit>
#include <QResizeEvent>

class ExtendedTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit ExtendedTextEdit(QWidget *parent = nullptr) : QTextEdit(parent) {
        setFrameShape(QFrame::NoFrame);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QTextEdit::resizeEvent(event);
        document()->setPageSize(QSizeF(714, -1));
    }
};

#endif // EXTENDEDTEXTEDIT_H
