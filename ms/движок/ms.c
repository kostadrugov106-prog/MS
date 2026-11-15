/*
 * ЯЗЫК «MS» — свой собственный язык программирования на русском языке.
 * Создан русской студией Mazing Studio. Сайт: https://mazing-studio.site
 *
 * Это НАСТОЯЩИЙ движок, написанный полностью на C. Внутри НЕТ Python.
 * Он сам читает код на русском языке из файла .ms и выполняет его.
 *
 * Использование:  ms.exe имя_файла.ms
 *
 * Только фундамент: числа, строки, переменные, арифметика, сравнения,
 * логика, условия (если/иначе), цикл (пока), функции (делать/вернуть),
 * вывод (печатать), списки и длина().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>

/* ошибка с номером строки и показом исходной строки — определяется ниже */
static void ошибка_строка(int строка,const char*формат,...);

/* ------- ВЫВОД В КОНСОЛЬ БЕЗ КРАКОЗЯБР -------
   Русский текст хранится в UTF-8. Если окно — настоящая консоль,
   пишем сразу Unicode через WriteConsoleW, и тогда кодировка окна
   (866, 1251, 65001) вообще не важна. Если вывод перенаправлен в файл
   или канал — пишем обычные байты UTF-8. */

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
static HANDLE ВЫВОД_РУЧКА=NULL, ОШИБКА_РУЧКА=NULL;
static int ВЫВОД_КОНСОЛЬ=0, ОШИБКА_КОНСОЛЬ=0;

/* настройки: %APPDATA%\MazingStudio\MS\настройки.ini */
static int настр_кодировка=65001;
static int настр_отступ=4;
static int настр_тема=0;        /* 0 светлая, 1 тёмная, 2 классическая */
static int настр_строгость=1;   /* 1 ошибки, 0 предупреждения */
static wchar_t настр_файл[MAX_PATH]=L"";
static void подготовить_вывод(void){
    ВЫВОД_РУЧКА=GetStdHandle(STD_OUTPUT_HANDLE);
    ОШИБКА_РУЧКА=GetStdHandle(STD_ERROR_HANDLE);
    DWORD режим=0;
    ВЫВОД_КОНСОЛЬ=(ВЫВОД_РУЧКА&&ВЫВОД_РУЧКА!=INVALID_HANDLE_VALUE&&GetConsoleMode(ВЫВОД_РУЧКА,&режим))?1:0;
    ОШИБКА_КОНСОЛЬ=(ОШИБКА_РУЧКА&&ОШИБКА_РУЧКА!=INVALID_HANDLE_VALUE&&GetConsoleMode(ОШИБКА_РУЧКА,&режим))?1:0;
}
static void вывести_в(HANDLE ручка,int консоль,FILE*поток,const char*текст){
    if(консоль){
        int нужно=MultiByteToWideChar(CP_UTF8,0,текст,-1,NULL,0);
        if(нужно>1){
            wchar_t*буфер=(wchar_t*)malloc((size_t)нужно*sizeof(wchar_t));
            if(буфер){
                MultiByteToWideChar(CP_UTF8,0,текст,-1,буфер,нужно);
                DWORD записано=0;
                WriteConsoleW(ручка,буфер,(DWORD)(нужно-1),&записано,NULL);
                free(буфер);
                return;
            }
        }else{
            return; /* пустая строка */
        }
    }
    if(настр_кодировка!=65001){
        int нужно=MultiByteToWideChar(CP_UTF8,0,текст,-1,NULL,0);
        if(нужно>0){
            wchar_t*ш=(wchar_t*)malloc((size_t)нужно*sizeof(wchar_t));
            if(ш){
                MultiByteToWideChar(CP_UTF8,0,текст,-1,ш,нужно);
                int байт=WideCharToMultiByte(настр_кодировка,0,ш,-1,NULL,0,NULL,NULL);
                if(байт>0){
                    char*б=(char*)malloc((size_t)байт);
                    if(б){
                        WideCharToMultiByte(настр_кодировка,0,ш,-1,б,байт,NULL,NULL);
                        fwrite(б,1,(size_t)(байт-1),поток);
                        free(б);
                        free(ш);
                        return;
                    }
                }
                free(ш);
            }
        }
    }
    fputs(текст,поток);
}
#else
#define подготовить_вывод() ((void)0)
#endif

/* чтение строки с клавиатуры: в консоли — через Unicode, чтобы русские буквы не ломались */
static int ввод_строки(char*буф,int размер){
#ifdef _WIN32
    HANDLE вх=GetStdHandle(STD_INPUT_HANDLE);DWORD режим=0;
    if(вх&&вх!=INVALID_HANDLE_VALUE&&GetConsoleMode(вх,&режим)){
        wchar_t ш[4096];DWORD проч=0;
        if(!ReadConsoleW(вх,ш,4095,&проч,NULL))return 0;
        while(проч>0&&(ш[проч-1]==L'\n'||ш[проч-1]==L'\r'))проч--;
        ш[проч]=0;
        int н=WideCharToMultiByte(CP_UTF8,0,ш,-1,NULL,0,NULL,NULL);
        if(н<=0||н>размер)return 0;
        WideCharToMultiByte(CP_UTF8,0,ш,-1,буф,размер,NULL,NULL);
        return 1;
    }
#endif
    if(!fgets(буф,размер,stdin))return 0;
    size_t д=strlen(буф);
    while(д>0&&(буф[д-1]=='\n'||буф[д-1]=='\r'))буф[--д]=0;
    return 1;
}

static void ввывод(const char*формат,...){
    va_list арг;
    va_start(арг,формат);
    int нужно=vsnprintf(NULL,0,формат,арг);
    va_end(арг);
    if(нужно<0)return;
    char*буфер=(char*)malloc((size_t)нужно+1);
    if(!буфер)return;
    va_start(арг,формат);
    vsnprintf(буфер,(size_t)нужно+1,формат,арг);
    va_end(арг);
#ifdef _WIN32
    вывести_в(ВЫВОД_РУЧКА,ВЫВОД_КОНСОЛЬ,stdout,буфер);
#else
    fputs(буфер,stdout);
#endif
    free(буфер);
}
static void вошибка(const char*формат,...){
    va_list арг;
    va_start(арг,формат);
    int нужно=vsnprintf(NULL,0,формат,арг);
    va_end(арг);
    if(нужно<0)return;
    char*буфер=(char*)malloc((size_t)нужно+1);
    if(!буфер)return;
    va_start(арг,формат);
    vsnprintf(буфер,(size_t)нужно+1,формат,арг);
    va_end(арг);
#ifdef _WIN32
    вывести_в(ОШИБКА_РУЧКА,ОШИБКА_КОНСОЛЬ,stderr,буфер);
#else
    fputs(буфер,stderr);
#endif
    free(буфер);
}

/* Запущен ли движок двойным щелчком из проводника?
   Тогда в консоли только наш процесс — окно закроется сразу, и человек
   не успеет прочитать результат. В этом случае ждём Enter. */
#ifdef _WIN32
static int запущен_из_проводника(void){
    DWORD список[2];
    DWORD n=GetConsoleProcessList(список,2);
    return n==1;
}
static int пауза_разрешена=1;
static void пауза_перед_закрытием(void){
    if(!пауза_разрешена)return;
    if(!запущен_из_проводника())return;
    ввывод("\nНажмите Enter, чтобы закрыть окно...");
    fflush(stdout);
    int c;
    while((c=getchar())!='\n'&&c!=EOF){}
}
#endif

/* ============================================================ */
/*  ДИНАМИЧЕСКАЯ СТРОКА                                          */
/* ============================================================ */

typedef struct { char *s; size_t len, cap; } Sb;

static void sb_init(Sb *b){
    b->len=0; b->cap=256; b->s=(char*)malloc(b->cap);
    if(!b->s){вошибка("Нет памяти\n");exit(1);}
    b->s[0]=0;
}
static void sb_ch(Sb *b,char c){
    if(b->len+2>b->cap){b->cap*=2;b->s=(char*)realloc(b->s,b->cap);}
    b->s[b->len++]=c;b->s[b->len]=0;
}
static void sb_str(Sb *b,const char*s){while(*s)sb_ch(b,*s++);}

/* ============================================================ */
/*  ЛЕКСЕМЫ                                                      */
/* ============================================================ */

typedef enum { T_NUM,T_STR,T_NAME,T_KEY,T_SYS,T_END } TKind;
typedef struct { TKind kind; char *txt; int line; } Tok;
typedef struct { Tok *a; int len; } TokArr;

static int is_keyword(const char*s){
    static const char*k[]={"пусть","печатать","если","иначе","пока","делать",
        "вернуть","правда","ложь","ничего","и","или","не","для","прервать",
        "продолжить","подключить","от","до","шаг",NULL};
    for(int i=0;k[i];i++)if(!strcmp(k[i],s))return 1;
    return 0;
}
static int is_dbl(const char*d){
    return !strcmp(d,"==")||!strcmp(d,"!=")||!strcmp(d,"<=")||
           !strcmp(d,">=")||!strcmp(d,"**")||!strcmp(d,"//")||
           !strcmp(d,"+=")||!strcmp(d,"-=")||!strcmp(d,"*=")||
           !strcmp(d,"/=")||!strcmp(d,"%=");
}
static void ta_push(TokArr*t,TKind k,const char*s,int line){
    t->a=(Tok*)realloc(t->a,(t->len+1)*sizeof(Tok));
    t->a[t->len].kind=k;t->a[t->len].txt=(char*)malloc(strlen(s)+1);
    strcpy(t->a[t->len].txt,s);t->a[t->len].line=line;t->len++;
}
static void ta_free(TokArr*t){for(int i=0;i<t->len;i++)free(t->a[i].txt);free(t->a);}

static TokArr tokenize(const char*src,int line){
    TokArr ta={0};
    size_t p=0,n=strlen(src);
    while(p<n){
        char c=src[p];
        if(c==' '||c=='\t'){p++;continue;}
        if(c=='"'||c=='\''){Sb b;sb_init(&b);size_t q=p+1;int закрыта=0;
            while(q<n){
                if(src[q]=='\\'&&q+1<n){
                    char е=src[q+1];
                    if(е=='n')sb_ch(&b,'\n');
                    else if(е=='t')sb_ch(&b,'\t');
                    else if(е=='r')sb_ch(&b,'\r');
                    else if(е=='\\')sb_ch(&b,'\\');
                    else if(е=='"'||е=='\'')sb_ch(&b,е);
                    else{sb_ch(&b,'\\');sb_ch(&b,е);}
                    q+=2;continue;
                }
                if(src[q]==c){закрыта=1;break;}
                sb_ch(&b,src[q++]);
            }
            if(!закрыта)ошибка_строка(line,"не закрыта кавычка в тексте");
            ta_push(&ta,T_STR,b.s,line);free(b.s);p=q+1;continue;}
        if(isdigit((unsigned char)c)||(c=='.'&&p+1<n&&isdigit((unsigned char)src[p+1]))){Sb b;sb_init(&b);size_t q=p;
            while(q<n&&(isdigit((unsigned char)src[q])||src[q]=='.'))sb_ch(&b,src[q++]);
            ta_push(&ta,T_NUM,b.s,line);free(b.s);p=q;continue;}
        if(p+1<n){char d[3]={src[p],src[p+1],0};if(is_dbl(d)){ta_push(&ta,T_SYS,d,line);p+=2;continue;}}
        if(strchr("=+-*/%<>()[],:^",c)){char o[2]={c,0};ta_push(&ta,T_SYS,o,line);p++;continue;}
        if((unsigned char)c>=0x80||c=='_'||isalpha((unsigned char)c)){ /* имя или ключевое слово (рус. буквы UTF-8) */
            Sb b;sb_init(&b);size_t q=p;
            while(q<n&&((unsigned char)src[q]>=0x80||isalnum((unsigned char)src[q])||src[q]=='_'))sb_ch(&b,src[q++]);
            ta_push(&ta,is_keyword(b.s)?T_KEY:T_NAME,b.s,line);free(b.s);p=q;continue;}
        вошибка("Ошибка на строке %d: непонятный знак '%c'\n",line,c);exit(1);
    }
    ta_push(&ta,T_END,"",line);
    return ta;
}

/* ============================================================ */
/*  ЗНАЧЕНИЯ                                                     */
/* ============================================================ */

typedef enum { V_NUM,V_STR,V_BOOL,V_NONE,V_LIST,V_DICT } VKind;
typedef struct Val Val;
typedef struct LItem LItem;
struct LItem{Val*v;LItem*next;};
typedef struct{Val*v;LItem*next;}List;
struct Val{VKind kind;double num;char*str;int boolean;LItem*head;int len;char**keys;struct Val**vals;int dcap;};

static Val*va_num(double d){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_NUM;v->num=d;return v;}
static Val*va_str(const char*s){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_STR;v->str=(char*)malloc(strlen(s)+1);strcpy(v->str,s);return v;}
static Val*va_bool(int b){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_BOOL;v->boolean=b;v->num=b?1:0;return v;}
static Val*va_none(void){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_NONE;return v;}
static Val*mk_list(void){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_LIST;return v;}
static Val*mk_dict(void){Val*v=(Val*)calloc(1,sizeof(Val));v->kind=V_DICT;return v;}
static int dict_find(Val*d,const char*k){for(int i=0;i<d->len;i++)if(!strcmp(d->keys[i],k))return i;return -1;}
static char*dup_str(const char*s){size_t n=strlen(s)+1;char*r=(char*)malloc(n);memcpy(r,s,n);return r;}
static void dict_set(Val*d,const char*k,Val*val){int i=dict_find(d,k);if(i>=0){d->vals[i]=val;return;}if(d->len==d->dcap){d->dcap=d->dcap?d->dcap*2:8;d->keys=(char**)realloc(d->keys,d->dcap*sizeof(char*));d->vals=(Val**)realloc(d->vals,d->dcap*sizeof(Val*));}d->keys[d->len]=dup_str(k);d->vals[d->len]=val;d->len++;}
static void dict_del(Val*d,const char*k){int i=dict_find(d,k);if(i<0)return;free(d->keys[i]);for(int j=i;j<d->len-1;j++){d->keys[j]=d->keys[j+1];d->vals[j]=d->vals[j+1];}d->len--;}
static void vl_add(Val*v,Val*el){
    LItem*it=malloc(sizeof(LItem));it->v=el;it->next=NULL;
    if(!v->head)v->head=it;else{LItem*c=v->head;while(c->next)c=c->next;c->next=it;}
    v->len++;
}
static int to_bool(Val*v){
    switch(v->kind){
        case V_BOOL:return v->boolean;
        case V_NUM:return v->num!=0;
        case V_STR:return v->str&&v->str[0];
        case V_LIST:return v->len>0;
        case V_DICT:return v->len>0;
        default:return 0;
    }
}
static void val_print(Val*v){
    switch(v->kind){
        case V_NUM:if(v->num==(long long)v->num)ввывод("%lld",(long long)v->num);else ввывод("%g",v->num);break;
        case V_STR:ввывод("%s",v->str);break;
        case V_BOOL:ввывод("%s",v->boolean?"правда":"ложь");break;
        case V_NONE:ввывод("%s","ничего");break;
        case V_DICT:{ввывод("{");for(int i=0;i<v->len;i++){if(i)ввывод(", ");ввывод("%s: ",v->keys[i]);val_print(v->vals[i]);}ввывод("}");break;}
        case V_LIST:{ввывод("[");LItem*c=v->head;int i=0;while(c){if(i++)ввывод(", ");val_print(c->v);c=c->next;}ввывод("]");break;}
    }
}
static void val_free(Val*v){
    if(!v)return;
    if(v->kind==V_STR)free(v->str);
    LItem*c=v->head;
    while(c){LItem*n=c->next;val_free(c->v);free(c);c=n;}
    free(v);
}
/* текст числа для конкатенации */
static void dbg_num(double d,char*buf){if(d==(long long)d)sprintf(buf,"%lld",(long long)d);else sprintf(buf,"%g",d);}

