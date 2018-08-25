#include <iostream> /* getline */
#include <fstream> // std::ifstream
#include <string>  // string class
#include <cstdio> /* readline, FILE */
#include <cstdlib> /* malloc, free */
#include <cstring> /* strcmp, strtok */
#include <readline/readline.h> /* readline */
#include <readline/history.h>/* add_history */
#include <vector> /* std::vector */
#include <queue>  /* std::priority_queue */
#include <list>  /*std::list*/
#include <time.h> /*clock*/

using namespace std;

struct processo {
    int pid; // pid do processo
    int t0; // tempo em que o processo chega 
    char *nome; // nome do processo
    int tf; // tempo em que o processo termina
    int b; // quantidade de unidades de alocação ocupada pelo processo
    int posp; // posição no vetor p do próximo endereço a ser acessado
    int post; // posição no vetor t do próximo instante de tempo a acessar algo
    int endereco; // endereço do processo na memória virtual
    vector<int> p; // vetor de endereços acessados na memória física
    vector<int> t; // vetor de instante de tempo que ocorre acesso na memória física
};

typedef struct processo processo;

struct comparador1 {
    bool operator()(processo const &a, processo const &b) { 
       return a.t[a.post] > b.t[b.post];
    }
};

struct comparador2 {
    bool operator()(processo const &a, processo const &b) { 
       return a.tf > b.tf;
    }
};

int total; // total de memória física
int virt; // total de memória virtual
int s; // tamanho da unidade de alocação para gerência de espaço livre
int p; // tamanho da página a ser considerada para os algoritmos de substituição
int algoritmoEspaco; // algoritmo para gerência de espaço livre
int algoritmoSubs; // algoritmo para a substituição de página
int intervalo = 0; // intervalo de tempo para imprimir o conteúdo das memórias
int next; // variável usado no algoritmo de gerência de memória next fit

int inicio, fim;

vector<processo> v1; // vetor dos processos lidos
vector<bool> bitvirt, bitmem; // bitmaps das memórias virtual e real
vector<bool> bitr; // bit r das páginas
vector<int> localPagina, conteudoQuadro; // localPagina[i] é quadro correspondente da pagina i
vector<int> clk;
vector<int> lru;
vector<bool> emUso;
list<int> secondChance;


/* Faz a simulação da gerência de memória*/
void simula ();

/* separa a linha em tokens com espaço como delimitador e manda para o vetor de parâmetros*/
void tokens (char *line, char *parameters[]);

/* Executa a gerência de memória para alocar o processo proc na memória virtual, retorna o endereço de proc*/
int alocaMemoria (processo proc);

/* Função que insere uma página acessa pelo processo proc na memória física, utilizando diversos
    algoritmos de substituição de página*/
void inserePagina (processo proc, int tempo);

void imprime ();

/* Função que retira o processo proc da memória virtual*/
void retira (processo proc);

int main () {
    char prompt[] = "(ep3): ", *line, *parameters[100];
    int num = 0, pid;
    parameters[0] = NULL;

    while (1) {
        line = readline (prompt);
        if (line == NULL || strcmp (line, "") == 0) {
            printf ("Comando inválido, digite novamente.\n");
            continue;
        }
        add_history (line);
        tokens (line, parameters);

        if (strcmp (parameters[0], "carrega") == 0) {
            v1.clear ();

            ifstream arquivo (parameters[1]);
            if (!arquivo) {
                printf("Erro ao ler arquivo, digite o comando novamente\n");
                continue;
            }
            string linha;
            int cont = 0;
            pid = 0;
            while (getline (arquivo, linha)) {
                char *trace = new char[linha.length() + 1];
                strcpy (trace, linha.c_str ());
                if (cont == 0) {
                    total = atoi (strtok (trace, " "));
                    virt = atoi (strtok (NULL, " "));
                    s = atoi (strtok (NULL, " "));
                    p = atoi (strtok (NULL, " "));
                }
                else {
                    processo aux;
                    char *p, *t;
                    aux.pid = pid++;
                    aux.t0 = atoi (strtok (trace, " "));
                    aux.nome = strtok (NULL, " ");
                    aux.tf = atoi (strtok (NULL, " "));
                    aux.b = atoi (strtok (NULL, " "));
                    aux.posp = aux.post = 0;
                    while ((p = strtok (NULL, " ")) != NULL) {
                        t = strtok (NULL, " ");
                        aux.p.push_back (atoi (p));
                        aux.t.push_back (atoi (t));
                    }
                    aux.p.push_back (-1);
                    aux.t.push_back (-1);
                    v1.push_back (aux);
                }
                cont++;
            }
            arquivo.close ();
        }
        else if (strcmp (parameters[0], "espaco") == 0) {
            num = atoi (parameters[1]);
            algoritmoEspaco = num;
        }
        else if (strcmp (parameters[0], "substitui") == 0) {
            num = atoi (parameters[1]);
            algoritmoSubs = num;
        }
        else if (strcmp (parameters[0], "executa") == 0) {
            int i;
            num = atoi (parameters[1]);
            intervalo = num;

            /* Inicializando variáveis e estruturas de dados*/
            secondChance.clear ();
            clk.clear ();
            lru.clear();
            emUso.clear();
            inicio = fim = 0;
            bitvirt.clear (); 
            bitmem.clear ();
            bitr.clear (); 
            localPagina.clear ();
            conteudoQuadro.clear ();
            for (i = 0; i < v1.size (); i++) v1[i].posp = v1[i].post = 0;
            next = 0;
            bitvirt.resize(virt/p, false);
            bitmem.resize(total/p, false);
            bitr.resize (virt/p, false);
            localPagina.resize (virt/p, -1);
            conteudoQuadro.resize (total/p, 0);
            clk.resize(total/p);
            lru.resize(total/p, 236);
            emUso.resize (virt/p, false);

            simula ();
        }
        else if (strcmp (parameters[0], "sai") == 0) {
            break;
        }
        else {
            printf ("Comando inválido, digite novamente.\n"); 
        }
    }   

    return 0;
}

