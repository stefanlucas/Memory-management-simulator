
Lucas Stefan Abe - 8531612
Mathias Van Sluys Menck - 4343470


Para compilar o programa deve-se rodar o Makefile.

O simulador não recebe nenhum argumento na linha de comando.
As instruções possíveis de serem executadas são:
  -substitui x, sendo x um número entre 1 e 4. Determina que algoritmo será usado se ocorrer PageFault.
    -1 = Optimal
    -2 = Second Chance
    -3 = Clock
    -4 = Least Recently Used 4a versão
  -espaco x, sendo x um número entre 1 e 4. Determina que algoritmo será usado para gerência de espaço livre.
    -1 = First Fit
    -2 = Next Fit
    -3 = Best Fit
    -4 = Worst Fit
  -carrega x, sendo x o nome ou endereço do arquivo de trace a ser usado na simulação.
  -executa x, sendo x um inteiro maior que 0, executa a simulação com as opções especificadas pelos comandos assima e imprime as memórias na tela de x em x unidades de tempo.