/* ============== ОШИБКИ: НОМЕР СТРОКИ И ПОДСКАЗКА ============== */

static char**ИСХОДНЫЕ_СТРОКИ=NULL;
static int ИСХОДНЫХ_СТРОК=0;

/* запомнить строки программы, чтобы показывать их при ошибке */
static void запомнить_строки(const char*src){
    int всего=1;
    for(const char*p=src;*p;p++)if(*p=='\n')всего++;
    ИСХОДНЫЕ_СТРОКИ=(char**)malloc(sizeof(char*)*(size_t)всего);
    ИСХОДНЫХ_СТРОК=0;
    const char*нач=src;
    while(1){
        const char*кон=нач;
        while(*кон&&*кон!='\n')кон++;
        size_t д=(size_t)(кон-нач);
        while(д>0&&нач[д-1]=='\r')д--;
        char*с=(char*)malloc(д+1);
        memcpy(с,нач,д);с[д]=0;
        ИСХОДНЫЕ_СТРОКИ[ИСХОДНЫХ_СТРОК++]=с;
        if(!*кон)break;
        нач=кон+1;
    }
}
static void показать_строку(int номер){
    if(номер<1||номер>ИСХОДНЫХ_СТРОК)return;
    вошибка("    %d | %s\n",номер,ИСХОДНЫЕ_СТРОКИ[номер-1]);
}
static void ошибка_строка(int номер,const char*формат,...){
    char буф[2048];
    va_list арг;va_start(арг,формат);
    vsnprintf(буф,sizeof(буф),формат,арг);
    va_end(арг);
    if(номер>0)вошибка("Ошибка в строке %d: %s\n",номер,буф);
    else вошибка("Ошибка: %s\n",буф);
    показать_строку(номер);
    exit(1);
}

static void предупреждение_строка(int номер,const char*формат,...){
    char буф[2048];
    va_list арг;va_start(арг,формат);
    vsnprintf(буф,sizeof(буф),формат,арг);
    va_end(арг);
    char итог[2300];
    snprintf(итог,sizeof(итог),"Предупреждение в строке %d: %s\n",номер,буф);
    вывести_в(ОШИБКА_РУЧКА,ОШИБКА_КОНСОЛЬ,stderr,итог);
    показать_строку(номер);
}

static const char*имя_типа(Val*v){
    switch(v->kind){
        case V_NUM:return "число";
        case V_STR:return "текст";
        case V_BOOL:return "логическое значение";
        case V_NONE:return "ничего";
        case V_LIST:return "список";
    }
    return "значение";
}
static double как_число(Val*v,int строка,const char*где){
    if(v->kind==V_NUM)return v->num;
    if(v->kind==V_BOOL)return v->boolean?1.0:0.0;
    ошибка_строка(строка,"%s: ожидалось число, а получилось — %s",где,имя_типа(v));
    return 0;
}
static int значения_равны(Val*a,Val*b){
    if(a->kind!=b->kind)return 0;
    switch(a->kind){
        case V_NUM:return a->num==b->num;
        case V_STR:return !strcmp(a->str,b->str);
        case V_BOOL:return a->boolean==b->boolean;
        case V_NONE:return 1;
        case V_LIST:{
            if(a->len!=b->len)return 0;
            LItem*x=a->head,*y=b->head;
            while(x&&y){if(!значения_равны(x->v,y->v))return 0;x=x->next;y=y->next;}
            return 1;
        }
    }
    return 0;
}

/* ============================================================ */
/*  AST (выражения)                                              */
/* ============================================================ */

typedef enum { E_VAL,E_BOOL,E_NONE,E_NAME,E_NEWLIST,E_ADD,E_MUL,E_POW,E_NEG,E_NOT,
               E_CMP,E_AND,E_OR,E_LIST,E_CALL } EKind;
typedef struct Expr Expr;
struct Expr{EKind kind;double num;char*str;int line;char*op;Expr**args;int nargs;int boolean;};
static Expr*ex_new(EKind k,int line){Expr*e=malloc(sizeof(Expr));e->kind=k;e->num=0;e->str=NULL;e->line=line;e->op=NULL;e->args=NULL;e->nargs=0;return e;}
static void ex_ap(Expr*e,Expr*a){e->args=(Expr**)realloc(e->args,(e->nargs+1)*sizeof(Expr*));e->args[e->nargs++]=a;}

/* AST (команды) */
typedef enum { C_LET,C_ASSIGN,C_PRINT,C_IF,C_ELSE,C_WHILE,C_FUNC,C_RETURN,C_EXPR,
               C_FOR,C_FOR_RANGE,C_IMPORT,C_BREAK,C_CONTINUE } CKind;
typedef struct Stmt Stmt;
struct Stmt{
    CKind kind;int line;
    char*name;
    Expr*expr;       /* правое значение / выражение вернуть / что перебираем в для */
    Expr*cond;       /* условие если/пока, конец диапазона в для … от … до */
    Expr*step;       /* шаг в для … от … до … шаг */
    Expr**print;int nprint;
    char**params;int nparam;
    Stmt**body;int nbody;      /* тело */
    Stmt**elsebody;int nelse;  /* тело иначе */
    int used_else;             /* уже привязан else */
    int is_elseif;             /* это ветка «иначе если» */
    struct Stmt*next_else;     /* следующая ветка цепочки если/иначе */
};
static Stmt*st_new(CKind k,int line){Stmt*s=malloc(sizeof(Stmt));s->kind=k;s->line=line;s->name=NULL;s->expr=NULL;s->cond=NULL;s->step=NULL;s->print=NULL;s->nprint=0;s->params=NULL;s->nparam=0;s->body=NULL;s->nbody=0;s->elsebody=NULL;s->nelse=0;s->used_else=0;s->is_elseif=0;s->next_else=NULL;return s;}
static void st_body(Stmt*s,Stmt*c){s->body=(Stmt**)realloc(s->body,(s->nbody+1)*sizeof(Stmt*));s->body[s->nbody++]=c;}

typedef struct{Stmt**a;int len;}StmtList;
static void sl_push(StmtList*l,Stmt*s){l->a=(Stmt**)realloc(l->a,(l->len+1)*sizeof(Stmt*));l->a[l->len++]=s;}

/* ============================================================ */
/*  ПАРСЕР ВЫРАЖЕНИЙ  (рекурсивный спуск по строке-лексемам)      */
/* ============================================================ */

static TokArr LT;static int pos;static int elo,ehi;   /* границы [elo,ehi) */
static int in_ex(void){return pos<ehi;}
static Tok*cur(void){return &LT.a[pos];}
static int cur_sym(const char*s){return pos<ehi&&LT.a[pos].kind==T_SYS&&!strcmp(LT.a[pos].txt,s);}
static int cur_key(const char*s){return pos<ehi&&LT.a[pos].kind==T_KEY&&!strcmp(LT.a[pos].txt,s);}
static void adv(void){pos++;}

static Expr*p_or(void);static Expr*p_and(void);static Expr*p_cmp(void);static Expr*p_add(void);
static Expr*p_mul(void);static Expr*p_pow(void);static Expr*p_unary(void);static Expr*p_prim(void);

static Expr*p_or(void){Expr*e=p_and();while(in_ex()&&cur_key("или")){adv();Expr*r=p_and();Expr*n=ex_new(E_OR,0);ex_ap(n,e);ex_ap(n,r);e=n;}return e;}
static Expr*p_and(void){Expr*e=p_cmp();while(in_ex()&&cur_key("и")){adv();Expr*r=p_cmp();Expr*n=ex_new(E_AND,0);ex_ap(n,e);ex_ap(n,r);e=n;}return e;}
static Expr*p_cmp(void){
    Expr*e=p_add();static const char*cmpk[]={"==","!=","<",">","<=",">=",NULL};
    while(in_ex()&&LT.a[pos].kind==T_SYS){
        int hi=-1;for(int i=0;cmpk[i];i++)if(!strcmp(cmpk[i],LT.a[pos].txt))hi=i;
        if(hi<0)break;
        Expr*n=ex_new(E_CMP,LT.a[pos].line);n->op=strdup(LT.a[pos].txt);ex_ap(n,e);adv();
        Expr*r=p_add();ex_ap(n,r);e=n;
    }
    return e;
}
static Expr*p_add(void){
    Expr*e=p_mul();
    while(in_ex()&&LT.a[pos].kind==T_SYS&&(!strcmp(LT.a[pos].txt,"+")||!strcmp(LT.a[pos].txt,"-"))){
        Expr*n=ex_new(E_ADD,LT.a[pos].line);n->op=strdup(LT.a[pos].txt);ex_ap(n,e);adv();
        Expr*r=p_mul();ex_ap(n,r);e=n;
    }
    return e;
}
static Expr*p_mul(void){
    Expr*e=p_pow();
    while(in_ex()&&LT.a[pos].kind==T_SYS){
        char*txt=LT.a[pos].txt;
        int m=(txt[0]=='*'&&!txt[1])||(txt[0]=='/'&&!txt[1])||(txt[0]=='%'&&!txt[1])||!strcmp(txt,"//");
        if(!m)break;
        Expr*n=ex_new(E_MUL,LT.a[pos].line);n->op=strdup(txt);ex_ap(n,e);adv();
        Expr*r=p_pow();ex_ap(n,r);e=n;
    }
    return e;
}
static Expr*p_pow(void){
    Expr*e=p_unary();
    while(in_ex()&&cur_sym("^")){Expr*n=ex_new(E_POW,LT.a[pos].line);ex_ap(n,e);adv();Expr*r=p_unary();ex_ap(n,r);e=n;}
    return e;
}
static Expr*p_unary(void){
    if(in_ex()&&cur_sym("-")){int ln=LT.a[pos].line;adv();Expr*i=p_unary();Expr*n=ex_new(E_NEG,ln);ex_ap(n,i);return n;}
    if(in_ex()&&cur_key("не")){int ln=LT.a[pos].line;adv();Expr*i=p_unary();Expr*n=ex_new(E_NOT,ln);ex_ap(n,i);return n;}
    return p_prim();
}

static int find_closing(int open){
    int d=1,i=open+1;
    while(i<ehi){
        if(LT.a[i].kind==T_SYS&&(!strcmp(LT.a[i].txt,"(")||!strcmp(LT.a[i].txt,"[")))d++;
        else if(LT.a[i].kind==T_SYS&&(!strcmp(LT.a[i].txt,")")||!strcmp(LT.a[i].txt,"]"))){d--;if(!d)return i;}
        i++;
    }
    вошибка("Ошибка: не закрыта скобка\n");exit(1);
}

/* разбор списка аргументов [s,e) с разделением по запятым */
static void parse_args(int s,int e,Expr*tgt){
    int i=s;
    if(i>=e)return;
    while(1){
        int d=0,st=i;
        while(i<e){
            if(LT.a[i].kind==T_SYS&&(!strcmp(LT.a[i].txt,"(")||!strcmp(LT.a[i].txt,"[")))d++;
            else if(LT.a[i].kind==T_SYS&&(!strcmp(LT.a[i].txt,")")||!strcmp(LT.a[i].txt,"]")))d--;
            if(d==0&&LT.a[i].kind==T_SYS&&!strcmp(LT.a[i].txt,","))break;
            i++;
        }
        int end=i<e?i:e;
        int oldp=pos,oldo=elo,oldh=ehi;
        pos=st;elo=st;ehi=end;
        ex_ap(tgt,p_or());
        pos=oldp;elo=oldo;ehi=oldh;
        if(i<e)i++;else break;
    }
}

static Expr*p_prim(void){
    if(!in_ex()){вошибка("Ошибка: пустое выражение\n");exit(1);}
    Tok*t=&LT.a[pos];int ln=t->line;
    if(t->kind==T_NUM){Expr*e=ex_new(E_VAL,ln);e->num=atof(t->txt);adv();return e;}
    if(t->kind==T_STR){Expr*e=ex_new(E_VAL,ln);e->str=strdup(t->txt);adv();return e;}
    if(t->kind==T_KEY){
        if(!strcmp(t->txt,"правда")){adv();Expr*e=ex_new(E_BOOL,ln);e->boolean=1;return e;}
        if(!strcmp(t->txt,"ложь")){adv();Expr*e=ex_new(E_BOOL,ln);e->boolean=0;return e;}
        if(!strcmp(t->txt,"ничего")){adv();Expr*e=ex_new(E_NONE,ln);return e;}
        вошибка("Ошибка на строке %d: неожиданное слово '%s'\n",ln,t->txt);exit(1);
    }
    if(t->kind==T_NAME){
        if(pos+1<ehi&&LT.a[pos+1].kind==T_SYS&&!strcmp(LT.a[pos+1].txt,"(")){
            Expr*e=ex_new(E_CALL,ln);e->str=strdup(t->txt);
            int cl=find_closing(pos+1);
            parse_args(pos+2,cl,e);
            pos=cl+1;
            return e;
        }
        if(pos+2<ehi&&LT.a[pos+1].kind==T_SYS&&!strcmp(LT.a[pos+1].txt,"[")&&
           LT.a[pos+2].kind==T_SYS&&!strcmp(LT.a[pos+2].txt,"]")){
            Expr*e=ex_new(E_NEWLIST,ln);e->str=strdup(t->txt);
            pos+=3;return e;
        }
        Expr*e=ex_new(E_NAME,ln);e->str=strdup(t->txt);adv();return e;
    }
    if(t->kind==T_SYS){
        if(!strcmp(t->txt,"(")){adv();Expr*e=p_or();if(cur_sym(")")){adv();return e;}вошибка("Не закрыта скобка\n");exit(1);}
        if(!strcmp(t->txt,"[")){
            Expr*e=ex_new(E_LIST,ln);adv();
            if(cur_sym("]")){adv();return e;}
            while(in_ex()){
                Expr*el=p_or();ex_ap(e,el);
                if(cur_sym(",")){adv();continue;}
                if(cur_sym("]")){adv();break;}
                вошибка("Не закрыт список\n");exit(1);
            }
            return e;
        }
    }
    вошибка("Ошибка на строке %d: неожиданный символ '%s'\n",ln,t->txt);exit(1);
}

