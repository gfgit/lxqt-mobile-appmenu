#include "appmenuwindow.h"
#include  "model/menuappmap.h"
#include  "model/menuappmodel.h"
#include  "model/menucategoriesmodel.h"

#include "utils/settings.h"

#include <QLineEdit>
#include <QToolButton>
#include <QComboBox>
#include <QListView>

#include <QBoxLayout>

#include <QMessageBox>

#include <QProcess>

#include <QKeyEvent>
#include <QGestureEvent>
#include <QCoreApplication>

#include <XdgMenu>

#include <QProxyStyle>

class SingleActivateStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption * option = nullptr, const QWidget * widget = nullptr, QStyleHintReturn * returnData = nullptr) const override
    {
        if(hint == QStyle::SH_ItemView_ActivateItemOnSingleClick)
            return 1;
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

AppMenuWindow::AppMenuWindow(bool stayOnTopFrameless, QWidget *parent)
    : QWidget{parent}
{
    mSearchEdit = new QLineEdit;
    mSearchEdit->setPlaceholderText(tr("Search..."));
    mSearchEdit->setClearButtonEnabled(true);
    connect(mSearchEdit, &QLineEdit::textEdited, this, &AppMenuWindow::setSearchQuery);
    connect(mSearchEdit, &QLineEdit::returnPressed, this, &AppMenuWindow::activateCurrentApp);


    mCloseButton = new QToolButton;
    mCloseButton->setIcon(QIcon::fromTheme(QStringLiteral("window-close-symbolic")));
    mCloseButton->setText(tr("Close"));
    mCloseButton->setToolTip(mCloseButton->text());
    connect(mCloseButton, &QToolButton::clicked, this, &AppMenuWindow::close);

    mSettingsButton = new QToolButton;
    mSettingsButton->setIcon(QIcon::fromTheme(QStringLiteral("preferences-desktop"))); //TODO: preferences-system?
    mSettingsButton->setText(tr("Settings"));
    mSettingsButton->setToolTip(mSettingsButton->text());
    connect(mSettingsButton, &QToolButton::clicked, this, &AppMenuWindow::runSystemConfigDialog);

    mPowerButton = new QToolButton;
    mPowerButton->setIcon(QIcon::fromTheme(QStringLiteral("system-shutdown")));
    mPowerButton->setText(tr("Shutdown"));
    mPowerButton->setToolTip(mPowerButton->text());
    connect(mPowerButton, &QToolButton::clicked, this, &AppMenuWindow::runPowerDialog);

    mAppMap = new MenuAppMap;

    mAppModel = new MenuAppModel(this);
    mAppModel->setAppMap(mAppMap);

    mCategoryModel = new MenuCategoriesModel(this);
    mCategoryModel->setAppMap(mAppMap);

    mAppView = new QListView;
    mAppView->setUniformItemSizes(true);
    mAppView->setSelectionMode(QListView::SingleSelection);
    mAppView->setViewMode(QListView::IconMode);
    mAppView->setMovement(QListView::Static);
    mAppView->setFlow(QListView::LeftToRight);
    mAppView->setWrapping(true);
    mAppView->setResizeMode(QListView::Adjust);
    mAppView->setModel(mAppModel);

    SingleActivateStyle *s = new SingleActivateStyle(mAppView->style());
    s->setParent(mAppView);
    mAppView->setStyle(s);

    // Use TapAndHold to show item tooltips
    mAppView->viewport()->grabGesture(Qt::TapAndHoldGesture);
    mAppView->viewport()->installEventFilter(this);

    mCategoryCombo = new QComboBox;
    mCategoryCombo->setModel(mCategoryModel);

    connect(mAppView, &QListView::activated, this, &AppMenuWindow::activateAppAtIndex);
    connect(mCategoryCombo, qOverload<int>(&QComboBox::activated), this, &AppMenuWindow::categoryClicked);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(mSettingsButton);
    buttonLayout->addWidget(mPowerButton);
    buttonLayout->addWidget(mCloseButton);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addWidget(mSearchEdit);

    mainLayout->addWidget(mCategoryCombo);
    mainLayout->addWidget(mAppView);

    setMinimumHeight(500);

    // Ensure all key presses go to search box
    setFocusProxy(mSearchEdit);
    mAppView->setFocusProxy(mSearchEdit);
    mCategoryCombo->setFocusProxy(mSearchEdit);

    // Filter navigation keys
    mSearchEdit->installEventFilter(this);

    if(stayOnTopFrameless)
    {
        setWindowFlag(Qt::FramelessWindowHint);
        setWindowFlag(Qt::WindowStaysOnTopHint);
    }
}

AppMenuWindow::~AppMenuWindow()
{
    mAppModel->setAppMap(nullptr);
    mCategoryModel->setAppMap(nullptr);
    delete mAppMap;
    mAppMap = nullptr;
}

bool AppMenuWindow::rebuildMenu(const XdgMenu &menu)
{
    mAppModel->reloadAppMap(false);
    mCategoryModel->reloadAppMap(false);
    mAppMap->rebuildModel(menu);
    mAppModel->reloadAppMap(true);
    mCategoryModel->reloadAppMap(true);

    setCurrentCategory(0);

    return true;
}

QSize AppMenuWindow::sizeHint() const
{
    //return QSize(450, 550);
    return QWidget::sizeHint();
}

void AppMenuWindow::categoryClicked(int idx)
{
    setCurrentCategory(idx);
}

void AppMenuWindow::activateAppAtIndex(const QModelIndex &idx)
{
    if(!idx.isValid())
        return;
    auto *app = mAppModel->getAppAt(idx.row());
    app->desktopFileCache.startDetached();
    close();
}

void AppMenuWindow::activateCurrentApp()
{
    activateAppAtIndex(mAppView->currentIndex());
}

void AppMenuWindow::runPowerDialog()
{
    runCommandHelper(QLatin1String("lxqt-leave"));
}

void AppMenuWindow::runSystemConfigDialog()
{
    runCommandHelper(QLatin1String("lxqt-config"));
}

void AppMenuWindow::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Escape)
        close();
}

