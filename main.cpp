/****************************************************************************
**
** Copyright (C) 2004-2009 Kevin Clague. All rights reserved.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QApplication>

#include "lpub_preferences.h"
#include "lpub.h"
#include "resolution.h"
#include <QMessageBox>

//**3D
#include "lc_global.h"
#include "lc_application.h"
#include "lc_qmainwindow.h"
#include "lc_qupdatedialog.h"
#include "lc_mainwindow.h"
#include "view.h"
#include "project.h"
#include "lc_colors.h"
#include <QApplication>
//**

#ifdef Q_OS_WIN

#include <dbghelp.h>
#include <direct.h>
#include <shlobj.h>

#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE
#endif
#endif

#include <tchar.h>

QAction *actions[LC_NUM_COMMANDS];

static TCHAR minidumpPath[_MAX_PATH];

static LONG WINAPI lcSehHandler(PEXCEPTION_POINTERS exceptionPointers)
{
    if (IsDebuggerPresent())
        return EXCEPTION_CONTINUE_SEARCH;

    HMODULE dbgHelp = LoadLibrary(TEXT("dbghelp.dll"));

    if (dbgHelp == NULL)
        return EXCEPTION_EXECUTE_HANDLER;

    HANDLE file = CreateFile(minidumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE)
        return EXCEPTION_EXECUTE_HANDLER;

    typedef BOOL (WINAPI *LPMINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType, CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, CONST PMINIDUMP_USER_STREAM_INFORMATION UserEncoderParam, CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
    LPMINIDUMPWRITEDUMP miniDumpWriteDump = (LPMINIDUMPWRITEDUMP)GetProcAddress(dbgHelp, "MiniDumpWriteDump");
    if (!miniDumpWriteDump)
        return EXCEPTION_EXECUTE_HANDLER;

    MINIDUMP_EXCEPTION_INFORMATION mei;

    mei.ThreadId = GetCurrentThreadId();
    mei.ExceptionPointers = exceptionPointers;
    mei.ClientPointers = TRUE;

    BOOL writeDump = miniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file, MiniDumpNormal, exceptionPointers ? &mei : NULL, NULL, NULL);

    CloseHandle(file);
    FreeLibrary(dbgHelp);

    if (writeDump)
    {
        TCHAR message[_MAX_PATH + 256];
        lstrcpy(message, TEXT("LPubV just crashed. Crash information was saved to the file: \n'"));
        lstrcat(message, minidumpPath);
        lstrcat(message, TEXT("'\n Please send it to the developers for debugging."));

        MessageBox(NULL, message, TEXT("LPubV"), MB_OK);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

static void lcSehInit()
{
    if (SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, minidumpPath) == S_OK)
    {
        lstrcat(minidumpPath, TEXT("\\LPub Software\\LPubV\\dump\\"));
        _tmkdir(minidumpPath);
        lstrcat(minidumpPath, TEXT("minidump.dmp"));
    }

    SetUnhandledExceptionFilter(lcSehHandler);
}


#endif

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(lpub);

    QApplication app(argc, argv);

    QCoreApplication::setOrganizationName("LPub Software");
    QCoreApplication::setApplicationName("LPubV");
    QCoreApplication::setApplicationVersion(LP_VERSION_TEXT);

    QTranslator Translator;
    Translator.load(QString("lpub_") + QLocale::system().name().section('_', 0, 0) + ".qm", ":/lc_lib/resources");
    app.installTranslator(&Translator);

    Preferences::ldrawPreferences(false);
    Preferences::unitsPreferences();
    Preferences::annotationPreferences();
    Preferences::viewerPreferences();

    defaultResolutionType(Preferences::preferCentimeters);
    setResolution(150);  // DPI

    g_App = new lcApplication();

#if defined(Q_OS_WIN)
    char libPath[LC_MAXPATH], *ptr;
    strcpy(libPath, argv[0]);
    ptr = strrchr(libPath,'\\');
    if (ptr)
        *(++ptr) = 0;

    lcSehInit();
#elif defined(Q_OS_MAC)
    QDir bundlePath = QDir(QCoreApplication::applicationDirPath());
    bundlePath.cdUp();
    bundlePath.cdUp();
    bundlePath = QDir::cleanPath(bundlePath.absolutePath() + "/Contents/lc_lib/Resources/");
    QByteArray pathArray = bundlePath.absolutePath().toLocal8Bit();
    const char* libPath = pathArray.data();
#else
    const char* libPath = LC_INSTALL_PREFIX "/share/lpubv/";
#endif
#ifdef LC_LDRAW_LIBRARY_PATH
    const char* LDrawPath = LC_LDRAW_LIBRARY_PATH;
#else
    const char* LDrawPath = NULL;
#endif

//    QMessageBox::information(NULL,QMessageBox::tr("LPubV"),QMessageBox::tr("Startup"));

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    QStringList cachePathList = QStandardPaths::standardLocations(QStandardPaths::CacheLocation);
    QString cachePath = cachePathList.first();
#else
    QString cachePath = QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif

    QDir dir;
    dir.mkpath(cachePath);
    gMainWindow = new lcMainWindow();

    if (!g_App->Initialize(argc, argv, libPath, LDrawPath, cachePath.toLocal8Bit().data()))
        return 1;

    Gui     LPubApp;
    gMainWindow->mHandle = LPubApp.lcqWindow;
    lcGetActiveModel()->UpdateInterface();
    gMainWindow->SetColorIndex(lcGetColorIndex(4));
    gMainWindow->UpdateRecentFiles();

    LPubApp.show();
    LPubApp.sizeit();

#if !LC_DISABLE_UPDATE_CHECK
    lcDoInitialUpdateCheck();
#endif

    int execReturn = app.exec();

    delete gMainWindow;
    gMainWindow = NULL;
    delete g_App;
    g_App = NULL;

    return execReturn;
}

// TODO: move somewhere else

int lcBaseWindow::DoMessageBox(const char* Text, const char* Caption, int Flags)
{
    QWidget* parent = (QWidget*)mHandle;

    QMessageBox::StandardButton	result;
    QMessageBox::StandardButtons buttons;

    switch (Flags & LC_MB_TYPEMASK)
    {
    default:
    case LC_MB_OK:
        buttons = QMessageBox::Ok;
        break;

    case LC_MB_OKCANCEL:
        buttons = QMessageBox::Ok | QMessageBox::Cancel;
        break;

    case LC_MB_YESNOCANCEL:
        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
        break;

    case LC_MB_YESNO:
        buttons = QMessageBox::Yes | QMessageBox::No;
        break;
    }

    switch (Flags & LC_MB_ICONMASK)
    {
    default:
    case LC_MB_ICONINFORMATION:
        result = QMessageBox::information(parent, Caption, Text, buttons);
        break;

    case LC_MB_ICONQUESTION:
        result = QMessageBox::question(parent, Caption, Text, buttons);
        break;

    case LC_MB_ICONWARNING:
        result = QMessageBox::warning(parent, Caption, Text, buttons);
        break;

    case LC_MB_ICONERROR:
        result = QMessageBox::critical(parent, Caption, Text, buttons);
        break;
    }

    switch (result)
    {
    default:
    case QMessageBox::Ok:
        return LC_OK;

    case QMessageBox::Cancel:
        return LC_CANCEL;

    case QMessageBox::Yes:
        return LC_YES;

    case QMessageBox::No:
        return LC_NO;
    }
}

#include "lc_qimagedialog.h"
#include "lc_qhtmldialog.h"
#include "lc_qpovraydialog.h"
#include "lc_qpropertiesdialog.h"
#include "lc_qfinddialog.h"
#include "lc_qselectdialog.h"
#include "lc_qminifigdialog.h"
#include "lc_qarraydialog.h"
#include "lc_qgroupdialog.h"
#include "lc_qeditgroupsdialog.h"
#include "lc_qpreferencesdialog.h"
#include "lc_qupdatedialog.h"
#include "lc_qaboutdialog.h"

bool lcBaseWindow::DoDialog(LC_DIALOG_TYPE Type, void* Data)
{
    QWidget* parent = (QWidget*)mHandle;

    switch (Type)
    {
    case LC_DIALOG_EXPORT_3DSTUDIO:
    case LC_DIALOG_EXPORT_BRICKLINK:
    case LC_DIALOG_EXPORT_CSV:
    case LC_DIALOG_EXPORT_WAVEFRONT:
        {
            char* FileName = (char*)Data;
            QString result;

            switch (Type)
            {
            case LC_DIALOG_EXPORT_3DSTUDIO:
                result = QFileDialog::getSaveFileName(parent, tr("Export 3D Studio"), FileName, tr("3DS Files (*.3ds);;All Files (*.*)"));
                break;

            case LC_DIALOG_EXPORT_BRICKLINK:
                result = QFileDialog::getSaveFileName(parent, tr("Export BrickLink"), FileName, tr("XML Files (*.xml);;All Files (*.*)"));
                break;

            case LC_DIALOG_EXPORT_CSV:
                result = QFileDialog::getSaveFileName(parent, tr("Export CSV"), FileName, tr("CSV Files (*.csv);;All Files (*.*)"));
                break;

            case LC_DIALOG_EXPORT_WAVEFRONT:
                result = QFileDialog::getSaveFileName(parent, tr("Export Wavefront"), FileName, tr("Wavefront Files (*.obj);;All Files (*.*)"));
                break;

            default:
                break;
            }

            if (!result.isEmpty())
            {
                strcpy(FileName, result.toLocal8Bit().data());
                return true;
            }
        } break;

    case LC_DIALOG_SAVE_IMAGE:
        {
            lcQImageDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_EXPORT_HTML:
        {
            lcQHTMLDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_EXPORT_POVRAY:
        {
            lcQPOVRayDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_PROPERTIES:
        {
            lcQPropertiesDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_PRINT:
        {
            lcQMainWindow *mainWindow = (lcQMainWindow*)parent;
            mainWindow->showPrintDialog();
            return true;
        } break;

    case LC_DIALOG_FIND:
        {
            lcQFindDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_SELECT_BY_NAME:
        {
            lcQSelectDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_MINIFIG:
        {
            lcQMinifigDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_PIECE_ARRAY:
        {
            lcQArrayDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_PIECE_GROUP:
        {
            lcQGroupDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_EDIT_GROUPS:
        {
            lcQEditGroupsDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_PREFERENCES:
        {
            lcQPreferencesDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_ABOUT:
        {
            lcQAboutDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;

    case LC_DIALOG_CHECK_UPDATES:
        {
            lcQUpdateDialog dialog(parent, Data);
            return dialog.exec() == QDialog::Accepted;
        } break;
    }

    return false;
}

void lcMainWindow::Close()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    window->close();
}

void lcMainWindow::SplitHorizontal()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->splitHorizontal();
}

void lcMainWindow::SplitVertical()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->splitVertical();
}

void lcMainWindow::RemoveView()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->removeView();
}

void lcMainWindow::ResetViews()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->resetViews();
}

void lcMainWindow::TogglePrintPreview()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->togglePrintPreview();
}

void lcMainWindow::ToggleFullScreen()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->toggleFullScreen();
}

void lcMainWindow::UpdateFocusObject(lcObject* Focus)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateFocusObject(Focus);
}

void lcMainWindow::UpdateSelectedObjects(int Flags, int SelectedCount, lcObject* Focus)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateSelectedObjects(Flags, SelectedCount, Focus);
}

void lcMainWindow::UpdateAction(int NewAction)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateAction(NewAction);
}

void lcMainWindow::UpdatePaste(bool Enabled)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updatePaste(Enabled);
}

void lcMainWindow::UpdateCurrentStep()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateCurrentStep();
}

void lcMainWindow::SetAddKeys(bool AddKeys)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->setAddKeys(AddKeys);

    mAddKeys = AddKeys;
}

void lcMainWindow::UpdateLockSnap()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateLockSnap();
}

void lcMainWindow::UpdateSnap()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateSnap();
}

void lcMainWindow::UpdateColor()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateColor();
}
void lcMainWindow::UpdateUndoRedo(const QString& UndoText, const QString& RedoText)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateUndoRedo(UndoText, RedoText);
}

void lcMainWindow::SetTransformType(lcTransformType TransformType)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    mTransformType = TransformType;

    if (window)
        window->updateTransformType(TransformType);
}

void lcMainWindow::SetRotateStepType(lcRotateStepType RotateStepType)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    mRotateStepType = RotateStepType;

    if (window)
        window->updateRotateStepType(RotateStepType);
}

void lcMainWindow::UpdateCameraMenu()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateCameraMenu();
}

void lcMainWindow::UpdateCurrentCamera(int CameraIndex)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateCurrentCamera(CameraIndex);
}

void lcMainWindow::UpdatePerspective()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updatePerspective(mActiveView);
}

void lcMainWindow::UpdateModels()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateModels();
}

void lcMainWindow::UpdateCategories()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateCategories();
}

void lcMainWindow::UpdateTitle()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateTitle(lcGetActiveProject()->GetTitle(), lcGetActiveProject()->IsModified());
}

void lcMainWindow::UpdateModified(bool Modified)
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateModified(Modified);
}

void lcMainWindow::UpdateRecentFiles()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateRecentFiles();
}

void lcMainWindow::UpdateShortcuts()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        window->updateShortcuts();
}

lcVector3 lcMainWindow::GetTransformAmount()
{
    lcQMainWindow* window = (lcQMainWindow*)mHandle;

    if (window)
        return window->getTransformAmount();

    return lcVector3(0.0f, 0.0f, 0.0f);
}