/* ============================================================ */
/*  ПОСТРОЕНИЕ УЗЛА-КОМАНДЫ ИЗ ОДНОЙ СТРОКИ                       */
/* ============================================================ */

/* разбор аргументов «печатать»: от токена нач до кон (не включая) */
static void аргументы_печати(TokArr t,int нач,int кон,Stmt*s){
    int i=нач;
    if(i>=кон)return;
    while(1){
        int d=0,st=i;
        while(i<кон){
            if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,"(")||!strcmp(t.a[i].txt,"[")))d++;
            else if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,")")||!strcmp(t.a[i].txt,"]")))d--;
            if(d==0&&t.a[i].kind==T_SYS&&!strcmp(t.a[i].txt,","))break;
            i++;
        }
        int кон2=i<кон?i:кон;
        int сох_п=pos,сох_о=elo,сох_е=ehi;
        LT=t;pos=st;elo=st;ehi=кон2;
        s->print=(Expr**)realloc(s->print,(size_t)(s->nprint+1)*sizeof(Expr*));
        s->print[s->nprint++]=p_or();
        pos=сох_п;elo=сох_о;ehi=сох_е;
        if(i<кон)i++;else break;
    }
}

static Stmt*build_one(TokArr t,int line){
    if(t.len<2){вошибка("Ошибка на строке %d: короткая строка\n",line);exit(1);}
    /* убираем финальное двоеточие перед END */
    if(t.len>=2&&t.a[t.len-2].kind==T_SYS&&!strcmp(t.a[t.len-2].txt,":")){t.a[t.len-2]=t.a[t.len-1];t.len--;}
    int end=t.len-1;  /* индекс T_END */
    Tok f=t.a[0];

    /* иначе / иначе если условие: */
    if(f.kind==T_KEY&&!strcmp(f.txt,"иначе")){
        if(t.len>=3&&t.a[1].kind==T_KEY&&!strcmp(t.a[1].txt,"если")){
            Stmt*s=st_new(C_IF,line);
            s->is_elseif=1;
            LT=t;pos=2;elo=2;ehi=end;s->cond=p_or();
            return s;
        }
        return st_new(C_ELSE,line);
    }
    /* прервать / продолжить */
    if(f.kind==T_KEY&&!strcmp(f.txt,"прервать"))return st_new(C_BREAK,line);
    if(f.kind==T_KEY&&!strcmp(f.txt,"продолжить"))return st_new(C_CONTINUE,line);
    if(f.kind==T_KEY&&!strcmp(f.txt,"подключить")){
        Stmt*s=st_new(C_IMPORT,line);
        int нач=1,кон=end;
        if(t.a[1].kind==T_SYS&&!strcmp(t.a[1].txt,"(")){
            int d=1,i=2,cl=-1;
            while(i<t.len){
                if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,"(")||!strcmp(t.a[i].txt,"[")))d++;
                else if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,")")||!strcmp(t.a[i].txt,"]"))){d--;if(!d){cl=i;break;}}
                i++;
            }
            if(cl<0)ошибка_строка(line,"не закрыта скобка после «подключить»");
            нач=2;кон=cl;
        }
        if(нач>=кон)ошибка_строка(line,"после «подключить» нужно имя расширения в кавычках");
        LT=t;pos=нач;elo=нач;ehi=кон;s->expr=p_or();
        return s;
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"печатать")){
        Stmt*s=st_new(C_PRINT,line);
        if(t.a[1].kind==T_SYS&&!strcmp(t.a[1].txt,"(")){
            int cl=end,найдено=0;
            {int d=1,i=2;while(i<t.len){if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,"(")||!strcmp(t.a[i].txt,"[")))d++;else if(t.a[i].kind==T_SYS&&(!strcmp(t.a[i].txt,")")||!strcmp(t.a[i].txt,"]"))){d--;if(!d){cl=i;найдено=1;break;}}i++;}}
            if(!найдено)ошибка_строка(line,"не закрыта скобка после «печатать»");
            аргументы_печати(t,2,cl,s);
        }else{
            аргументы_печати(t,1,end,s);
        }
        return s;
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"пусть")){
        if(t.a[1].kind!=T_NAME){вошибка("После 'пусть' жду имя на строке %d\n",line);exit(1);}
        if(!(t.a[2].kind==T_SYS&&!strcmp(t.a[2].txt,"="))){вошибка("Жду '=' на строке %d\n",line);exit(1);}
        Stmt*s=st_new(C_LET,line);s->name=strdup(t.a[1].txt);
        LT=t;pos=3;elo=3;ehi=end;s->expr=p_or();
        return s;
    }
    if(f.kind==T_NAME&&t.a[1].kind==T_SYS&&(
        !strcmp(t.a[1].txt,"=")||!strcmp(t.a[1].txt,"+=")||!strcmp(t.a[1].txt,"-=")||
        !strcmp(t.a[1].txt,"*=")||!strcmp(t.a[1].txt,"/=")||!strcmp(t.a[1].txt,"%="))){
        const char*оп=t.a[1].txt;
        Stmt*s=st_new(C_ASSIGN,line);s->name=strdup(f.txt);
        LT=t;pos=2;elo=2;ehi=end;s->expr=p_or();
        if(strcmp(оп,"=")){
            Expr*лев=ex_new(E_NAME,line);лев->str=strdup(f.txt);
            Expr*n=ex_new(E_ADD,line);
            if(оп[0]=='+')n->op=strdup("+");
            else if(оп[0]=='-')n->op=strdup("-");
            else{
                n->kind=E_MUL;
                n->op=strdup(оп[0]=='*'?"*":оп[0]=='/'?"/":"%");
            }
            ex_ap(n,лев);ex_ap(n,s->expr);s->expr=n;
        }
        return s;
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"если")){
        Stmt*s=st_new(C_IF,line);
        LT=t;pos=1;elo=1;ehi=end;s->cond=p_or();
        return s;
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"пока")){
        Stmt*s=st_new(C_WHILE,line);
        LT=t;pos=1;elo=1;ehi=end;s->cond=p_or();
        return s;
    }
    /* для имя в выражение: тело   ИЛИ   для имя от А до Б [шаг Ш]: тело */
    if(f.kind==T_KEY&&!strcmp(f.txt,"для")){
        if(t.a[1].kind!=T_NAME)
            ошибка_строка(line,"после «для» нужно имя переменной. Например: для х в [1, 2, 3]:");
        if(t.len<4)
            ошибка_строка(line,"после «для %s» нужно «в» или «от». Например: для х в [1, 2, 3]:",t.a[1].txt);
        const char*связка=t.a[2].txt;
        if(!strcmp(связка,"в")){
            Stmt*s=st_new(C_FOR,line);s->name=strdup(t.a[1].txt);
            LT=t;pos=3;elo=3;ehi=end;s->expr=p_or();
            return s;
        }
        if(!strcmp(связка,"от")){
            int инд_до=-1,инд_шаг=-1,глубина=0;
            for(int j=3;j<end;j++){
                if(t.a[j].kind==T_SYS){
                    const char*з=t.a[j].txt;
                    if(!strcmp(з,"(")||!strcmp(з,"["))глубина++;
                    else if(!strcmp(з,")")||!strcmp(з,"]"))глубина--;
                }
                if(глубина==0&&t.a[j].kind==T_KEY){
                    if(!strcmp(t.a[j].txt,"до")&&инд_до<0)инд_до=j;
                    else if(!strcmp(t.a[j].txt,"шаг")&&инд_шаг<0)инд_шаг=j;
                }
            }
            if(инд_до<0)
                ошибка_строка(line,"в цикле «для … от» нужно слово «до». Например: для и от 1 до 10:");
            if(инд_шаг>0&&инд_шаг<инд_до)
                ошибка_строка(line,"«шаг» пишется после «до». Например: для и от 10 до 1 шаг -1:");
            Stmt*s=st_new(C_FOR_RANGE,line);s->name=strdup(t.a[1].txt);
            LT=t;
            pos=3;elo=3;ehi=инд_до;s->expr=p_or();
            pos=инд_до+1;elo=инд_до+1;ehi=(инд_шаг>0?инд_шаг:end);s->cond=p_or();
            if(инд_шаг>0){pos=инд_шаг+1;elo=инд_шаг+1;ehi=end;s->step=p_or();}
            return s;
        }
        ошибка_строка(line,"после «для %s» нужно «в» или «от». Например: для х в [1, 2, 3]  или  для и от 1 до 10:",t.a[1].txt);
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"делать")){
        if(t.a[1].kind!=T_NAME){вошибка("После 'делать' жду имя на строке %d\n",line);exit(1);}
        Stmt*s=st_new(C_FUNC,line);s->name=strdup(t.a[1].txt);
        if(!(t.a[2].kind==T_SYS&&!strcmp(t.a[2].txt,"("))){вошибка("Жду '('. Строка %d\n",line);exit(1);}
        for(int j=3;j<t.len;j++){
            if(t.a[j].kind==T_SYS&&!strcmp(t.a[j].txt,")"))break;
            if(t.a[j].kind==T_NAME){s->params=(char**)realloc(s->params,(s->nparam+1)*sizeof(char*));s->params[s->nparam++]=strdup(t.a[j].txt);}
        }
        return s;
    }
    if(f.kind==T_KEY&&!strcmp(f.txt,"вернуть")){
        Stmt*s=st_new(C_RETURN,line);
        if(end<=1){s->expr=NULL;return s;}
        LT=t;pos=1;elo=1;ehi=end;s->expr=p_or();
        return s;
    }
    /* вызов функции как отдельная строка: имя(аргументы) */
    if(f.kind==T_NAME && t.a[1].kind==T_SYS && !strcmp(t.a[1].txt,"(")){
        Stmt*s=st_new(C_EXPR,line);
        LT=t;pos=0;elo=0;ehi=end;s->expr=p_or();
        return s;
    }
    вошибка("Ошибка на строке %d: не понимаю строку '%s'\n",line,f.txt);
    вошибка("Подсказка: строка должна начинаться с команды пусть, печатать, если,\n");
    вошибка("пока, делать, вернуть или с вызова функции. Проверьте отступы.\n");
    exit(1);
}

/* ============================================================ */
/*  СБОРКА ДЕРЕВА ПО ОТСТУПАМ                                     */
/* ============================================================ */

static void link_else(StmtList*l);

/* комментарий «#» обрезается, но не внутри текста в кавычках */
static void убрать_комментарий(char*s){
    int в_тексте=0;char кавычка=0;
    for(char*p=s;*p;p++){
        if(в_тексте){
            if(*p=='\\'&&p[1]){p++;continue;}
            if(*p==кавычка)в_тексте=0;
        }else{
            if(*p=='"'||*p=='\''){в_тексте=1;кавычка=*p;}
            else if(*p=='#'){*p=0;return;}
        }
    }
}
/* табуляция в начале строки = 4 пробела (чтобы отступы не путались) */
static char* расширить_отступы(char*s){
    size_t и=0;int кол=0;
    while(s[и]==' '||s[и]=='\t'){
        if(s[и]=='\t')кол+=4-(кол%4);else кол++;
        и++;
    }
    if(кол==0)return s;
    size_t остаток=strlen(s+и);
    char*р=(char*)malloc((size_t)кол+остаток+1);
    memset(р,' ',(size_t)кол);
    memcpy(р+кол,s+и,остаток+1);
    free(s);
    return р;
}

static StmtList build(const char*src){
    StmtList root={0};
    typedef struct{int ind;Stmt*st;int opens;}Line;
    Line*lines=NULL;int nlines=0;
    int no=0;size_t o=0;int ln=1;
    while(src[o]){
        Sb b;sb_init(&b);
        while(src[o]&&src[o]!='\n')sb_ch(&b,src[o++]);
        if(src[o]=='\n')o++;
        убрать_комментарий(b.s);
        b.s=расширить_отступы(b.s);
        size_t l=strlen(b.s);while(l>0&&(b.s[l-1]==' '||b.s[l-1]=='\t'||b.s[l-1]=='\r'))b.s[--l]=0;
        int ind=0;while(b.s[ind]==' ')ind++;
        if(b.s[ind]==0){free(b.s);ln++;continue;}
        TokArr ta=tokenize(b.s+ind,ln);
        int opens = ta.len>=2&&ta.a[0].kind==T_KEY&&(
            !strcmp(ta.a[0].txt,"если")||!strcmp(ta.a[0].txt,"пока")||
            !strcmp(ta.a[0].txt,"делать")||!strcmp(ta.a[0].txt,"иначе")||
            !strcmp(ta.a[0].txt,"для"));
        Stmt*st=build_one(ta,ln);
        free(b.s);
        /* ta_free здесь НЕ вызываем: build_one мутировал массив (удаляя двоеточие),
           и её вызов привёл бы к двойному освобождению. Память освободится при выходе. */
        lines=(Line*)realloc(lines,(nlines+1)*sizeof(Line));
        lines[nlines].ind=ind;lines[nlines].st=st;lines[nlines].opens=opens;
        nlines++;
        ln++;
    }
    (void)no;
    /* стек кадров: {ind, Stmt*} */
    typedef struct{int ind;Stmt*blk;}Frame;
    Frame*frames=NULL;int nf=0;
    StmtList cur=root;   /* верхний уровень */
    StmtList*gt=&cur;
    /* используем отдельный список для уровня */
    StmtList *top=&root;
    for(int i=0;i<nlines;i++){
        Line L=lines[i];
        while(nf>0&&L.ind<=frames[nf-1].ind)nf--;
        /* куда добавлять? если nf>0 -> в body верхнего кадра; иначе в root */
        if(nf>0){
            Stmt*parent=frames[nf-1].blk;
            /* собрать в body */
            parent->body=(Stmt**)realloc(parent->body,(parent->nbody+1)*sizeof(Stmt*));
            parent->body[parent->nbody++]=L.st;
        }else{
            sl_push(top,L.st);
        }
        if(L.opens){
            frames=(Frame*)realloc(frames,(nf+1)*sizeof(Frame));
            frames[nf].ind=L.ind;frames[nf].blk=L.st;nf++;
        }
    }
    (void)gt;(void)cur;
    /* связать иначе: идём по каждому списку, находим C_IF + следующий C_ELSE */
    /* пройдёмся по всем уровням рекурсивно */
    /* сначала верхний: */
    link_else(top);
    return root;
}

