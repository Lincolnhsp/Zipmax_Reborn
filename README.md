# Zipmax_Reborn
Modernização do software de recompactação e otimização - ZipMax

Sempre utilizei um software gratuito e de código aberto de Roman Scherzer para economizar espaço na HD. </br>
Original em: [Zipmax](https://mamedev.emulab.it/clrmamepro/#zipmax)</br>


Descobri o mesmo ao tentar organizar coleções de ROMS, posteriormente foi útil pra tudo... </br>
(arquivos de MID ou Wav compactados, Docs, Planilhas, ISOs...)</br>
Com as modernizações de sistema operacional, o programa começou a dar problemas, parando no meio da compactação de ISOs, deixando pastas temporárias, travando do nada.

Fui atrás do código fonte, e por utilizar um componente do ZipArchive não conseguia de jeito nenhum corrigir e compilar para ver se os problemas paravam (principalmente com arquivos de nomes longos ou que utilizavam caracteres japoneses).

Com o advento do Claude, DeepSeek, Qwen e Flint, finalmente consegui por em prática algumas idéias e melhorias, dentre elas:

1) Eliminar dependência do ZipArchive, utilizando o software livre 7zip (de código aberto)
2) Eliminar os bugs de recompactação de arquivos que possuiam caracteres em japonês ou chinês (ROMS, sobretudo)
3) Inserir arquivos e configurações em um menu </br>
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

# Disclaimer
Todos os direitos reservados aos criadores:</br>
Zipmax - Roman Scherzer (https://mamedev.emulab.it/clrmamepro/#zipmax)</br>
7Zip - Igor Pavlov (https://www.7-zip.org)

Modernização e features da versão 1.2.1:</br>
Zipmax Reborn - Lincoln H.