void simula () {
    int tempo, cont, i;
    FILE* memoVirtual = fopen ("/tmp/ep3.vir", "wb");
    FILE* memoFisica = fopen ("/tmp/ep3.mem", "wb");
    int menos1 = -1;
    priority_queue<processo, vector<processo>, comparador1> q1; // fila de prioridade ordenada pelo instante da proxima execução do processo
    priority_queue<processo, vector<processo>, comparador2> q2; // fila de prioridade ordenada pelo instante de fim de execução do processo

    for (i = 0; i < total; i++){
        fwrite (&menos1, sizeof (menos1), 1, memoFisica);
        bitmem[i%p] = false;
    }
    for (i = 0; i < virt; i++) {
        fwrite (&menos1, sizeof (menos1), 1, memoVirtual);
        bitvirt[i%p] = false;
    }
    fclose (memoFisica);
    fclose (memoVirtual);

    tempo = cont = 0;
    /* Condição de parada, agora está correta */
    while (cont < v1.size () || !q2.empty ()) {  
        while (!q2.empty () && q2.top().tf == tempo) {
            processo proc = q2.top ();
            q2.pop ();
            retira (proc);
        }
        while (cont < v1.size () && v1[cont].t0 == tempo) {
            v1[cont].endereco = alocaMemoria (v1[cont]);
            q1.push (v1[cont]);
            q2.push (v1[cont]);
            cont++;
        }
        while (!q1.empty ()) {
            processo proc = q1.top ();
            if (proc.t[proc.post] < tempo) q1.pop ();
            else if (proc.t[proc.post] == tempo) {
                inserePagina (proc, tempo);
                v1[proc.pid].post++; v1[proc.pid].post++;
                proc.post++; proc.posp++;
                q1.pop ();
                q1.push (proc);
            }
            else break;
        }
        
        if (tempo%intervalo == 0) {
            printf ("t = %d\n", tempo);
            imprime ();
        }

        if(tempo%5 == 0) {
        	for(i=0; i<bitr.size(); i++){
        		if(algoritmoSubs == 4) {
        			if(bitr[i])lru[i] += 128;
        			lru[i] /= 2;
        		}
        		bitr[i] = false;
        	}
        }
        while (!q1.empty()) q1.pop ();
        
        tempo++;
        for (i = 0; i < emUso.size (); i++) emUso[i] = false;
    }
}