/* рекурсивно связываем цепочки если / иначе если / иначе */
static void link_else_list(Stmt**a,int n){
    for(int i=0;i<n;i++){
        if(a[i]->kind==C_ELSE||(a[i]->kind==C_IF&&a[i]->is_elseif)){
            int есть=0;
            if(i>0&&(a[i-1]->kind==C_IF||a[i-1]->kind==C_ELSE))есть=1;
            if(!есть)
                ошибка_строка(a[i]->line,"«%s» без предыдущего «если»",a[i]->kind==C_ELSE?"иначе":"иначе если");
        }
        if(a[i]->kind==C_IF&&!a[i]->is_elseif){
            /* строим цепочку веток */
            Stmt*хвост=a[i];
            int j=i+1;
            while(j<n&&(a[j]->kind==C_ELSE||(a[j]->kind==C_IF&&a[j]->is_elseif))){
                хвост->next_else=a[j];
                хвост=a[j];
                j++;
            }
        }
        if(a[i]->nbody>0)link_else_list(a[i]->body,a[i]->nbody);
        if(a[i]->nelse>0)link_else_list(a[i]->elsebody,a[i]->nelse);
    }
}
static void link_else(StmtList*l){link_else_list(l->a,l->len);}

/* ============================================================ */
/*  ИСПОЛНИТЕЛЬ                                                  */
/* ============================================================ */

typedef struct Var Var;
struct Var{char*name;Val*val;Var*next;};
typedef struct{Var*head;}Scope;
typedef struct{char*name;Stmt*body;}Func;

static Scope scopes[512];static int nscopes=0;
static char г_папка_скрипта[1024]="";
static Func*funcs=NULL;static int nfuncs=0;
static int return_flag=0;
static Val*return_val=NULL;
static int break_flag=0;
static int continue_flag=0;

static void scope_set(Scope*sc,const char*n,Val*v){for(Var*x=sc->head;x;x=x->next)if(!strcmp(x->name,n)){x->val=v;return;}Var*nv=malloc(sizeof(Var));nv->name=malloc(strlen(n)+1);strcpy(nv->name,n);nv->val=v;nv->next=sc->head;sc->head=nv;}
static Val*scope_find(const char*n){for(int i=nscopes;i>=0;i--)for(Var*x=scopes[i].head;x;x=x->next)if(!strcmp(x->name,n))return x->val;return NULL;}

static Val*eval(Expr*e);
static void exec_body(Stmt**b,int n);

/* число символов в UTF-8 строке (русская буква = 1 символ, а не 2 байта) */
static int utf8_длина(const char*s){
    int n=0;
    for(const unsigned char*p=(const unsigned char*)s;*p;p++)
        if((*p&0xC0)!=0x80)n++;
    return n;
}

/* ================== НОВЫЕ ВСТРОЕННЫЕ ФУНКЦИИ ================== */

/* обрезать пробелы по краям */
static char* обрезать_края(const char*с){
    const char*н=с;
    while(*н==' '||*н=='\t'||*н=='\r'||*н=='\n')н++;
    const char*к=с+strlen(с);
    while(к>н&&(к[-1]==' '||к[-1]=='\t'||к[-1]=='\r'||к[-1]=='\n'))к--;
    size_t д=(size_t)(к-н);
    char*р=(char*)malloc(д+1);
    memcpy(р,н,д); р[д]=0;
    return р;
}

/* смена регистра: латиница + русские буквы (UTF-8) */
static char* сменить_регистр(const char*с,int верх){
    Sb б; sb_init(&б);
    const unsigned char*п=(const unsigned char*)с;
    while(*п){
        unsigned char а=*п;
        if((а==0xD0||а==0xD1)&&п[1]>=0x80){
            unsigned char б2=п[1], о1=а, о2=б2;
            if(верх){
                if(а==0xD0&&б2>=0xB0&&б2<=0xBF)о2=(unsigned char)(б2-0x20);
                else if(а==0xD1&&б2>=0x80&&б2<=0x8F){о1=0xD0;о2=(unsigned char)(б2+0x20);}
                else if(а==0xD1&&б2==0x91){о1=0xD0;о2=0x81;}
            }else{
                if(а==0xD0&&б2>=0x90&&б2<=0x9F)о2=(unsigned char)(б2+0x20);
                else if(а==0xD0&&б2>=0xA0&&б2<=0xAF){о1=0xD1;о2=(unsigned char)(б2-0x20);}
                else if(а==0xD0&&б2==0x81){о1=0xD1;о2=0x91;}
            }
            sb_ch(&б,(char)о1); sb_ch(&б,(char)о2); п+=2; continue;
        }
        if(верх&&а>='a'&&а<='z')sb_ch(&б,(char)(а-'a'+'A'));
        else if(!верх&&а>='A'&&а<='Z')sb_ch(&б,(char)(а-'A'+'a'));
        else sb_ch(&б,(char)а);
        п++;
    }
    return б.s;
}

/* заменить все вхождения */
static char* заменить_всё(const char*текст,const char*что,const char*на){
    size_t дл=strlen(что);
    Sb б; sb_init(&б);
    if(дл==0){sb_str(&б,текст);return б.s;}
    const char*п=текст;
    while(*п){
        if(strncmp(п,что,дл)==0){sb_str(&б,на);п+=дл;}
        else{sb_ch(&б,*п);п++;}
    }
    return б.s;
}

/* найти подстроку: номер символа или -1 */
static int найти_номер(const char*текст,const char*что){
    const char*п=strstr(текст,что);
    if(!п)return -1;
    int ном=0;
    for(const unsigned char*я=(const unsigned char*)текст;я<(const unsigned char*)п;я++)
        if((*я&0xC0)!=0x80)ном++;
    return ном;
}

/* разделить строку на список */
static Val* разделить_на_части(const char*текст,const char*разделитель){
    Val*сп=mk_list();
    size_t дл=strlen(разделитель);
    if(дл==0){vl_add(сп,va_str(текст));return сп;}
    const char*нач=текст;
    const char*п;
    while((п=strstr(нач,разделитель))!=NULL){
        size_t д=(size_t)(п-нач);
        char*ч=(char*)malloc(д+1);memcpy(ч,нач,д);ч[д]=0;
        vl_add(сп,va_str(ч));free(ч);
        нач=п+дл;
    }
    vl_add(сп,va_str(нач));
    return сп;
}

/* повторить строку несколько раз */
static char* повторить_раз(const char*с,int раз){
    if(раз<0)раз=0;
    size_t д=strlen(с);
    char*р=(char*)malloc(д*(size_t)раз+1);
    size_t п=0;
    for(int и=0;и<раз;и++){memcpy(р+п,с,д);п+=д;}
    р[п]=0;
    return р;
}

/* размер символа UTF-8 по первому байту */
static int utf8_размер(unsigned char с){
    if((с&0x80)==0)return 1;
    if((с&0xE0)==0xC0)return 2;
    if((с&0xF0)==0xE0)return 3;
    return 4;
}

/* срез строки по номерам символов: [от, до) */
static char* срез_строки(const char*с,int от,int до){
    int н=utf8_длина(с);
    if(от<0)от+=н;
    if(до<0)до+=н;
    if(от<0)от=0;
    if(до>н)до=н;
    if(до<от)до=от;
    const unsigned char*п=(const unsigned char*)с;
    const unsigned char*нач=NULL,*кон=NULL;
    int и=0;
    while(*п){
        if(и==от)нач=п;
        if(и==до){кон=п;break;}
        п+=utf8_размер(*п);и++;
    }
    if(!нач)нач=п;
    if(!кон)кон=п;
    size_t д=(size_t)(кон-нач);
    char*р=(char*)malloc(д+1);
    memcpy(р,нач,д);р[д]=0;
    return р;
}
/* срез списка: [от, до) */
static Val* срез_списка(Val*в,int от,int до){
    int н=в->len;
    if(от<0)от+=н;
    if(до<0)до+=н;
    if(от<0)от=0;
    if(до>н)до=н;
    if(до<от)до=от;
    Val*р=mk_list();
    int и=0;
    for(LItem*э=в->head;э;э=э->next,и++)
        if(и>=от&&и<до)vl_add(р,э->v);
    return р;
}
/* символ по коду Unicode */
static char* символ_по_коду(int к){
    char*р=(char*)malloc(5);
    if(к<0)к=0;
    if(к<0x80){р[0]=(char)к;р[1]=0;}
    else if(к<0x800){
        р[0]=(char)(0xC0|(к>>6));
        р[1]=(char)(0x80|(к&0x3F));
        р[2]=0;
    }else if(к<0x10000){
        р[0]=(char)(0xE0|(к>>12));
        р[1]=(char)(0x80|((к>>6)&0x3F));
        р[2]=(char)(0x80|(к&0x3F));
        р[3]=0;
    }else{
        р[0]=(char)(0xF0|(к>>18));
        р[1]=(char)(0x80|((к>>12)&0x3F));
        р[2]=(char)(0x80|((к>>6)&0x3F));
        р[3]=(char)(0x80|(к&0x3F));
        р[4]=0;
    }
    return р;
}
/* код первого символа строки */
static int код_символа(const char*с){
    const unsigned char*п=(const unsigned char*)с;
    if(!*п)return -1;
    if((*п&0x80)==0)return *п;
    if((*п&0xE0)==0xC0)return ((п[0]&0x1F)<<6)|(п[1]&0x3F);
    if((*п&0xF0)==0xE0)return ((п[0]&0x0F)<<12)|((п[1]&0x3F)<<6)|(п[2]&0x3F);
    return ((п[0]&0x07)<<18)|((п[1]&0x3F)<<12)|((п[2]&0x3F)<<6)|(п[3]&0x3F);
}

/* перевернуть строку или список */
static Val* перевернуть_значение(Val*в){
    if(в->kind==V_STR){
        const unsigned char*с=(const unsigned char*)в->str;
        int н=utf8_длина(в->str);
        int*нач=(int*)malloc(sizeof(int)*(size_t)(н?н:1));
        int и=0; const unsigned char*п=с;
        while(*п){нач[и++]=(int)(п-с);п+=utf8_размер(*п);}
        Sb б; sb_init(&б);
        for(int к=н-1;к>=0;к--){
            const unsigned char*т=с+нач[к];
            int л=utf8_размер(*т);
            for(int м=0;м<л;м++)sb_ch(&б,(char)т[м]);
        }
        free(нач);
        Val*р=va_str(б.s);free(б.s);return р;
    }
    if(в->kind==V_LIST){
        int н=в->len;
        Val**м=(Val**)malloc(sizeof(Val*)*(size_t)(н?н:1));
        int и=0;
        for(LItem*э=в->head;э;э=э->next)м[и++]=э->v;
        Val*р=mk_list();
        for(int к=н-1;к>=0;к--)vl_add(р,м[к]);
        free(м);return р;
    }
    return va_none();
}

static int ср_числа(const void*а,const void*б){
    double х=*(const double*)а, у=*(const double*)б;
    return (х<у)?-1:((х>у)?1:0);
}

/* сортировка списка: числа по возрастанию, строки по алфавиту */
static Val* отсортировать(Val*в){
    if(в->kind!=V_LIST)return va_none();
    int н=в->len;
    int только_числа=1;
    for(LItem*э=в->head;э;э=э->next)if(э->v->kind!=V_NUM){только_числа=0;break;}
    if(только_числа){
        double*ч=(double*)malloc(sizeof(double)*(size_t)(н?н:1));
        int и=0;for(LItem*э=в->head;э;э=э->next)ч[и++]=э->v->num;
        qsort(ч,(size_t)н,sizeof(double),ср_числа);
        Val*р=mk_list();for(int к=0;к<н;к++)vl_add(р,va_num(ч[к]));
        free(ч);return р;
    }
    char**т=(char**)malloc(sizeof(char*)*(size_t)(н?н:1));
    int и=0;for(LItem*э=в->head;э;э=э->next)т[и++]=(э->v->kind==V_STR)?э->v->str:(char*)"";
    for(int а=0;а<н;а++)for(int б=а+1;б<н;б++)
        if(strcmp(т[а],т[б])>0){char*х=т[а];т[а]=т[б];т[б]=х;}
    Val*р=mk_list();for(int к=0;к<н;к++)vl_add(р,va_str(т[к]));
    free(т);return р;
}

/* собрать числа из списка */
static int собрать_числа(Val*в,double**вых,int*кол){
    if(в->kind!=V_LIST||в->len==0)return 0;
    double*ч=(double*)malloc(sizeof(double)*(size_t)в->len);
    int и=0;
    for(LItem*э=в->head;э;э=э->next){
        if(э->v->kind!=V_NUM){free(ч);return 0;}
        ч[и++]=э->v->num;
    }
    *вых=ч;*кол=и;return 1;
}

/* значение в текст */
static char* значение_в_текст(Val*в){
    Sb б;sb_init(&б);
    if(в->kind==V_STR)sb_str(&б,в->str);
    else if(в->kind==V_NUM){
        char буф[64];
        if(в->num==(long long)в->num)snprintf(буф,sizeof(буф),"%lld",(long long)в->num);
        else snprintf(буф,sizeof(буф),"%g",в->num);
        sb_str(&б,буф);
    }
    else if(в->kind==V_BOOL)sb_str(&б,в->boolean?"правда":"ложь");
    else if(в->kind==V_NONE)sb_str(&б,"ничего");
    else if(в->kind==V_DICT)sb_str(&б,"{словарь}");
    else sb_str(&б,"[список]");
    return б.s;
}

