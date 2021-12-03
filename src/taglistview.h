#ifndef TAGLISTVIEW_H
#define TAGLISTVIEW_H

#include <QListView>

class TagListView : public QListView
{
    Q_OBJECT
public:
    explicit TagListView(QWidget *parent = nullptr);
    void setBackground(const QColor& color);
signals:
    // QAbstractItemView interface
public slots:
    virtual void reset() override;

    // QWidget interface
protected:
    virtual void resizeEvent(QResizeEvent *event) override;
};

#endif // TAGLISTVIEW_H
