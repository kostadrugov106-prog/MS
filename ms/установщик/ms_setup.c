/*
 * MS 1.1 — установщик с графическим интерфейсом (один файл).
 * Внутри этого .exe лежат движок ms.exe и «Центр MS» ms_center.exe (как ресурсы).
 *
 * Обычный запуск:   MS-1.1-Setup.exe              — мастер установки
 * Удаление:         MS-1.1-Setup.exe /uninstall   — мастер удаления
 * Тихая установка:  MS-1.1-Setup.exe /S [/D=путь]
 * Тихий запуск:     MS-1.1-Setup.exe /S /uninstall
 *
 * Работает на Windows 7, 8, 8.1, 10, 11 (64-бит).
 * Ставится без прав администратора, в папку пользователя.
 */

#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define _WIN32_IE 0x0700
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <windowsx.h>
#include <objbase.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdarg.h>
#include <wchar.h>
#include <string.h>

#define IDR_MS_EXE 101
#define IDR_CENTER_EXE 102

#define WM_STEP  (WM_APP + 1)
#define WM_DONE  (WM_APP + 2)

#define ID_PATH       1001
#define ID_BROWSE     1002
#define ID_CM_START   1003
#define ID_CM_DESKTOP 1004
#define ID_CM_ASSOC   1005
#define ID_CM_PATH    1006
#define ID_CM_PATH2   1007
#define ID_MAIN       1010
#define ID_BACK       1011

enum { PAGE_WELCOME = 0, PAGE_CUSTOM = 1, PAGE_PROGRESS = 2, PAGE_DONE = 3 };

enum {
    ST_PREP = 0, ST_FOLDER, ST_ENGINE, ST_ASSOC, ST_SHORTCUTS, ST_PATH, ST_FINISH,
    ST_U_PATH, ST_U_REG, ST_U_SHORTCUTS, ST_U_FILES
};

static const wchar_t *СТАТУСЫ[] = {
    L"Подготовка…",
    L"Создание папки установки…",
    L"Копирование движка MS…",
    L"Настройка открытия файлов .ms…",
    L"Создание ярлыков…",
    L"Добавление команды ms в PATH…",
    L"Завершение…",
    L"Удаление команды ms из PATH…",
    L"Удаление настроек…",
    L"Удаление ярлыков…",
    L"Удаление файлов…"
};

/* ---------- цвета и размеры ---------- */

#define ЦВ_ФОН         RGB(241, 242, 242)
#define ЦВ_ТЕКСТ       RGB(42, 44, 43)
#define ЦВ_СЕРЫЙ       RGB(110, 112, 111)
#define ЦВ_ССЫЛКА      RGB(0, 0, 0)
#define ЦВ_ССЫЛКА_НАВ  RGB(42, 44, 43)
#define ЦВ_ЛИНИЯ       RGB(216, 217, 217)
#define ЦВ_БАННЕР      RGB(42, 44, 43)
#define ЦВ_СВЕТЛЫЙ     RGB(241, 242, 242)
#define ЦВ_БАННЕР_ТИХ  RGB(176, 178, 177)

#define БАННЕР_Ш 200
#define ОКНО_Ш   660
#define ОКНО_В   430

typedef struct {
    RECT место;
    const wchar_t *заголовок;
    const wchar_t *строки[3];
    int строк;
    int действие;
} ССЫЛКА;

static HINSTANCE г_инст;
static HWND г_окно;
static UINT г_dpi = 96;
static HBRUSH г_кисть_фон;

static HFONT ш_заголовок, ш_текст, ш_ссылка, ш_мелкий, ш_жирный;
static HFONT ш_лого, ш_марка, ш_марка2, ш_марка3;

static int г_страница = PAGE_WELCOME;
static int г_режим = 0;              /* 0 — установка, 1 — удаление */
static int г_галочка_пуск = 1;
static int г_галочка_стол = 0;
static int г_галочка_ассоц = 1;
static int г_галочка_path = 1;
static wchar_t г_папка[MAX_PATH];

static HWND э_путь, к_обзор, к_пуск, к_стол, к_ассоц, к_path, к_path2, б_основная, б_назад;

static int г_прогресс = 0;
static int г_цель = 0;
static int г_статус = ST_PREP;
static HANDLE г_поток = NULL;
static wchar_t г_ошибка[512];

static ССЫЛКА г_ссылки[4];
static int г_ссылок = 0;
static int г_наведение = -1;

static wchar_t г_буфер_подписи[MAX_PATH + 64];

/* ---------- утилиты ---------- */

static int S(int v){ return MulDiv(v, (int)г_dpi, 96); }

static void формат(wchar_t *буф, size_t размер, const wchar_t *шаблон, ...){
    va_list ап;
    va_start(ап, шаблон);
    _vsnwprintf(буф, размер, шаблон, ап);
    va_end(ап);
    if (размер) буф[размер - 1] = 0;
}

static void задать_ошибку(const wchar_t *шаблон, ...){
    va_list ап;
    va_start(ап, шаблон);
    _vsnwprintf(г_ошибка, 512, шаблон, ап);
    va_end(ап);
    г_ошибка[511] = 0;
}

static HFONT шрифт(const wchar_t *лицо, int пункты, int вес){
    return CreateFontW(-MulDiv(пункты, (int)г_dpi, 72), 0, 0, 0, вес, 0, 0, 0,
                       DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, лицо);
}

static void надпись(HDC dc, const wchar_t *с, int x, int y, int ш, int в,
                    HFONT ф, COLORREF ц, UINT фл){
    RECT r;
    r.left = x; r.top = y; r.right = x + ш; r.bottom = y + в;
    HGDIOBJ старый = SelectObject(dc, ф);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, ц);
    DrawTextW(dc, с, -1, &r, фл | DT_NOPREFIX);
    SelectObject(dc, старый);
}

/* ---------- реестр и файлы ---------- */

static int реестр_строка(HKEY корень, const wchar_t *путь, const wchar_t *имя, const wchar_t *значение){
    HKEY ключ;
    if (RegCreateKeyExW(корень, путь, 0, NULL, 0, KEY_WRITE, NULL, &ключ, NULL) != ERROR_SUCCESS)
        return 0;
    LONG р = RegSetValueExW(ключ, имя, 0, REG_SZ, (const BYTE*)значение,
                            (DWORD)((wcslen(значение) + 1) * sizeof(wchar_t)));
    RegCloseKey(ключ);
    return р == ERROR_SUCCESS;
}

static int реестр_прочитать(HKEY корень, const wchar_t *путь, const wchar_t *имя,
                            wchar_t *буф, DWORD симв){
    HKEY ключ;
    if (RegOpenKeyExW(корень, путь, 0, KEY_READ, &ключ) != ERROR_SUCCESS) return 0;
    DWORD тип = 0, разм = симв * sizeof(wchar_t);
    LONG р = RegQueryValueExW(ключ, имя, NULL, &тип, (BYTE*)буф, &разм);
    RegCloseKey(ключ);
    if (р != ERROR_SUCCESS) return 0;
    буф[симв - 1] = 0;
    return 1;
}

