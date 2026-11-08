# <u>ZipMax Reborn</u>

Modernização do software de recompactação e otimização - ZipMax (base da versão 0.51).<br>
Troca do engine de compactação do ZipArchive, para o 7zip (código aberto e livre)

## <u>Breve Histórico</u>
Sempre utilizei um software gratuito e de código aberto de Roman Scherzer para economizar espaço na HD. </br>Original em: [Zipmax](https://mamedev.emulab.it/clrmamepro/#zipmax)</br>

Descobri o mesmo ao organizar coleções de ROMS, posteriormente foi útil pra tudo...<br>(arquivos de MID ou Wav compactados, Docs, Planilhas, ISOs...)</br>
Com as modernizações de sistema operacional, o programa começou a dar problemas, parando no meio da compactação de ISOs, deixando pastas temporárias, fechando ou travando do nada.

Fui atrás do código fonte, e por utilizar um componente do ZipArchive não conseguia de jeito nenhum corrigir e compilar para ver se os problemas paravam (principalmente com arquivos de nomes longos ou que utilizavam caracteres japoneses ou especiais).

Com o advento do Claude, DeepSeek, Qwen, finalmente consegui por em prática algumas idéias e melhorias, dentre elas:

1) Eliminar dependência do ZipArchive, utilizando o software livre 7zip (de código aberto)
2) Eliminar os bugs de recompactação de arquivos que possuiam caracteres especiais, em japonês ou chinês (ROMS, sobretudo)
3) Inserir arquivos e configurações em um menu da própria aplicação</br>
4) Configurar o INI por menu e não por edição de arquivo.
### Arquivos e configurações
<img width="705" height="181" alt="image" src="https://github.com/user-attachments/assets/18a2c6a9-2be9-451e-bff1-09be5521daf1" /> </br>
<img width="701" height="136" alt="image" src="https://github.com/user-attachments/assets/7ac2d244-954d-48a5-b547-1dbe1d3ecf2c" /></br>
5) Permitir parametrização da caixa de diálogo, com opções de tamanho antes, depois, etc..
<img width="508" height="362" alt="image" src="https://github.com/user-attachments/assets/7d600f61-b637-41af-a5e8-0e49ae0072e6" /></br>
6) Escolha do idioma via menu, sem ter que editar o INI
<img width="703" height="150" alt="image" src="https://github.com/user-attachments/assets/713422aa-0ce8-440e-849e-7e4576be07f9" /></br>
7) Documentar e criar um histórico interno (será repensado futuramente).
<img width="501" height="337" alt="image" src="https://github.com/user-attachments/assets/6be86417-d852-427e-ab52-69b0c2dcbe3e" /></br>


Não sou expert de programação (entendo apenas de arquivos em .BAT, HTML5, CSS e muito pouco Javascript).</br>
Porém foi possível compilar o projeto em Visual Studio 2026, com flags de otimizações e em AVX2.
<img width="708" height="443" alt="image" src="https://github.com/user-attachments/assets/f45330de-0752-4847-af9d-c4a7e2c4bcad" /></br>
Até agora... funcionou sem problemas!</br>

O uso desse programa fica por sua conta e risco!</br>
Espero que seja útil pra alguém, em uma época onde SSD/M2 e RAM estão a preço de ouro.

# Mudanças das últimas 3 versões:
## Versão 1.2.1 - Escolha do idioma no menu

* **Configurações > Idioma:** lista os idiomas disponíveis (English e Português (Brasil)), marca o que está em uso e
  troca na hora, sem reiniciar. A escolha é gravada no `zipmax.ini` (`language = ...`).<br>Para adicionar um idioma, coloque um `.lng` na pasta `language` com a chave `IDS_LANGNAME` (nome exibido): ele aparece na lista sozinho. A troca fica indisponível enquanto o ZipMax está compactando.
* **Bug de fechamento inesperado:** quando o arquivo a ser compactado possuia %_ em seu nome ou sequências específicas, o programa fechava sem nenhum aviso, interrompendo o trabalho. Gerava um dump na pasta Users\Roaming. Através dele implementamos as correções.

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
* **Criação de documentação para evoluir o programa:** `docs\DEVELOPMENT.md` (arquitetura e receitas: texto traduzível, coluna,
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

Os fontes do 7-Zip 26.05 necessários já estão na pasta `7zip\`. Para usar outra cópia:
`msbuild ZipMax.sln /p:SevenZipDir=C:\caminho\7zip\` (a pasta que contém `C` e `CPP`).

### Para executar
* Copie o `zipmax.ini` para a pasta do `ZipMax.exe`.
* Modos DLL: coloque o `7z.dll` (mesma arquitetura do exe) na pasta do ZipMax, ou instale o 7-Zip
  (a pasta padrão é procurada), ou use a nova chave opcional `sevenzip-dll = C:\...\7z.dll` no ini.
* O empacotador externo e outros opcionais do INI (`7za.exe` por padrão) continuam funcionando como antes.

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

## Estado dos testes:

* **Testado de verdade (Linux, g++, 7-Zip 26.05):** a camada `Zip7Engine` nos dois modos (7z.so via `dlopen` e
  7-Zip ligado estaticamente, usando exatamente a lista de arquivos do `Release Static`): criar, extrair, testar,
  ler cabeçalhos, ZIP corrompido, "zip slip", nomes UTF-8 e com `\`, e a montagem do ZIP final por cópia de
  fluxos (conferido com o `zipfile` do Python). Fontes em `tests\` (`test_engine.cpp` e `test_inifile.cpp`, este último também testa a gravação do `zipmax.ini` com os comentários preservados).
* **Versão 1.2.0:** o código MFC novo (menus, `OptionsDlg`, `ViewSettingsDlg`, `AboutDlg`, `ViewConfig`) e o `.rc` novo
  (menu e três diálogos) **não foram compilados**. Só a leitura/gravação do `.ini` (`IniFile`) foi testada de verdade (Linux).
* Itens de risco a observar no primeiro build: a conversão para Unicode (muitos literais viraram `_T("...")`) e
  o novo controle `GreenProgress`.

# Disclaimer
Todos os direitos reservados aos criadores (fique claro que em nenhum momento tomo crédito do trabalho deles):</br>
Zipmax - Roman Scherzer (https://mamedev.emulab.it/clrmamepro/#zipmax)</br>
7Zip - Igor Pavlov (https://www.7-zip.org)

Modernização e features da versão 1.1.0 em diante:</br>
Zipmax Reborn - Lincoln H.
