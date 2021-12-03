#include "notelistdelegateeditor.h"
#include <QPainter>
#include <QEvent>
#include <QDebug>
#include <QApplication>
#include <QFontDatabase>
#include <QtMath>
#include <QPainterPath>
#include <QScrollBar>
#include "notelistmodel.h"
#include "noteeditorlogic.h"
#include "tagpool.h"
#include "nodepath.h"
#include "notelistdelegate.h"
#include "taglistview.h"
#include "taglistmodel.h"
#include "taglistdelegate.h"

NoteListDelegateEditor::NoteListDelegateEditor(const NoteListDelegate *delegate,
                                               QListView *view,
                                               const QStyleOptionViewItem &option,
                                               const QModelIndex &index,
                                               TagPool *tagPool,
                                               QWidget *parent)
    : QWidget(parent),
      m_delegate(delegate),
      m_option(option),
      m_id(index.data(NoteListModel::NoteID).toInt()),
      m_view(view),
      m_tagPool{tagPool},
      #ifdef __APPLE__
      m_displayFont(QFont(QStringLiteral("SF Pro Text")).exactMatch() ? QStringLiteral("SF Pro Text") : QStringLiteral("Roboto")),
      #elif _WIN32
      m_displayFont(QFont(QStringLiteral("Segoe UI")).exactMatch() ? QStringLiteral("Segoe UI") : QStringLiteral("Roboto")),
      #else
      m_displayFont(QStringLiteral("Roboto")),
      #endif

      #ifdef __APPLE__
      m_titleFont(m_displayFont, 13, 65),
      m_titleSelectedFont(m_displayFont, 13),
      m_dateFont(m_displayFont, 13),
      #else
      m_titleFont(m_displayFont, 10, 60),
      m_titleSelectedFont(m_displayFont, 10),
      m_dateFont(m_displayFont, 10),
      #endif
      m_titleColor(26, 26, 26),
      m_dateColor(26, 26, 26),
      m_contentColor(142, 146, 150),
      m_ActiveColor(218, 233, 239),
      m_notActiveColor(175, 212, 228),
      m_hoverColor(207, 207, 207),
      m_applicationInactiveColor(207, 207, 207),
      m_separatorColor(221, 221, 221),
      m_defaultColor(255, 255, 255),
      m_rowHeight(106),
      m_rowRightOffset(0),
      m_isActive(false),
      m_theme(Theme::Light)
{
    setContentsMargins(0, 0, 0, 0);
    m_folderIcon = QImage(":/images/folder.png");
    m_tagListView = new TagListView(this);
    m_tagListModel = new TagListModel(this);
    m_tagListDelegate = new TagListDelegate(this);
    m_tagListView->setFrameStyle(QFrame::NoFrame);
    m_tagListView->setModel(m_tagListModel);
    m_tagListView->setItemDelegate(m_tagListDelegate);
    m_tagListModel->setTagPool(tagPool);
    m_tagListModel->setModelData(index.data(NoteListModel::NoteTagsList).value<QSet<int>>());
    if (m_delegate->isInAllNotes()) {
        m_tagListView->setGeometry(10, 90, rect().width(), m_tagListView->height());
    } else {
        m_tagListView->setGeometry(10, 70, rect().width(), m_tagListView->height());
    }
    connect(m_tagListView->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this] {
        auto m_index = dynamic_cast<NoteListModel*>(m_view->model())->getNoteIndex(m_id);
        dynamic_cast<NoteListModel*>(m_view->model())
                ->setData(m_index, getScrollBarPos(), NoteListModel::NoteTagListScrollbarPos);
    });
    QTimer::singleShot(0, this, [this] {
        auto index = dynamic_cast<NoteListModel*>(m_view->model())->getNoteIndex(m_id);
        setScrollBarPos(index.data(NoteListModel::NoteTagListScrollbarPos).toInt());
    });
}

