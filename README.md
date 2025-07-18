# Aplicação P2P baseada no sistema de reputação do Freechains

Uma aplicação de criar e votar regras de forma totalmente descentralizada.

---

## Manual de Utilização da Aplicação

1. É necessário ter previamente instalado o [Freechains](https://github.com/Freechains/README).
2. Para entender o sistema de reputação, avaliação e publicação, acesse: [Freechains](https://github.com/Freechains/README).
3. Este protótipo deve ser executado no mesmo diretório do freechains.
4. Ao iniciar a aplicação, informe uma chave única, que será utilizada para assinar suas publicações e computar sua reputação na rede.
5. Na interface principal, as seguintes opções estarão disponíveis:


#### [1] Acessar Quadro de Regras
- Ver todas as regras em vigor, ou seja, propostas que foram aprovadas pelos usuários.
- As propostas são aprovadas caso estejam avaliadas positivamente e tenham sido postadas a mais de 24 horas atrás.


#### [2] Propor Nova Regra
- Publicar uma nova proposta de regra.
- Toda proposta deve conter uma TAG e uma SUBTAG.
- Existem TAGS pré-definidas, mas também é possível inserir novas por votação.
- Se já existir uma regra em vigor com as mesmas TAG e SUBTAG, a proposta será considerada uma proposta de alteração da regra em vigor.


#### [3] Propor Alteração de Regra
- Publicar uma proposta de alteração de regra.
- Seleciona a regra referente e indica as novas especificações. 
- A proposta só será aprovada se obter um número maior de likes do que a regra em vigor, após 24 horas desde sua publicação.


#### [4] Propor Nova TAG
- Publicar uma proposta de nova TAG.
- Só é aceita na lista de tags caso sua avaliação for positiva após 24 horas.


### [5] Propor Nova SUBTAG
- Publicar uma proposta de Subtag.
- Uma Subtag só é aceita na lista de subtags caso sua avaliação for positiva após 24 horas.


### [6] Visualizar todas as propostas (consensus e bloqueadas)
- Exibe as propostas do consensus e as bloqueadas.
- Propostas do consensus sao propostas de usuários com reputação, essas são automaticamente aceitas na cadeia do freechains.
- Propostas bloqueadas são de usuários que não possuem reputação, essas precisam de um like de outro usuário para serem aceitas na cadeia.
- Para melhor compreensão, acesse: [Freechains/blocks](https://github.com/Freechains/README/blob/master/docs/blocks.md)


### [7] Visualizar as propostas do consensus
- Exibe somente as propostas já aceitas na cadeia do freechains.


### [8] Visualizar as propostas bloqueadas
- Exibe somente as propostas bloqueadas, para que outro usuário de um like (ou não).

---

### [9] Ver sua reputação
- Verifica a reputação do usuário na rede.

---

## Funcionalidades Implementadas no Protótipo

- Mecanismo de publicação de propostas na rede.
- Visualização das propostas de usuários.
- Visualização das regras em vigor.
- Publicação de propostas de alteração de regras.
- Avaliação de propostas e regras em vigor (likes/dislikes).
- Substituição de regras por propostas de alteração aprovadas.
- Mecanismo de setar TAGS e SUBTAGS para todas as propostas.
- Publicar propostas de novas TAGS e SUBTAGS.
- Inserção de TAGS e SUBTAGS aprovadas na rede.

---

## Funcionalidades Não Implementadas

- Uso de .json para melhorar a eficiência da execução.
- Correção de alguns bugs.
- Interface gráfica.

---

## Ferramentas utilizadas para a implementação.

1 - Tempo baseado no tempo do Freechains

2 - Armazenamento das publicações na cadeia do Freechains

3 - Geração de chaves dos usuários pelo Freechains  

4 - Sistema de likes/dislikes do freechains

5 - Consensus da cadeia do freechains, para obter as propostas publicadas.

6 - Heads blocked do Freechains, para obter as propostas bloqueadas 

7 - Sistema de reputação do Freechains.

8 - Funcionalidade de payload do Freechains, para pegar o conteúdo das publicações da cadeia

9 - Funcionalidade de get block para pegar o tempo de publicação de cada proposta 

---
