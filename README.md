# ZipMax Reborn (ZipMax 0.51 + 7-Zip 26.04)

ZipMax 0.51 de Roman Scherzer, refatorado para compilar no **Visual Studio 2026**.
A biblioteca **ZipArchive** (que sumiu da internet) foi **removida** e todo o trabalho com ZIP
que ela fazia agora é feito pelo **7-Zip**.

## Versão 1.2.1 - escolha do idioma no menu

* **Configurações > Idioma:** lista os idiomas disponíveis (English e Português (Brasil)), marca o que está em uso e
  troca na hora, sem reiniciar. A escolha é gravada no `zipmax.ini` (`language = ...`). Para adicionar um idioma,
  coloque um `.lng` na pasta `language` com a chave `IDS_LANGNAME` (nome exibido): ele aparece na lista sozinho.
  A troca fica indisponível enquanto o ZipMax está compactando.

## Versão 1.2.0 - menus e configurações

* **Barra de menus:** *Arquivos*, *Configurações* e *Sobre* (traduzidos conforme o idioma escolhido).
* **Arquivos:** adicionar um arquivo, vários arquivos, uma pasta ou uma pasta com subpastas (abre a janela de busca do
  Windows); limpar a lista; sair.
* **Configurações > Opções (zipmax.ini):** todos os parâmetros do `zipmax.ini` numa lista com caixas de seleção
  (marcar = ativar, desmarcar = desativar). Os que precisam de valor (prioridade, idioma, `7z.dll`, empacotadores
  externos, etapa final) têm um campo de texto, de caminho ou uma lista de opções logo abaixo. O arquivo é gravado
  preservando os comentários (com backup em `zipmax.ini.bak`) e as mudanças valem na hora, sem reiniciar.
* **Configurações > Caixa de diálogo:** escolha quais **colunas** da lista e quais **itens** da base da janela aparecem,
  e em que ordem (botões Mover para cima/baixo; também dá para arrastar os títulos das colunas). **O padrão é o visual
  original do ZipMax**: Size Before, Size After, as informações do método e a barra verde começam desligados, é só marcá-los.
* **Sobre:** descreve o ZipMax Reborn e lista as melhorias de cada versão.
* **Documentação para evoluir o programa:** `docs\DEVELOPMENT.md` (arquitetura e receitas: texto traduzível, coluna,
  item da janela, parâmetro do .ini, menu) e `docs\CHANGELOG.md`.

## Versão 1.1.0

* **Unicode:** o ZipMax agora é um programa Unicode (UTF-16). Nomes de arquivo e de pastas em qualquer idioma
  (japonês, etc.) funcionam: extração, compactação, lista, log (gravado em UTF-8) e `zipmax.ini`.
  Corrige o erro `Can't read some files while compressing` e a cascata de `CAN'T CLEAN TEMPORARY-FOLDERS`
  (a limpeza também passou a apagar arquivos somente leitura, ocultos/de sistema e caminhos longos).