void inserePagina (processo proc, int tempo) { //Insere paginas na memoria fisica
    FILE *memoFisica = fopen ("/tmp/ep3.mem", "rb+");
    int pagina = ((proc.endereco + proc.p[proc.posp])/p)%(virt/p);
    int i, locbit;
    int max = 64;
    emUso[pagina] = true;

    if (localPagina[pagina] != -1 && conteudoQuadro[localPagina[pagina]] == pagina) {
      if(algoritmoSubs==4) lru[pagina] += max;
      return; //Se a pagina estiver na memoria
    }

    for (i = 0; i < total/p; i++) { //Se tiver espaco livre
        if (!bitmem[i]) {
            if (algoritmoSubs == 2) secondChance.push_back (pagina);
            else if (algoritmoSubs == 3) {
                int pg = conteudoQuadro[i];
                if (pg == -1) clk[(fim++)%(total/p)] = pagina;
                else {
                    int j;
                    for (j = 0; j < clk.size (); j++) {
                        if (pg == clk[j]) clk[j] = pagina;
                    }
                }
            }
            else if(algoritmoSubs == 4) lru[i] = max;
            
            conteudoQuadro[i] = pagina;
            localPagina[pagina] = i;
            bitr[pagina] = true;
            bitmem[i] = true;
            fseek (memoFisica, i*p*sizeof (int), SEEK_SET);
            for (i = 0; i < p; i++) fwrite (&proc.pid, sizeof (int), 1, memoFisica);
            fclose (memoFisica);
            return;   
        }
    }

    if (algoritmoSubs == 1) { //optimal
        int i, j, k, tMenor, pgsubs, maior, pg;
        vector<int> rotulos (total/p, 0);

        for (i = 0; i < total/p; i++) {
            pg = conteudoQuadro[i];
            tMenor = -1;
            for (j = 0; j < v1.size (); j++) {
                if (v1[j].t0 > tempo) {
                    for (k = v1[j].posp; k < v1[j].p.size ()-1; k++) {
                        if ((v1[j].p[k]+v1[j].endereco)/p == pg) {
                            if (tMenor == -1) tMenor = v1[j].t[k];
                            else if (tMenor > v1[j].t[k]) {
                                tMenor = v1[j].t[k];
                                break;
                            }
                        }
                    }
                }
            }
            for (j = 0; j < v1.size (); j++) {
                for (k = v1[j].posp; k < v1[j].p.size (); k++) 
                    if (v1[j].t[k] < tMenor) rotulos[pg]++;
            }
        }   
        maior = -1;
        for (i = 0; i < total/p; i++) {
            pg = conteudoQuadro[i];
            if (!emUso[pg] && rotulos[pg] > maior) {
                maior = rotulos[pg];
                pgsubs = pg;
            }
        }
        localPagina[pagina] = localPagina[pgsubs];
        conteudoQuadro[localPagina[pagina]] = pagina;
    }
    else if (algoritmoSubs == 2) { //second chance
        int pgsubs;

        while (emUso[secondChance.front()] && bitr[secondChance.front()]) {
            pgsubs = secondChance.front();
            secondChance.pop_front ();
            bitr[pgsubs] = false;
            secondChance.push_back (pgsubs);
        }
        pgsubs = secondChance.front ();
        secondChance.pop_front ();
        localPagina[pagina] = localPagina[pgsubs];
        conteudoQuadro[localPagina[pagina]] = pagina;
        bitr[pagina] = true;
        secondChance.push_back (pagina);
    } 
    else if (algoritmoSubs == 3) { //clock
        int pgsubs;
        while (emUso[clk[inicio]] && bitr[clk[inicio]]) {
            pgsubs = clk[inicio];
            bitr[pgsubs] = false;
            inicio = (inicio+1)%(total/p);
        }
        pgsubs = clk[inicio];
        localPagina[pagina] = localPagina[pgsubs];
        conteudoQuadro[localPagina[pagina]] = pagina;
        bitr[pagina] = true;
        clk[inicio] = pagina;
        inicio = (inicio+1)%(total/p);
    }
    else if (algoritmoSubs == 4) { //least recently used 4
    	int pgsubs = 0;
    	for (i=1; i<lru.size(); i++) {
    		if (lru[i] < lru[pgsubs] && localPagina[i]!=-1) pgsubs = i;
    	}
    	localPagina[pagina] = localPagina[pgsubs];
        conteudoQuadro[localPagina[pagina]] = pagina;
        bitr[pagina] = true;
    }
    bitmem[localPagina[pagina]] = true;
    fseek (memoFisica, localPagina[pagina]*p*sizeof (int), SEEK_SET);
    for (i = 0; i < p; i++) fwrite (&proc.pid, sizeof (int), 1, memoFisica);
    fclose (memoFisica);
}