void NoteListDelegateEditor::paintBackground(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(m_view->selectionModel()->isSelected(index)){
        if(qApp->applicationState() == Qt::ApplicationActive){
            if(m_isActive){
                painter->fillRect(rect(), QBrush(m_ActiveColor));
                m_tagListView->setBackground(m_ActiveColor);
            }else{
                painter->fillRect(rect(), QBrush(m_notActiveColor));
                m_tagListView->setBackground(m_notActiveColor);
            }
        }else if(qApp->applicationState() == Qt::ApplicationInactive){
            painter->fillRect(rect(), QBrush(m_applicationInactiveColor));
            m_tagListView->setBackground(m_applicationInactiveColor);
        }
    }else if(underMouse()){
        painter->fillRect(rect(), QBrush(m_hoverColor));
        m_tagListView->setBackground(m_hoverColor);
    }else if((index.row() != m_delegate->currentSelectedIndex().row() - 1)
             && (index.row() !=  m_delegate->currentSelectedIndex().row() - 1)){
        painter->fillRect(rect(), QBrush(m_defaultColor));
        m_tagListView->setBackground(m_defaultColor);
        paintSeparator(painter, option, index);
    } else {
        painter->fillRect(rect(), QBrush(m_defaultColor));
        m_tagListView->setBackground(m_defaultColor);
    }
}

void NoteListDelegateEditor::paintLabels(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);
    const int leftOffsetX = 10;
    const int topOffsetY = 5;   // space on top of title
    const int spaceY = 1;       // space between title and date

    QString title{index.data(NoteListModel::NoteFullTitle).toString()};
    QFont titleFont = m_view->selectionModel()->isSelected(index) ? m_titleSelectedFont : m_titleFont;
    QFontMetrics fmTitle(titleFont);
    QRect fmRectTitle = fmTitle.boundingRect(title);

    QString date = parseDateTime(index.data(NoteListModel::NoteLastModificationDateTime).toDateTime());
    QFontMetrics fmDate(m_dateFont);
    QRect fmRectDate = fmDate.boundingRect(date);

    QString parentName{index.data(NoteListModel::NoteParentName).toString()};
    QFontMetrics fmParentName(titleFont);
    QRect fmRectParentName = fmParentName.boundingRect(parentName);

    QString content{index.data(NoteListModel::NoteContent).toString()};
    content = NoteEditorLogic::getSecondLine(content);
    QFontMetrics fmContent(titleFont);
    QRect fmRectContent = fmContent.boundingRect(content);

    double rowPosX = rect().x();
    double rowPosY = rect().y();
    double rowWidth = rect().width();

    double titleRectPosX = rowPosX + leftOffsetX;
    double titleRectPosY = rowPosY;
    double titleRectWidth = rowWidth - 2.0 * leftOffsetX;
    double titleRectHeight = fmRectTitle.height() + topOffsetY;

    double dateRectPosX = rowPosX + leftOffsetX;
    double dateRectPosY = rowPosY + fmRectTitle.height() + topOffsetY;
    double dateRectWidth = rowWidth - 2.0 * leftOffsetX;
    double dateRectHeight = fmRectDate.height() + spaceY;

    double contentRectPosX = rowPosX + leftOffsetX;
    double contentRectPosY = rowPosY + fmRectTitle.height() + fmRectDate.height() + topOffsetY + 3;
    double contentRectWidth = rowWidth - 2.0 * leftOffsetX;
    double contentRectHeight = fmRectContent.height() + spaceY;

    double folderNameRectPosX = 0;
    double folderNameRectPosY = 0;
    double folderNameRectWidth = 0;
    double folderNameRectHeight = 0;

    if (m_delegate->isInAllNotes()) {
        folderNameRectPosX = rowPosX + leftOffsetX + 20;
        folderNameRectPosY = rowPosY + fmRectContent.height() + fmRectTitle.height() + fmRectDate.height() + topOffsetY + 3 + 5;
        folderNameRectWidth = rowWidth - 2.0 * leftOffsetX;
        folderNameRectHeight = fmRectParentName.height() + spaceY;
    }
    auto drawStr = [painter](double posX, double posY, double width, double height, QColor color, QFont font, QString str){
        QRectF rect(posX, posY, width, height);
        painter->setPen(color);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignBottom, str);
    };

    // draw title & date
    title = fmTitle.elidedText(title, Qt::ElideRight, int(titleRectWidth));
    content = fmContent.elidedText(content, Qt::ElideRight, int(titleRectWidth));
    drawStr(titleRectPosX, titleRectPosY, titleRectWidth, titleRectHeight, m_titleColor, titleFont, title);
    drawStr(dateRectPosX, dateRectPosY, dateRectWidth, dateRectHeight, m_dateColor, m_dateFont, date);
    if (m_delegate->isInAllNotes()) {
        painter->drawImage(QRect(rowPosX + leftOffsetX,
                                 folderNameRectPosY,
                                 16, 16), m_folderIcon);
        drawStr(folderNameRectPosX, folderNameRectPosY, folderNameRectWidth, folderNameRectHeight, m_contentColor, titleFont, parentName);
    }
    drawStr(contentRectPosX, contentRectPosY, contentRectWidth, contentRectHeight, m_contentColor, titleFont, content);
}