* **Idiomas:** pasta `language\` com todos os textos do programa (`English.lng` e `Portugues-Brasil.lng`).
  Escolha no `zipmax.ini`: `language = Portugues-Brasil` (ou `auto`, que segue o idioma do Windows).
  O padrão continua sendo inglês. Para criar outro idioma, copie `English.lng`, traduza e use o nome do arquivo.
  Linhas ausentes ou com `%d/%s` diferentes do inglês são ignoradas (usa-se o inglês embutido).
* **Barra de progresso verde** na base da janela, com a porcentagem escrita dentro, que enche conforme o
  progresso total (peso pelo tamanho dos arquivos e pelas etapas de cada um).
* **Novas colunas** entre *Status* e *Gained Free Bytes*: **Size Before**, **Size After** e
  **Type | Compression | Dictionary | Method** (ex.: `ZIP | Deflate/Store | Dict: 32 KB | Method: Deflate, Store`,
  lido do zip que ficou no disco). As colunas numéricas ordenam como número. As larguras ficam salvas.
  Se a janela estiver pequena (tamanho salvo antes), aumente-a: o padrão novo é mais largo.

## Como compilar

1. Abra `ZipMax.sln` no Visual Studio 2026
   (workload *Desktop development with C++* + componente *C++ MFC for latest v14x build tools*).
2. Escolha a configuração e a plataforma (x64 ou Win32):

| Configuração | O que faz |
|---|---|
| `Debug`, `Release` | O ZipMax carrega o **7z.dll** em tempo de execução (LoadLibrary). |
| `Release Static` | O código do 7-Zip (handler de ZIP + Deflate etc.) é **compilado dentro do ZipMax.exe**. Não precisa de 7z.dll. |

3. *Build → Build Solution*. Saída: `build\<plataforma>\<configuração>\ZipMax.exe`.

Os fontes do 7-Zip 26.04 necessários já estão na pasta `7zip\`. Para usar outra cópia:
`msbuild ZipMax.sln /p:SevenZipDir=C:\caminho\7zip\` (a pasta que contém `C` e `CPP`).

### Para executar
* Copie o `zipmax.ini` para a pasta do `ZipMax.exe`.
* Modos DLL: coloque o `7z.dll` (mesma arquitetura do exe) na pasta do ZipMax, ou instale o 7-Zip
  (a pasta padrão é procurada), ou use a nova chave opcional `sevenzip-dll = C:\...\7z.dll` no ini.
* O empacotador externo (`7za.exe` por padrão) continua funcionando como antes.

## O que mudou

| Antes (ZipArchive) | Agora (7-Zip) |
|---|---|
| `UnzipAllFiles`: `CZipArchive::ExtractFile` | `Zip7::ExtractAll` (IInArchive do 7-Zip; restaura data e atributos; recusa caminhos com `..`, absolutos ou `:`) |
| `InternalZipFiles`: `AddNewFile(... zipsmSmartest)` | `Zip7::CompressFiles` (IOutArchive do 7-Zip, Deflate, nível 9, 15 passadas, 258 fast bytes; grava "Store" quando o Deflate não compensa) |
| `TestNewZip`: `CZipArchive::TestFile` | `Zip7::TestArchive` (teste de CRC/descompressão do 7-Zip) |
| `GetHeaders`: `CZipFileHeader` | `Zip7::ReadHeaders` (ver nota abaixo) |

Todo o restante foi mantido: arrastar/soltar, linha de comando/SendTo, pastas recursivas, pausa,
log, ordenação das colunas, vários empacotadores externos, `finalstep`, `SkipReadOnly/SkipArchive/
SetCurrentReadOnly`, escolha do menor fluxo por arquivo (ORG/INT/01..99), `+a`, preservação de data, etc.

**Nota sobre `ReadHeaders`:** o ZipMax monta o ZIP final copiando os fluxos comprimidos *byte a byte* e
reescrevendo os cabeçalhos, então precisa dos campos crus do diretório central (versão, flags, hora/data DOS,
atributos internos/externos, offset). A API do 7-Zip não expõe esses campos crus; por isso essa leitura é um
parser pequeno (~100 linhas) dentro de `Zip7Engine.cpp`. Os nomes também passam a ser comparados já
decodificados (UTF-8 ou OEM) e com `\` = `/`, o que evita falsos "erros" com nomes acentuados.

### Correções feitas para compilar / robustez
`stdafx.h` (alvo Windows 7+, Unicode), escopo de variáveis de `for` (`i`, `x`), `delete[]`, ponteiro+char em
`FileExists`, `CreateProcess` com buffer gravável, `CString` em `Format`, `.rc` sem os recursos alemães
(`l.deu`) e sem o manifesto duplicado, novo `.sln/.vcxproj` (VS 2026, toolset padrão do VS instalado),
ganho calculado com sinal, verificação de leituras curtas ao copiar fluxos, recusa de ZIP64 e multi-volume.

## Estado dos testes (leia isto)

* **Não havia Visual Studio disponível onde isto foi feito.** Os arquivos MFC (`ZipMaxDlg`, `ZipMax`, `misc`,
  `.rc`) e os `.vcxproj` **não foram compilados**; foram revisados à mão. Se aparecer algum erro de compilação
  ao abrir no VS 2026, me envie a mensagem que eu corrijo.
* **Testado de verdade (Linux, g++, 7-Zip 26.04):** a camada `Zip7Engine` nos dois modos (7z.so via `dlopen` e
  7-Zip ligado estaticamente, usando exatamente a lista de arquivos do `Release Static`): criar, extrair, testar,
  ler cabeçalhos, ZIP corrompido, "zip slip", nomes UTF-8 e com `\`, e a montagem do ZIP final por cópia de
  fluxos (conferido com o `zipfile` do Python). Fontes em `tests\` (`test_engine.cpp` e `test_inifile.cpp`, este último também testa a gravação do `zipmax.ini` com os comentários preservados).
* **Versão 1.2.0:** o código MFC novo (menus, `OptionsDlg`, `ViewSettingsDlg`, `AboutDlg`, `ViewConfig`) e o `.rc` novo
  (menu e três diálogos) **não foram compilados**. Só a leitura/gravação do `.ini` (`IniFile`) foi testada de verdade (Linux).
* Itens de risco a observar no primeiro build: a conversão para Unicode (muitos literais viraram `_T("...")`) e
  o novo controle `GreenProgress`.