int alocaMemoria (processo proc) { //insere processos na memoria virtual
    int unidades;
    bool aux = true;
    
    FILE *memoVirtual = fopen ("/tmp/ep3.vir", "rb+");
    unidades = proc.b/p;
    if (proc.b%p != 0) unidades++;
    if (algoritmoEspaco <= 2) { // first fit e next fit 
        int i, j, cont;

        for (cont = algoritmoEspaco; cont > 0; cont--) {
            for (i = next*(cont-1); i <= virt/p - unidades; i++) {
                for (j = 0; j < unidades; j++) {
                    if (bitvirt[i+j]) {aux = true; break;}
                    else aux = false;
                }
                if (!aux) break;
            }
            if (!aux) break;
        }
        proc.endereco = i*p;
        next = i+unidades;

        for (j = i; j < unidades+i; j++) bitvirt[j] = true;
        fseek (memoVirtual, i*p*sizeof (int), SEEK_SET);
        for (i = 0; i < unidades*p; i++)
            fwrite (&proc.pid, sizeof (proc.pid), 1, memoVirtual);
    }
    else if (algoritmoEspaco == 3) { // best fit
        int menor = virt, iMenor = 0, unidadesLivres, i, j;

        for (i = 0; i <= virt/p - unidades; i++) {
            unidadesLivres = 0;
            for (j = i; j < virt/p; j++) {
                aux = bitvirt[j];
                if (aux) break;
                unidadesLivres++;
            }
            if (unidadesLivres >= unidades && menor > unidadesLivres) {
            	while(!bitvirt[i] && i>0) i--;
            	if(bitvirt[i])i++;
                iMenor = i;
                menor = unidadesLivres;
            }
        }
        proc.endereco = iMenor*p;
        for (j = iMenor; j < unidades+iMenor; j++) bitvirt[j] = true;
        fseek (memoVirtual, iMenor*p*sizeof (int), SEEK_SET);
        for (i = 0; i < unidades*p; i++)
            fwrite (&proc.pid, sizeof (proc.pid), 1, memoVirtual);
    }
    else { // worst fit
        int maior = -1, iMaior = -1, unidadesLivres, i, j;

        for (i = 0; i <= virt/p - unidades; i++) {
            unidadesLivres = 0;
            for (j = i; j < virt/p; j++) {
                aux = bitvirt[j];
                if (aux) break;
                unidadesLivres++;
            }
            if (unidadesLivres >= unidades && maior < unidadesLivres) {
                iMaior = i;
                maior = unidadesLivres;
            }
        }
        proc.endereco = iMaior*p;
        for (j = iMaior; j < unidades+iMaior; j++) bitvirt[j] = true;
        fseek (memoVirtual, iMaior*p*sizeof (int), SEEK_SET);
        for (i = 0; i < unidades*p; i++)
            fwrite (&proc.pid, sizeof (proc.pid), 1, memoVirtual);
    }
    fclose (memoVirtual);

    return proc.endereco;
}

void retira (processo proc) {
	int unidades, locB, i;
	FILE *memoVirtual = fopen ("/tmp/ep3.vir", "rb+");
	int menos1 = -1;
    
	unidades = proc.b/p;
    if (proc.b%p != 0) unidades++;
    locB = proc.endereco/p;
    for(i = 0; i<unidades; i++) { bitvirt[locB+i] = false; if(localPagina[locB+i]!= -1) bitmem[localPagina[locB+i]] = false; }
    fseek (memoVirtual, proc.endereco*sizeof (int), SEEK_SET);
    for (i = 0; i < unidades*p; i++) fwrite (&menos1, sizeof (int), 1, memoVirtual);
    fclose (memoVirtual);
}

void tokens (char *line, char *parameters[]) {
    char *token;
    int i;
    
    token = strtok (line, " ");
    for (i = 0; token != NULL; i++) {
        parameters[i] = token;
        token = strtok (NULL, " ");
    }
    parameters[i] = NULL;
}

void imprime () {
    int aux;
    FILE* memoVirtual = fopen ("/tmp/ep3.vir", "rb");
    FILE* memoFisica = fopen ("/tmp/ep3.mem", "rb");
    printf ("MEMÓRIA VIRTUAL\n");
    while (fread (&aux, sizeof (aux), 1, memoVirtual) == 1) 
        printf ("%d ", aux);
    printf("\nBitmap: ");
    for (aux = 0; aux<bitvirt.size(); aux++) if(bitvirt[aux]) printf("1 "); else printf("0 ");
    printf("\nMEMÓRIA FÍSICA\n");
    while (fread (&aux, sizeof (aux), 1, memoFisica) == 1) 
        printf ("%d ", aux);
    printf("\nBitmap: ");
    for (aux = 0; aux<bitmem.size(); aux++) if(bitmem[aux]) printf("1 "); else printf("0 ");
    printf ("\n\n");
    fclose (memoVirtual);
    fclose (memoFisica);        
}
