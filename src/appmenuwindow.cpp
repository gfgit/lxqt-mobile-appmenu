#include "appmenuwindow.h"
#include  "menuappmap.h"
#include  "menuappmodel.h"
#include  "menucategoriesmodel.h"

#include <QLineEdit>
#include <QToolButton>
#include <QListView>

#include <QBoxLayout>

#include <QMessageBox>

#include <QProcess>

#include <QKeyEvent>
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

    mAppView = new QListView;
    mAppView->setUniformItemSizes(true);
    mAppView->setSelectionMode(QListView::SingleSelection);

    mCategoryView = new QListView;
    mCategoryView->setUniformItemSizes(true);
    mCategoryView->setSelectionMode(QListView::SingleSelection);

    // Meld category view with whole popup window
    // So remove the frame and set same background as the window
    mCategoryView->setFrameShape(QFrame::NoFrame);
    mCategoryView->viewport()->setBackgroundRole(QPalette::Window);

    mAppMap = new MenuAppMap;

    mAppModel = new MenuAppModel(this);
    mAppModel->setAppMap(mAppMap);
    mAppView->setModel(mAppModel);

    mCategoryModel = new MenuCategoriesModel(this);
    mCategoryModel->setAppMap(mAppMap);
    mCategoryView->setModel(mCategoryModel);

    connect(mAppView, &QListView::clicked, this, &AppMenuWindow::appClicked);
    connect(mCategoryView, &QListView::clicked, this, &AppMenuWindow::categoryClicked);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(mSettingsButton);
    buttonLayout->addWidget(mPowerButton);
    mainLayout->addLayout(buttonLayout);

    mainLayout->addWidget(mSearchEdit);

    // Use 3:2 stretch factors so app view is slightly wider than category view
    QHBoxLayout *viewLayout = new QHBoxLayout;
    viewLayout->addWidget(mAppView, 3);
    viewLayout->addWidget(mCategoryView, 2);
    mainLayout->addLayout(viewLayout);

    setMinimumHeight(500);

    // Ensure all key presses go to search box
    setFocusProxy(mSearchEdit);
    mAppView->setFocusProxy(mSearchEdit);
    mCategoryView->setFocusProxy(mSearchEdit);

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

void AppMenuWindow::categoryClicked(const QModelIndex &idx)
{
    setCurrentCategory(idx.row());
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
    setCurrentCategory(0);
    setSearchQuery(QString());

    //TODO: focus proxy doesn't seem to work on first show...
    mSearchEdit->setFocus();
}

void AppMenuWindow::setCurrentCategory(int cat)
{
    QModelIndex idx = mCategoryModel->index(cat, 0);
    mCategoryView->setCurrentIndex(idx);
    mCategoryView->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
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
