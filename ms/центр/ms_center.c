/*
 * MS Центр — графическое управление языком MS 1.1.
 * Вкладки: Расширения, Настройки, Учебник, О программе.
 * Это фундамент: часть возможностей появится позже.
 */

#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define _WIN32_IE 0x0700
#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <shlobj.h>
#include <commdlg.h>
#include <winhttp.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define ID_TAB        100
#define ID_LIST       101
#define ID_STATUS     102
#define ID_EDITOR     103
#define ID_OUTPUT     104

#define ID_B_REFRESH  2001
#define ID_B_DOWNLOAD 2002
#define ID_B_ENABLE   2003
#define ID_B_DISABLE  2004
#define ID_B_DELETE   2005
#define ID_B_FOLDER   2006
#define ID_B_DOWNLOAD_WEB 2007
#define ID_B_CAT_REFRESH 2008
#define ID_B_CAT_GET     2009
#define ID_LIST_CAT      110

#define ID_C_ENCODING 2010
#define ID_C_INDENT   2011
#define ID_C_THEME    2012
#define ID_C_STRICT   2013
#define ID_B_SAVE     2020
#define ID_B_RESET    2021

#define ID_B_SITE1    2030
#define ID_B_EXAMPLES 2031
#define ID_B_SITE2    2032

#define ID_B_OPEN     2040
#define ID_B_SAVEFILE 2041
#define ID_B_RUN      2042
#define ID_B_SAMPLE   2043

#define ID_M_OPEN      3001
#define ID_M_SAVE      3002
#define ID_M_SAVEAS    3003
#define ID_M_RUN       3004
#define ID_M_EXIT      3005
#define ID_M_UNDO      3010
#define ID_M_CUT       3011
#define ID_M_COPY      3012
#define ID_M_PASTE     3013
#define ID_M_SELALL    3014
#define ID_M_LIGHT     3020
#define ID_M_DARK      3021
#define ID_M_CLASSIC   3022
#define ID_M_TAB0      3030
#define ID_M_SITE      3040
#define ID_M_ABOUT     3041

static int г_тема = 0;
static COLORREF г_ц_фон, г_ц_шапка, г_ц_текст, г_ц_серый, г_ц_линия, г_ц_поле;

static HINSTANCE г_инст;
static HWND г_окно, г_таб, г_список, г_список_каталога, г_статус, г_редактор, г_вывод;