static int записать_ресурс(int id, const wchar_t *путь){
    HRSRC ресурс = FindResourceW(NULL, MAKEINTRESOURCEW(id), MAKEINTRESOURCEW(10));
    if (!ресурс) return 0;
    DWORD размер = SizeofResource(NULL, ресурс);
    HGLOBAL данные_рес = LoadResource(NULL, ресурс);
    if (!данные_рес || !размер) return 0;
    const void *данные = LockResource(данные_рес);
    if (!данные) return 0;

    HANDLE ф = CreateFileW(путь, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (ф == INVALID_HANDLE_VALUE) return 0;
    DWORD записано = 0;
    BOOL ок = WriteFile(ф, данные, размер, &записано, NULL);
    CloseHandle(ф);
    return ок && записано == размер;
}

static int создать_папку(const wchar_t *п){
    int р = SHCreateDirectoryExW(NULL, п, NULL);
    return р == ERROR_SUCCESS || р == ERROR_ALREADY_EXISTS;
}

static void path_изменить(const wchar_t *папка, int добавить){
    HKEY ключ;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_READ | KEY_WRITE, &ключ) != ERROR_SUCCESS)
        return;

    DWORD тип = REG_SZ, размер = 0;
    RegQueryValueExW(ключ, L"Path", NULL, &тип, NULL, &размер);
    wchar_t *старый = (wchar_t*)malloc(размер + sizeof(wchar_t));
    if (!старый) { RegCloseKey(ключ); return; }
    старый[0] = 0;
    if (размер > 0) {
        if (RegQueryValueExW(ключ, L"Path", NULL, &тип, (BYTE*)старый, &размер) == ERROR_SUCCESS)
            старый[размер / sizeof(wchar_t)] = 0;
    }

    size_t дл = wcslen(старый) + wcslen(папка) + 4;
    wchar_t *новый = (wchar_t*)malloc(дл * sizeof(wchar_t));
    if (!новый) { free(старый); RegCloseKey(ключ); return; }
    новый[0] = 0;

    if (добавить) {
        if (wcsstr(старый, папка) == NULL) {
            if (wcslen(старый) > 0) формат(новый, дл, L"%s;%s", старый, папка);
            else wcscpy(новый, папка);
        } else {
            wcsncpy(новый, старый, дл - 1);
            новый[дл - 1] = 0;
        }
    } else {
        const wchar_t *п = старый;
        while (*п) {
            const wchar_t *к = wcschr(п, L';');
            size_t д = к ? (size_t)(к - п) : wcslen(п);
            if (д > 0) {
                wchar_t часть[1024];
                if (д >= 1024) д = 1023;
                wcsncpy(часть, п, д);
                часть[д] = 0;
                if (_wcsicmp(часть, папка) != 0) {
                    if (wcslen(новый) > 0) wcscat(новый, L";");
                    wcscat(новый, часть);
                }
            }
            if (!к) break;
            п = к + 1;
        }
    }

    RegSetValueExW(ключ, L"Path", 0, (тип == REG_EXPAND_SZ) ? REG_EXPAND_SZ : REG_SZ,
                   (const BYTE*)новый, (DWORD)((wcslen(новый) + 1) * sizeof(wchar_t)));
    free(новый);
    free(старый);
    RegCloseKey(ключ);
}

static int создать_ярлык(int место, const wchar_t *exe, const wchar_t *рабочая, const wchar_t *имя){
    wchar_t папка[MAX_PATH];
    if (FAILED(SHGetFolderPathW(NULL, место, NULL, SHGFP_TYPE_CURRENT, папка))) return 0;

    wchar_t файл[MAX_PATH + 16];
    формат(файл, MAX_PATH + 16, L"%s\\%s.lnk", папка, имя);

    IShellLinkW *сс = NULL;
    if (FAILED(CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                &IID_IShellLinkW, (void**)&сс))) return 0;

    сс->lpVtbl->SetPath(сс, exe);
    сс->lpVtbl->SetWorkingDirectory(сс, рабочая);
    сс->lpVtbl->SetDescription(сс, L"Язык MS (Mazing Studio)");
    сс->lpVtbl->SetIconLocation(сс, exe, 0);

    IPersistFile *ф = NULL;
    int ок = 0;
    if (SUCCEEDED(сс->lpVtbl->QueryInterface(сс, &IID_IPersistFile, (void**)&ф))) {
        ок = SUCCEEDED(ф->lpVtbl->Save(ф, файл, TRUE));
        ф->lpVtbl->Release(ф);
    }
    сс->lpVtbl->Release(сс);
    return ок;
}

static void удалить_ярлык(int место, const wchar_t *имя){
    wchar_t папка[MAX_PATH], файл[MAX_PATH + 16];
    if (FAILED(SHGetFolderPathW(NULL, место, NULL, SHGFP_TYPE_CURRENT, папка))) return;
    формат(файл, MAX_PATH + 16, L"%s\\%s.lnk", папка, имя);
    DeleteFileW(файл);
}

/* ---------- работа установщика ---------- */

static void сообщить(int процент, int статус){
    if (г_окно) PostMessageW(г_окно, WM_STEP, (WPARAM)процент, (LPARAM)статус);
}

static void пауза_шага(int мс){
    if (г_окно) Sleep((DWORD)мс);
}

static int установить_ассоциации(const wchar_t *exe){
    wchar_t команда[MAX_PATH * 2], иконка[MAX_PATH * 2];
    формат(команда, MAX_PATH * 2, L"\"%s\" \"%%1\"", exe);
    формат(иконка, MAX_PATH * 2, L"\"%s\",0", exe);

    int ок = 1;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\MSProgram", NULL, L"Программа MS") && ок;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\MSProgram\\DefaultIcon", NULL, иконка) && ок;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\MSProgram\\shell\\open\\command", NULL, команда) && ок;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\.ms", NULL, L"MSProgram") && ок;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\.ms\\shell\\open\\command", NULL, команда) && ок;
    ок = реестр_строка(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\ms.exe\\shell\\open\\command", NULL, команда) && ок;
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.ms\\UserChoice");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
    return ок;
}

static void удалить_ассоциации(void){
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\MSProgram");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\.ms");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Classes\\Applications\\ms.exe");
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\.ms\\UserChoice");
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

