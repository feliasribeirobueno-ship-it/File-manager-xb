
#include <xenon_soc/xenon_power.h>
#include <xenon_smc/xenon_smc.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <diskio/ata.h>
#include <diskio/usb.h>
#include <fat/fat.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ITENS 256
#define MAX_PATH  512

struct item { char nome[256]; int eh_dir; };

static struct item itens[MAX_ITENS];
static int  total_itens = 0;
static int  sel = 0;
static char caminho_atual[MAX_PATH] = "/";

static void montar_dispositivos(void)
{
    xenon_ata_init();
    usb_init();
    usb_do_poll();
    fatMountAll();
    fatMount("usb:", "usb", 0);
}

static int comparar(const void *a, const void *b)
{
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    if (ia->eh_dir != ib->eh_dir) return ib->eh_dir - ia->eh_dir;
    return strcasecmp(ia->nome, ib->nome);
}

static void listar(const char *path)
{
    DIR *d; struct dirent *e; struct stat st; char full[MAX_PATH];
    total_itens = 0; sel = 0;
    strcpy(itens[total_itens].nome, "..");
    itens[total_itens].eh_dir = 1;
    total_itens++;
    d = opendir(path);
    if (!d) return;
    while ((e = readdir(d)) != NULL && total_itens < MAX_ITENS) {
        if (!strcmp(e->d_name,".") || !strcmp(e->d_name,"..")) continue;
        snprintf(full, sizeof(full), "%s/%s", path, e->d_name);
        if (stat(full, &st) != 0) continue;
        strncpy(itens[total_itens].nome, e->d_name, 255);
        itens[total_itens].nome[255] = 0;
        itens[total_itens].eh_dir = S_ISDIR(st.st_mode) ? 1 : 0;
        total_itens++;
    }
    closedir(d);
    qsort(itens + 1, total_itens - 1, sizeof(struct item), comparar);
}

static void desenhar(void)
{
    int i;
    console_clrscr();
    printf("========================================\n");
    printf("      GERENCIADOR DE ARQUIVOS XB        \n");
    printf("========================================\n");
    printf(" Caminho: %s\n", caminho_atual);
    printf("----------------------------------------\n");
    for (i = 0; i < total_itens && i < 20; i++) {
        if (i == sel) printf(" >> "); else printf("    ");
        printf("%s %s\n", itens[i].eh_dir ? "[DIR]" : "[ARQ]", itens[i].nome);
    }
    printf("----------------------------------------\n");
    printf(" CIMA/BAIXO: navegar  A: abrir  B: voltar\n");
    printf(" X: HD  Y: USB  START: sair              \n");
}

static void entrar(void)
{
    char novo[MAX_PATH];
    if (!strcmp(itens[sel].nome, "..")) {
        char *p = strrchr(caminho_atual, '/');
        if (p && p != caminho_atual) *p = 0;
        else strcpy(caminho_atual, "/");
    } else if (itens[sel].eh_dir) {
        if (!strcmp(caminho_atual, "/"))
            snprintf(novo, sizeof(novo), "/%s", itens[sel].nome);
        else
            snprintf(novo, sizeof(novo), "%s/%s", caminho_atual, itens[sel].nome);
        strcpy(caminho_atual, novo);
    }
    listar(caminho_atual);
}

int main(void)
{
    unsigned int pad, pad_old = 0;
    xenon_init();
    console_init();
    console_clrscr();
    montar_dispositivos();
    listar("/");
    for (;;) {
        desenhar();
        pad = xenon_smc_read_gamepad(0);
        if ((pad & XENON_PAD_DOWN)  && !(pad_old & XENON_PAD_DOWN))  if (sel < total_itens - 1) sel++;
        if ((pad & XENON_PAD_UP)    && !(pad_old & XENON_PAD_UP))    if (sel > 0) sel--;
        if ((pad & XENON_PAD_A)     && !(pad_old & XENON_PAD_A))     entrar();
        if ((pad & XENON_PAD_B)     && !(pad_old & XENON_PAD_B))     { sel = 0; entrar(); }
        if ((pad & XENON_PAD_X)     && !(pad_old & XENON_PAD_X))     { strcpy(caminho_atual, "/");    listar(caminho_atual); }
        if ((pad & XENON_PAD_Y)     && !(pad_old & XENON_PAD_Y))     { strcpy(caminho_atual, "/usb"); listar(caminho_atual); }
        if ((pad & XENON_PAD_START) && !(pad_old & XENON_PAD_START)) break;
        pad_old = pad;
    }
    return 0;
}