void NoteListDelegateEditor::paintSeparator(QPainter*painter, const QStyleOptionViewItem& option, const QModelIndex&index) const
{
    Q_UNUSED(index)
    Q_UNUSED(option);

    painter->setPen(QPen(m_separatorColor));
    const int leftOffsetX = 11;
    int posX1 = rect().x() + leftOffsetX;
    int posX2 = rect().x() + rect().width() - leftOffsetX - 1;
    int posY = rect().y() + rect().height() - 1;

    painter->drawLine(QPoint(posX1, posY),
                      QPoint(posX2, posY));
}

QString NoteListDelegateEditor::parseDateTime(const QDateTime &dateTime) const
{
    QLocale usLocale(QLocale("en_US"));

    auto currDateTime = QDateTime::currentDateTime();

    if(dateTime.date() == currDateTime.date()){
        return usLocale.toString(dateTime.time(),"h:mm A");
    }else if(dateTime.daysTo(currDateTime) == 1){
        return "Yesterday";
    }else if(dateTime.daysTo(currDateTime) >= 2 &&
             dateTime.daysTo(currDateTime) <= 7){
        return usLocale.toString(dateTime.date(), "dddd");
    }

    return dateTime.date().toString("M/d/yy");
}

void NoteListDelegateEditor::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QStyleOptionViewItem opt = m_option;
    opt.rect.setWidth(m_option.rect.width() - m_rowRightOffset);
    auto m_index = dynamic_cast<NoteListModel*>(m_view->model())->getNoteIndex(m_id);
    paintBackground(&painter, opt, m_index);
    paintLabels(&painter, m_option, m_index);
    QWidget::paintEvent(event);
}

void NoteListDelegateEditor::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_delegate->isInAllNotes()) {
        m_tagListView->setGeometry(10, 90, rect().width() - 15, m_tagListView->height());
    } else {
        m_tagListView->setGeometry(10, 70, rect().width() - 15, m_tagListView->height());
    }
    recalculateSize();
}

void NoteListDelegateEditor::setActive(bool isActive)
{
    m_isActive = isActive;
}

void NoteListDelegateEditor::recalculateSize()
{
    QSize result;
    int rowHeight = 70;
    result.setHeight(rowHeight);
    if (m_delegate->isInAllNotes()) {
        result.setHeight(result.height() + 20);
    }
    result.setHeight(result.height() + m_tagListView->height() + 2);
    result.setWidth(rect().width());
    auto m_index = dynamic_cast<NoteListModel*>(m_view->model())->getNoteIndex(m_id);
    emit updateSizeHint(m_id, result, m_index);
}

void NoteListDelegateEditor::setScrollBarPos(int pos)
{
    m_tagListView->verticalScrollBar()->setValue(pos);
}

int NoteListDelegateEditor::getScrollBarPos()
{
    return m_tagListView->verticalScrollBar()->value();
}

void NoteListDelegateEditor::setRowRightOffset(int rowRightOffset)
{
    m_rowRightOffset = rowRightOffset;
}

void NoteListDelegateEditor::setTheme(Theme theme)
{
    m_theme = theme;
    switch(m_theme){
    case Theme::Light:
    {
        m_titleColor = QColor(26, 26, 26);
        m_dateColor = QColor(26, 26, 26);
        m_defaultColor = QColor(255, 255, 255);
        m_ActiveColor = QColor(218, 233, 239);
        m_notActiveColor = QColor(175, 212, 228);
        m_hoverColor = QColor(207, 207, 207);
        break;
    }
    case Theme::Dark:
    {
        m_titleColor = QColor(204, 204, 204);
        m_dateColor = QColor(204, 204, 204);
        m_defaultColor = QColor(16, 16, 16);
        m_ActiveColor = QColor(0, 59, 148);
        m_notActiveColor = QColor(0, 59, 148);
        m_hoverColor = QColor(15, 45, 90);
        break;
    }
    case Theme::Sepia:
    {
        m_titleColor = QColor(26, 26, 26);
        m_dateColor = QColor(26, 26, 26);
        m_defaultColor = QColor(251, 240, 217);
        m_ActiveColor = QColor(218, 233, 239);
        m_notActiveColor = QColor(175, 212, 228);
        m_hoverColor = QColor(207, 207, 207);
        break;
    }
    }
}
