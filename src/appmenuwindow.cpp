#include "appmenuwindow.h"
#include  "menuappmap.h"
#include  "menuappmodel.h"
#include  "menucategoriesmodel.h"

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

#include <QSettings>
#include <XdgMenu>

AppMenuWindow::AppMenuWindow(QWidget *parent)
    : QWidget{parent}
{
    mSearchEdit = new QLineEdit;
    mSearchEdit->setPlaceholderText(tr("Search..."));
    mSearchEdit->setClearButtonEnabled(true);
    connect(mSearchEdit, &QLineEdit::textEdited, this, &AppMenuWindow::setSearchQuery);

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
    int sz = 2 * style()->pixelMetric(QStyle::PM_LargeIconSize);
    mAppView->setIconSize(QSize(sz, sz));
    mAppView->setFlow(QListView::LeftToRight);
    mAppView->setWrapping(true);
    mAppView->setResizeMode(QListView::Adjust);
    mAppView->setModel(mAppModel);

    // Use TapAndHold to show item tooltips
    mAppView->viewport()->grabGesture(Qt::TapAndHoldGesture);
    mAppView->viewport()->installEventFilter(this);

    mCategoryCombo = new QComboBox;
    mCategoryCombo->setModel(mCategoryModel);

    connect(mAppView, &QListView::clicked, this, &AppMenuWindow::appClicked);
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

    setWindowFlag(Qt::FramelessWindowHint);
    setWindowFlag(Qt::WindowStaysOnTopHint);
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

void AppMenuWindow::appClicked(const QModelIndex &idx)
{
    if(!idx.isValid())
        return;
    auto *app = mAppModel->getAppAt(idx.row());
    app->desktopFileCache.startDetached();
    close();
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
        if(ev->key() == Qt::Key_Return)
        {
            //TODO: move to appropriate place
            // Use Return Key to launch current application
            appClicked(mAppView->currentIndex());
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
    const QLatin1String defMenuFile("/etc/xdg/menus/lxqt-applications.menu");
    const QLatin1String defEnv("X-LXQT;LXQt");

    QSettings settings(QSettings::UserScope);
    settings.beginGroup(QLatin1String("Menu"));
    QString menuFile = settings.value(QLatin1String("menuFile"), defMenuFile).toString();
    QStringList environments = settings.value("desktopEnvironments", defEnv).toString().split(QLatin1Char(';'));
    settings.endGroup();

    loadMenuFile(menuFile, environments);
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