static int выполнить_установку(void){
    wchar_t exe[MAX_PATH], центр[MAX_PATH];

    сообщить(8, ST_PREP);
    пауза_шага(250);

    сообщить(22, ST_FOLDER);
    if (!создать_папку(г_папка)) {
        задать_ошибку(L"Не удалось создать папку установки:\n%s\n\nПроверьте права на запись.", г_папка);
        return 0;
    }
    пауза_шага(300);

    сообщить(48, ST_ENGINE);
    формат(exe, MAX_PATH, L"%s\\ms.exe", г_папка);
    if (!записать_ресурс(IDR_MS_EXE, exe)) {
        задать_ошибку(L"Не удалось записать движок в папку:\n%s\n\nВозможно, файл ms.exe занят другой программой.", г_папка);
        return 0;
    }
    формат(центр, MAX_PATH, L"%s\\ms_center.exe", г_папка);
    if (!записать_ресурс(IDR_CENTER_EXE, центр)) {
        задать_ошибку(L"Не удалось записать «Центр MS» в папку:\n%s\n\nВозможно, файл ms_center.exe занят другой программой.", г_папка);
        return 0;
    }
    пауза_шага(350);

    if (г_галочка_ассоц) {
        сообщить(66, ST_ASSOC);
        установить_ассоциации(exe);
        пауза_шага(300);
    }

    реестр_строка(HKEY_CURRENT_USER, L"Software\\MazingStudio\\MS", L"InstallDir", г_папка);

    сообщить(80, ST_SHORTCUTS);
    if (г_галочка_пуск) {
        создать_ярлык(CSIDL_PROGRAMS, exe, г_папка, L"MS");
        создать_ярлык(CSIDL_PROGRAMS, центр, г_папка, L"Центр MS");
    }
    if (г_галочка_стол) создать_ярлык(CSIDL_DESKTOPDIRECTORY, exe, г_папка, L"MS");
    пауза_шага(300);

    if (г_галочка_path) {
        сообщить(92, ST_PATH);
        path_изменить(г_папка, 1);
        пауза_шага(250);
    }

    сообщить(100, ST_FINISH);
    пауза_шага(300);
    return 1;
}

static int выполнить_удаление(void){
    wchar_t exe[MAX_PATH];

    сообщить(8, ST_PREP);
    пауза_шага(250);

    сообщить(28, ST_U_PATH);
    path_изменить(г_папка, 0);
    пауза_шага(250);

    сообщить(46, ST_U_REG);
    удалить_ассоциации();
    RegDeleteTreeW(HKEY_CURRENT_USER, L"Software\\MazingStudio\\MS");
    пауза_шага(300);

    сообщить(64, ST_U_SHORTCUTS);
    удалить_ярлык(CSIDL_PROGRAMS, L"MS");
    удалить_ярлык(CSIDL_PROGRAMS, L"Центр MS");
    удалить_ярлык(CSIDL_DESKTOPDIRECTORY, L"MS");
    пауза_шага(250);

    сообщить(84, ST_U_FILES);
    формат(exe, MAX_PATH, L"%s\\ms.exe", г_папка);
    if (!DeleteFileW(exe) && GetLastError() != ERROR_FILE_NOT_FOUND) {
        задать_ошибку(L"Не удалось удалить файл:\n%s\n\nЗакройте все окна MS и повторите удаление.", exe);
        return 0;
    }
    формат(exe, MAX_PATH, L"%s\\ms_center.exe", г_папка);
    if (!DeleteFileW(exe) && GetLastError() != ERROR_FILE_NOT_FOUND) {
        задать_ошибку(L"Не удалось удалить файл:\n%s\n\nЗакройте «Центр MS» и повторите удаление.", exe);
        return 0;
    }
    RemoveDirectoryW(г_папка);
    пауза_шага(300);

    сообщить(100, ST_FINISH);
    пауза_шага(300);
    return 1;
}

static DWORD WINAPI работник_установки(void *арг){
    (void)арг;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    int ок = выполнить_установку();
    CoUninitialize();
    PostMessageW(г_окно, WM_DONE, ок ? 0 : 2, 0);
    return 0;
}

static DWORD WINAPI работник_удаления(void *арг){
    (void)арг;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    int ок = выполнить_удаление();
    CoUninitialize();
    PostMessageW(г_окно, WM_DONE, ок ? 0 : 2, 0);
    return 0;
}

/* ---------- путь установки ---------- */

static void папка_по_умолчанию(wchar_t *п, size_t симв){
    wchar_t база[MAX_PATH] = L"";
    if (!GetEnvironmentVariableW(L"LOCALAPPDATA", база, MAX_PATH) || !база[0])
        SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, база);
    формат(п, симв, L"%s\\Programs\\MS", база);
}

static void почистить_путь(wchar_t *п){
    wchar_t *н = п;
    while (*н == L' ' || *н == L'\t') н++;
    if (*н == L'"') {
        н++;
        wchar_t *к = wcschr(н, L'"');
        if (к) *к = 0;
    }
    if (н != п) memmove(п, н, (wcslen(н) + 1) * sizeof(wchar_t));

    size_t д = wcslen(п);
    while (д > 0 && (п[д - 1] == L' ' || п[д - 1] == L'\t' || п[д - 1] == L'\\')) {
        if (д == 3 && п[1] == L':') break;
        п[--д] = 0;
    }
}

static int путь_годен(const wchar_t *п){
    if (wcslen(п) < 3) return 0;
    if (п[1] == L':') return 1;
    if (п[0] == L'\\' && п[1] == L'\\') return 1;
    return 0;
}

static void определить_папку_удаления(void){
    wchar_t из_реестра[MAX_PATH];
    if (реестр_прочитать(HKEY_CURRENT_USER, L"Software\\MazingStudio\\MS",
                         L"InstallDir", из_реестра, MAX_PATH) && из_реестра[0]) {
        wcscpy(г_папка, из_реестра);
        return;
    }

    wchar_t своя[MAX_PATH], проверка[MAX_PATH];
    if (GetModuleFileNameW(NULL, своя, MAX_PATH)) {
        wchar_t *пос = wcsrchr(своя, L'\\');
        if (пос) {
            *пос = 0;
            формат(проверка, MAX_PATH, L"%s\\ms.exe", своя);
            if (GetFileAttributesW(проверка) != INVALID_FILE_ATTRIBUTES) {
                wcscpy(г_папка, своя);
                return;
            }
        }
    }
}

