#ifndef LXQTFANCYMENUWINDOW_H
#define LXQTFANCYMENUWINDOW_H

#include <QWidget>

class QLineEdit;
class QToolButton;
class QComboBox;
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
    explicit AppMenuWindow(bool stayOnTopFrameless, QWidget *parent = nullptr);
    ~AppMenuWindow();

    bool rebuildMenu(const XdgMenu &menu);

    virtual QSize sizeHint() const override;

    void resetUi();

    void setCurrentCategory(int cat);

    bool eventFilter(QObject *watched, QEvent *e) override;

    void loadSettings();
    void loadMenuFile(const QString& menuFile, const QStringList &environments);

    bool event(QEvent *e) override;

public slots:
    void setSearchQuery(const QString& text);

private slots:
    void categoryClicked(int idx);
    void appClicked(const QModelIndex& idx);

    void runPowerDialog();
    void runSystemConfigDialog();

protected:
    void keyPressEvent(QKeyEvent *e);

private:
    void runCommandHelper(const QString& cmd);

private:
    QToolButton *mCloseButton;
    QToolButton *mSettingsButton;
    QToolButton *mPowerButton;
    QLineEdit *mSearchEdit;
    QListView *mAppView;
    QComboBox *mCategoryCombo;

    MenuAppMap *mAppMap;
    MenuAppModel *mAppModel;
    MenuCategoriesModel *mCategoryModel;
};

#endif // LXQTFANCYMENUWINDOW_H