/* русские числительные: выбор формы слова */
static const char* форма(int н,const char*а,const char*б,const char*в){
    int с=н%100;
    if(с>=11&&с<=14)return в;
    int д=н%10;
    if(д==1)return а;
    if(д>=2&&д<=4)return б;
    return в;
}
static void триада(int н,int жен,Sb*б){
    static const char*сотни[]={"","сто","двести","триста","четыреста","пятьсот","шестьсот","семьсот","восемьсот","девятьсот"};
    static const char*десятки[]={"","десять","двадцать","тридцать","сорок","пятьдесят","шестьдесят","семьдесят","восемьдесят","девяносто"};
    static const char*ед_м[]={"","один","два","три","четыре","пять","шесть","семь","восемь","девять"};
    static const char*ед_ж[]={"","одна","две","три","четыре","пять","шесть","семь","восемь","девять"};
    static const char*подр[]={"десять","одиннадцать","двенадцать","тринадцать","четырнадцать","пятнадцать","шестнадцать","семнадцать","восемнадцать","девятнадцать"};
    int с=н/100,д=(н/10)%10,е=н%10;
    if(с){if(б->len)sb_ch(б,' ');sb_str(б,сотни[с]);}
    if(д==1){if(б->len)sb_ch(б,' ');sb_str(б,подр[е]);}
    else{
        if(д){if(б->len)sb_ch(б,' ');sb_str(б,десятки[д]);}
        if(е){if(б->len)sb_ch(б,' ');sb_str(б,жен?ед_ж[е]:ед_м[е]);}
    }
}

/* число прописью по-русски */
static char* прописью(long long н){
    Sb б; sb_init(&б);
    if(н==0){sb_str(&б,"ноль");return б.s;}
    if(н<0){sb_str(&б,"минус");н=-н;}
    if(н>=1000000000LL){sb_str(&б,"очень много");return б.s;}
    long long млн=н/1000000, тыс=(н/1000)%1000, ед=н%1000;
    if(млн){
        триада((int)млн,0,&б);
        sb_ch(&б,' ');
        sb_str(&б,форма((int)млн,"миллион","миллиона","миллионов"));
    }
    if(тыс){
        триада((int)тыс,1,&б);
        sb_ch(&б,' ');
        sb_str(&б,форма((int)тыс,"тысяча","тысячи","тысяч"));
    }
    if(ед||(!млн&&!тыс)){
        триада((int)ед,0,&б);
    }
    return б.s;
}

/* случайное целое [от, до) */
static int случайное(int от,int до){
    static int засеяно=0;
    if(!засеяно){srand((unsigned)time(NULL));засеяно=1;}
    if(до<=от)return от;
    return от+rand()%(до-от);
}