static int выбрать_папку(wchar_t *путь, DWORD размер){
    IFileDialog *д = NULL;
    if (SUCCEEDED(CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER,
                                   &IID_IFileDialog, (void**)&д))) {
        DWORD опции = 0;
        д->lpVtbl->GetOptions(д, &опции);
        д->lpVtbl->SetOptions(д, опции | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        д->lpVtbl->SetTitle(д, L"Выберите папку для установки MS");

        int выбрано = 0;
        if (SUCCEEDED(д->lpVtbl->Show(д, г_окно))) {
            IShellItem *элем = NULL;
            if (SUCCEEDED(д->lpVtbl->GetResult(д, &элем))) {
                wchar_t *стр = NULL;
                if (SUCCEEDED(элем->lpVtbl->GetDisplayName(элем, SIGDN_FILESYSPATH, &стр)) && стр) {
                    wcsncpy(путь, стр, размер - 1);
                    путь[размер - 1] = 0;
                    CoTaskMemFree(стр);
                    выбрано = 1;
                }
                элем->lpVtbl->Release(элем);
            }
        }
        д->lpVtbl->Release(д);
        return выбрано;
    }

    BROWSEINFOW би;
    ZeroMemory(&би, sizeof(би));
    би.hwndOwner = г_окно;
    би.lpszTitle = L"Выберите папку для установки MS";
    би.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST ид = SHBrowseForFolderW(&би);
    if (!ид) return 0;
    int ок = SHGetPathFromIDListW(ид, путь);
    CoTaskMemFree(ид);
    return ок;
}

/* ---------- раскладка ---------- */

static void добавить_ссылку(int x, int y, const wchar_t *заголовок, int действие,
                            const wchar_t *а, const wchar_t *б, const wchar_t *в){
    if (г_ссылок >= 4) return;
    ССЫЛКА *с = &г_ссылки[г_ссылок++];
    с->заголовок = заголовок;
    с->действие = действие;
    с->строк = 0;
    const wchar_t *все[3];
    все[0] = а; все[1] = б; все[2] = в;
    for (int i = 0; i < 3; i++)
        if (все[i]) с->строки[с->строк++] = все[i];

    int выс = S(26) + (с->строк ? S(6) + с->строк * S(17) : 0) + S(10);
    с->место.left = x;
    с->место.top = y;
    с->место.right = x + S(430);
    с->место.bottom = y + выс;
}

static void разложить(void){
    if (!г_окно) return;
    RECT кл;
    GetClientRect(г_окно, &кл);
    int лев = S(220);
    int прав = кл.right - S(20);
    int низ = кл.bottom;

    MoveWindow(э_путь, лев, S(140), S(300), S(25), TRUE);
    MoveWindow(к_обзор, лев + S(308), S(139), прав - (лев + S(308)), S(27), TRUE);

    int гал_y[4];
    гал_y[0] = S(196); гал_y[1] = S(222); гал_y[2] = S(248); гал_y[3] = S(274);
    HWND гал[4];
    гал[0] = к_пуск; гал[1] = к_стол; гал[2] = к_ассоц; гал[3] = к_path;
    for (int i = 0; i < 4; i++)
        MoveWindow(гал[i], лев, гал_y[i], прав - лев, S(22), TRUE);

    MoveWindow(к_path2, лев, низ - S(58), S(300), S(22), TRUE);

    int бш = S(140), бв = S(34);
    int бy = низ - S(18) - бв;
    MoveWindow(б_основная, прав - бш, бy, бш, бв, TRUE);
    MoveWindow(б_назад, прав - бш - S(10) - S(110), бy, S(110), бв, TRUE);

    г_ссылок = 0;
    if (г_страница == PAGE_WELCOME && г_режим == 0) {
        формат(г_буфер_подписи, MAX_PATH + 64, L"Папка по умолчанию: %s", г_папка);
        добавить_ссылку(лев, S(150), L"Установить сейчас", 1, г_буфер_подписи,
                        L"Включает движок MS, справку и примеры",
                        L"Создаёт ярлыки и открывает файлы .ms");
        добавить_ссылку(лев, S(256), L"Настроить установку", 2,
                        L"Выбрать папку и компоненты", NULL, NULL);
    } else if (г_страница == PAGE_DONE && г_режим == 0) {
        добавить_ссылку(лев, S(210), L"Открыть папку установки", 3, NULL, NULL, NULL);
    }
}

static void показать(HWND э, int в){
    if (!э) return;
    ShowWindow(э, в ? SW_SHOW : SW_HIDE);
}

