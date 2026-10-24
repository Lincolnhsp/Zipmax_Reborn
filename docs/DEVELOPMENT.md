# ZipMax Reborn - guia de desenvolvimento

Este documento explica como o programa é organizado e **como acrescentar as coisas mais comuns** (texto traduzido,
coluna, item da janela, parâmetro do `.ini`, item de menu). Leia junto com o `README.md` (como compilar).

## 1. Como o ZipMax funciona (resumo)

Para cada zip da lista, uma thread (`CZipMaxDlg::WorkThread`) faz:

1. **Descompacta** o zip numa pasta temporária (`_unzip`) - 7-Zip (`Zip7::ExtractAll`).
2. **Recompacta** o conteúdo com vários métodos: a compactação *interna* (Deflate máximo do 7-Zip,
   `Zip7::CompressFiles`, pasta `_intern`) e os empacotadores *externos* do `zipmax.ini` (`packer-exe-N`).
3. **Escolhe, arquivo por arquivo, o menor fluxo comprimido** entre o zip original e todos os novos.
4. **Monta o zip final copiando os fluxos byte a byte** (`BuildNewZip`, `WriteLFH`, `WriteCFH`, `WriteEOZ`) - por isso
   precisa dos cabeçalhos "crus" (`Zip7::ReadHeaders`), que o 7-Zip não expõe.
5. **Testa** o zip novo (`Zip7::TestArchive`) e, se ficou menor, substitui o original.

## 2. Mapa dos arquivos (`zipmax\`)

| Arquivo | Para que serve |
|---|---|
| `ZipMax.cpp/.h` | Aplicativo: lê o `zipmax.ini` (`LoadSettings`, `ReloadSettings`), inicia o 7-Zip e a janela. Guarda as variáveis `g_...` |
| `ZipMaxDlg.cpp/.h` | **Janela principal**: lista, menus, thread de trabalho, colunas/itens dinâmicos (`ApplyView`, `LayoutControls`) |
| `Zip7Engine.cpp/.h` | **Camada do 7-Zip** (sem MFC): extrair, compactar, testar, ler cabeçalhos, descrever o método. Modo DLL ou estático |
| `OptionsDlg.cpp/.h` | Configurações > Opções (parâmetros do `zipmax.ini`) |
| `IniFile.cpp/.h` | Lê/grava o `zipmax.ini` preservando comentários (sem MFC; testado no Linux) |
| `ViewSettingsDlg.cpp/.h` | Configurações > Caixa de diálogo |
| `ViewConfig.cpp/.h` | Quais colunas/itens aparecem e em que ordem (registro do Windows) |
| `AboutDlg.cpp/.h`, `Version.h` | Caixa Sobre e número da versão |
| `Language.cpp/.h`, `LangIds.h` | Textos traduzíveis (`language\*.lng`) e lista de idiomas do menu |
| `GreenProgress.cpp/.h` | Barra de progresso verde com a porcentagem |
| `misc.cpp/.h` | Utilitários (arquivos, pastas, ini antigo, processos) |
| `ZipMax.rc`, `resource.h` | Menu, diálogos, textos em inglês (padrão), versão |

Fora de `zipmax\`: `language\` (idiomas), `7zip\` (fontes do 7-Zip usados no build), `tests\`, `docs\`.

## 3. Convenções

- **Unicode (UTF-16)** em todo o programa: use `CString`, `TCHAR`, `_T("...")` e as funções `W`/`T` do Windows.
  Nada de `char`/`LPSTR` para nomes de arquivo (é o que quebrava nomes japoneses). Só `ZipHeader::rawName` é
  `std::string`, porque são bytes crus do zip.
- **Todo texto visível ao usuário é traduzível** (veja 4.1). Não escreva texto fixo em inglês no código.
- Configuração do **processamento** (empacotadores, flags) fica no `zipmax.ini`; configuração da **aparência**
  (larguras, colunas, itens, posição da janela) fica no registro (`HKCU\Software\ZipMax`).
- O thread de trabalho não mexe em controles além dos ponteiros já passados em `THREADPARAM`.

## 4. Receitas

### 4.1 Acrescentar um texto traduzível
1. `resource.h`: `#define IDS_MEU_TEXTO <próximo número>` (os textos vão de 20000 em diante).
2. `ZipMax.rc`, string table: `IDS_MEU_TEXTO "English text"` (**só ASCII**; o `.rc` é cp1252).
3. `LangIds.h`: acrescente `{ "IDS_MEU_TEXTO", IDS_MEU_TEXTO },`.
4. `language\English.lng` e `language\Portugues-Brasil.lng`: `IDS_MEU_TEXTO = "..."` (UTF-8).
5. No código: `CLang::Get(IDS_MEU_TEXTO)`.

