# BIBLIOTECA DE THREADS - RELATÓRIO

1. Termo de compromisso

    Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item
3 deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

   * Yuri Niitsuma <yuriniitsuma@dcc.ufmg.br> XX%
   * Cássios Kley Martins Marques <email@domain> XX%

3. Referências bibliográficas

   - [Blog][Blog]
   - [man page][Timer]

[Blog]: http://nitish712.blogspot.com.br/2012/10/thread-library-using-context-switching.html
[Timer]: http://man7.org/linux/man-pages/man2/timer_create.2.html

4. Estruturas de dados

  1. **Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).**

  - A estrutura contendo o contexto foi adicionado alguns metadados pra indicar se alguma thread a está esperando que é setado pra NULL durante o exit.


  2. **Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).**

  - A construção do temporizador utilizei baseado no exemplo dado na doc [timer_create][Timer].

  - O Signal usei a ideia semelhante ao [Blog][Blog] que utilizou "sys/time.h".