static void обновить_страницу(void){
    int стр = г_страница;
    int настройка = (стр == PAGE_CUSTOM);

    показать(э_путь, настройка);
    показать(к_обзор, настройка);
    показать(к_пуск, настройка);
    показать(к_стол, настройка);
    показать(к_ассоц, настройка);
    показать(к_path, настройка);
    показать(к_path2, FALSE);
    показать(б_основная, TRUE);

    if (настройка) {
        SetWindowTextW(э_путь, г_папка);
        SendMessageW(к_пуск, BM_SETCHECK, г_галочка_пуск ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(к_стол, BM_SETCHECK, г_галочка_стол ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(к_ассоц, BM_SETCHECK, г_галочка_ассоц ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(к_path, BM_SETCHECK, г_галочка_path ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    if (стр == PAGE_WELCOME && г_режим == 0)
        SendMessageW(к_path2, BM_SETCHECK, г_галочка_path ? BST_CHECKED : BST_UNCHECKED, 0);

    const wchar_t *осн = L"Отмена";
    if (стр == PAGE_CUSTOM) осн = L"Установить";
    else if (стр == PAGE_PROGRESS) осн = (г_режим == 1) ? L"Удаление…" : L"Установка…";
    else if (стр == PAGE_DONE) осн = L"Закрыть";
    else if (стр == PAGE_WELCOME && г_режим == 1) осн = L"Удалить";
    SetWindowTextW(б_основная, осн);
    EnableWindow(б_основная, стр != PAGE_PROGRESS);

    int показывать_назад = (стр == PAGE_CUSTOM) || (стр == PAGE_WELCOME && г_режим == 1);
    показать(б_назад, показывать_назад);
    if (стр == PAGE_CUSTOM) SetWindowTextW(б_назад, L"Назад");
    else if (стр == PAGE_WELCOME) SetWindowTextW(б_назад, L"Отмена");

    разложить();
    InvalidateRect(г_окно, NULL, FALSE);
}

/* ---------- рисование ---------- */

static void знак(HDC dc, int x, int y, int р, COLORREF круг, COLORREF символ, int толщина){
    HBRUSH кисть = CreateSolidBrush(круг);
    HPEN перо = CreatePen(PS_SOLID, 1, круг);
    HGDIOBJ ст_к = SelectObject(dc, кисть);
    HGDIOBJ ст_п = SelectObject(dc, перо);
    Ellipse(dc, x, y, x + р, y + р);
    SelectObject(dc, ст_к);
    SelectObject(dc, ст_п);
    DeleteObject(кисть);
    DeleteObject(перо);

    LOGBRUSH лб;
    лб.lbStyle = BS_SOLID;
    лб.lbColor = символ;
    лб.lbHatch = 0;
    HPEN глиф = ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                             (DWORD)толщина, &лб, 0, NULL);
    if (!глиф) return;
    HGDIOBJ ст_г = SelectObject(dc, глиф);

    int x1 = x + р * 320 / 1000;
    int x2 = x + р * 484 / 1000;
    int y1 = y + р * 289 / 1000;
    int y2 = y + р * 430 / 1000;
    int y3 = y + р * 570 / 1000;
    MoveToEx(dc, x1, y1, NULL);
    LineTo(dc, x2, y2);
    LineTo(dc, x1, y3);

    int п1 = x + р * 5625 / 10000;
    int п2 = x + р * 7266 / 10000;
    MoveToEx(dc, п1, y3, NULL);
    LineTo(dc, п2, y3);

    SelectObject(dc, ст_г);
    DeleteObject(глиф);
}

static void баннер(HDC dc, int ш, int в){
    RECT область = { 0, 0, ш, в };
    HBRUSH фон = CreateSolidBrush(ЦВ_БАННЕР);
    FillRect(dc, &область, фон);
    DeleteObject(фон);

    int р = S(88);
    int x = (ш - р) / 2;
    int y = S(72);
    знак(dc, x, y, р, ЦВ_СВЕТЛЫЙ, ЦВ_БАННЕР, S(7));

    надпись(dc, L"MS", 0, S(184), ш, S(46), ш_марка, ЦВ_СВЕТЛЫЙ,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    надпись(dc, L"русский язык", 0, S(232), ш, S(22), ш_марка2, ЦВ_БАННЕР_ТИХ,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    надпись(dc, L"Mazing Studio", S(20), в - S(52), ш - S(40), S(18),
            ш_марка3, ЦВ_БАННЕР_ТИХ, DT_LEFT | DT_SINGLELINE);
    надпись(dc, L"mazing-studio.site", S(20), в - S(34), ш - S(40), S(18),
            ш_марка3, ЦВ_БАННЕР_ТИХ, DT_LEFT | DT_SINGLELINE);
}

static void стрелка(HDC dc, int x, int y, int р, COLORREF ц){
    HPEN перо = CreatePen(PS_SOLID, S(2), ц);
    HGDIOBJ ст = SelectObject(dc, перо);
    int сер = y + р / 2;
    MoveToEx(dc, x, сер, NULL);
    LineTo(dc, x + р - S(4), сер);
    MoveToEx(dc, x + р - S(8), сер - S(4), NULL);
    LineTo(dc, x + р - S(3), сер);
    LineTo(dc, x + р - S(8), сер + S(4));
    SelectObject(dc, ст);
    DeleteObject(перо);
}

static void нарисовать_ссылки(HDC dc){
    for (int i = 0; i < г_ссылок; i++) {
        RECT *м = &г_ссылки[i].место;
        COLORREF ц = (i == г_наведение) ? ЦВ_ССЫЛКА_НАВ : ЦВ_ССЫЛКА;
        стрелка(dc, м->left, м->top + S(5), S(18), ц);
        надпись(dc, г_ссылки[i].заголовок, м->left + S(28), м->top,
                м->right - м->left - S(28), S(26), ш_ссылка, ц,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        int y = м->top + S(26) + S(6);
        for (int к = 0; к < г_ссылки[i].строк; к++) {
            надпись(dc, г_ссылки[i].строки[к], м->left + S(28), y,
                    м->right - м->left - S(28), S(17), ш_текст, ЦВ_СЕРЫЙ,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            y += S(17);
        }
    }
}

static void страница_главная(HDC dc, RECT кл){
    int лев = S(220), прав = кл.right - S(20), ш = прав - лев;

    if (г_режим == 0) {
        надпись(dc, L"Установка MS 1.1", лев, S(22), ш, S(44),
                ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        надпись(dc, L"Нажмите «Установить сейчас», чтобы поставить MS со стандартными настройками, "
                    L"или «Настроить установку», чтобы выбрать папку и компоненты.",
                лев, S(72), ш, S(76), ш_текст, ЦВ_СЕРЫЙ, DT_LEFT | DT_WORDBREAK);
        нарисовать_ссылки(dc);
    } else {
        надпись(dc, L"Удаление MS 1.1", лев, S(22), ш, S(44),
                ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        надпись(dc, L"MS будет удалён с этого компьютера: движок, ярлыки, настройки "
                    L"и связь с файлами .ms.\n\nВаши файлы .ms останутся на месте.",
                лев, S(80), ш, S(110), ш_текст, ЦВ_СЕРЫЙ, DT_LEFT | DT_WORDBREAK);
        надпись(dc, г_папка, лев, S(200), ш, S(40), ш_мелкий, ЦВ_СЕРЫЙ,
                DT_LEFT | DT_WORDBREAK | DT_END_ELLIPSIS);
    }
}

static void страница_настройка(HDC dc, RECT кл){
    int лев = S(220), прав = кл.right - S(20), ш = прав - лев;

    надпись(dc, L"Дополнительные параметры", лев, S(22), ш, S(44),
            ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    надпись(dc, L"Выберите папку установки и компоненты. По умолчанию всё уже выбрано — "
                L"можно сразу нажать «Установить».",
            лев, S(72), ш, S(44), ш_текст, ЦВ_СЕРЫЙ, DT_LEFT | DT_WORDBREAK);

    надпись(dc, L"Папка установки:", лев, S(118), ш, S(20),
            ш_текст, ЦВ_ТЕКСТ, DT_LEFT | DT_SINGLELINE);
    надпись(dc, L"Займёт около 1 МБ на диске.", лев, S(168), ш, S(20),
            ш_мелкий, ЦВ_СЕРЫЙ, DT_LEFT | DT_SINGLELINE);
}

static void страница_процесс(HDC dc, RECT кл){
    int лев = S(220), прав = кл.right - S(20), ш = прав - лев;

    надпись(dc, (г_режим == 1) ? L"Удаление MS 1.1" : L"Установка MS 1.1", лев, S(22), ш, S(44),
            ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    надпись(dc, (г_режим == 1) ? L"Удаление:" : L"Установка:", лев, S(118), ш, S(20),
            ш_текст, ЦВ_СЕРЫЙ, DT_LEFT | DT_SINGLELINE);

    const wchar_t *статус = (г_статус >= 0 && г_статус < (int)(sizeof(СТАТУСЫ)/sizeof(СТАТУСЫ[0])))
                            ? СТАТУСЫ[г_статус] : L"";
    надпись(dc, статус, лев, S(140), ш - S(60), S(24),
            ш_жирный, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    wchar_t процент[16];
    формат(процент, 16, L"%d%%", г_прогресс);
    надпись(dc, процент, прав - S(60), S(140), S(60), S(24),
            ш_текст, ЦВ_СЕРЫЙ, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    RECT полоса;
    полоса.left = лев;
    полоса.top = S(182);
    полоса.right = прав;
    полоса.bottom = S(206);

    HBRUSH рамка = CreateSolidBrush(ЦВ_ЛИНИЯ);
    FrameRect(dc, &полоса, рамка);
    DeleteObject(рамка);

    RECT внутр;
    внутр.left = полоса.left + 1;
    внутр.top = полоса.top + 1;
    внутр.right = полоса.right - 1;
    внутр.bottom = полоса.bottom - 1;

    HBRUSH фон = CreateSolidBrush(RGB(231, 232, 232));
    FillRect(dc, &внутр, фон);
    DeleteObject(фон);

    int ширина = внутр.right - внутр.left;
    int заполнено = ширина * г_прогресс / 100;
    if (заполнено > 0) {
        RECT зал = { внутр.left, внутр.top, внутр.left + заполнено, внутр.bottom };
        HBRUSH кисть = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(dc, &зал, кисть);
        DeleteObject(кисть);
    }
}

static void страница_готово(HDC dc, RECT кл){
    int лев = S(220), прав = кл.right - S(20), ш = прав - лев;

    if (г_режим == 0) {
        надпись(dc, L"Установка завершена", лев, S(22), ш, S(44),
                ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        wchar_t итог[1024];
        формат(итог, 1024,
               L"MS 1.1 установлен в папку:\n%s\n\n"
               L"В командной строке наберите ms — язык запустится.\n"
               L"«Центр MS» — расширения, настройки и учебник.\n"
               L"Файлы .ms открываются двойным щелчком мыши.",
               г_папка);
        надпись(dc, итог, лев, S(76), ш, S(120), ш_текст, ЦВ_ТЕКСТ,
                DT_LEFT | DT_WORDBREAK);
        нарисовать_ссылки(dc);
    } else {
        надпись(dc, L"Удаление завершено", лев, S(22), ш, S(44),
                ш_заголовок, ЦВ_ТЕКСТ, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        надпись(dc, L"MS удалён с компьютера.\n\n"
                    L"Спасибо, что попробовали MS. Ваши файлы .ms остались на месте.",
                лев, S(80), ш, S(100), ш_текст, ЦВ_ТЕКСТ, DT_LEFT | DT_WORDBREAK);
    }
}

static void рисовать(HDC dc, RECT кл){
    HBRUSH фон = CreateSolidBrush(ЦВ_ФОН);
    FillRect(dc, &кл, фон);
    DeleteObject(фон);

    int шб = S(БАННЕР_Ш);
    баннер(dc, шб, кл.bottom);

    HBRUSH линия = CreateSolidBrush(ЦВ_ЛИНИЯ);
    RECT р = { шб, 0, шб + 1, кл.bottom };
    FillRect(dc, &р, линия);
    DeleteObject(линия);

    разложить();

    switch (г_страница) {
        case PAGE_WELCOME:  страница_главная(dc, кл); break;
        case PAGE_CUSTOM:   страница_настройка(dc, кл); break;
        case PAGE_PROGRESS: страница_процесс(dc, кл); break;
        case PAGE_DONE:     страница_готово(dc, кл); break;
    }

    {
        int лев2 = шб + S(20), прав2 = кл.right - S(18);
        HBRUSH л = CreateSolidBrush(ЦВ_ЛИНИЯ);
        RECT черта = { лев2, S(70), прав2, S(71) };
        FillRect(dc, &черта, л);
        DeleteObject(л);
        надпись(dc, L"Windows 7–11 · без прав администратора · mazing-studio.site",
                лев2, кл.bottom - S(50), прав2 - лев2 - S(200), S(18),
                ш_мелкий, ЦВ_СЕРЫЙ, DT_LEFT | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
}

/* ---------- окно ---------- */

static HWND элемент(const wchar_t *класс, const wchar_t *текст, DWORD стиль, int ид, DWORD расширенный){
    HWND э = CreateWindowExW(расширенный, класс, текст, WS_CHILD | стиль,
                             0, 0, 10, 10, г_окно, (HMENU)(INT_PTR)ид, г_инст, NULL);
    if (э) SendMessageW(э, WM_SETFONT, (WPARAM)ш_текст, TRUE);
    return э;
}

static void создать_элементы(void){
    э_путь = элемент(L"EDIT", L"", ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP, ID_PATH, WS_EX_CLIENTEDGE);
    к_обзор = элемент(L"BUTTON", L"Обзор…", BS_OWNERDRAW | WS_TABSTOP, ID_BROWSE, 0);
    к_пуск = элемент(L"BUTTON", L"Создать ярлык в меню «Пуск»",
                     BS_AUTOCHECKBOX | WS_TABSTOP, ID_CM_START, 0);
    к_стол = элемент(L"BUTTON", L"Создать ярлык на рабочем столе",
                     BS_AUTOCHECKBOX | WS_TABSTOP, ID_CM_DESKTOP, 0);
    к_ассоц = элемент(L"BUTTON", L"Открывать файлы .ms двойным щелчком",
                      BS_AUTOCHECKBOX | WS_TABSTOP, ID_CM_ASSOC, 0);
    к_path = элемент(L"BUTTON", L"Добавить команду ms в PATH",
                     BS_AUTOCHECKBOX | WS_TABSTOP, ID_CM_PATH, 0);
    к_path2 = элемент(L"BUTTON", L"Добавить ms в PATH",
                      BS_AUTOCHECKBOX | WS_TABSTOP, ID_CM_PATH2, 0);

    б_основная = элемент(L"BUTTON", L"Отмена", BS_OWNERDRAW | WS_TABSTOP, ID_MAIN, 0);
    б_назад = элемент(L"BUTTON", L"Назад", BS_OWNERDRAW | WS_TABSTOP, ID_BACK, 0);
}

static void считать_настройки(void){
    if (г_страница == PAGE_WELCOME) {
        г_галочка_path = SendMessageW(к_path2, BM_GETCHECK, 0, 0) == BST_CHECKED;
        return;
    }
    if (г_страница != PAGE_CUSTOM) return;

    wchar_t путь[MAX_PATH];
    GetWindowTextW(э_путь, путь, MAX_PATH);
    почистить_путь(путь);
    if (путь[0]) {
        wcsncpy(г_папка, путь, MAX_PATH - 1);
        г_папка[MAX_PATH - 1] = 0;
    }

    г_галочка_пуск = SendMessageW(к_пуск, BM_GETCHECK, 0, 0) == BST_CHECKED;
    г_галочка_стол = SendMessageW(к_стол, BM_GETCHECK, 0, 0) == BST_CHECKED;
    г_галочка_ассоц = SendMessageW(к_ассоц, BM_GETCHECK, 0, 0) == BST_CHECKED;
    г_галочка_path = SendMessageW(к_path, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static void начать(int режим){
    г_режим = режим;
    г_страница = PAGE_PROGRESS;
    г_прогресс = 0;
    г_цель = 0;
    г_статус = ST_PREP;
    г_ошибка[0] = 0;
    обновить_страницу();
    SetTimer(г_окно, 1, 25, NULL);

    if (режим == 0) г_поток = CreateThread(NULL, 0, работник_установки, NULL, 0, NULL);
    else г_поток = CreateThread(NULL, 0, работник_удаления, NULL, 0, NULL);
}

static void выполнить_ссылку(int действие){
    if (действие == 1) {
        считать_настройки();
        начать(0);
    } else if (действие == 2) {
        считать_настройки();
        г_страница = PAGE_CUSTOM;
        обновить_страницу();
    } else if (действие == 3) {
        ShellExecuteW(NULL, L"open", г_папка, NULL, NULL, SW_SHOWNORMAL);
    }
}

static int ссылка_по_точке(int x, int y){
    POINT т;
    т.x = x;
    т.y = y;
    for (int i = 0; i < г_ссылок; i++)
        if (PtInRect(&г_ссылки[i].место, т)) return г_ссылки[i].действие;
    return 0;
}

static void кнопка_рисовать(const DRAWITEMSTRUCT *д){
    HDC dc = д->hDC;
    RECT р = д->rcItem;
    int основная = (д->CtlID == ID_MAIN);
    int выключена = (д->itemState & ODS_DISABLED) != 0;

    COLORREF фон, текст, рамка;
    if (основная) {
        фон = выключена ? RGB(206, 207, 207) : RGB(0, 0, 0);
        текст = выключена ? RGB(140, 142, 141) : ЦВ_СВЕТЛЫЙ;
        рамка = фон;
    } else {
        фон = ЦВ_ФОН;
        текст = выключена ? RGB(150, 152, 151) : ЦВ_ТЕКСТ;
        рамка = выключена ? RGB(206, 207, 207) : RGB(168, 170, 169);
    }
    if (!выключена && (д->itemState & ODS_SELECTED))
        фон = основная ? ЦВ_ТЕКСТ : RGB(229, 230, 229);

    int радиус = р.bottom - р.top;
    HBRUSH кисть = CreateSolidBrush(фон);
    HPEN перо = CreatePen(PS_SOLID, 1, основная ? фон : рамка);
    HGDIOBJ ст_к = SelectObject(dc, кисть);
    HGDIOBJ ст_п = SelectObject(dc, перо);
    RoundRect(dc, р.left, р.top, р.right, р.bottom, радиус, радиус);
    SelectObject(dc, ст_к);
    SelectObject(dc, ст_п);
    DeleteObject(кисть);
    DeleteObject(перо);

    wchar_t подпись[128];
    GetWindowTextW(д->hwndItem, подпись, 128);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, текст);
    HFONT старый = SelectObject(dc, ш_жирный);
    DrawTextW(dc, подпись, -1, &р, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(dc, старый);

    if (д->itemState & ODS_FOCUS) {
        RECT фокус = р;
        InflateRect(&фокус, -S(5), -S(4));
        DrawFocusRect(dc, &фокус);
    }
}

static LRESULT CALLBACK окно(HWND h, UINT м, WPARAM w, LPARAM l){
    switch (м) {
    case WM_CREATE: {
        г_окно = h;
        HDC dc = GetDC(h);
        г_dpi = (UINT)GetDeviceCaps(dc, LOGPIXELSY);
        ReleaseDC(h, dc);
        if (г_dpi < 96) г_dpi = 96;

        г_кисть_фон = CreateSolidBrush(ЦВ_ФОН);
        ш_заголовок = шрифт(L"Segoe UI", 21, FW_SEMIBOLD);
        ш_текст = шрифт(L"Segoe UI", 10, FW_NORMAL);
        ш_ссылка = шрифт(L"Segoe UI", 12, FW_NORMAL);
        ш_мелкий = шрифт(L"Segoe UI", 8, FW_NORMAL);
        ш_жирный = шрифт(L"Segoe UI", 10, FW_SEMIBOLD);
        ш_лого = шрифт(L"Segoe UI", 34, FW_BOLD);
        ш_марка = шрифт(L"Segoe UI", 26, FW_BOLD);
        ш_марка2 = шрифт(L"Segoe UI", 11, FW_NORMAL);
        ш_марка3 = шрифт(L"Segoe UI", 7, FW_NORMAL);

        создать_элементы();
        обновить_страницу();
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT пс;
        HDC dc = BeginPaint(h, &пс);
        RECT кл;
        GetClientRect(h, &кл);
        if (кл.right > 0 && кл.bottom > 0) {
            HDC пам = CreateCompatibleDC(dc);
            HBITMAP бм = CreateCompatibleBitmap(dc, кл.right, кл.bottom);
            HGDIOBJ старый = SelectObject(пам, бм);
            рисовать(пам, кл);
            BitBlt(dc, 0, 0, кл.right, кл.bottom, пам, 0, 0, SRCCOPY);
            SelectObject(пам, старый);
            DeleteObject(бм);
            DeleteDC(пам);
        }
        EndPaint(h, &пс);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC dc = (HDC)w;
        SetBkColor(dc, ЦВ_ФОН);
        SetTextColor(dc, ЦВ_ТЕКСТ);
        return (LRESULT)г_кисть_фон;
    }

    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT *д = (const DRAWITEMSTRUCT*)l;
        if (д && (д->CtlID == ID_MAIN || д->CtlID == ID_BACK || д->CtlID == ID_BROWSE)) {
            кнопка_рисовать(д);
            return TRUE;
        }
        break;
    }

    case WM_MOUSEMOVE: {
        POINT т;
        т.x = GET_X_LPARAM(l);
        т.y = GET_Y_LPARAM(l);
        int новый = -1;
        for (int i = 0; i < г_ссылок; i++)
            if (PtInRect(&г_ссылки[i].место, т)) { новый = i; break; }
        if (новый != г_наведение) {
            г_наведение = новый;
            InvalidateRect(h, NULL, FALSE);
        }
        SetCursor(LoadCursorW(NULL, новый >= 0 ? IDC_HAND : IDC_ARROW));
        return 0;
    }

    case WM_SETCURSOR:
        if (LOWORD(l) == HTCLIENT) return TRUE;
        break;

    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(l);
        int y = GET_Y_LPARAM(l);
        разложить();
        int действие = ссылка_по_точке(x, y);
        if (действие) выполнить_ссылку(действие);
        return 0;
    }

    case WM_COMMAND: {
        int ид = LOWORD(w);
        int код = HIWORD(w);
        if (ид == ID_MAIN && код == BN_CLICKED) {
            if (г_страница == PAGE_WELCOME) {
                if (г_режим == 1) {
                    считать_настройки();
                    начать(1);
                } else {
                    PostMessageW(h, WM_CLOSE, 0, 0);
                }
            } else if (г_страница == PAGE_CUSTOM) {
                считать_настройки();
                if (!путь_годен(г_папка)) {
                    MessageBoxW(h, L"Укажите полный путь к папке установки, например:\n"
                                   L"C:\\Users\\Имя\\MS",
                                L"MS", MB_OK | MB_ICONWARNING);
                    SetFocus(э_путь);
                    return 0;
                }
                начать(0);
            } else if (г_страница == PAGE_DONE) {
                PostMessageW(h, WM_CLOSE, 0, 0);
            }
            return 0;
        }
        if (ид == ID_BACK && код == BN_CLICKED) {
            if (г_страница == PAGE_CUSTOM) {
                считать_настройки();
                г_страница = PAGE_WELCOME;
                обновить_страницу();
            } else {
                PostMessageW(h, WM_CLOSE, 0, 0);
            }
            return 0;
        }
        if (ид == ID_BROWSE && код == BN_CLICKED) {
            wchar_t путь[MAX_PATH];
            GetWindowTextW(э_путь, путь, MAX_PATH);
            почистить_путь(путь);
            if (выбрать_папку(путь, MAX_PATH))
                SetWindowTextW(э_путь, путь);
            return 0;
        }
        break;
    }

    case WM_STEP:
        г_цель = (int)w;
        г_статус = (int)l;
        if (г_цель > 0 && г_цель > г_прогресс && !IsWindowVisible(г_окно)) г_прогресс = г_цель;
        InvalidateRect(h, NULL, FALSE);
        return 0;

    case WM_TIMER:
        if (г_страница == PAGE_PROGRESS && г_прогресс < г_цель) {
            г_прогресс += 1;
            InvalidateRect(h, NULL, FALSE);
        }
        return 0;

    case WM_DONE: {
        KillTimer(h, 1);
        int причина = (int)w;
        if (г_поток) { CloseHandle(г_поток); г_поток = NULL; }
        if (причина == 0) {
            г_прогресс = 100;
            г_страница = PAGE_DONE;
            обновить_страницу();
        } else {
            MessageBoxW(h, г_ошибка[0] ? г_ошибка : L"Не удалось выполнить операцию.",
                        L"MS", MB_OK | MB_ICONERROR);
            г_страница = PAGE_WELCOME;
            обновить_страницу();
        }
        return 0;
    }

    case WM_CLOSE:
        if (г_страница == PAGE_PROGRESS) return 0;
        DestroyWindow(h);
        return 0;

    case WM_DESTROY:
        KillTimer(h, 1);
        DeleteObject(ш_заголовок);
        DeleteObject(ш_текст);
        DeleteObject(ш_ссылка);
        DeleteObject(ш_мелкий);
        DeleteObject(ш_жирный);
        DeleteObject(ш_лого);
        DeleteObject(ш_марка);
        DeleteObject(ш_марка2);
        DeleteObject(ш_марка3);
        DeleteObject(г_кисть_фон);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, м, w, l);
}

/* ---------- тихий режим ---------- */

static int тихая_установка(const wchar_t *папка){
    if (папка && папка[0]) {
        wcsncpy(г_папка, папка, MAX_PATH - 1);
        г_папка[MAX_PATH - 1] = 0;
    }
    г_галочка_пуск = 1;
    г_галочка_стол = 0;
    г_галочка_ассоц = 1;
    г_галочка_path = 1;
    return выполнить_установку();
}

/* ---------- точка входа ---------- */

int APIENTRY WinMain(HINSTANCE инст, HINSTANCE пред, LPSTR строка, int показ){
    (void)пред; (void)строка; (void)показ;
    г_инст = инст;

    int режим = 0, тихо = 0;
    const wchar_t *путь_арг = NULL;

    int арг_количество = 0;
    LPWSTR *арг = CommandLineToArgvW(GetCommandLineW(), &арг_количество);
    if (арг) {
        for (int i = 1; i < арг_количество; i++) {
            const wchar_t *а = арг[i];
            if (!_wcsicmp(а, L"/uninstall") || !_wcsicmp(а, L"/удалить") || !_wcsicmp(а, L"-u"))
                режим = 1;
            else if (!_wcsicmp(а, L"/s") || !_wcsicmp(а, L"/silent") || !_wcsicmp(а, L"/тихо"))
                тихо = 1;
            else if (!wcsncmp(а, L"/D=", 3) || !wcsncmp(а, L"/d=", 3))
                путь_арг = а + 3;
        }
    }

    г_режим = режим;
    папка_по_умолчанию(г_папка, MAX_PATH);
    if (путь_арг && путь_арг[0]) {
        wcsncpy(г_папка, путь_арг, MAX_PATH - 1);
        г_папка[MAX_PATH - 1] = 0;
        почистить_путь(г_папка);
    }

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    if (режим == 1) определить_папку_удаления();

    if (тихо) {
        int ок;
        if (режим == 1) ок = выполнить_удаление();
        else ок = тихая_установка(NULL);
        CoUninitialize();
        if (арг) LocalFree(арг);
        return ок ? 0 : 1;
    }

    WNDCLASSEXW кл;
    ZeroMemory(&кл, sizeof(кл));
    кл.cbSize = sizeof(кл);
    кл.style = CS_HREDRAW | CS_VREDRAW;
    кл.lpfnWndProc = окно;
    кл.hInstance = инст;
    кл.hCursor = LoadCursorW(NULL, IDC_ARROW);
    кл.hIcon = LoadIconW(инст, MAKEINTRESOURCEW(1));
    кл.hIconSm = LoadIconW(инст, MAKEINTRESOURCEW(1));
    if (!кл.hIcon) кл.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    if (!кл.hIconSm) кл.hIconSm = кл.hIcon;
    кл.lpszClassName = L"MS_Setup_Window";
    if (!RegisterClassExW(&кл)) {
        CoUninitialize();
        if (арг) LocalFree(арг);
        return 1;
    }

    DWORD стиль = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT рамка;
    рамка.left = 0; рамка.top = 0;
    рамка.right = MulDiv(ОКНО_Ш, (int)г_dpi, 96);
    рамка.bottom = MulDiv(ОКНО_В, (int)г_dpi, 96);
    AdjustWindowRectEx(&рамка, стиль, FALSE, 0);

    int ш = рамка.right - рамка.left;
    int в = рамка.bottom - рамка.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - ш) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - в) / 2;

    HWND окно_главное = CreateWindowExW(0, L"MS_Setup_Window",
        (режим == 1) ? L"Удаление MS 1.1" : L"Установка MS 1.1",
        стиль, x, y, ш, в, NULL, NULL, инст, NULL);
    if (!окно_главное) {
        CoUninitialize();
        if (арг) LocalFree(арг);
        return 1;
    }

    ShowWindow(окно_главное, SW_SHOW);
    UpdateWindow(окно_главное);

    MSG сообщение;
    while (GetMessageW(&сообщение, NULL, 0, 0) > 0) {
        TranslateMessage(&сообщение);
        DispatchMessageW(&сообщение);
    }

    CoUninitialize();
    if (арг) LocalFree(арг);
    return 0;
}
