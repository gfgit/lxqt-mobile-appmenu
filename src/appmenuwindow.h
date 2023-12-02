#ifndef LXQTFANCYMENUWINDOW_H
#define LXQTFANCYMENUWINDOW_H

#include <QWidget>

class QLineEdit;
class QToolButton;
class QListView;
class QModelIndex;

class XdgMenu;

class MenuAppMap;
class MenuAppModel;
class MenuCategoriesModel;

class AppMenuWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AppMenuWindow(QWidget *parent = nullptr);
    ~AppMenuWindow();

    bool rebuildMenu(const XdgMenu &menu);

    virtual QSize sizeHint() const override;

    void resetUi();

    void setCurrentCategory(int cat);

    bool eventFilter(QObject *watched, QEvent *e) override;

public slots:
    void setSearchQuery(const QString& text);

private slots:
    void categoryClicked(const QModelIndex& idx);
    void appClicked(const QModelIndex& idx);

    void runPowerDialog();
    void runSystemConfigDialog();

private:
    void runCommandHelper(const QString& cmd);

private:
    QToolButton *mSettingsButton;
    QToolButton *mPowerButton;
    QLineEdit *mSearchEdit;
    QListView *mAppView;
    QListView *mCategoryView;

    MenuAppMap *mAppMap;
    MenuAppModel *mAppModel;
    MenuCategoriesModel *mCategoryModel;
};

#endif // LXQTFANCYMENUWINDOW_H