/* ---- файлы (пути в UTF-8, работают с русскими именами) ---- */
#ifdef _WIN32
static wchar_t* путь_широкий(const char*п){
    int н=MultiByteToWideChar(CP_UTF8,0,п,-1,NULL,0);
    wchar_t*р=(wchar_t*)malloc(sizeof(wchar_t)*(size_t)(н>0?н:1));
    if(н>0)MultiByteToWideChar(CP_UTF8,0,п,-1,р,н);
    else р[0]=0;
    return р;
}
static char* широкий_в_utf8(const wchar_t*ш){
    int н=WideCharToMultiByte(CP_UTF8,0,ш,-1,NULL,0,NULL,NULL);
    if(н<=0)return strdup("");
    char*р=(char*)malloc((size_t)н);
    WideCharToMultiByte(CP_UTF8,0,ш,-1,р,н,NULL,NULL);
    return р;
}
/* настройки читаются из %APPDATA%\MazingStudio\MS\настройки.ini */
static void загрузить_настройки(void){
    wchar_t база[MAX_PATH];
    if(!GetEnvironmentVariableW(L"APPDATA",база,MAX_PATH)||!база[0]){
        if(!GetEnvironmentVariableW(L"LOCALAPPDATA",база,MAX_PATH))return;
    }
    _snwprintf(настр_файл,MAX_PATH,L"%s\\MazingStudio\\MS\\настройки.ini",база);
    настр_файл[MAX_PATH-1]=0;

    настр_кодировка=(int)GetPrivateProfileIntW(L"ms",L"encoding",65001,настр_файл);
    if(настр_кодировка!=65001&&настр_кодировка!=1251&&настр_кодировка!=866)настр_кодировка=65001;
    настр_отступ=(int)GetPrivateProfileIntW(L"ms",L"indent",4,настр_файл);
    if(настр_отступ!=2&&настр_отступ!=4&&настр_отступ!=8)настр_отступ=4;

    wchar_t т[32]=L"light";
    GetPrivateProfileStringW(L"ms",L"theme",L"light",т,32,настр_файл);
    настр_тема=!wcscmp(т,L"dark")?1:(!wcscmp(т,L"classic")?2:0);
    настр_строгость=GetPrivateProfileIntW(L"ms",L"strict",1,настр_файл)?1:0;
}
/* аргументы командной строки в UTF-8 — чтобы работали русские имена файлов */
static void подготовить_аргументы(int*pargc,char***pargv){
    int н=0;
    wchar_t**ш=CommandLineToArgvW(GetCommandLineW(),&н);
    if(!ш||н<=0)return;
    char**а=(char**)malloc(sizeof(char*)*(size_t)(н+1));
    for(int i=0;i<н;i++){
        int д=WideCharToMultiByte(CP_UTF8,0,ш[i],-1,NULL,0,NULL,NULL);
        а[i]=(char*)malloc((size_t)(д>0?д:1));
        if(д>0)WideCharToMultiByte(CP_UTF8,0,ш[i],-1,а[i],д,NULL,NULL);
        else а[i][0]=0;
    }
    LocalFree(ш);
    *pargc=н;*pargv=а;
}
static FILE* открыть_для_чтения(const char*п){
    wchar_t*ш=путь_широкий(п);
    FILE*ф=_wfopen(ш,L"rb");
    free(ш);
    return ф;
}
static int прочитать_файл(const char*п,Sb*б){
    wchar_t*ш=путь_широкий(п);
    HANDLE ф=CreateFileW(ш,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    free(ш);
    if(ф==INVALID_HANDLE_VALUE)return 0;
    char буф[8192];DWORD проч=0;
    while(ReadFile(ф,буф,sizeof(буф),&проч,NULL)&&проч>0)
        for(DWORD и=0;и<проч;и++)sb_ch(б,буф[и]);
    CloseHandle(ф);return 1;
}
static int записать_файл(const char*п,const char*т){
    wchar_t*ш=путь_широкий(п);
    HANDLE ф=CreateFileW(ш,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    free(ш);
    if(ф==INVALID_HANDLE_VALUE)return 0;
    DWORD дл=(DWORD)strlen(т),зап=0;
    BOOL ок=WriteFile(ф,т,дл,&зап,NULL);
    CloseHandle(ф);return ок&&зап==дл;
}
static int файл_есть(const char*п){
    wchar_t*ш=путь_широкий(п);
    DWORD а=GetFileAttributesW(ш);
    free(ш);
    return а!=INVALID_FILE_ATTRIBUTES;
}
static int дописать_файл(const char*п,const char*т){
    wchar_t*ш=путь_широкий(п);
    HANDLE ф=CreateFileW(ш,FILE_APPEND_DATA,FILE_SHARE_READ,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    free(ш);
    if(ф==INVALID_HANDLE_VALUE)return 0;
    DWORD дл=(DWORD)strlen(т),зап=0;
    BOOL ок=WriteFile(ф,т,дл,&зап,NULL);
    CloseHandle(ф);
    return ок&&зап==дл;
}
static int удалить_файл(const char*п){
    wchar_t*ш=путь_широкий(п);
    BOOL ок=DeleteFileW(ш);
    free(ш);
    return ок?1:0;
}
static double размер_файла(const char*п){
    wchar_t*ш=путь_широкий(п);
    WIN32_FILE_ATTRIBUTE_DATA д;
    BOOL ок=GetFileAttributesExW(ш,GetFileExInfoStandard,&д);
    free(ш);
    if(!ок)return -1;
    unsigned long long р=((unsigned long long)д.nFileSizeHigh<<32)|д.nFileSizeLow;
    return (double)р;
}
static Val* файлы_в_папке(const char*п){
    size_t дл=strlen(п);
    wchar_t*ш=(wchar_t*)malloc(sizeof(wchar_t)*(дл+4));
    int н=MultiByteToWideChar(CP_UTF8,0,п,-1,ш,(int)(дл+1));
    if(н<=0)ш[0]=0;
    size_t к=0;while(ш[к])к++;
    if(к==0||(ш[к-1]!=L'\\'&&ш[к-1]!=L'/'))ш[к++]=L'\\';
    ш[к++]=L'*';ш[к]=0;
    Val*р=mk_list();
    WIN32_FIND_DATAW д;
    HANDLE ф=FindFirstFileW(ш,&д);
    if(ф!=INVALID_HANDLE_VALUE){
        do{
            if(д.cFileName[0]==L'.'&&(д.cFileName[1]==0||(д.cFileName[1]==L'.'&&д.cFileName[2]==0)))continue;
            int байт=WideCharToMultiByte(CP_UTF8,0,д.cFileName,-1,NULL,0,NULL,NULL);
            char*имя=(char*)malloc((size_t)(байт>0?байт:1));
            if(байт>0)WideCharToMultiByte(CP_UTF8,0,д.cFileName,-1,имя,байт,NULL,NULL);
            else имя[0]=0;
            vl_add(р,va_str(имя));
            free(имя);
        }while(FindNextFileW(ф,&д));
        FindClose(ф);
    }
    free(ш);
    return р;
}
#else
static FILE* открыть_для_чтения(const char*п){return fopen(п,"rb");}
static int прочитать_файл(const char*п,Sb*б){
    FILE*ф=fopen(п,"rb");if(!ф)return 0;
    char буф[8192];size_t проч;
    while((проч=fread(буф,1,sizeof(буф),ф))>0)for(size_t и=0;и<проч;и++)sb_ch(б,буф[и]);
    fclose(ф);return 1;
}
static int записать_файл(const char*п,const char*т){
    FILE*ф=fopen(п,"wb");if(!ф)return 0;
    size_t дл=strlen(т),зап=fwrite(т,1,дл,ф);
    fclose(ф);return зап==дл;
}
static int файл_есть(const char*п){FILE*ф=fopen(п,"rb");if(!ф)return 0;fclose(ф);return 1;}
static int дописать_файл(const char*п,const char*т){
    FILE*ф=fopen(п,"ab");if(!ф)return 0;
    size_t дл=strlen(т),зап=fwrite(т,1,дл,ф);
    fclose(ф);return зап==дл;
}
static int удалить_файл(const char*п){return remove(п)==0;}
static double размер_файла(const char*п){
    FILE*ф=fopen(п,"rb");if(!ф)return -1;
    fseek(ф,0,SEEK_END);long д=ftell(ф);fclose(ф);return (double)д;
}
static Val* файлы_в_папке(const char*п){(void)п;return mk_list();}
#endif

/* прочитать файл построчно в список */
static Val* строки_файла(const char*путь){
    Sb б;sb_init(&б);
    if(!прочитать_файл(путь,&б)){free(б.s);return NULL;}
    Val*р=mk_list();
    if(б.len==0){free(б.s);return р;}
    const char*нач=б.s;
    while(1){
        const char*кон=нач;
        while(*кон&&*кон!='\n')кон++;
        size_t д=(size_t)(кон-нач);
        while(д>0&&нач[д-1]=='\r')д--;
        char*строка=(char*)malloc(д+1);
        memcpy(строка,нач,д);строка[д]=0;
        vl_add(р,va_str(строка));
        free(строка);
        if(!*кон)break;
        нач=кон+1;
    }
    free(б.s);
    return р;
}
/* дата и время по-русски */
static char* дата_текст(int с_временем){
    time_t т=time(NULL);
    struct tm*м=localtime(&т);
    char б[64];
    if(с_временем)strftime(б,sizeof(б),"%d.%m.%Y %H:%S:%S",м);
    else strftime(б,sizeof(б),"%d.%m.%Y",м);
    return strdup(б);
}

static Val*call(const char*name,Val**args,int nargs,int line){
    /* свои функции важнее встроенных: имя можно перекрыть */
    for(int i=0;i<nfuncs;i++){
        if(!strcmp(funcs[i].name,name)){
            Func*fd=&funcs[i];
            char**params=fd->body->params;int nparam=fd->body->nparam;
            if(nparam!=nargs)ошибка_строка(line,"функция «%s» ждёт %d аргумент(ов), а получила %d",name,nparam,nargs);
            if(nscopes+1>=512){вошибка("Слишком глубокая вложенность\n");exit(1);}
            nscopes++;scopes[nscopes].head=NULL;
            for(int k=0;k<nparam;k++)scope_set(&scopes[nscopes],params[k],args[k]);
            int sr=return_flag;Val*sv=return_val;return_flag=0;return_val=NULL;
            exec_body(fd->body->body,fd->body->nbody);
            Val*res=return_val?return_val:va_none();
            if(return_flag){return_flag=0;}
            return_flag=sr;return_val=sv;
            nscopes--;
            return res;
        }
    }
    if(!strcmp(name,"длина")){
        if(nargs!=1){вошибка("длина() ждёт один аргумент (строка %d)\n",line);exit(1);}
        if(args[0]->kind==V_LIST)return va_num(args[0]->len);
        if(args[0]->kind==V_STR)return va_num(utf8_длина(args[0]->str));
        вошибка("длина() не может измерить значение (строка %d)\n",line);exit(1);
    }
    /* взять(список, номер) — элемент по номеру, начиная с 0 */
    if(!strcmp(name,"взять")){
        if(nargs!=2){вошибка("взять() ждёт два аргумента: список и номер (строка %d)\n",line);exit(1);}
        if(args[1]->kind!=V_NUM){вошибка("взять(): номер должен быть числом (строка %d)\n",line);exit(1);}
        int idx=(int)args[1]->num;
        if(args[0]->kind==V_LIST){
            int n=args[0]->len;
            if(idx<0)idx+=n;
            if(idx<0||idx>=n){
                вошибка("взять(): нет элемента с номером %d, всего %d (строка %d)\n",
                        (int)args[1]->num,n,line);exit(1);
            }
            LItem*it=args[0]->head;
            for(int i=0;i<idx;i++)it=it->next;
            return it->v;
        }
        if(args[0]->kind==V_STR){
            /* посимвольно, с учётом UTF-8 */
            const unsigned char*s=(const unsigned char*)args[0]->str;
            int n=utf8_длина(args[0]->str);
            if(idx<0)idx+=n;
            if(idx<0||idx>=n){
                вошибка("взять(): в строке нет символа с номером %d, всего %d (строка %d)\n",
                        (int)args[1]->num,n,line);exit(1);
            }
            int i=0;
            while(*s){
                int len=1;
                if((*s&0x80)==0)len=1;
                else if((*s&0xE0)==0xC0)len=2;
                else if((*s&0xF0)==0xE0)len=3;
                else len=4;
                if(i==idx){
                    char buf[8]={0};
                    for(int k=0;k<len;k++)buf[k]=s[k];
                    return va_str(buf);
                }
                s+=len;i++;
            }
        }
        вошибка("взять() умеет работать со списком или строкой (строка %d)\n",line);exit(1);
    }
    /* добавить(список, значение) — дописать в конец */
    if(!strcmp(name,"добавить")){
        if(nargs!=2){вошибка("добавить() ждёт два аргумента: список и значение (строка %d)\n",line);exit(1);}
        if(args[0]->kind!=V_LIST){вошибка("добавить(): первый аргумент должен быть списком (строка %d)\n",line);exit(1);}
        vl_add(args[0],args[1]);
        return args[0];
    }
    /* соединить(список, разделитель) — склеить в текст */
    if(!strcmp(name,"соединить")){
        if(nargs!=2){вошибка("соединить() ждёт список и разделитель (строка %d)\n",line);exit(1);}
        if(args[0]->kind!=V_LIST){вошибка("соединить(): первый аргумент должен быть списком (строка %d)\n",line);exit(1);}
        Sb b;sb_init(&b);
        int first=1;
        for(LItem*it=args[0]->head;it;it=it->next){
            if(!first && args[1]->kind==V_STR)sb_str(&b,args[1]->str);
            first=0;
            if(it->v->kind==V_STR)sb_str(&b,it->v->str);
            else if(it->v->kind==V_NUM){char buf[64];dbg_num(it->v->num,buf);sb_str(&b,buf);}
            else if(it->v->kind==V_BOOL)sb_str(&b,it->v->boolean?"правда":"ложь");
            else if(it->v->kind==V_NONE)sb_str(&b,"ничего");
        }
        Val*res=va_str(b.s);free(b.s);return res;
    }
    /* содержит(список, значение) / содержит(строка, подстрока) */
    if(!strcmp(name,"содержит")){
        if(nargs!=2){вошибка("содержит() ждёт два аргумента (строка %d)\n",line);exit(1);}
        if(args[0]->kind==V_LIST){
            for(LItem*it=args[0]->head;it;it=it->next){
                if(it->v->kind==V_STR&&args[1]->kind==V_STR&&!strcmp(it->v->str,args[1]->str))return va_bool(1);
                if(it->v->kind==V_NUM&&args[1]->kind==V_NUM&&it->v->num==args[1]->num)return va_bool(1);
            }
            return va_bool(0);
        }
        if(args[0]->kind==V_STR&&args[1]->kind==V_STR)
            return va_bool(strstr(args[0]->str,args[1]->str)!=NULL);
        return va_bool(0);
    }
    /* строка(значение) — превратить в текст */
    if(!strcmp(name,"строка")){
        if(nargs!=1)return va_none();
        Sb b;sb_init(&b);
        if(args[0]->kind==V_STR)sb_str(&b,args[0]->str);
        else if(args[0]->kind==V_NUM){char buf[64];dbg_num(args[0]->num,buf);sb_str(&b,buf);}
        else if(args[0]->kind==V_BOOL)sb_str(&b,args[0]->boolean?"правда":"ложь");
        else if(args[0]->kind==V_NONE)sb_str(&b,"ничего");
        else sb_str(&b,"[список]");
        Val*res=va_str(b.s);free(b.s);return res;
    }
    /* число(значение) — превратить текст в число */
    if(!strcmp(name,"число")){
        if(nargs!=1){вошибка("число() ждёт один аргумент (строка %d)\n",line);exit(1);}
        if(args[0]->kind==V_NUM)return args[0];
        if(args[0]->kind==V_STR)return va_num(atof(args[0]->str));
        return va_num(0);
    }
    /* ввод(подсказка) / спросить(подсказка) — спросить у человека строку */
    if(!strcmp(name,"ввод")||!strcmp(name,"спросить")){
        if(nargs>=1&&args[0]->kind==V_STR){ввывод("%s",args[0]->str);fflush(stdout);}
        static char buf[16384];
        if(!ввод_строки(buf,sizeof(buf)))return va_str("");
        return va_str(buf);
    }
    /* ================= новые встроенные функции ================= */
    if(!strcmp(name,"верхний")||!strcmp(name,"нижний")){
        if(nargs!=1||args[0]->kind!=V_STR){вошибка("%s() ждёт строку (строка %d)\n",name,line);exit(1);}
        char*р=сменить_регистр(args[0]->str,!strcmp(name,"верхний"));
        Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"обрезать")){
        if(nargs!=1||args[0]->kind!=V_STR){вошибка("обрезать() ждёт строку (строка %d)\n",line);exit(1);}
        char*р=обрезать_края(args[0]->str);Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"заменить")){
        if(nargs!=3||args[0]->kind!=V_STR||args[1]->kind!=V_STR||args[2]->kind!=V_STR){
            вошибка("заменить(текст, что, на что) ждёт три строки (строка %d)\n",line);exit(1);}
        char*р=заменить_всё(args[0]->str,args[1]->str,args[2]->str);
        Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"найти")){
        if(nargs!=2||args[0]->kind!=V_STR||args[1]->kind!=V_STR){
            вошибка("найти(текст, что) ждёт две строки (строка %d)\n",line);exit(1);}
        return va_num(найти_номер(args[0]->str,args[1]->str));
    }
    if(!strcmp(name,"разделить")){
        if(nargs!=2||args[0]->kind!=V_STR||args[1]->kind!=V_STR){
            вошибка("разделить(текст, разделитель) ждёт две строки (строка %d)\n",line);exit(1);}
        return разделить_на_части(args[0]->str,args[1]->str);
    }
    if(!strcmp(name,"повторить")){
        if(nargs!=2||args[0]->kind!=V_STR||args[1]->kind!=V_NUM){
            вошибка("повторить(текст, раз) ждёт строку и число (строка %d)\n",line);exit(1);}
        char*р=повторить_раз(args[0]->str,(int)args[1]->num);
        Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"корень")||!strcmp(name,"модуль")||!strcmp(name,"округлить")||!strcmp(name,"целое")){
        if(nargs!=1||args[0]->kind!=V_NUM){вошибка("%s() ждёт число (строка %d)\n",name,line);exit(1);}
        double х=args[0]->num;
        if(!strcmp(name,"корень")){
            if(х<0){вошибка("корень(): отрицательное число (строка %d)\n",line);exit(1);}
            return va_num(sqrt(х));
        }
        if(!strcmp(name,"модуль"))return va_num(fabs(х));
        if(!strcmp(name,"округлить"))return va_num(floor(х+0.5));
        return va_num((double)(long long)х);
    }
    if(!strcmp(name,"случайное")){
        if(nargs!=2||args[0]->kind!=V_NUM||args[1]->kind!=V_NUM){
            вошибка("случайное(от, до) ждёт два числа (строка %d)\n",line);exit(1);}
        return va_num(случайное((int)args[0]->num,(int)args[1]->num));
    }
    if(!strcmp(name,"сумма")||!strcmp(name,"среднее")||!strcmp(name,"минимум")||!strcmp(name,"максимум")){
        if(nargs!=1||args[0]->kind!=V_LIST){вошибка("%s() ждёт список (строка %d)\n",name,line);exit(1);}
        double*ч=NULL;int кол=0;
        if(!собрать_числа(args[0],&ч,&кол)){
            вошибка("%s(): нужен непустой список чисел (строка %d)\n",name,line);exit(1);}
        double итог=0;
        if(!strcmp(name,"сумма")){for(int и=0;и<кол;и++)итог+=ч[и];}
        else if(!strcmp(name,"среднее")){for(int и=0;и<кол;и++)итог+=ч[и];итог/=кол;}
        else if(!strcmp(name,"минимум")){итог=ч[0];for(int и=1;и<кол;и++)if(ч[и]<итог)итог=ч[и];}
        else{итог=ч[0];for(int и=1;и<кол;и++)if(ч[и]>итог)итог=ч[и];}
        free(ч);return va_num(итог);
    }
    if(!strcmp(name,"сортировать")){
        if(nargs!=1||args[0]->kind!=V_LIST){вошибка("сортировать() ждёт список (строка %d)\n",line);exit(1);}
        return отсортировать(args[0]);
    }
    if(!strcmp(name,"перевернуть")){
        if(nargs!=1){вошибка("перевернуть() ждёт строку или список (строка %d)\n",line);exit(1);}
        return перевернуть_значение(args[0]);
    }
    if(!strcmp(name,"прочитать")){
        if(nargs!=1||args[0]->kind!=V_STR){вошибка("прочитать(путь) ждёт строку (строка %d)\n",line);exit(1);}
        Sb б;sb_init(&б);
        if(!прочитать_файл(args[0]->str,&б)){
            вошибка("прочитать(): не могу открыть '%s' (строка %d)\n",args[0]->str,line);exit(1);}
        Val*в=va_str(б.s);free(б.s);return в;
    }
    if(!strcmp(name,"записать")){
        if(nargs!=2||args[0]->kind!=V_STR){
            вошибка("записать(путь, текст) ждёт две строки (строка %d)\n",line);exit(1);}
        char*текс=значение_в_текст(args[1]);
        int ок=записать_файл(args[0]->str,текс);
        free(текс);
        if(!ок){вошибка("записать(): не могу записать '%s' (строка %d)\n",args[0]->str,line);exit(1);}
        return va_none();
    }
    if(!strcmp(name,"существует")){
        if(nargs!=1||args[0]->kind!=V_STR){вошибка("существует(путь) ждёт строку (строка %d)\n",line);exit(1);}
        return va_bool(файл_есть(args[0]->str));
    }
    if(!strcmp(name,"прописью")){
        if(nargs!=1||args[0]->kind!=V_NUM){вошибка("прописью(число) ждёт число (строка %d)\n",line);exit(1);}
        char*р=прописью((long long)args[0]->num);
        Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"склонение")){
        if(nargs!=4||args[0]->kind!=V_NUM||args[1]->kind!=V_STR||args[2]->kind!=V_STR||args[3]->kind!=V_STR){
            вошибка("склонение(число, \"год\", \"года\", \"лет\") ждёт число и три строки (строка %d)\n",line);exit(1);}
        return va_str(форма((int)args[0]->num,args[1]->str,args[2]->str,args[3]->str));
    }
    if(!strcmp(name,"время")){
        return va_num((double)time(NULL));
    }
    /* ================== срезы и символы ================== */
    if(!strcmp(name,"срез")||!strcmp(name,"подстрока")){
        if(nargs!=3)ошибка_строка(line,"%s(значение, от, до) ждёт три аргумента",name);
        if(args[0]->kind!=V_STR&&args[0]->kind!=V_LIST)
            ошибка_строка(line,"%s() работает со строкой или списком",name);
        if(!strcmp(name,"подстрока")&&args[0]->kind!=V_STR)
            ошибка_строка(line,"подстрока() работает только со строкой");
        int от=(int)как_число(args[1],line,"начало среза");
        int до=(int)как_число(args[2],line,"конец среза");
        if(args[0]->kind==V_STR){
            char*р=срез_строки(args[0]->str,от,до);
            Val*в=va_str(р);free(р);return в;
        }
        return срез_списка(args[0],от,до);
    }
    if(!strcmp(name,"символ")){
        if(nargs!=1)ошибка_строка(line,"символ(код) ждёт один аргумент");
        char*р=символ_по_коду((int)как_число(args[0],line,"код символа"));
        Val*в=va_str(р);free(р);return в;
    }
    if(!strcmp(name,"код")){
        if(nargs!=1||args[0]->kind!=V_STR)ошибка_строка(line,"код(текст) ждёт текст");
        return va_num((double)код_символа(args[0]->str));
    }
    if(!strcmp(name,"начинается_с")||!strcmp(name,"заканчивается_на")){
        if(nargs!=2||args[0]->kind!=V_STR||args[1]->kind!=V_STR)
            ошибка_строка(line,"%s(текст, кусочек) ждёт две строки",name);
        size_t дл0=strlen(args[0]->str),дл1=strlen(args[1]->str);
        int да=0;
        if(дл1<=дл0){
            if(!strcmp(name,"начинается_с"))да=!memcmp(args[0]->str,args[1]->str,дл1);
            else да=!memcmp(args[0]->str+дл0-дл1,args[1]->str,дл1);
        }
        return va_bool(да);
    }
    if(!strcmp(name,"индекс")){
        if(nargs!=2)ошибка_строка(line,"индекс(значение, что) ждёт два аргумента");
        if(args[0]->kind==V_LIST){
            int и=0;
            for(LItem*э=args[0]->head;э;э=э->next,и++)
                if(значения_равны(э->v,args[1]))return va_num(и);
            return va_num(-1);
        }
        if(args[0]->kind==V_STR&&args[1]->kind==V_STR)
            return va_num(найти_номер(args[0]->str,args[1]->str));
        return va_num(-1);
    }
    /* ================== списки ================== */
    if(!strcmp(name,"вставить")){
        if(nargs!=3)ошибка_строка(line,"вставить(список, номер, значение) ждёт три аргумента");
        if(args[0]->kind!=V_LIST)ошибка_строка(line,"вставить(): первый аргумент должен быть списком");
        int и=(int)как_число(args[1],line,"номер");
        int н=args[0]->len;
        if(и<0)и+=н;
        if(и<0)и=0;
        if(и>н)и=н;
        LItem*нов=(LItem*)malloc(sizeof(LItem));нов->v=args[2];нов->next=NULL;
        if(и==0){нов->next=args[0]->head;args[0]->head=нов;}
        else{
            LItem*п=args[0]->head;
            for(int к=0;к<и-1&&п;к++)п=п->next;
            if(п){нов->next=п->next;п->next=нов;}
            else{нов->next=args[0]->head;args[0]->head=нов;}
        }
        args[0]->len++;
        return args[0];
    }
    if(!strcmp(name,"удалить")){
        if(nargs!=2)ошибка_строка(line,"удалить(список, номер) ждёт два аргумента");
        if(args[0]->kind!=V_LIST)ошибка_строка(line,"удалить(): первый аргумент должен быть списком");
        int и=(int)как_число(args[1],line,"номер");
        int н=args[0]->len;
        if(и<0)и+=н;
        if(и<0||и>=н)ошибка_строка(line,"удалить(): нет элемента с номером %d, всего %d",(int)args[1]->num,н);
        LItem*п=args[0]->head;
        if(и==0)args[0]->head=п->next;
        else{
            for(int к=0;к<и-1;к++)п=п->next;
            п->next=п->next->next;
        }
        args[0]->len--;
        return args[0];
    }
    if(!strcmp(name,"очистить")){
        if(nargs!=1||args[0]->kind!=V_LIST)ошибка_строка(line,"очистить(список) ждёт список");
        args[0]->head=NULL;args[0]->len=0;
        return args[0];
    }
    /* ================== математика ================== */
    if(!strcmp(name,"пи")){
        return va_num(3.14159265358979323846);
    }
    if(!strcmp(name,"степень")){
        if(nargs!=2)ошибка_строка(line,"степень(число, степень) ждёт два числа");
        return va_num(pow(как_число(args[0],line,"степень"),как_число(args[1],line,"степень")));
    }
    if(!strcmp(name,"синус")||!strcmp(name,"косинус")||!strcmp(name,"тангенс")){
        if(nargs!=1)ошибка_строка(line,"%s(угол) ждёт число в радианах",name);
        double х=как_число(args[0],line,name);
        if(!strcmp(name,"синус"))return va_num(sin(х));
        if(!strcmp(name,"косинус"))return va_num(cos(х));
        return va_num(tan(х));
    }
    if(!strcmp(name,"логарифм")){
        if(nargs<1||nargs>2)ошибка_строка(line,"логарифм(число[, основание]) ждёт одно или два числа");
        double х=как_число(args[0],line,"логарифм");
        if(х<=0)ошибка_строка(line,"логарифм() берёт только положительные числа");
        if(nargs==1)return va_num(log(х));
        double осн=как_число(args[1],line,"основание");
        if(осн<=0||осн==1)ошибка_строка(line,"основание логарифма должно быть больше 0 и не равно 1");
        return va_num(log(х)/log(осн));
    }
    if(!strcmp(name,"случайное_дробное")){
        static int засеяно=0;
        if(!засеяно){srand((unsigned)time(NULL));засеяно=1;}
        return va_num((double)rand()/((double)RAND_MAX+1.0));
    }
    /* ================== дата и время ================== */
    if(!strcmp(name,"дата")||!strcmp(name,"сейчас")){
        if(nargs!=0)ошибка_строка(line,"%s() не нужны аргументы",name);
        char*р=дата_текст(!strcmp(name,"сейчас"));
        Val*в=va_str(р);free(р);return в;
    }
    /* ================== файлы ================== */
    if(!strcmp(name,"дописать")){
        if(nargs!=2||args[0]->kind!=V_STR)ошибка_строка(line,"дописать(путь, текст) ждёт путь и текст");
        char*т=значение_в_текст(args[1]);
        int ок=дописать_файл(args[0]->str,т);
        free(т);
        if(!ок)ошибка_строка(line,"дописать(): не могу записать «%s»",args[0]->str);
        return va_none();
    }
    if(!strcmp(name,"удалить_файл")){
        if(nargs!=1||args[0]->kind!=V_STR)ошибка_строка(line,"удалить_файл(путь) ждёт путь");
        if(!удалить_файл(args[0]->str))ошибка_строка(line,"удалить_файл(): не могу удалить «%s»",args[0]->str);
        return va_none();
    }
    if(!strcmp(name,"размер_файла")){
        if(nargs!=1||args[0]->kind!=V_STR)ошибка_строка(line,"размер_файла(путь) ждёт путь");
        double р=размер_файла(args[0]->str);
        if(р<0)ошибка_строка(line,"размер_файла(): нет файла «%s»",args[0]->str);
        return va_num(р);
    }
    if(!strcmp(name,"строки_файла")){
        if(nargs!=1||args[0]->kind!=V_STR)ошибка_строка(line,"строки_файла(путь) ждёт путь");
        Val*р=строки_файла(args[0]->str);
        if(!р)ошибка_строка(line,"строки_файла(): не могу открыть «%s»",args[0]->str);
        return р;
    }
    if(!strcmp(name,"файлы")){
        if(nargs!=1||args[0]->kind!=V_STR)ошибка_строка(line,"файлы(папка) ждёт путь к папке");
        return файлы_в_папке(args[0]->str);
    }
    /* ================== прочее ================== */
    if(!strcmp(name,"словарь")){
        Val*d=mk_dict();
        for(int i=0;i+1<nargs;i+=2){char*k=значение_в_текст(args[i]);dict_set(d,k,args[i+1]);free(k);}
        return d;
    }
    if(!strcmp(name,"словарь_получить")){
        if(nargs!=2||args[0]->kind!=V_DICT){вошибка("словарь_получить(словарь, ключ) (строка %d)\\n",line);exit(1);}
        char*k=значение_в_текст(args[1]);int i=dict_find(args[0],k);free(k);
        return i<0?va_none():args[0]->vals[i];
    }
    if(!strcmp(name,"словарь_установить")){
        if(nargs!=3||args[0]->kind!=V_DICT){вошибка("словарь_установить(словарь, ключ, значение) (строка %d)\\n",line);exit(1);}
        char*k=значение_в_текст(args[1]);dict_set(args[0],k,args[2]);free(k);return args[0];
    }
    if(!strcmp(name,"словарь_есть")){
        if(nargs!=2||args[0]->kind!=V_DICT){вошибка("словарь_есть(словарь, ключ) (строка %d)\\n",line);exit(1);}
        char*k=значение_в_текст(args[1]);int i=dict_find(args[0],k);free(k);return va_bool(i>=0);
    }
    if(!strcmp(name,"словарь_размер")){
        if(nargs!=1||args[0]->kind!=V_DICT){вошибка("словарь_размер(словарь) (строка %d)\\n",line);exit(1);}
        return va_num(args[0]->len);
    }
    if(!strcmp(name,"словарь_ключи")){
        if(nargs!=1||args[0]->kind!=V_DICT){вошибка("словарь_ключи(словарь) (строка %d)\\n",line);exit(1);}
        Val*с=mk_list();for(int i=0;i<args[0]->len;i++)vl_add(с,va_str(args[0]->keys[i]));return с;
    }
    if(!strcmp(name,"словарь_удалить")){
        if(nargs!=2||args[0]->kind!=V_DICT){вошибка("словарь_удалить(словарь, ключ) (строка %d)\\n",line);exit(1);}
        char*k=значение_в_текст(args[1]);int i=dict_find(args[0],k);dict_del(args[0],k);free(k);return va_bool(i>=0);
    }
    if(!strcmp(name,"тип")){
        if(nargs!=1)ошибка_строка(line,"тип(значение) ждёт один аргумент");
        return va_str(имя_типа(args[0]));
    }
    if(!strcmp(name,"ошибка")){
        if(nargs<1||args[0]->kind!=V_STR)ошибка_строка(line,"ошибка(\"текст\") ждёт текст");
        ошибка_строка(line,"%s",args[0]->str);
    }
    if(!strcmp(name,"выйти")){
        int код=nargs>=1?(int)как_число(args[0],line,"код выхода"):0;
        if(код<0)код=0;
        exit(код);
    }
    ошибка_строка(line,"нет функции «%s»",name);
}

