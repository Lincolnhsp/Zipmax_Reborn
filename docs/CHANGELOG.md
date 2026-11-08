# Changelog do ZipMax Reborn

Formato: versão, e o que mudou (do mais novo para o mais antigo). **Ao implementar uma melhoria, acrescente-a aqui
e ao texto da caixa Sobre** (`IDS_ABOUT_TEXT` em `zipmax\ZipMax.rc` e em `language\*.lng`).

## 1.2.2
- **Correção de falha:** o programa fechava sem mensagem quando o nome de um zip tinha o caractere `%` (ex.:
  `[T+Bra100%_Nome].zip`). O nome do arquivo estava dentro do texto de formato de `CString::Format`, e o `%_` fazia o
  runtime do C abortar o processo (`0xC0000409`, `__fastfail(5)`). Agora só o texto fixo é formato e o nome é anexado depois.
- Linha de comando dos empacotadores: `%2` é trocado antes de `%1`, para que um `%2` dentro do nome do zip não seja alterado.
- A thread de trabalho passou a capturar exceções inesperadas (o programa não some; a linha de status mostra ERROR).

## 1.2.1
- Configurações > **Idioma**: escolha do idioma direto no menu (English e Português (Brasil) acompanham o programa).
  A lista é montada com os arquivos da pasta `language`, o idioma em uso fica marcado, a troca vale na hora e é
  gravada no `zipmax.ini` (`language = ...`). Qualquer `.lng` acrescentado aparece na lista automaticamente.

## 1.2.0
- Barra de menus: **Arquivos**, **Configurações** e **Sobre**.
- Menu Arquivos: adicionar um arquivo, vários arquivos, uma pasta ou uma pasta com subpastas; limpar a lista; sair.
- Configurações > Opções: todos os parâmetros do `zipmax.ini` com caixa de seleção (e texto/lista para os que
  precisam de valor); gravado no `zipmax.ini` (comentários preservados, backup em `zipmax.ini.bak`) e aplicado
  na hora, sem reiniciar.
- Configurações > Caixa de diálogo: escolher quais colunas e itens a janela mostra e em que ordem (também dá para
  arrastar os títulos das colunas). O padrão voltou a ser o visual original do ZipMax.
- Caixa Sobre com a descrição do projeto e a lista de melhorias.

## 1.1.0
- Suporte completo a Unicode (nomes de arquivo em qualquer idioma, ex.: japonês); log em UTF-8.
- Arquivos de idioma (`language\*.lng`): English e Português (Brasil).
- Barra de progresso total verde, com a porcentagem dentro.
- Novas colunas: Size Before, Size After e Type | Compression | Dictionary | Method.
- Limpeza das pastas temporárias mais robusta (somente leitura, ocultos, caminhos longos).

## 1.0.0
- A biblioteca ZipArchive foi substituída pelo 7-Zip (`7z.dll` ou 7-Zip ligado estaticamente no `ZipMax.exe`).
- Compila no Visual Studio 2026 (x64 e Win32).

## 0.51 (ZipMax original, Roman Scherzer, 2002-2005)
- Versão que serviu de base.