static LRESULT CALLBACK шапка_проц(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static void заполнить_каталог(void);
static void скачать_каталог(void);
static void скачать_из_каталога(void);
static HACCEL г_акс;
static wchar_t г_файл_редактора[MAX_PATH];
static int г_отступ = 4;
static HFONT г_шрифт, г_шрифт_жирный, г_шрифт_лого, г_шрифт_заголовок, г_шрифт_мелкий, г_шрифт_код;
static HBRUSH г_кисть_фон, г_кисть_белая, г_кисть_шапка;
static int г_вкладка = 0;
static HACCEL г_акс = NULL;

static LRESULT CALLBACK вкладка_проц(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static LRESULT CALLBACK редактор_проц(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static int загрузить_в_редактор(const wchar_t *путь);

static wchar_t г_каталог[MAX_PATH];
static wchar_t г_папка_расширений[MAX_PATH];
static wchar_t г_ини[MAX_PATH];

#define МАКС_ЭЛ 24
static HWND г_эл[5][МАКС_ЭЛ];
static int г_кол[5] = {0, 0, 0, 0, 0};

static void доб(int стр, HWND h) {
    if (стр >= 0 && стр < 5 && г_кол[стр] < МАКС_ЭЛ)
        г_эл[стр][г_кол[стр]++] = h;
}

static void открыть_url(const wchar_t *url) {
    ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);
}

static void создать_пути(void) {
    wchar_t база[MAX_PATH] = L"";
    if (!GetEnvironmentVariableW(L"APPDATA", база, MAX_PATH) || !база[0])
        GetEnvironmentVariableW(L"LOCALAPPDATA", база, MAX_PATH);
    _snwprintf(г_каталог, MAX_PATH, L"%s\\MazingStudio\\MS", база);
    _snwprintf(г_папка_расширений, MAX_PATH, L"%s\\расширения", г_каталог);
    _snwprintf(г_ини, MAX_PATH, L"%s\\настройки.ini", г_каталог);
    г_каталог[MAX_PATH - 1] = 0;
    г_папка_расширений[MAX_PATH - 1] = 0;
    г_ини[MAX_PATH - 1] = 0;
    SHCreateDirectoryExW(NULL, г_папка_расширений, NULL);
}

/* ---------- настройки ---------- */

static void выбрать(HWND c, int значение) {
    int н = (int)SendMessageW(c, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < н; i++) {
        if ((int)SendMessageW(c, CB_GETITEMDATA, i, 0) == значение) {
            SendMessageW(c, CB_SETCURSEL, i, 0);
            return;
        }
    }
    SendMessageW(c, CB_SETCURSEL, 0, 0);
}

static int выбранное(HWND c) {
    int i = (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
    if (i < 0) return 0;
    return (int)SendMessageW(c, CB_GETITEMDATA, i, 0);
}

static int прочитать_тему(void) {
    wchar_t т[32] = L"light";
    GetPrivateProfileStringW(L"ms", L"theme", L"light", т, 32, г_ини);
    if (!_wcsicmp(т, L"dark")) return 1;
    if (!_wcsicmp(т, L"classic")) return 2;
    return 0;
}

static void задать_цвета(int тема) {
    г_тема = тема;
    if (тема == 1) {
        г_ц_фон = RGB(27, 28, 27);
        г_ц_шапка = RGB(36, 38, 37);
        г_ц_текст = RGB(241, 242, 242);
        г_ц_серый = RGB(166, 168, 167);
        г_ц_линия = RGB(58, 60, 59);
        г_ц_поле = RGB(34, 36, 35);
    } else if (тема == 2) {
        г_ц_фон = RGB(255, 255, 255);
        г_ц_шапка = RGB(220, 220, 220);
        г_ц_текст = RGB(0, 0, 0);
        г_ц_серый = RGB(85, 85, 85);
        г_ц_линия = RGB(160, 160, 160);
        г_ц_поле = RGB(255, 255, 255);
    } else {
        г_ц_фон = RGB(255, 255, 255);
        г_ц_шапка = RGB(241, 242, 242);
        г_ц_текст = RGB(42, 44, 43);
        г_ц_серый = RGB(110, 112, 111);
        г_ц_линия = RGB(216, 217, 217);
        г_ц_поле = RGB(255, 255, 255);
    }
}

static void пересоздать_кисти(void) {
    if (г_кисть_фон) DeleteObject(г_кисть_фон);
    if (г_кисть_белая) DeleteObject(г_кисть_белая);
    if (г_кисть_шапка) DeleteObject(г_кисть_шапка);
    г_кисть_фон = CreateSolidBrush(г_ц_фон);
    г_кисть_белая = CreateSolidBrush(г_ц_поле);
    г_кисть_шапка = CreateSolidBrush(г_ц_шапка);
}

static void перекрасить_детей(void);

static void применить_тему(void) {
    перекрасить_детей();
    if (г_список) {
        ListView_SetBkColor(г_список, г_ц_поле);
        ListView_SetTextBkColor(г_список, г_ц_поле);
        ListView_SetTextColor(г_список, г_ц_текст);
        InvalidateRect(г_список, NULL, TRUE);
    }
    if (г_окно) {
        InvalidateRect(г_окно, NULL, TRUE);
        DrawMenuBar(г_окно);
    }
    if (г_таб) InvalidateRect(г_таб, NULL, TRUE);
}

static void загрузить_настройки_в_интерфейс(void) {
    HWND энк = GetDlgItem(г_окно, ID_C_ENCODING);
    HWND отст = GetDlgItem(г_окно, ID_C_INDENT);
    HWND тема = GetDlgItem(г_окно, ID_C_THEME);
    HWND строг = GetDlgItem(г_окно, ID_C_STRICT);
    if (!энк) return;

    выбрать(энк, (int)GetPrivateProfileIntW(L"ms", L"encoding", 65001, г_ини));
    выбрать(отст, (int)GetPrivateProfileIntW(L"ms", L"indent", 4, г_ини));
    выбрать(тема, прочитать_тему());
    выбрать(строг, (int)GetPrivateProfileIntW(L"ms", L"strict", 1, г_ини) ? 1 : 0);
}

static void сохранить_настройки(void) {
    int энк = выбранное(GetDlgItem(г_окно, ID_C_ENCODING));
    int отст = выбранное(GetDlgItem(г_окно, ID_C_INDENT));
    int тема = выбранное(GetDlgItem(г_окно, ID_C_THEME));
    int строг = выбранное(GetDlgItem(г_окно, ID_C_STRICT));
    const wchar_t *тема_имя = (тема == 1) ? L"dark" : ((тема == 2) ? L"classic" : L"light");
    wchar_t б[32];
    _snwprintf(б, 32, L"%d", энк);
    WritePrivateProfileStringW(L"ms", L"encoding", б, г_ини);
    _snwprintf(б, 32, L"%d", отст);
    WritePrivateProfileStringW(L"ms", L"indent", б, г_ини);
    WritePrivateProfileStringW(L"ms", L"theme", тема_имя, г_ини);
    _snwprintf(б, 32, L"%d", строг);
    WritePrivateProfileStringW(L"ms", L"strict", б, г_ини);

    задать_цвета(тема);
    пересоздать_кисти();
    применить_тему();
    г_отступ = отст;

    MessageBoxW(г_окно,
        L"Настройки сохранены.\n\n"
        L"Кодировка и строгость применятся при следующем запуске ms.exe.\n"
        L"Отступ используется редактором Центра, тема — оформлением Центра.",
        L"Центр MS", MB_OK | MB_ICONINFORMATION);
}

static void сбросить_настройки(void) {
    выбрать(GetDlgItem(г_окно, ID_C_ENCODING), 65001);
    выбрать(GetDlgItem(г_окно, ID_C_INDENT), 4);
    выбрать(GetDlgItem(г_окно, ID_C_THEME), 0);
    выбрать(GetDlgItem(г_окно, ID_C_STRICT), 1);
}

/* ---------- расширения ---------- */

static void обновить_расширения(void) {
    if (!г_список) return;

    wchar_t выбрано[MAX_PATH] = L"";
    int был_выбор = ListView_GetNextItem(г_список, -1, LVNI_SELECTED);
    if (был_выбор >= 0)
        ListView_GetItemText(г_список, был_выбор, 0, выбрано, MAX_PATH);

    ListView_DeleteAllItems(г_список);

    wchar_t шаблон[MAX_PATH];
    _snwprintf(шаблон, MAX_PATH, L"%s\\*", г_папка_расширений);
    WIN32_FIND_DATAW д;
    HANDLE ф = FindFirstFileW(шаблон, &д);
    int всего = 0;
    if (ф != INVALID_HANDLE_VALUE) {
        do {
            if (д.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            size_t дл = wcslen(д.cFileName);
            size_t отрезать = 0;
            int включено = 0;
            if (дл > 7 && !_wcsicmp(д.cFileName + дл - 7, L".ms.off")) { отрезать = дл - 7; включено = 0; }
            else if (дл > 3 && !_wcsicmp(д.cFileName + дл - 3, L".ms")) { отрезать = дл - 3; включено = 1; }
            else continue;

            wchar_t база[MAX_PATH];
            if (отрезать >= MAX_PATH) отрезать = MAX_PATH - 1;
            wcsncpy(база, д.cFileName, отрезать);
            база[отрезать] = 0;

            LVITEMW it;
            ZeroMemory(&it, sizeof(it));
            it.mask = LVIF_TEXT | LVIF_PARAM;
            it.iItem = всего;
            it.pszText = база;
            it.lParam = включено;
            int idx = (int)ListView_InsertItem(г_список, &it);
            ListView_SetItemText(г_список, idx, 1, (LPWSTR)(включено ? L"Включено" : L"Выключено"));
            ListView_SetItemText(г_список, idx, 2, д.cFileName);
            всего++;
        } while (FindNextFileW(ф, &д));
        FindClose(ф);
    }

    if (выбрано[0]) {
        for (int i = 0; i < всего; i++) {
            wchar_t имя[MAX_PATH];
            ListView_GetItemText(г_список, i, 0, имя, MAX_PATH);
            if (!_wcsicmp(имя, выбрано)) {
                ListView_SetItemState(г_список, i,
                    LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
                ListView_EnsureVisible(г_список, i, FALSE);
                break;
            }
        }
    }

    wchar_t б[600];
    _snwprintf(б, 600, L"Расширений: %d. Папка: %s", всего, г_папка_расширений);
    SetWindowTextW(г_статус, б);
}

static int выбранное_расширение(wchar_t *база, size_t размер, int *включено) {
    int sel = ListView_GetNextItem(г_список, -1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(г_окно, L"Сначала выберите расширение в списке.", L"Центр MS", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    ListView_GetItemText(г_список, sel, 0, база, (int)размер);
    LVITEMW it;
    ZeroMemory(&it, sizeof(it));
    it.mask = LVIF_PARAM;
    it.iItem = sel;
    ListView_GetItem(г_список, &it);
    *включено = (int)it.lParam;
    return 1;
}

static void переключить_расширение(int включить) {
    wchar_t база[MAX_PATH];
    int включено = 0;
    if (!выбранное_расширение(база, MAX_PATH, &включено)) return;
    if (включить && включено) return;
    if (!включить && !включено) return;

    wchar_t старый[MAX_PATH * 2], новый[MAX_PATH * 2];
    if (включить) {
        _snwprintf(старый, MAX_PATH * 2, L"%s\\%s.ms.off", г_папка_расширений, база);
        _snwprintf(новый, MAX_PATH * 2, L"%s\\%s.ms", г_папка_расширений, база);
    } else {
        _snwprintf(старый, MAX_PATH * 2, L"%s\\%s.ms", г_папка_расширений, база);
        _snwprintf(новый, MAX_PATH * 2, L"%s\\%s.ms.off", г_папка_расширений, база);
    }
    if (!MoveFileW(старый, новый)) {
        MessageBoxW(г_окно, L"Не удалось изменить состояние расширения.\n\n"
                             L"Возможно, файл занят или уже изменён.",
                    L"Центр MS", MB_OK | MB_ICONERROR);
    }
    обновить_расширения();
}

static void удалить_расширение(void) {
    wchar_t база[MAX_PATH];
    int включено = 0;
    if (!выбранное_расширение(база, MAX_PATH, &включено)) return;
    wchar_t путь[MAX_PATH * 2];
    _snwprintf(путь, MAX_PATH * 2, L"%s\\%s.%s", г_папка_расширений, база, включено ? L"ms" : L"ms.off");
    if (MessageBoxW(г_окно, L"Удалить расширение без возможности восстановления?",
                    L"Центр MS", MB_YESNO | MB_ICONWARNING) != IDYES)
        return;
    if (!DeleteFileW(путь)) {
        MessageBoxW(г_окно, L"Не удалось удалить файл расширения.", L"Центр MS", MB_OK | MB_ICONERROR);
    }
    обновить_расширения();
}

static int скачать_файл(const wchar_t *url, const wchar_t *путь) {
    URL_COMPONENTS ч;
    ZeroMemory(&ч, sizeof(ч));
    ч.dwStructSize = sizeof(ч);
    wchar_t хост[256] = L"";
    wchar_t путь_в_url[2048] = L"";
    ч.lpszHostName = хост;
    ч.dwHostNameLength = 256;
    ч.lpszUrlPath = путь_в_url;
    ч.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(url, 0, 0, &ч)) return 0;

    HINTERNET сессия = WinHttpOpen(L"MS-Center/1.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!сессия) return 0;
    WinHttpSetTimeouts(сессия, 8000, 8000, 15000, 15000);

    HINTERNET соединение = WinHttpConnect(сессия, ч.lpszHostName, ч.nPort, 0);
    if (!соединение) { WinHttpCloseHandle(сессия); return 0; }

    DWORD флаги = (ч.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET запрос = WinHttpOpenRequest(соединение, L"GET", ч.lpszUrlPath, NULL,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, флаги);
    if (!запрос) {
        WinHttpCloseHandle(соединение);
        WinHttpCloseHandle(сессия);
        return 0;
    }

    int ок = 0;
    if (WinHttpSendRequest(запрос, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(запрос, NULL)) {
        DWORD код = 0, разм = sizeof(код);
        WinHttpQueryHeaders(запрос, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &код, &разм, WINHTTP_NO_HEADER_INDEX);
        if (код >= 200 && код < 300) {
            HANDLE ф = CreateFileW(путь, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
            if (ф != INVALID_HANDLE_VALUE) {
                ок = 1;
                char буф[4096];
                DWORD прочитано = 0, записано = 0;
                for (;;) {
                    if (!WinHttpQueryDataAvailable(запрос, &прочитано)) { ок = 0; break; }
                    if (!прочитано) break;
                    if (прочитано > sizeof(буф)) прочитано = sizeof(буф);
                    if (!WinHttpReadData(запрос, буф, прочитано, &записано)) { ок = 0; break; }
                    if (!записано) break;
                    DWORD всего = 0;
                    if (!WriteFile(ф, буф, записано, &всего, NULL) || всего != записано) { ок = 0; break; }
                }
                CloseHandle(ф);
            }
        }
    }

    WinHttpCloseHandle(запрос);
    WinHttpCloseHandle(соединение);
    WinHttpCloseHandle(сессия);
    if (!ок) DeleteFileW(путь);
    return ок;
}

static void скачать_расширение_имя(const wchar_t *url, const wchar_t *имя_файла,
                                   const wchar_t *название, const wchar_t *команда) {
    wchar_t tmp[MAX_PATH * 2], dst[MAX_PATH * 2], вопрос[512], готово[512];
    _snwprintf(tmp, MAX_PATH * 2, L"%s\\%s.tmp", г_папка_расширений, имя_файла);
    _snwprintf(dst, MAX_PATH * 2, L"%s\\%s.ms", г_папка_расширений, имя_файла);

    if (GetFileAttributesW(dst) != INVALID_FILE_ATTRIBUTES) {
        _snwprintf(вопрос, 512, L"Расширение «%s» уже установлено.\n\nСкачать заново и заменить файл?", название);
        if (MessageBoxW(г_окно, вопрос, L"Центр MS", MB_YESNO | MB_ICONQUESTION) != IDYES)
            return;
    }

    DeleteFileW(tmp);
    if (!скачать_файл(url, tmp)) {
        DeleteFileW(tmp);
        MessageBoxW(г_окно,
            L"Не удалось скачать расширение с сайта.\n\n"
            L"Проверьте подключение к интернету или скачайте файл вручную:\n"
            L"mazing-studio.site/download/extensions/",
            L"Центр MS", MB_OK | MB_ICONERROR);
        return;
    }
    if (!MoveFileExW(tmp, dst, MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmp);
        MessageBoxW(г_окно, L"Файл скачан, но не удалось положить его в папку расширений.",
                    L"Центр MS", MB_OK | MB_ICONERROR);
        return;
    }

    обновить_расширения();
    _snwprintf(готово, 512, L"Расширение «%s» установлено.\n\nПодключите его в программе:\nподключить(\"%s\")",
               название, команда);
    MessageBoxW(г_окно, готово, L"Центр MS", MB_OK | MB_ICONINFORMATION);
}

static void скачать_веб(void) {
    скачать_расширение_имя(L"https://mazing-studio.site/download/extensions/web.ms",
                           L"веб", L"Веб", L"веб");
}

/* ---------- интерфейс ---------- */

static HWND кнопка(const wchar_t *т, int x, int y, int ш, int id, int стр) {
    HWND h = CreateWindowExW(0, L"BUTTON", т, WS_CHILD | BS_OWNERDRAW,
                             x, y, ш, 40, г_окно, (HMENU)(INT_PTR)id, г_инст, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    доб(стр, h);
    return h;
}

static HWND надпись(const wchar_t *т, int x, int y, int ш, int в, int стр, DWORD стиль) {
    HWND h = CreateWindowExW(0, L"STATIC", т, WS_CHILD | стиль,
                             x, y, ш, в, г_окно, NULL, г_инст, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    доб(стр, h);
    return h;
}

static HWND выпадающий(int x, int y, int ш, int id, int стр) {
    HWND h = CreateWindowExW(0, L"COMBOBOX", L"", WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
                             x, y, ш, 220, г_окно, (HMENU)(INT_PTR)id, г_инст, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    доб(стр, h);
    return h;
}

static void заполнить_список(HWND c, const wchar_t **тексты, const int *данные, int н) {
    for (int i = 0; i < н; i++) {
        int idx = (int)SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)тексты[i]);
        SendMessageW(c, CB_SETITEMDATA, idx, данные[i]);
    }
}

static void показать_страницу(int стр) {
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < г_кол[i]; j++)
            ShowWindow(г_эл[i][j], (i == стр) ? SW_SHOW : SW_HIDE);
    г_вкладка = стр;
}

static void выбрать_тему(int тема) {
    const wchar_t *имя = (тема == 1) ? L"dark" : ((тема == 2) ? L"classic" : L"light");
    WritePrivateProfileStringW(L"ms", L"theme", имя, г_ини);
    задать_цвета(тема);
    пересоздать_кисти();
    применить_тему();
    выбрать(GetDlgItem(г_окно, ID_C_THEME), тема);
    HMENU м = GetMenu(г_окно);
    if (м) CheckMenuRadioItem(м, ID_M_LIGHT, ID_M_CLASSIC,
        тема == 1 ? ID_M_DARK : (тема == 2 ? ID_M_CLASSIC : ID_M_LIGHT), MF_BYCOMMAND);
}

static HMENU создать_меню(void) {
    HMENU гл = CreateMenu();
    HMENU ф = CreatePopupMenu();
    AppendMenuW(ф, MF_STRING, ID_M_OPEN, L"Открыть…\tCtrl+O");
    AppendMenuW(ф, MF_STRING, ID_M_SAVE, L"Сохранить\tCtrl+S");
    AppendMenuW(ф, MF_STRING, ID_M_SAVEAS, L"Сохранить как…");
    AppendMenuW(ф, MF_SEPARATOR, 0, NULL);
    AppendMenuW(ф, MF_STRING, ID_M_RUN, L"Запустить\tF5");
    AppendMenuW(ф, MF_SEPARATOR, 0, NULL);
    AppendMenuW(ф, MF_STRING, ID_M_EXIT, L"Выход");
    AppendMenuW(гл, MF_POPUP, (UINT_PTR)ф, L"Файл");

    HMENU п = CreatePopupMenu();
    AppendMenuW(п, MF_STRING, ID_M_UNDO, L"Отменить");
    AppendMenuW(п, MF_SEPARATOR, 0, NULL);
    AppendMenuW(п, MF_STRING, ID_M_CUT, L"Вырезать");
    AppendMenuW(п, MF_STRING, ID_M_COPY, L"Копировать");
    AppendMenuW(п, MF_STRING, ID_M_PASTE, L"Вставить");
    AppendMenuW(п, MF_STRING, ID_M_SELALL, L"Выделить всё");
    AppendMenuW(гл, MF_POPUP, (UINT_PTR)п, L"Правка");

    HMENU в = CreatePopupMenu();
    AppendMenuW(в, MF_STRING, ID_M_LIGHT, L"Светлая тема");
    AppendMenuW(в, MF_STRING, ID_M_DARK, L"Тёмная тема");
    AppendMenuW(в, MF_STRING, ID_M_CLASSIC, L"Классическая тема");
    AppendMenuW(в, MF_SEPARATOR, 0, NULL);
    AppendMenuW(в, MF_STRING, ID_M_TAB0 + 0, L"Расширения");
    AppendMenuW(в, MF_STRING, ID_M_TAB0 + 1, L"Настройки");
    AppendMenuW(в, MF_STRING, ID_M_TAB0 + 2, L"Учебник");
    AppendMenuW(в, MF_STRING, ID_M_TAB0 + 3, L"О программе");
    AppendMenuW(в, MF_STRING, ID_M_TAB0 + 4, L"Редактор");
    AppendMenuW(гл, MF_POPUP, (UINT_PTR)в, L"Вид");

    HMENU с = CreatePopupMenu();
    AppendMenuW(с, MF_STRING, ID_M_SITE, L"Сайт MS");
    AppendMenuW(с, MF_STRING, ID_M_ABOUT, L"О программе");
    AppendMenuW(гл, MF_POPUP, (UINT_PTR)с, L"Справка");

    CheckMenuRadioItem(гл, ID_M_LIGHT, ID_M_CLASSIC,
        г_тема == 1 ? ID_M_DARK : (г_тема == 2 ? ID_M_CLASSIC : ID_M_LIGHT), MF_BYCOMMAND);
    return гл;
}
static void создать_интерфейс(HWND окно) {
    г_таб = CreateWindowExW(0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_OWNERDRAWFIXED,
        20, 102, 940, 520, окно, (HMENU)ID_TAB, г_инст, NULL);
    SendMessageW(г_таб, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    SendMessageW(г_таб, TCM_SETITEMSIZE, 0, MAKELPARAM(0, 34));
    TCITEMW it;
    ZeroMemory(&it, sizeof(it));
    it.mask = TCIF_TEXT;
    it.pszText = L"Расширения"; TabCtrl_InsertItem(г_таб, 0, &it);
    it.pszText = L"Настройки";  TabCtrl_InsertItem(г_таб, 1, &it);
    it.pszText = L"Учебник";    TabCtrl_InsertItem(г_таб, 2, &it);
    it.pszText = L"О программе"; TabCtrl_InsertItem(г_таб, 3, &it);
    it.pszText = L"Редактор";   TabCtrl_InsertItem(г_таб, 4, &it);
    SetWindowSubclass(г_таб, вкладка_проц, 1, 0);

    /* --- страница 0: расширения --- */
    г_список = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        36, 150, 900, 300, окно, (HMENU)ID_LIST, г_инст, NULL);
    SendMessageW(г_список, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    ListView_SetExtendedListViewStyle(г_список, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(ListView_GetHeader(г_список), L"", L"");
    SetWindowSubclass(ListView_GetHeader(г_список), шапка_проц, 2, 0);
    ListView_SetBkColor(г_список, г_ц_поле);
    ListView_SetTextBkColor(г_список, г_ц_поле);
    ListView_SetTextColor(г_список, г_ц_текст);
    LVCOLUMNW c;
    ZeroMemory(&c, sizeof(c));
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = L"Расширение"; c.cx = 260; ListView_InsertColumn(г_список, 0, &c);
    c.pszText = L"Состояние";  c.cx = 160; ListView_InsertColumn(г_список, 1, &c);
    c.pszText = L"Файл";       c.cx = 460; ListView_InsertColumn(г_список, 2, &c);
    доб(0, г_список);

    HWND подпись_каталога = надпись(L"Каталог наборов — загружается с сайта", 36, 306, 520, 20, 0, SS_LEFT);
    доб(0, подпись_каталога);
    г_список_каталога = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        36, 326, 900, 126, окно, (HMENU)ID_LIST_CAT, г_инст, NULL);
    SendMessageW(г_список_каталога, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    ListView_SetExtendedListViewStyle(г_список_каталога, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    SetWindowTheme(ListView_GetHeader(г_список_каталога), L"", L"");
    SetWindowSubclass(ListView_GetHeader(г_список_каталога), шапка_проц, 2, 0);
    ListView_SetBkColor(г_список_каталога, г_ц_поле);
    ListView_SetTextBkColor(г_список_каталога, г_ц_поле);
    ListView_SetTextColor(г_список_каталога, г_ц_текст);
    {
        LVCOLUMNW cc;
        ZeroMemory(&cc, sizeof(cc));
        cc.mask = LVCF_TEXT | LVCF_WIDTH;
        cc.pszText = L"Набор";   cc.cx = 220; ListView_InsertColumn(г_список_каталога, 0, &cc);
        cc.pszText = L"Версия";  cc.cx = 90;  ListView_InsertColumn(г_список_каталога, 1, &cc);
        cc.pszText = L"Файл";    cc.cx = 160; ListView_InsertColumn(г_список_каталога, 2, &cc);
        cc.pszText = L"Описание";cc.cx = 430; ListView_InsertColumn(г_список_каталога, 3, &cc);
    }
    доб(0, г_список_каталога);
    кнопка(L"Обновить", 36, 470, 95, ID_B_REFRESH, 0);
    кнопка(L"Каталог с сайта", 141, 470, 140, ID_B_CAT_REFRESH, 0);
    кнопка(L"Скачать и включить", 291, 470, 175, ID_B_CAT_GET, 0);
    кнопка(L"Включить", 476, 470, 95, ID_B_ENABLE, 0);
    кнопка(L"Выключить", 581, 470, 105, ID_B_DISABLE, 0);
    кнопка(L"Удалить", 696, 470, 90, ID_B_DELETE, 0);
    кнопка(L"Открыть папку", 796, 470, 130, ID_B_FOLDER, 0);

    /* --- страница 1: настройки --- */
    const wchar_t *подписи[4] = {L"Кодировка вывода", L"Отступ", L"Тема терминала", L"Уровень строгости"};
    for (int i = 0; i < 4; i++)
        надпись(подписи[i], 48, 170 + i * 55, 240, 24, 1, SS_LEFT);

    HWND энк = выпадающий(320, 166, 340, ID_C_ENCODING, 1);
    const wchar_t *энк_т[3] = {L"UTF-8 (рекомендуется)", L"Windows-1251", L"CP866"};
    const int энк_д[3] = {65001, 1251, 866};
    заполнить_список(энк, энк_т, энк_д, 3);

    HWND отст = выпадающий(320, 221, 340, ID_C_INDENT, 1);
    const wchar_t *отст_т[3] = {L"2 пробела", L"4 пробела", L"8 пробелов"};
    const int отст_д[3] = {2, 4, 8};
    заполнить_список(отст, отст_т, отст_д, 3);

    HWND тема = выпадающий(320, 276, 340, ID_C_THEME, 1);
    const wchar_t *тема_т[3] = {L"Светлая", L"Тёмная", L"Классическая"};
    const int тема_д[3] = {0, 1, 2};
    заполнить_список(тема, тема_т, тема_д, 3);

    HWND строг = выпадающий(320, 331, 340, ID_C_STRICT, 1);
    const wchar_t *строг_т[2] = {L"Ошибки (строго)", L"Предупреждения (мягко)"};
    const int строг_д[2] = {1, 0};
    заполнить_список(строг, строг_т, строг_д, 2);

    кнопка(L"Сохранить", 320, 395, 150, ID_B_SAVE, 1);
    кнопка(L"Сбросить", 480, 395, 150, ID_B_RESET, 1);
    надпись(L"Кодировка и строгость применяются движком. Отступ сохраняется "
            L"для будущего редактора. Тема пока влияет только на оформление Центра.",
            48, 450, 880, 40, 1, SS_LEFT);

    /* --- страница 2: учебник --- */
    HWND учебник = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
        36, 150, 900, 330, окно, NULL, г_инст, NULL);
    SendMessageW(учебник, WM_SETFONT, (WPARAM)г_шрифт, TRUE);
    доб(2, учебник);

    static const wchar_t УЧЕБНИК[] =
        L"MS — русский язык программирования\r\n\r\n"
        L"1. Первая программа\r\n"
        L"   печатать(\"Привет, мир!\")\r\n\r\n"
        L"2. Переменные\r\n"
        L"   пусть имя = \"Аня\"\r\n"
        L"   печатать(\"Привет,\", имя)\r\n\r\n"
        L"3. Условие\r\n"
        L"   если возраст >= 18:\r\n"
        L"       печатать(\"взрослый\")\r\n"
        L"   иначе:\r\n"
        L"       печатать(\"ребёнок\")\r\n\r\n"
        L"4. Цикл\r\n"
        L"   для н от 1 до 5:\r\n"
        L"       печатать(н)\r\n\r\n"
        L"5. Функция\r\n"
        L"   делать квадрат(х):\r\n"
        L"       вернуть х * х\r\n"
        L"   печатать(квадрат(7))\r\n\r\n"
        L"6. Расширения\r\n"
        L"   подключить(\"веб\")\r\n"
        L"   печатать(квадрат(7))\r\n\r\n"
        L"Файлы программ сохраняйте в UTF-8. Подробности — на сайте.";
    SetWindowTextW(учебник, УЧЕБНИК);
    RECT отступ = {8, 8, 892, 322};
    SendMessageW(учебник, EM_SETRECT, 0, (LPARAM)&отступ);

    кнопка(L"Открыть сайт", 36, 500, 170, ID_B_SITE1, 2);
    кнопка(L"Примеры программ", 216, 500, 190, ID_B_EXAMPLES, 2);

    /* --- страница 3: о программе --- */
    static const wchar_t О_ПРОГРАММЕ[] =
        L"MS 1.1\r\n\r\n"
        L"Русский язык программирования Mazing Studio.\r\n"
        L"Движок написан на C, работает без Python.\r\n"
        L"Windows 7–11, без прав администратора.\r\n\r\n"
        L"Центр MS — основа управления языком:\r\n"
        L"• расширения: включить, выключить, удалить;\r\n"
        L"• настройки кодировки, отступа, темы и строгости;\r\n"
        L"• небольшой учебник.\r\n\r\n"
        L"Каталог наборов загружается с сайта и обновляется в один клик.\r\n\r\n"
        L"Автор: GG · mazing-studio.site";
    надпись(О_ПРОГРАММЕ, 48, 165, 860, 300, 3, SS_LEFT);
    кнопка(L"Открыть сайт", 48, 480, 170, ID_B_SITE2, 3);

    /* --- страница 4: редактор --- */
    г_редактор = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
        WS_VSCROLL | WS_HSCROLL | ES_NOHIDESEL,
        36, 150, 900, 236, окно, (HMENU)ID_EDITOR, г_инст, NULL);
    SendMessageW(г_редактор, WM_SETFONT, (WPARAM)г_шрифт_код, TRUE);
    SendMessageW(г_редактор, EM_SETLIMITTEXT, 200000, 0);
    SetWindowSubclass(г_редактор, редактор_проц, 2, 0);
    доб(4, г_редактор);

    кнопка(L"Открыть", 36, 470, 120, ID_B_OPEN, 4);
    кнопка(L"Сохранить", 166, 470, 130, ID_B_SAVEFILE, 4);
    кнопка(L"Запустить", 306, 470, 120, ID_B_RUN, 4);
    кнопка(L"Пример", 436, 470, 120, ID_B_SAMPLE, 4);
    надпись(L"Tab — отступ из настроек, Enter — с сохранением отступа.",
            570, 470, 380, 36, 4, SS_LEFT);

    /* --- статус --- */
    г_статус = CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_ENDELLIPSIS,
        24, 636, 930, 20, окно, (HMENU)ID_STATUS, г_инст, NULL);
    SendMessageW(г_статус, WM_SETFONT, (WPARAM)г_шрифт_мелкий, TRUE);

    показать_страницу(0);
    загрузить_настройки_в_интерфейс();
    обновить_расширения();
    заполнить_каталог();
}

/* ---------- редактор ---------- */

static int прочитать_текст(const wchar_t *путь, wchar_t **текст) {
    HANDLE ф = CreateFileW(путь, GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (ф == INVALID_HANDLE_VALUE) return 0;
    DWORD разм = GetFileSize(ф, NULL);
    if (разм == INVALID_FILE_SIZE || разм > 5 * 1024 * 1024) { CloseHandle(ф); return 0; }
    char *б = (char*)malloc(разм + 1);
    if (!б) { CloseHandle(ф); return 0; }
    DWORD проч = 0;
    BOOL ок = ReadFile(ф, б, разм, &проч, NULL);
    CloseHandle(ф);
    if (!ок) { free(б); return 0; }
    б[проч] = 0;

    int смещ = 0;
    if (проч >= 3 && (unsigned char)б[0] == 0xEF &&
        (unsigned char)б[1] == 0xBB && (unsigned char)б[2] == 0xBF) смещ = 3;

    UINT код = CP_UTF8;
    int н = MultiByteToWideChar(код, MB_ERR_INVALID_CHARS, б + смещ, (int)проч - смещ, NULL, 0);
    if (!н) {
        код = CP_ACP;
        н = MultiByteToWideChar(код, 0, б + смещ, (int)проч - смещ, NULL, 0);
    }
    wchar_t *т = (wchar_t*)malloc((н + 1) * sizeof(wchar_t));
    if (!т) { free(б); return 0; }
    MultiByteToWideChar(код, 0, б + смещ, (int)проч - смещ, т, н);
    т[н] = 0;
    free(б);
    *текст = т;
    return 1;
}

/* ---------- каталог расширений с сайта ---------- */

static void заполнить_каталог(void) {
    if (!г_список_каталога) return;
    ListView_DeleteAllItems(г_список_каталога);

    wchar_t путь[MAX_PATH];
    _snwprintf(путь, MAX_PATH, L"%s\\каталог.txt", г_каталог);
    wchar_t *т = NULL;
    if (!прочитать_текст(путь, &т)) return;

    wchar_t *п = т;
    int ряд = 0;
    while (п && *п) {
        wchar_t *к = wcschr(п, L'\n');
        if (к) *к = 0;
        wchar_t *кр = wcsrchr(п, L'\r');
        if (кр) *кр = 0;
        if (*п && п[0] != L'#') {
            wchar_t *поля[6] = {0};
            int н = 0;
            wchar_t *с = п;
            while (с && н < 6) {
                поля[н++] = с;
                wchar_t *р = wcschr(с, L'|');
                if (!р) break;
                *р = 0;
                с = р + 1;
            }
            if (н >= 3 && поля[1] && поля[2]) {
                LVITEMW it;
                ZeroMemory(&it, sizeof(it));
                it.mask = LVIF_TEXT;
                it.iItem = ряд;
                it.pszText = поля[1];
                int i = (int)ListView_InsertItem(г_список_каталога, &it);
                ListView_SetItemText(г_список_каталога, i, 1, поля[3] ? поля[3] : L"");
                ListView_SetItemText(г_список_каталога, i, 2, поля[2]);
                ListView_SetItemText(г_список_каталога, i, 3, н >= 5 ? поля[4] : L"");
                ряд++;
            }
        }
        п = к ? к + 1 : NULL;
    }
    free(т);
}

static void скачать_каталог(void) {
    wchar_t tmp[MAX_PATH], dst[MAX_PATH];
    _snwprintf(tmp, MAX_PATH, L"%s\\каталог.tmp", г_каталог);
    _snwprintf(dst, MAX_PATH, L"%s\\каталог.txt", г_каталог);
    DeleteFileW(tmp);
    if (!скачать_файл(L"https://mazing-studio.site/download/extensions/catalog.txt", tmp)) {
        DeleteFileW(tmp);
        MessageBoxW(г_окно,
            L"Не удалось скачать каталог с сайта.\n\nПроверьте подключение к интернету.",
            L"Центр MS", MB_OK | MB_ICONERROR);
        return;
    }
    MoveFileExW(tmp, dst, MOVEFILE_REPLACE_EXISTING);
    заполнить_каталог();
    MessageBoxW(г_окно, L"Каталог расширений обновлён.", L"Центр MS", MB_OK | MB_ICONINFORMATION);
}

static void скачать_из_каталога(void) {
    if (!г_список_каталога) return;
    int sel = ListView_GetNextItem(г_список_каталога, -1, LVNI_SELECTED);
    if (sel < 0) {
        MessageBoxW(г_окно, L"Сначала выберите набор в каталоге.", L"Центр MS", MB_OK | MB_ICONINFORMATION);
        return;
    }

    wchar_t файл[MAX_PATH] = L"", имя[MAX_PATH] = L"";
    ListView_GetItemText(г_список_каталога, sel, 2, файл, MAX_PATH);
    ListView_GetItemText(г_список_каталога, sel, 0, имя, MAX_PATH);
    if (!файл[0]) return;

    wchar_t url[1024], dst[MAX_PATH], tmp[MAX_PATH], off[MAX_PATH], база[MAX_PATH];
    _snwprintf(url, 1024, L"https://mazing-studio.site/download/extensions/%s", файл);
    _snwprintf(dst, MAX_PATH, L"%s\\%s", г_папка_расширений, файл);
    _snwprintf(tmp, MAX_PATH, L"%s.tmp", dst);
    _snwprintf(off, MAX_PATH, L"%s.off", dst);
    wcsncpy(база, файл, MAX_PATH - 1);
    база[MAX_PATH - 1] = 0;
    size_t дл = wcslen(база);
    if (дл > 3 && !_wcsicmp(база + дл - 3, L".ms")) база[дл - 3] = 0;

    DeleteFileW(tmp);
    if (!скачать_файл(url, tmp)) {
        DeleteFileW(tmp);
        MessageBoxW(г_окно, L"Не удалось скачать набор с сайта.", L"Центр MS", MB_OK | MB_ICONERROR);
        return;
    }
    DeleteFileW(off);
    if (!MoveFileExW(tmp, dst, MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(tmp);
        MessageBoxW(г_окно, L"Файл скачан, но не удалось положить его в папку расширений.",
                    L"Центр MS", MB_OK | MB_ICONERROR);
        return;
    }
    обновить_расширения();

    wchar_t готово[600];
    _snwprintf(готово, 600,
        L"«%s» скачано и включено.\n\nПодключите в программе:\nподключить(\"%s\")",
        имя, база);
    MessageBoxW(г_окно, готово, L"Центр MS", MB_OK | MB_ICONINFORMATION);
}
static int записать_текст(const wchar_t *путь, const wchar_t *текст) {
    int н = WideCharToMultiByte(CP_UTF8, 0, текст, -1, NULL, 0, NULL, NULL);
    if (н <= 0) return 0;
    char *б = (char*)malloc(н);
    if (!б) return 0;
    WideCharToMultiByte(CP_UTF8, 0, текст, -1, б, н, NULL, NULL);

    HANDLE ф = CreateFileW(путь, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, NULL);
    if (ф == INVALID_HANDLE_VALUE) { free(б); return 0; }
    DWORD записано = 0;
    BOOL ок = WriteFile(ф, б, (DWORD)(н - 1), &записано, NULL);
    CloseHandle(ф);
    free(б);
    return ок && записано == (DWORD)(н - 1);
}

static void обновить_заголовок_редактора(void) {
    wchar_t имя[MAX_PATH] = L"без имени";
    if (г_файл_редактора[0]) {
        const wchar_t *с = wcsrchr(г_файл_редактора, L'\\');
        lstrcpynW(имя, с ? с + 1 : г_файл_редактора, MAX_PATH);
    }
    wchar_t з[400];
    _snwprintf(з, 400, L"Центр MS — %s", имя);
    SetWindowTextW(г_окно, з);
}

static int загрузить_в_редактор(const wchar_t *путь) {
    wchar_t *т = NULL;
    if (!прочитать_текст(путь, &т)) return 0;
    SetWindowTextW(г_редактор, т);
    free(т);
    lstrcpynW(г_файл_редактора, путь, MAX_PATH);
    обновить_заголовок_редактора();
    SetFocus(г_редактор);
    return 1;
}

static void открыть_файл_редактора(void) {
    wchar_t путь[MAX_PATH] = L"";
    OPENFILENAMEW о;
    ZeroMemory(&о, sizeof(о));
    о.lStructSize = sizeof(о);
    о.hwndOwner = г_окно;
    о.lpstrFilter = L"Программы MS (*.ms)\0*.ms\0Все файлы\0*.*\0";
    о.lpstrFile = путь;
    о.nMaxFile = MAX_PATH;
    о.lpstrDefExt = L"ms";
    о.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetOpenFileNameW(&о)) return;

    if (!загрузить_в_редактор(путь))
        MessageBoxW(г_окно, L"Не удалось прочитать файл.", L"Центр MS", MB_OK | MB_ICONERROR);
}

static int сохранить_файл_редактора(int как) {
    if (как || !г_файл_редактора[0]) {
        wchar_t путь[MAX_PATH];
        if (г_файл_редактора[0]) lstrcpynW(путь, г_файл_редактора, MAX_PATH);
        else lstrcpynW(путь, L"программа.ms", MAX_PATH);

        OPENFILENAMEW о;
        ZeroMemory(&о, sizeof(о));
        о.lStructSize = sizeof(о);
        о.hwndOwner = г_окно;
        о.lpstrFilter = L"Программы MS (*.ms)\0*.ms\0Все файлы\0*.*\0";
        о.lpstrFile = путь;
        о.nMaxFile = MAX_PATH;
        о.lpstrDefExt = L"ms";
        о.Flags = OFN_OVERWRITEPROMPT;
        if (!GetSaveFileNameW(&о)) return 0;
        lstrcpynW(г_файл_редактора, путь, MAX_PATH);
    }

    int дл = GetWindowTextLengthW(г_редактор);
    wchar_t *т = (wchar_t*)malloc((дл + 1) * sizeof(wchar_t));
    if (!т) return 0;
    GetWindowTextW(г_редактор, т, дл + 1);
    int ок = записать_текст(г_файл_редактора, т);
    free(т);

    if (!ок) {
        MessageBoxW(г_окно, L"Не удалось сохранить файл.", L"Центр MS", MB_OK | MB_ICONERROR);
        return 0;
    }
    обновить_заголовок_редактора();
    SetWindowTextW(г_статус, г_файл_редактора);
    return 1;
}

static void запустить_программу(void) {
    if (!сохранить_файл_редактора(0)) return;

    wchar_t движок[MAX_PATH];
    GetModuleFileNameW(NULL, движок, MAX_PATH);
    wchar_t *сл = wcsrchr(движок, L'\\');
    if (сл) *сл = 0;
    wcscat(движок, L"\\ms.exe");

    if (GetFileAttributesW(движок) == INVALID_FILE_ATTRIBUTES) {
        wchar_t папка[MAX_PATH] = L"";
        HKEY к;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\MazingStudio\\MS", 0, KEY_READ, &к) == ERROR_SUCCESS) {
            DWORD р = sizeof(папка);
            RegQueryValueExW(к, L"InstallDir", NULL, NULL, (BYTE*)папка, &р);
            RegCloseKey(к);
        }
        if (папка[0]) _snwprintf(движок, MAX_PATH, L"%s\\ms.exe", папка);
    }

    wchar_t параметры[MAX_PATH + 8];
    _snwprintf(параметры, MAX_PATH + 8, L"\"%s\"", г_файл_редактора);
    INT_PTR р = (INT_PTR)ShellExecuteW(NULL, L"open", движок, параметры, NULL, SW_SHOWNORMAL);
    if (р <= 32)
        MessageBoxW(г_окно,
            L"Не удалось запустить ms.exe.\n\n"
            L"Убедитесь, что MS установлен, или положите ms.exe рядом с Центром.",
            L"Центр MS", MB_OK | MB_ICONERROR);
}

static void вставить_пример(void) {
    SetWindowTextW(г_редактор,
        L"печатать(\"Привет, мир!\")\r\n"
        L"пусть имя = спросить(\"Как тебя зовут? \")\r\n"
        L"печатать(\"Здравствуй,\", имя, \"!\")\r\n");
    SetFocus(г_редактор);
}

static LRESULT CALLBACK редактор_проц(HWND h, UINT m, WPARAM wp, LPARAM lp,
                                      UINT_PTR id, DWORD_PTR ref) {
    (void)id; (void)ref;
    if (m == WM_CHAR && wp == L'\t') {
        int н = г_отступ;
        if (н < 1) н = 4;
        if (н > 12) н = 12;
        wchar_t пробелы[16];
        for (int i = 0; i < н; i++) пробелы[i] = L' ';
        пробелы[н] = 0;
        SendMessageW(h, EM_REPLACESEL, TRUE, (LPARAM)пробелы);
        return 0;
    }
    if (m == WM_CHAR && wp == L'\r') {
        int стр = (int)SendMessageW(h, EM_LINEFROMCHAR, (WPARAM)-1, 0);
        int нач = (int)SendMessageW(h, EM_LINEINDEX, стр, 0);
        wchar_t строка[512];
        *(WORD*)строка = 510;
        int ск = (int)SendMessageW(h, EM_GETLINE, стр, (LPARAM)строка);
        if (ск < 0) ск = 0;
        строка[ск] = 0;

        int отст = 0;
        while (строка[отст] == L' ' || строка[отст] == L'\t') отст++;
        int конец = (int)wcslen(строка) - 1;
        while (конец >= 0 && (строка[конец] == L' ' || строка[конец] == L'\t')) конец--;

        int добавить = 0;
        if (конец >= 0 && строка[конец] == L':') {
            добавить = г_отступ;
            if (добавить < 1) добавить = 4;
        }
        if (отст > 400) отст = 400;

        wchar_t вставка[600];
        int п = 0;
        вставка[п++] = L'\r'; вставка[п++] = L'\n';
        for (int i = 0; i < отст; i++) вставка[п++] = L' ';
        for (int i = 0; i < добавить; i++) вставка[п++] = L' ';
        вставка[п] = 0;
        SendMessageW(h, EM_REPLACESEL, TRUE, (LPARAM)вставка);
        return 0;
    }
    return DefSubclassProc(h, m, wp, lp);
}

/* ---------- рисование ---------- */

static void рисовать_кнопку(const DRAWITEMSTRUCT *d) {
    wchar_t т[160];
    GetWindowTextW(d->hwndItem, т, 160);
    RECT r = d->rcItem;
    int primary = (d->CtlID == ID_B_DOWNLOAD || d->CtlID == ID_B_DOWNLOAD_WEB || d->CtlID == ID_B_CAT_GET || d->CtlID == ID_B_SAVE ||
                   d->CtlID == ID_B_SITE1 || d->CtlID == ID_B_SITE2);
    int disabled = (d->itemState & ODS_DISABLED) != 0;
    COLORREF фон = primary ? г_ц_текст : г_ц_поле;
    COLORREF текст = primary ? г_ц_фон : г_ц_текст;
    if (disabled) { фон = RGB(230, 231, 231); текст = г_ц_серый; }
    if ((d->itemState & ODS_SELECTED) && !disabled)
        фон = primary ? RGB(20, 22, 21) : RGB(232, 233, 233);

    HBRUSH кисть = CreateSolidBrush(фон);
    HPEN перо = CreatePen(PS_SOLID, 1, primary ? фон : г_ц_линия);
    HGDIOBJ ст_к = SelectObject(d->hDC, кисть);
    HGDIOBJ ст_п = SelectObject(d->hDC, перо);
    RoundRect(d->hDC, r.left, r.top, r.right, r.bottom, 12, 12);
    SelectObject(d->hDC, ст_к);
    SelectObject(d->hDC, ст_п);
    DeleteObject(кисть);
    DeleteObject(перо);

    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, текст);
    HGDIOBJ ст_ф = SelectObject(d->hDC, г_шрифт_жирный);
    RECT rt = r;
    rt.left += 8; rt.right -= 8;
    DrawTextW(d->hDC, т, -1, &rt, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(d->hDC, ст_ф);
    if (d->itemState & ODS_FOCUS)
        DrawFocusRect(d->hDC, &r);
}

static void рисовать_вкладку(const DRAWITEMSTRUCT *d) {
    wchar_t т[64] = L"";
    TCITEMW it;
    ZeroMemory(&it, sizeof(it));
    it.mask = TCIF_TEXT;
    it.pszText = т;
    it.cchTextMax = 64;
    TabCtrl_GetItem(d->hwndItem, d->itemID, &it);

    int выбрана = (d->itemState & ODS_SELECTED) != 0;
    HBRUSH к = CreateSolidBrush(выбрана ? г_ц_фон : г_ц_шапка);
    FillRect(d->hDC, &d->rcItem, к);
    DeleteObject(к);

    RECT r = d->rcItem;
    if (выбрана) r.bottom -= 1;
    SetBkMode(d->hDC, TRANSPARENT);
    SetTextColor(d->hDC, выбрана ? г_ц_текст : г_ц_серый);
    HGDIOBJ ст = SelectObject(d->hDC, г_шрифт);
    DrawTextW(d->hDC, т, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(d->hDC, ст);
    if (d->itemState & ODS_FOCUS) DrawFocusRect(d->hDC, &d->rcItem);
}

static LRESULT CALLBACK вкладка_проц(HWND h, UINT m, WPARAM wp, LPARAM lp,
                                     UINT_PTR id, DWORD_PTR ref) {
    (void)id; (void)ref;
    if (m == WM_ERASEBKGND) {
        RECT r;
        GetClientRect(h, &r);
        FillRect((HDC)wp, &r, г_кисть_фон);
        return 1;
    }
    return DefSubclassProc(h, m, wp, lp);
}

static void перекрасить_детей(void) {
    for (int i = 0; i < 5; i++)
        for (int j = 0; j < г_кол[i]; j++)
            if (г_эл[i][j]) InvalidateRect(г_эл[i][j], NULL, TRUE);
}

static LRESULT CALLBACK шапка_проц(HWND h, UINT m, WPARAM wp, LPARAM lp,
                                   UINT_PTR id, DWORD_PTR ref) {
    (void)wp; (void)id; (void)ref;
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT пс;
        HDC dc = BeginPaint(h, &пс);
        RECT rc;
        GetClientRect(h, &rc);
        HBRUSH b = CreateSolidBrush(г_ц_поле);
        FillRect(dc, &rc, b);
        DeleteObject(b);

        int н = Header_GetItemCount(h);
        for (int i = 0; i < н; i++) {
            RECT ir;
            if (!Header_GetItemRect(h, i, &ir)) continue;
            wchar_t т[128] = L"";
            HDITEMW hi;
            ZeroMemory(&hi, sizeof(hi));
            hi.mask = HDI_TEXT;
            hi.pszText = т;
            hi.cchTextMax = 128;
            Header_GetItem(h, i, &hi);

            RECT tr = ir;
            tr.left += 12;
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, г_ц_серый);
            HGDIOBJ ст = SelectObject(dc, г_шрифт_мелкий);
            DrawTextW(dc, т, -1, &tr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, ст);

            HPEN перо = CreatePen(PS_SOLID, 1, г_ц_линия);
            HGDIOBJ ст_п = SelectObject(dc, перо);
            MoveToEx(dc, ir.right - 1, ir.top + 6, NULL);
            LineTo(dc, ir.right - 1, ir.bottom - 6);
            SelectObject(dc, ст_п);
            DeleteObject(перо);
        }

        HPEN перо = CreatePen(PS_SOLID, 1, г_ц_линия);
        HGDIOBJ ст_п = SelectObject(dc, перо);
        MoveToEx(dc, rc.left, rc.bottom - 1, NULL);
        LineTo(dc, rc.right, rc.bottom - 1);
        SelectObject(dc, ст_п);
        DeleteObject(перо);

        EndPaint(h, &пс);
        return 0;
    }
    return DefSubclassProc(h, m, wp, lp);
}
/* ---------- окно ---------- */

static LRESULT CALLBACK окно_процедура(HWND hwnd, UINT сообщение, WPARAM wp, LPARAM lp) {
    switch (сообщение) {
    case WM_CREATE: {
        г_окно = hwnd;
        г_шрифт = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        г_шрифт_жирный = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        г_шрифт_лого = CreateFontW(-22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Consolas");
        г_шрифт_заголовок = CreateFontW(-30, 0, 0, 0, FW_BLACK, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Black");
        г_шрифт_мелкий = CreateFontW(-12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        г_шрифт_код = CreateFontW(-16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, FIXED_PITCH, L"Consolas");
        создать_интерфейс(hwnd);
        SetMenu(hwnd, создать_меню());
        if (г_файл_редактора[0]) {
            if (загрузить_в_редактор(г_файл_редактора)) {
                TabCtrl_SetCurSel(г_таб, 4);
                показать_страницу(4);
            }
        }
        return 0;
    }
    case WM_SIZE:
        if (г_таб) {
            MoveWindow(г_таб, 20, 102, LOWORD(lp) - 40, HIWORD(lp) - 160, TRUE);
            MoveWindow(г_статус, 24, HIWORD(lp) - 34, LOWORD(lp) - 48, 20, TRUE);
        }
        return 0;
    case WM_ERASEBKGND: {
        RECT r;
        GetClientRect(hwnd, &r);
        FillRect((HDC)wp, &r, г_кисть_фон);
        RECT шапка = {0, 0, r.right, 96};
        FillRect((HDC)wp, &шапка, г_кисть_шапка);
        return 1;
    }
    case WM_PAINT: {
        PAINTSTRUCT п;
        HDC dc = BeginPaint(hwnd, &п);
        RECT r;
        GetClientRect(hwnd, &r);
        FillRect(dc, &r, г_кисть_фон);
        RECT шапка = {0, 0, r.right, 96};
        FillRect(dc, &шапка, г_кисть_шапка);

        HBRUSH линия = CreateSolidBrush(г_ц_линия);
        RECT рл = {0, 95, r.right, 96};
        FillRect(dc, &рл, линия);
        DeleteObject(линия);

        HBRUSH тёмная = CreateSolidBrush(г_ц_текст);
        HGDIOBJ ст_к = SelectObject(dc, тёмная);
        HGDIOBJ ст_п = SelectObject(dc, GetStockObject(NULL_PEN));
        Ellipse(dc, 24, 22, 72, 70);
        SelectObject(dc, ст_к);
        SelectObject(dc, ст_п);
        DeleteObject(тёмная);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, г_ц_поле);
        HGDIOBJ ст_ф = SelectObject(dc, г_шрифт_лого);
        RECT rл = {24, 22, 72, 70};
        DrawTextW(dc, L">_", -1, &rл, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(dc, г_шрифт_заголовок);
        SetTextColor(dc, г_ц_текст);
        TextOutW(dc, 86, 18, L"MS", 2);
        SelectObject(dc, г_шрифт);
        SetTextColor(dc, г_ц_серый);
        TextOutW(dc, 88, 56, L"Центр управления", 16);

        RECT rв = {520, 32, 956, 58};
        SetTextColor(dc, г_ц_серый);
        DrawTextW(dc, L"Версия 1.1", -1, &rв, DT_RIGHT | DT_SINGLELINE);
        SelectObject(dc, ст_ф);
        EndPaint(hwnd, &п);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, г_ц_текст);
        SetBkColor(dc, г_ц_фон);
        return (LRESULT)г_кисть_фон;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC dc = (HDC)wp;
        SetTextColor(dc, г_ц_текст);
        SetBkColor(dc, г_ц_поле);
        return (LRESULT)г_кисть_белая;
    }
    case WM_DRAWITEM: {
        const DRAWITEMSTRUCT *d = (const DRAWITEMSTRUCT*)lp;
        if (d->CtlType == ODT_BUTTON) {
            рисовать_кнопку(d);
            return TRUE;
        }
        if (d->CtlType == ODT_TAB) {
            рисовать_вкладку(d);
            return TRUE;
        }
        break;
    }
    case WM_NOTIFY: {
        NMHDR *н = (NMHDR*)lp;
        if (н->code == NM_CUSTOMDRAW && (н->hwndFrom == ListView_GetHeader(г_список) ||
                                           н->hwndFrom == ListView_GetHeader(г_список_каталога))) {
            LPNMCUSTOMDRAW c = (LPNMCUSTOMDRAW)lp;
            if (c->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (c->dwDrawStage == CDDS_ITEMPREPAINT) {
                HBRUSH b = CreateSolidBrush(г_ц_поле);
                FillRect(c->hdc, &c->rc, b);
                DeleteObject(b);
                wchar_t текст[128] = L"";
                HDITEMW hi; ZeroMemory(&hi, sizeof(hi));
                hi.mask = HDI_TEXT; hi.pszText = текст; hi.cchTextMax = 128;
                Header_GetItem(c->hdr.hwndFrom, (int)c->dwItemSpec, &hi);
                RECT rr = c->rc; rr.left += 12;
                SetBkMode(c->hdc, TRANSPARENT);
                SetTextColor(c->hdc, г_ц_серый);
                HGDIOBJ ст = SelectObject(c->hdc, г_шрифт_мелкий);
                DrawTextW(c->hdc, текст, -1, &rr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                SelectObject(c->hdc, ст);
                HPEN перо = CreatePen(PS_SOLID, 1, г_ц_линия);
                HGDIOBJ ст_п = SelectObject(c->hdc, перо);
                MoveToEx(c->hdc, c->rc.left, c->rc.bottom - 1, NULL);
                LineTo(c->hdc, c->rc.right, c->rc.bottom - 1);
                SelectObject(c->hdc, ст_п);
                DeleteObject(перо);
                return CDRF_SKIPDEFAULT;
            }
            return CDRF_DODEFAULT;
        }
        if (н->idFrom == ID_TAB && н->code == TCN_SELCHANGE) {
            показать_страницу(TabCtrl_GetCurSel(г_таб));
            return 0;
        }
        if (н->idFrom == ID_LIST && н->code == LVN_GETEMPTYMARKUP) {
            NMLVEMPTYMARKUP *пм = (NMLVEMPTYMARKUP*)lp;
            пм->dwFlags = EMF_CENTERED;
            lstrcpynW(пм->szMarkup,
                L"Расширений пока нет.\r\nНажмите «Каталог с сайта», выберите набор и скачайте его.",
                sizeof(пм->szMarkup) / sizeof(пм->szMarkup[0]));
            return TRUE;
        }
        if (н->idFrom == ID_LIST && н->code == NM_DBLCLK) {
            int sel = ListView_GetNextItem(г_список, -1, LVNI_SELECTED);
            if (sel >= 0) {
                LVITEMW it; ZeroMemory(&it, sizeof(it));
                it.mask = LVIF_PARAM; it.iItem = sel;
                ListView_GetItem(г_список, &it);
                переключить_расширение(!(int)it.lParam);
            }
            return 0;
        }
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wp)) {
        case ID_B_REFRESH:   обновить_расширения(); break;
        case ID_B_DOWNLOAD_WEB: скачать_веб(); break;
        case ID_B_ENABLE:    переключить_расширение(1); break;
        case ID_B_DISABLE:   переключить_расширение(0); break;
        case ID_B_DELETE:    удалить_расширение(); break;
        case ID_B_FOLDER:    ShellExecuteW(NULL, L"open", г_папка_расширений, NULL, NULL, SW_SHOWNORMAL); break;
        case ID_B_SAVE:      сохранить_настройки(); break;
        case ID_B_RESET:     сбросить_настройки(); break;
        case ID_B_SITE1:
        case ID_B_SITE2:     открыть_url(L"https://mazing-studio.site"); break;
        case ID_B_EXAMPLES:  открыть_url(L"https://mazing-studio.site/language"); break;
        case ID_B_OPEN:      открыть_файл_редактора(); break;
        case ID_B_SAVEFILE:  сохранить_файл_редактора(0); break;
        case ID_B_RUN:       запустить_программу(); break;
        case ID_B_SAMPLE:    вставить_пример(); break;
        case ID_M_OPEN:      открыть_файл_редактора(); break;
        case ID_M_SAVE:      сохранить_файл_редактора(0); break;
        case ID_M_SAVEAS:    сохранить_файл_редактора(1); break;
        case ID_M_RUN:       запустить_программу(); break;
        case ID_M_EXIT:      PostMessageW(hwnd, WM_CLOSE, 0, 0); break;
        case ID_M_UNDO:      SendMessageW(г_редактор, WM_UNDO, 0, 0); break;
        case ID_M_CUT:       SendMessageW(г_редактор, WM_CUT, 0, 0); break;
        case ID_M_COPY:      SendMessageW(г_редактор, WM_COPY, 0, 0); break;
        case ID_M_PASTE:     SendMessageW(г_редактор, WM_PASTE, 0, 0); break;
        case ID_M_SELALL:    SendMessageW(г_редактор, EM_SETSEL, 0, (LPARAM)-1); break;
        case ID_M_LIGHT:     выбрать_тему(0); break;
        case ID_M_DARK:      выбрать_тему(1); break;
        case ID_M_CLASSIC:   выбрать_тему(2); break;
        case ID_M_TAB0:
        case ID_M_TAB0 + 1:
        case ID_M_TAB0 + 2:
        case ID_M_TAB0 + 3:
        case ID_M_TAB0 + 4: {
            int с = LOWORD(wp) - ID_M_TAB0;
            TabCtrl_SetCurSel(г_таб, с);
            показать_страницу(с);
            break;
        }        case ID_M_SITE:      открыть_url(L"https://mazing-studio.site"); break;
        case ID_M_ABOUT:     TabCtrl_SetCurSel(г_таб, 3); показать_страницу(3); break;
        default: break;
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, сообщение, wp, lp);
}

int WINAPI WinMain(HINSTANCE инст, HINSTANCE пред, LPSTR cmd, int показ) {
    (void)пред; (void)cmd; (void)показ;
    г_инст = инст;

    INITCOMMONCONTROLSEX ic;
    ic.dwSize = sizeof(ic);
    ic.dwICC = ICC_TAB_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&ic);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    создать_пути();
    г_отступ = (int)GetPrivateProfileIntW(L"ms", L"indent", 4, г_ини);
    {
        int арг = 0;
        LPWSTR *аргументы = CommandLineToArgvW(GetCommandLineW(), &арг);
        if (аргументы) {
            if (арг > 1 && аргументы[1][0] && аргументы[1][0] != L'-')
                lstrcpynW(г_файл_редактора, аргументы[1], MAX_PATH);
            LocalFree(аргументы);
        }
    }
    задать_цвета(прочитать_тему());
    пересоздать_кисти();

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = окно_процедура;
    wc.hInstance = инст;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIconW(инст, MAKEINTRESOURCEW(1));
    if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hbrBackground = г_кисть_фон;
    wc.lpszClassName = L"MSCenter";
    RegisterClassExW(&wc);

    RECT r = {0, 0, 980, 680};
    AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
    int ш = r.right - r.left, в = r.bottom - r.top;
    RECT экран;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &экран, 0);
    int x = экран.left + ((экран.right - экран.left) - ш) / 2;
    int y = экран.top + ((экран.bottom - экран.top) - в) / 2;

    г_окно = CreateWindowExW(0, L"MSCenter", L"Центр MS — Mazing Studio",
        (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX),
        x, y, ш, в, NULL, NULL, инст, NULL);
    if (!г_окно) return 1;

    { ACCEL акс[3];
      акс[0].fVirt = FVIRTKEY | FCONTROL; акс[0].key = (WORD)'O'; акс[0].cmd = ID_M_OPEN;
      акс[1].fVirt = FVIRTKEY | FCONTROL; акс[1].key = (WORD)'S'; акс[1].cmd = ID_M_SAVE;
      акс[2].fVirt = FVIRTKEY;           акс[2].key = VK_F5;      акс[2].cmd = ID_M_RUN;
      г_акс = CreateAcceleratorTableW(акс, 3); }

    ShowWindow(г_окно, SW_SHOW);
    UpdateWindow(г_окно);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!г_акс || !TranslateAcceleratorW(г_окно, г_акс, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}