static Val*eval(Expr*e){
    switch(e->kind){
        case E_VAL:if(e->str)return va_str(e->str);return va_num(e->num);
        case E_BOOL:return va_bool(e->boolean);
        case E_NONE:return va_none();
        case E_NAME:{Val*v=scope_find(e->str);if(!v){
            ошибка_строка(e->line,"переменная «%s» не объявлена. Напишите: пусть %s = ...",e->str,e->str);
        }return v;}
        case E_NEWLIST:return mk_list();
        case E_ADD:{
            Val*a=eval(e->args[0]),*b=eval(e->args[1]);
            if(e->op[0]=='+'){
                if(a->kind==V_LIST||b->kind==V_LIST){
                    Val*р=mk_list();
                    if(a->kind==V_LIST){for(LItem*и=a->head;и;и=и->next)vl_add(р,и->v);}
                    else vl_add(р,a);
                    if(b->kind==V_LIST){for(LItem*и=b->head;и;и=и->next)vl_add(р,и->v);}
                    else vl_add(р,b);
                    return р;
                }
                if(a->kind==V_STR||b->kind==V_STR){
                    char*т1=значение_в_текст(a),*т2=значение_в_текст(b);
                    Sb р;sb_init(&р);sb_str(&р,т1);sb_str(&р,т2);
                    free(т1);free(т2);
                    Val*рез=va_str(р.s);free(р.s);return рез;
                }
                return va_num(как_число(a,e->line,"сложение")+как_число(b,e->line,"сложение"));
            }
            if(a->kind==V_LIST||b->kind==V_LIST)
                ошибка_строка(e->line,"нельзя вычитать списки");
            return va_num(как_число(a,e->line,"вычитание")-как_число(b,e->line,"вычитание"));
        }
        case E_MUL:{
            Val*a=eval(e->args[0]),*b=eval(e->args[1]);
            const char*оп=e->op;
            if(!strcmp(оп,"*")){
                if(a->kind==V_STR||b->kind==V_STR){
                    Val*т=(a->kind==V_STR)?a:b;
                    Val*ч=(a->kind==V_STR)?b:a;
                    int раз=(int)как_число(ч,e->line,"повтор текста");
                    char*р=повторить_раз(т->str,раз);
                    Val*в=va_str(р);free(р);return в;
                }
                if(a->kind==V_LIST&&b->kind==V_NUM){
                    Val*р=mk_list();int раз=(int)b->num;
                    for(int к=0;к<раз;к++)for(LItem*и=a->head;и;и=и->next)vl_add(р,и->v);
                    return р;
                }
                return va_num(как_число(a,e->line,"умножение")*как_число(b,e->line,"умножение"));
            }
            if(a->kind==V_STR||b->kind==V_STR)
                ошибка_строка(e->line,"нельзя выполнять «%s» с текстом",оп);
            double х=как_число(a,e->line,"деление"),у=как_число(b,e->line,"деление");
            if(у==0)ошибка_строка(e->line,"деление на ноль");
            if(!strcmp(оп,"/"))return va_num(х/у);
            if(!strcmp(оп,"//"))return va_num(floor(х/у));
            return va_num(х-floor(х/у)*у);
        }
        case E_POW:{Val*a=eval(e->args[0]),*b=eval(e->args[1]);
            return va_num(pow(как_число(a,e->line,"степень"),как_число(b,e->line,"степень")));}
        case E_NEG:{Val*a=eval(e->args[0]);return va_num(-как_число(a,e->line,"минус"));}
        case E_NOT:{Val*a=eval(e->args[0]);return va_bool(!to_bool(a));}
        case E_CMP:{
            Val*a=eval(e->args[0]),*b=eval(e->args[1]);const char*о=e->op;
            if(!strcmp(о,"=="))return va_bool(значения_равны(a,b));
            if(!strcmp(о,"!="))return va_bool(!значения_равны(a,b));
            if(a->kind==V_STR&&b->kind==V_STR){
                int с=strcmp(a->str,b->str);
                if(!strcmp(о,"<"))return va_bool(с<0);
                if(!strcmp(о,">"))return va_bool(с>0);
                if(!strcmp(о,"<="))return va_bool(с<=0);
                return va_bool(с>=0);
            }
            double х=как_число(a,e->line,"сравнение"),у=как_число(b,e->line,"сравнение");
            if(!strcmp(о,"<"))return va_bool(х<у);
            if(!strcmp(о,">"))return va_bool(х>у);
            if(!strcmp(о,"<="))return va_bool(х<=у);
            return va_bool(х>=у);
        }
        case E_AND:{if(!to_bool(eval(e->args[0])))return va_bool(0);return va_bool(to_bool(eval(e->args[1])));}
        case E_OR:{if(to_bool(eval(e->args[0])))return va_bool(1);return va_bool(to_bool(eval(e->args[1])));}
        case E_LIST:{Val*l=mk_list();for(int i=0;i<e->nargs;i++)vl_add(l,eval(e->args[i]));return l;}
        case E_CALL:{
            Val**args=malloc(sizeof(Val*)*e->nargs);
            for(int i=0;i<e->nargs;i++)args[i]=eval(e->args[i]);
            Val*r=call(e->str,args,e->nargs,e->line);
            free(args);
            return r;
        }
    }
    вошибка("Внутренняя ошибка\n");exit(1);
}

/* ---------- расширения ---------- */

static char* папка_расширений_приложения(void){
    wchar_t база[MAX_PATH];
    if(!GetEnvironmentVariableW(L"APPDATA",база,MAX_PATH)||!база[0])return NULL;
    char*б=широкий_в_utf8(база);
    size_t д=strlen(б)+64;
    char*р=(char*)malloc(д);
    snprintf(р,д,"%s\\MazingStudio\\MS\\расширения",б);
    free(б);
    return р;
}
static char* папка_расширений_движка(void){
    wchar_t п[MAX_PATH];
    if(!GetModuleFileNameW(NULL,п,MAX_PATH))return NULL;
    wchar_t*с=wcsrchr(п,L'\\');
    if(с)*с=0;
    char*б=широкий_в_utf8(п);
    size_t д=strlen(б)+32;
    char*р=(char*)malloc(д);
    snprintf(р,д,"%s\\расширения",б);
    free(б);
    return р;
}
static char* загруженные_расширения[64];
static int загружено_расширений=0;
static int расширение_загружено(const char*имя){
    for(int i=0;i<загружено_расширений;i++)
        if(!strcmp(загруженные_расширения[i],имя))return 1;
    return 0;
}
static void импортировать(const char*имя,int строка){
    if(!имя||!*имя)ошибка_строка(строка,"пустое имя расширения");
    if(strstr(имя,"..")||strpbrk(имя,"\\/:*?\"<>|"))
        ошибка_строка(строка,"недопустимое имя расширения «%s»",имя);
    if(расширение_загружено(имя))return;
    if(загружено_расширений>=64)
        ошибка_строка(строка,"слишком много расширений (не больше 64)");

    char*базы[3]={папка_расширений_приложения(),папка_расширений_движка(),NULL};
    if(г_папка_скрипта[0]){
        size_t д=strlen(г_папка_скрипта)+64;
        char*с=(char*)malloc(д);
        snprintf(с,д,"%s\\расширения",г_папка_скрипта);
        базы[2]=с;
    }

    char путь[2048]="";
    int найдено=0, отключено=0;
    for(int i=0;i<3;i++){
        if(!базы[i])continue;
        snprintf(путь,sizeof(путь),"%s\\%s.ms",базы[i],имя);
        if(файл_есть(путь)){найдено=1;break;}
        snprintf(путь,sizeof(путь),"%s\\%s.ms.off",базы[i],имя);
        if(файл_есть(путь))отключено=1;
    }
    if(!найдено){
        for(int i=0;i<3;i++)free(базы[i]);
        if(отключено)
            ошибка_строка(строка,"расширение «%s» отключено. Включите его в Центре MS",имя);
        ошибка_строка(строка,"не найдено расширение «%s». Положите файл %s.ms в папку расширений",имя,имя);
    }

    Sb src;sb_init(&src);
    if(!прочитать_файл(путь,&src)){
        for(int i=0;i<3;i++)free(базы[i]);
        free(src.s);
        ошибка_строка(строка,"не удалось прочитать расширение «%s»",имя);
    }
    for(int i=0;i<3;i++)free(базы[i]);

    загруженные_расширения[загружено_расширений++]=strdup(имя);

    char**старые=ИСХОДНЫЕ_СТРОКИ;
    int старых=ИСХОДНЫХ_СТРОК;
    запомнить_строки(src.s);
    StmtList дерево=build(src.s);
    free(src.s);

    int ср=return_flag,сб=break_flag,сп=continue_flag;
    return_flag=0;break_flag=0;continue_flag=0;
    exec_body(дерево.a,дерево.len);
    return_flag=ср;break_flag=сб;continue_flag=сп;

    ИСХОДНЫЕ_СТРОКИ=старые;
    ИСХОДНЫХ_СТРОК=старых;
}

