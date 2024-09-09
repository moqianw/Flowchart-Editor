/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.7.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFontComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *newAction;
    QAction *openAction;
    QAction *saveAction;
    QAction *copyAction;
    QAction *pasteAction;
    QAction *cutAction;
    QAction *revertAction;
    QAction *allAction;
    QAction *enlargeAction;
    QAction *lessenAction;
    QAction *leftAction;
    QAction *rightAction;
    QAction *actionsvg;
    QAction *actionpng;
    QAction *action;
    QAction *action_2;
    QAction *closeaction;
    QAction *action_4;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout_6;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QToolButton *toolButton_3;
    QToolButton *toolButton_2;
    QToolButton *toolButton;
    QFontComboBox *fontComboBox;
    QComboBox *comboBox;
    QToolButton *toolButton_4;
    QToolButton *toolButton_5;
    QToolButton *toolButton_6;
    QComboBox *comboBox_2;
    QComboBox *comboBox_4;
    QComboBox *comboBox_3;
    QToolButton *toolButton_17;
    QToolButton *toolButton_18;
    QToolButton *toolButton_19;
    QToolButton *toolButton_20;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_5;
    QToolBox *toolBox;
    QWidget *page;
    QHBoxLayout *horizontalLayout_2;
    QFormLayout *formLayout;
    QGridLayout *gridLayout;
    QMenuBar *menubar;
    QMenu *menu;
    QMenu *menu_4;
    QMenu *menu_2;
    QMenu *menu_3;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(910, 600);
        MainWindow->setMinimumSize(QSize(0, 40));
        newAction = new QAction(MainWindow);
        newAction->setObjectName("newAction");
        QIcon icon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        newAction->setIcon(icon);
        newAction->setMenuRole(QAction::NoRole);
        openAction = new QAction(MainWindow);
        openAction->setObjectName("openAction");
        QIcon icon1(QIcon::fromTheme(QIcon::ThemeIcon::DocumentOpen));
        openAction->setIcon(icon1);
        openAction->setMenuRole(QAction::NoRole);
        saveAction = new QAction(MainWindow);
        saveAction->setObjectName("saveAction");
        QIcon icon2(QIcon::fromTheme(QIcon::ThemeIcon::DocumentSave));
        saveAction->setIcon(icon2);
        saveAction->setMenuRole(QAction::NoRole);
        copyAction = new QAction(MainWindow);
        copyAction->setObjectName("copyAction");
        QIcon icon3(QIcon::fromTheme(QIcon::ThemeIcon::EditCopy));
        copyAction->setIcon(icon3);
        copyAction->setMenuRole(QAction::NoRole);
        pasteAction = new QAction(MainWindow);
        pasteAction->setObjectName("pasteAction");
        QIcon icon4(QIcon::fromTheme(QIcon::ThemeIcon::EditPaste));
        pasteAction->setIcon(icon4);
        pasteAction->setMenuRole(QAction::NoRole);
        cutAction = new QAction(MainWindow);
        cutAction->setObjectName("cutAction");
        QIcon icon5(QIcon::fromTheme(QIcon::ThemeIcon::EditCut));
        cutAction->setIcon(icon5);
        cutAction->setMenuRole(QAction::NoRole);
        revertAction = new QAction(MainWindow);
        revertAction->setObjectName("revertAction");
        QIcon icon6(QIcon::fromTheme(QIcon::ThemeIcon::DocumentRevert));
        revertAction->setIcon(icon6);
        revertAction->setMenuRole(QAction::NoRole);
        allAction = new QAction(MainWindow);
        allAction->setObjectName("allAction");
        QIcon icon7(QIcon::fromTheme(QIcon::ThemeIcon::EditSelectAll));
        allAction->setIcon(icon7);
        allAction->setMenuRole(QAction::NoRole);
        enlargeAction = new QAction(MainWindow);
        enlargeAction->setObjectName("enlargeAction");
        QIcon icon8(QIcon::fromTheme(QIcon::ThemeIcon::ZoomIn));
        enlargeAction->setIcon(icon8);
        enlargeAction->setMenuRole(QAction::NoRole);
        lessenAction = new QAction(MainWindow);
        lessenAction->setObjectName("lessenAction");
        QIcon icon9(QIcon::fromTheme(QIcon::ThemeIcon::ZoomOut));
        lessenAction->setIcon(icon9);
        lessenAction->setMenuRole(QAction::NoRole);
        leftAction = new QAction(MainWindow);
        leftAction->setObjectName("leftAction");
        QIcon icon10(QIcon::fromTheme(QIcon::ThemeIcon::ViewRestore));
        leftAction->setIcon(icon10);
        leftAction->setMenuRole(QAction::NoRole);
        rightAction = new QAction(MainWindow);
        rightAction->setObjectName("rightAction");
        QIcon icon11(QIcon::fromTheme(QIcon::ThemeIcon::ViewRefresh));
        rightAction->setIcon(icon11);
        rightAction->setMenuRole(QAction::NoRole);
        actionsvg = new QAction(MainWindow);
        actionsvg->setObjectName("actionsvg");
        actionpng = new QAction(MainWindow);
        actionpng->setObjectName("actionpng");
        action = new QAction(MainWindow);
        action->setObjectName("action");
        action_2 = new QAction(MainWindow);
        action_2->setObjectName("action_2");
        closeaction = new QAction(MainWindow);
        closeaction->setObjectName("closeaction");
        action_4 = new QAction(MainWindow);
        action_4->setObjectName("action_4");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        horizontalLayout_6 = new QHBoxLayout(centralwidget);
        horizontalLayout_6->setObjectName("horizontalLayout_6");
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName("verticalLayout");
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(5);
        horizontalLayout->setObjectName("horizontalLayout");
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        toolButton_3 = new QToolButton(centralwidget);
        toolButton_3->setObjectName("toolButton_3");
        toolButton_3->setMinimumSize(QSize(30, 30));
        toolButton_3->setMaximumSize(QSize(30, 40));
        QIcon icon12;
        icon12.addFile(QString::fromUtf8(":/image/delete.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_3->setIcon(icon12);
        toolButton_3->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_3);

        toolButton_2 = new QToolButton(centralwidget);
        toolButton_2->setObjectName("toolButton_2");
        toolButton_2->setMinimumSize(QSize(30, 30));
        QIcon icon13;
        icon13.addFile(QString::fromUtf8(":/image/bring-to-front.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_2->setIcon(icon13);
        toolButton_2->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_2);

        toolButton = new QToolButton(centralwidget);
        toolButton->setObjectName("toolButton");
        toolButton->setMinimumSize(QSize(30, 30));
        QIcon icon14;
        icon14.addFile(QString::fromUtf8(":/image/send-to-back.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton->setIcon(icon14);
        toolButton->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton);

        fontComboBox = new QFontComboBox(centralwidget);
        fontComboBox->setObjectName("fontComboBox");
        fontComboBox->setMinimumSize(QSize(0, 30));
        fontComboBox->setMaximumSize(QSize(160, 16777215));

        horizontalLayout->addWidget(fontComboBox);

        comboBox = new QComboBox(centralwidget);
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->addItem(QString());
        comboBox->setObjectName("comboBox");
        comboBox->setMinimumSize(QSize(50, 30));
        comboBox->setMaximumSize(QSize(50, 16777215));

        horizontalLayout->addWidget(comboBox);

        toolButton_4 = new QToolButton(centralwidget);
        toolButton_4->setObjectName("toolButton_4");
        toolButton_4->setMinimumSize(QSize(30, 30));
        QIcon icon15;
        icon15.addFile(QString::fromUtf8(":/image/bold.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_4->setIcon(icon15);
        toolButton_4->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_4);

        toolButton_5 = new QToolButton(centralwidget);
        toolButton_5->setObjectName("toolButton_5");
        toolButton_5->setMinimumSize(QSize(30, 30));
        QIcon icon16;
        icon16.addFile(QString::fromUtf8(":/image/italic.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_5->setIcon(icon16);
        toolButton_5->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_5);

        toolButton_6 = new QToolButton(centralwidget);
        toolButton_6->setObjectName("toolButton_6");
        toolButton_6->setMinimumSize(QSize(30, 30));
        QIcon icon17;
        icon17.addFile(QString::fromUtf8(":/image/underline.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_6->setIcon(icon17);
        toolButton_6->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_6);

        comboBox_2 = new QComboBox(centralwidget);
        QIcon icon18;
        icon18.addFile(QString::fromUtf8(":/image/black.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_2->addItem(icon18, QString());
        QIcon icon19;
        icon19.addFile(QString::fromUtf8(":/image/blue.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_2->addItem(icon19, QString());
        QIcon icon20;
        icon20.addFile(QString::fromUtf8(":/image/red.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_2->addItem(icon20, QString());
        QIcon icon21;
        icon21.addFile(QString::fromUtf8(":/image/yellow.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_2->addItem(icon21, QString());
        QIcon icon22;
        icon22.addFile(QString::fromUtf8(":/image/green.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_2->addItem(icon22, QString());
        comboBox_2->addItem(QString());
        comboBox_2->setObjectName("comboBox_2");
        comboBox_2->setMinimumSize(QSize(50, 30));
        comboBox_2->setMaximumSize(QSize(100, 16777215));

        horizontalLayout->addWidget(comboBox_2);

        comboBox_4 = new QComboBox(centralwidget);
        QIcon icon23;
        icon23.addFile(QString::fromUtf8(":/image/white.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        comboBox_4->addItem(icon23, QString());
        comboBox_4->addItem(icon18, QString());
        comboBox_4->addItem(icon19, QString());
        comboBox_4->addItem(icon20, QString());
        comboBox_4->addItem(icon21, QString());
        comboBox_4->addItem(icon22, QString());
        comboBox_4->addItem(QString());
        comboBox_4->setObjectName("comboBox_4");
        comboBox_4->setMinimumSize(QSize(0, 30));
        comboBox_4->setMaximumSize(QSize(100, 16777215));

        horizontalLayout->addWidget(comboBox_4);

        comboBox_3 = new QComboBox(centralwidget);
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->addItem(QString());
        comboBox_3->setObjectName("comboBox_3");
        comboBox_3->setMinimumSize(QSize(50, 30));
        comboBox_3->setMaximumSize(QSize(50, 16777215));

        horizontalLayout->addWidget(comboBox_3);

        toolButton_17 = new QToolButton(centralwidget);
        toolButton_17->setObjectName("toolButton_17");
        toolButton_17->setMinimumSize(QSize(30, 30));
        QIcon icon24;
        icon24.addFile(QString::fromUtf8(":/image/magnify.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_17->setIcon(icon24);
        toolButton_17->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_17);

        toolButton_18 = new QToolButton(centralwidget);
        toolButton_18->setObjectName("toolButton_18");
        toolButton_18->setMinimumSize(QSize(30, 30));
        QIcon icon25;
        icon25.addFile(QString::fromUtf8(":/image/lessen.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_18->setIcon(icon25);
        toolButton_18->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_18);

        toolButton_19 = new QToolButton(centralwidget);
        toolButton_19->setObjectName("toolButton_19");
        toolButton_19->setMinimumSize(QSize(30, 30));
        QIcon icon26;
        icon26.addFile(QString::fromUtf8(":/image/left.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_19->setIcon(icon26);
        toolButton_19->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_19);

        toolButton_20 = new QToolButton(centralwidget);
        toolButton_20->setObjectName("toolButton_20");
        toolButton_20->setMinimumSize(QSize(30, 30));
        QIcon icon27;
        icon27.addFile(QString::fromUtf8(":/image/right.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        toolButton_20->setIcon(icon27);
        toolButton_20->setIconSize(QSize(20, 20));

        horizontalLayout->addWidget(toolButton_20);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName("horizontalLayout_5");
        toolBox = new QToolBox(centralwidget);
        toolBox->setObjectName("toolBox");
        toolBox->setMaximumSize(QSize(200, 16777215));
        page = new QWidget();
        page->setObjectName("page");
        page->setGeometry(QRect(0, 0, 200, 485));
        horizontalLayout_2 = new QHBoxLayout(page);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        formLayout = new QFormLayout();
        formLayout->setObjectName("formLayout");

        horizontalLayout_2->addLayout(formLayout);

        toolBox->addItem(page, QString::fromUtf8("\345\233\276\345\275\242\351\200\211\346\213\251"));

        horizontalLayout_5->addWidget(toolBox);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName("gridLayout");

        horizontalLayout_5->addLayout(gridLayout);


        verticalLayout->addLayout(horizontalLayout_5);


        horizontalLayout_6->addLayout(verticalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 910, 17));
        menu = new QMenu(menubar);
        menu->setObjectName("menu");
        menu_4 = new QMenu(menu);
        menu_4->setObjectName("menu_4");
        menu_2 = new QMenu(menubar);
        menu_2->setObjectName("menu_2");
        menu_3 = new QMenu(menubar);
        menu_3->setObjectName("menu_3");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menu->menuAction());
        menubar->addAction(menu_2->menuAction());
        menubar->addAction(menu_3->menuAction());
        menu->addAction(newAction);
        menu->addAction(openAction);
        menu->addAction(action);
        menu->addAction(menu_4->menuAction());
        menu->addSeparator();
        menu->addAction(closeaction);
        menu->addAction(action_4);
        menu_4->addAction(actionsvg);
        menu_4->addAction(actionpng);
        menu_2->addAction(copyAction);
        menu_2->addAction(pasteAction);
        menu_2->addAction(cutAction);
        menu_2->addAction(revertAction);
        menu_2->addAction(allAction);
        menu_3->addAction(enlargeAction);
        menu_3->addAction(lessenAction);
        menu_3->addAction(leftAction);
        menu_3->addAction(rightAction);

        retranslateUi(MainWindow);

        toolBox->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        newAction->setText(QCoreApplication::translate("MainWindow", "\346\226\260\345\273\272", nullptr));
#if QT_CONFIG(shortcut)
        newAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+N", nullptr));
#endif // QT_CONFIG(shortcut)
        openAction->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200", nullptr));
#if QT_CONFIG(shortcut)
        openAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+O", nullptr));
#endif // QT_CONFIG(shortcut)
        saveAction->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(shortcut)
        saveAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        copyAction->setText(QCoreApplication::translate("MainWindow", "\345\244\215\345\210\266", nullptr));
#if QT_CONFIG(shortcut)
        copyAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+C", nullptr));
#endif // QT_CONFIG(shortcut)
        pasteAction->setText(QCoreApplication::translate("MainWindow", "\347\262\230\350\264\264", nullptr));
#if QT_CONFIG(shortcut)
        pasteAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+V", nullptr));
#endif // QT_CONFIG(shortcut)
        cutAction->setText(QCoreApplication::translate("MainWindow", "\345\211\252\345\210\207", nullptr));
#if QT_CONFIG(shortcut)
        cutAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+X", nullptr));
#endif // QT_CONFIG(shortcut)
        revertAction->setText(QCoreApplication::translate("MainWindow", "\346\222\244\351\224\200", nullptr));
#if QT_CONFIG(shortcut)
        revertAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Z", nullptr));
#endif // QT_CONFIG(shortcut)
        allAction->setText(QCoreApplication::translate("MainWindow", "\345\205\250\351\200\211", nullptr));
#if QT_CONFIG(shortcut)
        allAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+A", nullptr));
#endif // QT_CONFIG(shortcut)
        enlargeAction->setText(QCoreApplication::translate("MainWindow", "\346\224\276\345\244\247", nullptr));
#if QT_CONFIG(shortcut)
        enlargeAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Up", nullptr));
#endif // QT_CONFIG(shortcut)
        lessenAction->setText(QCoreApplication::translate("MainWindow", "\347\274\251\345\260\217", nullptr));
#if QT_CONFIG(shortcut)
        lessenAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Down", nullptr));
#endif // QT_CONFIG(shortcut)
        leftAction->setText(QCoreApplication::translate("MainWindow", "\345\267\246\346\227\213", nullptr));
#if QT_CONFIG(shortcut)
        leftAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Left", nullptr));
#endif // QT_CONFIG(shortcut)
        rightAction->setText(QCoreApplication::translate("MainWindow", "\345\217\263\346\227\213", nullptr));
#if QT_CONFIG(shortcut)
        rightAction->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+Right", nullptr));
#endif // QT_CONFIG(shortcut)
        actionsvg->setText(QCoreApplication::translate("MainWindow", "svg", nullptr));
        actionpng->setText(QCoreApplication::translate("MainWindow", "png", nullptr));
        action->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230", nullptr));
#if QT_CONFIG(shortcut)
        action->setShortcut(QCoreApplication::translate("MainWindow", "Ctrl+S", nullptr));
#endif // QT_CONFIG(shortcut)
        action_2->setText(QCoreApplication::translate("MainWindow", "\346\211\223\345\274\200", nullptr));
        closeaction->setText(QCoreApplication::translate("MainWindow", "\345\205\263\351\227\255", nullptr));
        action_4->setText(QCoreApplication::translate("MainWindow", "\344\277\235\345\255\230\345\271\266\345\205\263\351\227\255", nullptr));
#if QT_CONFIG(whatsthis)
        toolButton_3->setWhatsThis(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\345\210\240\351\231\244</p><p><br/></p></body></html>", nullptr));
#endif // QT_CONFIG(whatsthis)
        toolButton_3->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
#if QT_CONFIG(shortcut)
        toolButton_3->setShortcut(QCoreApplication::translate("MainWindow", "Del", nullptr));
#endif // QT_CONFIG(shortcut)
#if QT_CONFIG(whatsthis)
        toolButton_2->setWhatsThis(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\346\230\276\347\244\272\345\234\250\344\270\212\345\261\202</p></body></html>", nullptr));
#endif // QT_CONFIG(whatsthis)
        toolButton_2->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
#if QT_CONFIG(whatsthis)
        toolButton->setWhatsThis(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\346\230\276\347\244\272\345\234\250\344\270\213\345\261\202</p></body></html>", nullptr));
#endif // QT_CONFIG(whatsthis)
        toolButton->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        comboBox->setItemText(0, QCoreApplication::translate("MainWindow", "5", nullptr));
        comboBox->setItemText(1, QCoreApplication::translate("MainWindow", "6", nullptr));
        comboBox->setItemText(2, QCoreApplication::translate("MainWindow", "7", nullptr));
        comboBox->setItemText(3, QCoreApplication::translate("MainWindow", "8", nullptr));
        comboBox->setItemText(4, QCoreApplication::translate("MainWindow", "9", nullptr));
        comboBox->setItemText(5, QCoreApplication::translate("MainWindow", "12", nullptr));
        comboBox->setItemText(6, QCoreApplication::translate("MainWindow", "15", nullptr));
        comboBox->setItemText(7, QCoreApplication::translate("MainWindow", "18", nullptr));
        comboBox->setItemText(8, QCoreApplication::translate("MainWindow", "21", nullptr));
        comboBox->setItemText(9, QCoreApplication::translate("MainWindow", "25", nullptr));
        comboBox->setItemText(10, QCoreApplication::translate("MainWindow", "30", nullptr));
        comboBox->setItemText(11, QCoreApplication::translate("MainWindow", "40", nullptr));

        toolButton_4->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_5->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_6->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        comboBox_2->setItemText(0, QCoreApplication::translate("MainWindow", "\351\273\221\350\211\262", nullptr));
        comboBox_2->setItemText(1, QCoreApplication::translate("MainWindow", "\350\223\235\350\211\262", nullptr));
        comboBox_2->setItemText(2, QCoreApplication::translate("MainWindow", "\347\272\242\350\211\262", nullptr));
        comboBox_2->setItemText(3, QCoreApplication::translate("MainWindow", "\351\273\204\350\211\262", nullptr));
        comboBox_2->setItemText(4, QCoreApplication::translate("MainWindow", "\347\273\277\350\211\262", nullptr));
        comboBox_2->setItemText(5, QCoreApplication::translate("MainWindow", "\345\205\266\344\273\226\351\242\234\350\211\262", nullptr));

#if QT_CONFIG(tooltip)
        comboBox_2->setToolTip(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\351\242\234\350\211\262", nullptr));
#endif // QT_CONFIG(tooltip)
        comboBox_2->setCurrentText(QString());
        comboBox_2->setPlaceholderText(QCoreApplication::translate("MainWindow", "\345\241\253\345\205\205\351\242\234\350\211\262", nullptr));
        comboBox_4->setItemText(0, QCoreApplication::translate("MainWindow", "\347\231\275\350\211\262", nullptr));
        comboBox_4->setItemText(1, QCoreApplication::translate("MainWindow", "\351\273\221\350\211\262", nullptr));
        comboBox_4->setItemText(2, QCoreApplication::translate("MainWindow", "\350\223\235\350\211\262", nullptr));
        comboBox_4->setItemText(3, QCoreApplication::translate("MainWindow", "\347\272\242\350\211\262", nullptr));
        comboBox_4->setItemText(4, QCoreApplication::translate("MainWindow", "\351\273\204\350\211\262", nullptr));
        comboBox_4->setItemText(5, QCoreApplication::translate("MainWindow", "\347\273\277\350\211\262", nullptr));
        comboBox_4->setItemText(6, QCoreApplication::translate("MainWindow", "\345\205\266\344\273\226\351\242\234\350\211\262", nullptr));

#if QT_CONFIG(tooltip)
        comboBox_4->setToolTip(QCoreApplication::translate("MainWindow", "\345\233\276\345\275\242\351\242\234\350\211\262", nullptr));
#endif // QT_CONFIG(tooltip)
#if QT_CONFIG(whatsthis)
        comboBox_4->setWhatsThis(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\350\276\271\346\241\206\351\242\234\350\211\262</p></body></html>", nullptr));
#endif // QT_CONFIG(whatsthis)
        comboBox_3->setItemText(0, QCoreApplication::translate("MainWindow", "1", nullptr));
        comboBox_3->setItemText(1, QCoreApplication::translate("MainWindow", "4", nullptr));
        comboBox_3->setItemText(2, QCoreApplication::translate("MainWindow", "7", nullptr));
        comboBox_3->setItemText(3, QCoreApplication::translate("MainWindow", "10", nullptr));
        comboBox_3->setItemText(4, QCoreApplication::translate("MainWindow", "13", nullptr));
        comboBox_3->setItemText(5, QCoreApplication::translate("MainWindow", "16", nullptr));
        comboBox_3->setItemText(6, QCoreApplication::translate("MainWindow", "19", nullptr));
        comboBox_3->setItemText(7, QCoreApplication::translate("MainWindow", "22", nullptr));
        comboBox_3->setItemText(8, QCoreApplication::translate("MainWindow", "25", nullptr));
        comboBox_3->setItemText(9, QCoreApplication::translate("MainWindow", "28", nullptr));

#if QT_CONFIG(whatsthis)
        comboBox_3->setWhatsThis(QCoreApplication::translate("MainWindow", "<html><head/><body><p>\350\276\271\346\241\206\347\262\227\347\273\206</p><p><br/></p></body></html>", nullptr));
#endif // QT_CONFIG(whatsthis)
        toolButton_17->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_18->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_19->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolButton_20->setText(QCoreApplication::translate("MainWindow", "...", nullptr));
        toolBox->setItemText(toolBox->indexOf(page), QCoreApplication::translate("MainWindow", "\345\233\276\345\275\242\351\200\211\346\213\251", nullptr));
        menu->setTitle(QCoreApplication::translate("MainWindow", "\346\226\207\344\273\266", nullptr));
        menu_4->setTitle(QCoreApplication::translate("MainWindow", "\345\257\274\345\207\272\344\270\272", nullptr));
        menu_2->setTitle(QCoreApplication::translate("MainWindow", "\347\274\226\350\276\221", nullptr));
        menu_3->setTitle(QCoreApplication::translate("MainWindow", "\350\247\206\350\247\222", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