Regras: se o texto tem `%d`, `%s`... a tradução precisa ter os mesmos, na mesma ordem (senão é ignorada).
Para texto de botão/rótulo de diálogo use uma chave `IDC_xxx` e `CLang::TranslateDialog(...)`.

### 4.1.1 Acrescentar um idioma novo
1. Copie `language\English.lng` para `language\<Nome>.lng` (ex.: `Espanol.lng`) e traduza os textos (UTF-8).
2. Defina a chave `IDS_LANGNAME` do arquivo com o nome que deve aparecer no menu (ex.: `"Español"`).
3. Pronto: o idioma aparece sozinho em Configurações > Idioma (a lista é montada lendo a pasta `language`,
   `CLang::ListLanguages`) e pode ser usado no `zipmax.ini` com `language = Espanol`. Nenhuma linha de código muda.

Quem escolhe o idioma pelo menu grava `language = <Nome>` no `zipmax.ini` (`CIniFile::SetValue`).

### 4.2 Acrescentar uma coluna na lista de arquivos
1. `ViewConfig.h`: novo valor no `enum` das colunas, **antes** de `NUMCOLUMNS` (o número nunca muda depois).
2. `ViewConfig.cpp`: `ColumnNameId` (texto do título) e `SetDefaults` (visível por padrão?).
3. `ZipMaxDlg.cpp`: `GetColumnInfo` (título e alinhamento) e `UpdateList` (`SetItem` do novo subitem; o número do
   subitem é o da coluna). Se for numérica, inclua-a em `CompareProc`.
4. Larguras: crie `g_nWidthN` em `ZipMax.h/.cpp` (padrão, leitura e gravação no registro) e inclua em
   `SaveColumnState`/`ApplyView`.

### 4.3 Acrescentar um item na base da janela
`ViewConfig.h` (enum `ITEM_...`), `ViewConfig.cpp` (`ItemNameId`, padrão), altura em `LayoutControls`
(`kHeight` e um `case`), e o controle em `OnInitDialog`.

### 4.4 Acrescentar um parâmetro do `zipmax.ini`
1. Ler: `CZipMaxApp::LoadSettings` (`g_...` em `ZipMax.h`).
2. Tela de opções: `OptionsDlg.cpp` - parâmetro liga/desliga entra na tabela `kBoolOptions` (com um texto
   `IDS_OPT_...`, veja 4.1); parâmetro com valor entra em `LoadOptions` (tipo `OPT_TEXT`, `OPT_PATH` ou lista).
3. `IniFile.cpp`: inclua a chave em `IsManagedKey` (senão a linha antiga não é removida ao gravar).
4. Documente em `zipmax.txt` e no `zipmax.ini`.

### 4.5 Acrescentar um item de menu
`ZipMax.rc` (menu `IDR_MAINMENU`), `resource.h` (`IDM_...`), texto `IDS_MI_...` (4.1), `ApplyMenuLanguage`,
`ON_COMMAND` no `BEGIN_MESSAGE_MAP` e a função `OnXxx`. Se não pode ser usado durante a compactação, trate em
`OnInitMenuPopup` e use `CheckNotBusy()`.

### 4.6 Lançar uma versão
`Version.h`, `FILEVERSION`/`PRODUCTVERSION` no `ZipMax.rc`, `docs\CHANGELOG.md` e o texto da caixa Sobre.

## 5. Testes

Sem Visual Studio, a parte **sem MFC** pode ser testada no Linux (g++):
`tests\test_engine.cpp` (camada do 7-Zip, DLL e estático) e `tests\test_inifile.cpp` (leitura/gravação do `.ini`).
O código MFC precisa ser compilado e testado no Visual Studio. Lista de verificação manual após mudanças na janela:
arrastar zips, menu Arquivos (arquivo, vários, pasta), Opções (marcar/desmarcar e conferir o `zipmax.ini`),
Caixa de diálogo (ligar/desligar/reordenar colunas e itens, reabrir o programa), trocar o idioma, Sobre.

## 6. Limitações conhecidas
- Zips ZIP64 e multi-volume são recusados.
- Zips protegidos por senha não são suportados.
- Mudar o `sevenzip-dll` ou os empacotadores não afeta uma compactação em andamento (por isso Opções fica
  indisponível enquanto compacta).