static void exec_stmt(Stmt*s){
    switch(s->kind){
        case C_LET:scope_set(&scopes[nscopes],s->name,eval(s->expr));break;
        case C_ASSIGN:{
            Val*nv=eval(s->expr);
            int found=0;
            for(int i=nscopes;i>=0&&!found;i--)for(Var*x=scopes[i].head;x;x=x->next)if(!strcmp(x->name,s->name)){x->val=nv;found=1;break;}
            if(!found){
                if(настр_строгость==0){
                    scope_set(&scopes[nscopes],s->name,nv);
                    предупреждение_строка(s->line,"переменная «%s» не объявлена — создана автоматически",s->name);
                }else{
                    ошибка_строка(s->line,"переменная «%s» не объявлена. Напишите: пусть %s = ...",s->name,s->name);
                }
            }
            break;
        }
        case C_PRINT:
            for(int i=0;i<s->nprint;i++){Val*v=eval(s->print[i]);if(i>0)ввывод(" ");val_print(v);}
            ввывод("\n");
            break;
        case C_IF:{
            /* цепочка: если / иначе если / иначе */
            Stmt*cur=s;
            while(cur){
                if(cur->kind==C_ELSE){exec_body(cur->body,cur->nbody);break;}
                Val*c=eval(cur->cond);
                int fl=to_bool(c);
                if(fl){exec_body(cur->body,cur->nbody);break;}
                cur=cur->next_else;
            }
            break;
        }
        case C_WHILE:
            while(1){
                Val*c=eval(s->cond);
                int fl=to_bool(c);
                if(!fl)break;
                exec_body(s->body,s->nbody);
                if(return_flag)break;
                if(break_flag){break_flag=0;break;}
                if(continue_flag){continue_flag=0;continue;}
            }
            break;
        case C_FOR:{
            Val*it=eval(s->expr);
            if(it->kind==V_LIST){
                for(LItem*li=it->head;li;li=li->next){
                    scope_set(&scopes[nscopes],s->name,li->v);
                    exec_body(s->body,s->nbody);
                    if(return_flag)break;
                    if(break_flag){break_flag=0;break;}
                    if(continue_flag){continue_flag=0;continue;}
                }
            }else if(it->kind==V_STR){
                /* перебор по буквам, с учётом UTF-8 */
                const unsigned char*p=(const unsigned char*)it->str;
                while(*p){
                    int len=1;
                    if((*p&0x80)==0)len=1;
                    else if((*p&0xE0)==0xC0)len=2;
                    else if((*p&0xF0)==0xE0)len=3;
                    else len=4;
                    char buf[8]={0};
                    for(int k=0;k<len;k++)buf[k]=p[k];
                    p+=len;
                    scope_set(&scopes[nscopes],s->name,va_str(buf));
                    exec_body(s->body,s->nbody);
                    if(return_flag)break;
                    if(break_flag){break_flag=0;break;}
                    if(continue_flag){continue_flag=0;continue;}
                }
            }else{
                ошибка_строка(s->line,"«для … в» перебирает список или строку. Например: для х в [1, 2, 3]:");
            }
            break;
        }
        case C_FOR_RANGE:{
            double от=как_число(eval(s->expr),s->line,"начало цикла");
            double до=как_число(eval(s->cond),s->line,"конец цикла");
            double шаг=s->step?как_число(eval(s->step),s->line,"шаг цикла"):1.0;
            if(шаг==0)ошибка_строка(s->line,"шаг цикла не может быть нулём");
            for(double х=от; шаг>0?х<=до:х>=до; х+=шаг){
                scope_set(&scopes[nscopes],s->name,va_num(х));
                exec_body(s->body,s->nbody);
                if(return_flag)break;
                if(break_flag){break_flag=0;break;}
                if(continue_flag){continue_flag=0;continue;}
            }
            break;
        }
        case C_BREAK:break_flag=1;break;
        case C_CONTINUE:continue_flag=1;break;
        case C_FUNC:{
            int found=-1;for(int i=0;i<nfuncs;i++)if(!strcmp(funcs[i].name,s->name))found=i;
            if(found<0){funcs=(Func*)realloc(funcs,(nfuncs+1)*sizeof(Func));nfuncs++;found=nfuncs-1;}
            funcs[found].name=s->name;funcs[found].body=s;
            break;
        }
        case C_IMPORT:{
            Val*и=eval(s->expr);
            if(и->kind!=V_STR)ошибка_строка(s->line,"после «подключить» нужно имя расширения в кавычках");
            импортировать(и->str,s->line);
            break;
        }
        case C_RETURN:return_val=s->expr?eval(s->expr):va_none();return_flag=1;break;
        case C_EXPR:eval(s->expr);break;
        default:break;
    }
}

static void exec_body(Stmt**b,int n){
    for(int i=0;i<n;i++){
        /* ветки иначе/иначе если исполняются через цепочку, отдельно не идут */
        if(b[i]->kind==C_ELSE)continue;
        if(b[i]->kind==C_IF&&b[i]->is_elseif)continue;
        exec_stmt(b[i]);
        if(return_flag||break_flag||continue_flag)return;
    }
}

/* ============================================================ */
/*  MAIN                                                          */
/* ============================================================ */

int main(int argc,char**argv){
#ifdef _WIN32
    подготовить_аргументы(&argc,&argv);   /* русские имена файлов в UTF-8 */
    загрузить_настройки();
    SetConsoleOutputCP((UINT)настр_кодировка);   /* кодировка из настроек */
    SetConsoleCP((UINT)настр_кодировка);
    подготовить_вывод();         /* вывод через Unicode, без кракозябр */
#endif
    setlocale(LC_ALL,"");
    setlocale(LC_NUMERIC,"C");   /* десятичная точка — как в коде 2.5 */
#ifdef _WIN32
    atexit(пауза_перед_закрытием);   /* не даём окну закрыться мгновенно */
#endif
    if(argc<2){
        ввывод("Язык MS (Mazing Studio) — русский язык программирования.\n");
        ввывод("Сайт студии: https://mazing-studio.site\n\n");
        ввывод("Запуск программы:   ms имя_файла.ms\n");
        ввывод("Справка:            ms --help\n");
        ввывод("Версия:             ms --version\n");
        return 1;
    }
    /* флаги: латинские всегда, русские — через GetCommandLineW (Windows) */
    int хочу_помощь=0,хочу_версию=0,хочу_настройки=0;
    if(!strcmp(argv[1],"--help")||!strcmp(argv[1],"-h")||!strcmp(argv[1],"/?"))хочу_помощь=1;
    if(!strcmp(argv[1],"--version")||!strcmp(argv[1],"-v"))хочу_версию=1;
    if(!strcmp(argv[1],"--настройки")||!strcmp(argv[1],"--settings"))хочу_настройки=1;
#ifdef _WIN32
    {
        wchar_t*cmd=GetCommandLineW();
        if(cmd){
            if(wcsstr(cmd,L"--помощь"))хочу_помощь=1;
            if(wcsstr(cmd,L"--версия"))хочу_версию=1;
            if(wcsstr(cmd,L"--настройки"))хочу_настройки=1;
        }
    }
#endif
    if(хочу_версию){
        ввывод("MS 1.1\n");
        ввывод("Русский язык программирования Mazing Studio. Движок на C, без Python.\n");
        return 0;
    }
    if(хочу_настройки){
        char*путь=широкий_в_utf8(настр_файл);
        ввывод("Настройки MS 1.1\n");
        if(настр_файл[0])ввывод("Файл: %s\n",путь);
        else ввывод("Файл: не найден, используются значения по умолчанию\n");
        ввывод("Кодировка вывода: %d\n",настр_кодировка);
        ввывод("Отступ: %d\n",настр_отступ);
        ввывод("Тема терминала: %s\n",настр_тема==1?"тёмная":настр_тема==2?"классическая":"светлая");
        ввывод("Строгость: %s\n",настр_строгость?"ошибки":"предупреждения");
        free(путь);
        return 0;
    }
    if(хочу_помощь){
        ввывод("ЯЗЫК MS — краткая справка\n");
        ввывод("=========================\n\n");
        ввывод("Запуск:  ms имя_файла.ms\n");
        ввывод("Справка: ms --помощь    Версия: ms --версия    Настройки: ms --настройки\n\n");
        ввывод("Команды:\n");
        ввывод("  пусть х = 5              создать переменную\n");
        ввывод("  х = 10   /   х += 1      изменить переменную\n");
        ввывод("  печатать(\"Привет\", х)    вывести (скобки можно не писать)\n");
        ввывод("  если условие:            условие\n");
        ввывод("  иначе если условие:      ещё одно условие\n");
        ввывод("  иначе:                   иначе\n");
        ввывод("  пока условие:            цикл с условием\n");
        ввывод("  для х в [1, 2, 3]:       перебор списка или строки\n");
        ввывод("  для и от 1 до 10:        счёт от 1 до 10 включительно\n");
        ввывод("  для и от 10 до 1 шаг -1: счёт с шагом\n");
        ввывод("  прервать                 выйти из цикла\n");
        ввывод("  продолжить               перейти к следующему шагу\n");
        ввывод("  делать имя(а, б):        объявить функцию\n");
        ввывод("  вернуть значение         вернуть из функции\n");
        ввывод("  подключить(\"имя\")        подключить расширение из папки расширения\n\n");
        ввывод("Значения: числа, \"текст\", правда, ложь, ничего, списки [1, 2, 3]\n");
        ввывод("Действия: + - * / // %% ^, сравнения == != < > <= >=, и / или / не\n");
        ввывод("Тексты: \"а\" + \"б\", \"а\" * 3, срез и сравнение по буквам\n\n");
        ввывод("Строки:\n");
        ввывод("  длина, взять, срез, подстрока, найти, индекс\n");
        ввывод("  верхний, нижний, обрезать, заменить, разделить, повторить\n");
        ввывод("  начинается_с, заканчивается_на, символ, код, перевернуть\n\n");
        ввывод("Числа:\n");
        ввывод("  корень, модуль, округлить, целое, степень, логарифм\n");
        ввывод("  синус, косинус, тангенс, пи, случайное, случайное_дробное\n");
        ввывод("  сумма, среднее, минимум, максимум, прописью, склонение\n\n");
        ввывод("Списки:\n");
        ввывод("  добавить, вставить, удалить, очистить, соединить, содержит\n");
        ввывод("  сортировать, срез, индекс, сумма / среднее / минимум / максимум\n\n");
        ввывод("Файлы и время:\n");
        ввывод("  прочитать, записать, дописать, строки_файла, существует\n");
        ввывод("  удалить_файл, размер_файла, файлы\n");
        ввывод("  время, дата, сейчас\n\n");
        ввывод("Ещё:\n");
        ввывод("  ввод(\"вопрос\"), строка, число, тип\n");
        ввывод("  ошибка(\"сообщение\") — остановить программу\n");
        ввывод("  выйти(код) — закончить программу\n\n");
        ввывод("Пример:\n");
        ввывод("  пусть имя = ввод(\"Как тебя зовут? \")\n");
        ввывод("  печатать(\"Привет,\", имя)\n");
        return 0;
    }
    /* скрытая самопроверка вывода: печатает русский текст и проверяет,
       что консоль получила его как Unicode (без кракозябр) */
    if(!strcmp(argv[1],"--output-check")){
        пауза_разрешена=0;
        const char*тест="Русский тест: Привет, мир! Ёжик";
        ввывод("%s\n",тест);
        fflush(stdout);
#ifdef _WIN32
        int нужно=MultiByteToWideChar(CP_UTF8,0,тест,-1,NULL,0);
        wchar_t*wb=(wchar_t*)malloc((size_t)нужно*sizeof(wchar_t));
        if(wb){
            MultiByteToWideChar(CP_UTF8,0,тест,-1,wb,нужно);
            DWORD записано=0;
            BOOL ок=WriteConsoleW(ВЫВОД_РУЧКА,wb,(DWORD)(нужно-1),&записано,NULL);
            FILE*выход=fopen("ms_output_check.txt","wb");
            if(выход){
                char строка[256];
                snprintf(строка,sizeof(строка),
                    "консоль=%d writeconsole=%d записано=%lu ожидалось=%d\n",
                    ВЫВОД_КОНСОЛЬ,(int)ок,(unsigned long)записано,нужно-1);
                fputs(строка,выход);
                fclose(выход);
            }
            free(wb);
        }
#endif
        return 0;
    }
    if(argv[1]&&argv[1][0]!='-'){
        snprintf(г_папка_скрипта,sizeof(г_папка_скрипта),"%s",argv[1]);
        char*с=strrchr(г_папка_скрипта,'\\');
        char*с2=strrchr(г_папка_скрипта,'/');
        if(с2>с)с=с2;
        if(с)*с=0;
        else г_папка_скрипта[0]=0;
    }
    FILE*f=открыть_для_чтения(argv[1]);
    if(!f){вошибка("Не могу открыть файл: %s\n",argv[1]);return 1;}
    Sb src;sb_init(&src);
    int c;while((c=fgetc(f))!=EOF)sb_ch(&src,(char)c);
    fclose(f);
    /* пропустить BOS (метку UTF-8) в начале файла, если есть */
    if(src.len>=3 && (unsigned char)src.s[0]==0xEF && (unsigned char)src.s[1]==0xBB && (unsigned char)src.s[2]==0xBF){
        src.s+=3; src.len-=3;
    }

    запомнить_строки(src.s);
    StmtList tree=build(src.s);
    nscopes=0;scopes[0].head=NULL;
    exec_body(tree.a,tree.len);
    return 0;
}