void AppMenuWindow::resetUi()
{
    setCurrentCategory(1); //Default to "All Applications"
    setSearchQuery(QString());

    //TODO: focus proxy doesn't seem to work on first show...
    mSearchEdit->setFocus();
}

void AppMenuWindow::setCurrentCategory(int cat)
{
    mCategoryCombo->setCurrentIndex(cat);
    mAppModel->setCurrentCategory(cat);
    mAppModel->endSearch();
}

bool AppMenuWindow::eventFilter(QObject *watched, QEvent *e)
{
    if(watched == mSearchEdit && e->type() == QEvent::KeyPress)
    {
        QKeyEvent *ev = static_cast<QKeyEvent *>(e);
        if(ev->key() == Qt::Key_Up || ev->key() == Qt::Key_Down)
        {
            // Use Up/Down arrows to navigate app view
            QCoreApplication::sendEvent(mAppView, ev);
            return true;
        }
    }
    else if(watched == mAppView->viewport() && e->type() == QEvent::Gesture)
    {
        QGestureEvent *ev = static_cast<QGestureEvent *>(e);
        if(QGesture *gesture = ev->gesture(Qt::TapAndHoldGesture))
        {
            ev->setAccepted(Qt::TapAndHoldGesture);
            if(gesture->state() == Qt::GestureFinished)
            {
                //Send tooltip event on tap and hold
                QPoint globalPos = gesture->hotSpot().toPoint();
                QPoint vpPos = mAppView->viewport()->mapFromGlobal(vpPos);
                QHelpEvent tooltipEvent(QEvent::ToolTip, vpPos, globalPos);
                QCoreApplication::sendEvent(mAppView->viewport(), &tooltipEvent);
            }
            return true;
        }
    }

    return QWidget::eventFilter(watched, e);
}

void AppMenuWindow::loadSettings()
{
    Settings settings;

    // Menu
    loadMenuFile(settings.getMenuFile(), settings.getDesktopEnv().split(QLatin1Char(';')));

    // Buttons
    int buttonIconSz = settings.getButtonIconSize();
    if(buttonIconSz == -1)
    {
        // Default to double of style size
        buttonIconSz = mCloseButton->style()->pixelMetric(QStyle::PM_ButtonIconSize, nullptr, mCloseButton);
        buttonIconSz *= 2;
        settings.setViewIconSize(buttonIconSz);
    }

    QSize buttonIconSize(buttonIconSz, buttonIconSz);
    mCloseButton->setIconSize(buttonIconSize);
    mSettingsButton->setIconSize(buttonIconSize);
    mPowerButton->setIconSize(buttonIconSize);

    // View
    mAppView->setSpacing(settings.getViewSpacing());

    int viewIconSz = settings.getViewIconSize();
    if(viewIconSz == -1)
    {
        // Default to double of style size
        viewIconSz = mAppView->style()->pixelMetric(QStyle::PM_LargeIconSize, nullptr, mAppView);
        viewIconSz *= 2;
        settings.setViewIconSize(viewIconSz);
    }

    QSize viewIconSize(viewIconSz, viewIconSz);
    mAppView->setIconSize(viewIconSize);
}

void AppMenuWindow::loadMenuFile(const QString &menuFile, const QStringList& environments)
{
    XdgMenu xdgMenu;
    xdgMenu.setEnvironments(environments);
    if(!xdgMenu.read(menuFile))
    {
        QMessageBox::warning(this, tr("Invalid Menu"),
                             tr("Menu file: %1\n"
                                "Error: %2")
                                 .arg(menuFile, xdgMenu.errorString()));
    }

    rebuildMenu(xdgMenu);
}

void AppMenuWindow::setSearchQuery(const QString &text)
{
    QSignalBlocker blk(mSearchEdit);
    mSearchEdit->setText(text);

    if(text.isEmpty())
    {
        mAppModel->endSearch();
        return;
    }

    setCurrentCategory(1);

    auto apps = mAppMap->getMatchingApps(text);
    mAppModel->showSearchResults(apps);
}

void AppMenuWindow::runCommandHelper(const QString &cmd)
{
    if(QProcess::startDetached(cmd, QStringList()))
    {
        close();
    }
    else
    {
        QMessageBox::warning(this, tr("No Executable"),
                             tr("Cannot find <b>%1</b> executable.").arg(cmd));
    }
}
