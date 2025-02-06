# UnlocKey

## UnlockR v1 Prototype

O `UnlockR v1` é um protótipo em C usado no próprio terminal para testar a biblioteca `rsa.h` que será usada eventualmente no projeto de Arduino na branch main. Ele permite cadastrar usuários, enviar mensagens criptografadas e ler mensagens criptografadas.

### Biblioteca `rsa.h`

A biblioteca `rsa.h` implementa as funções necessárias para gerar chaves RSA, criptografar e descriptografar mensagens. Ela inclui funções para verificar se um número é primo, gerar números primos, calcular o Máximo Divisor Comum (MDC), calcular o inverso modular, calcular a exponenciação modular, gerenciar memória para as chaves, gerar chaves RSA, mostrar chaves, salvar chaves em um arquivo, ler chaves privadas de um arquivo e criptografar e descriptografar mensagens.

### Instruções para rodar o protótipo `UnlockR v1`

1. Clone o repositório:
   ```sh
   git clone https://github.com/Ryuu89/UnlocKey.git
   cd UnlocKey
   ```

2. Compile o código:
   ```sh
   cd 'UnlockR v1'
   gcc -g main.c rsa.c -o main -lm
   ```

3. Execute o protótipo:
   ```sh
   ./main
   ```

### Funcionamento do arquivo `rsa.c`

O arquivo `rsa.c` contém a implementação das funções necessárias para o funcionamento do algoritmo RSA. Ele inclui funções para verificar se um número é primo, gerar números primos, calcular o Máximo Divisor Comum (MDC), calcular o inverso modular, calcular a exponenciação modular, gerenciar memória para as chaves, gerar chaves RSA, mostrar chaves, salvar chaves em um arquivo, ler chaves privadas de um arquivo e criptografar e descriptografar mensagens.

#### Funções principais:

- `EhPrimo`: Verifica se um número é primo.
  - Entrada: `unsigned long num` - número a ser verificado.
  - Saída: `int` - retorna 1 se o número for primo, caso contrário, retorna 0.
- `GeraPrimos`: Gera dois números primos aleatórios.
  - Entrada: `unsigned long *p, unsigned long *q` - ponteiros para armazenar os números primos gerados.
  - Saída: Nenhuma.
- `MDC`: Calcula o Máximo Divisor Comum (MDC).
  - Entrada: `unsigned long a, unsigned long b` - números para calcular o MDC.
  - Saída: `unsigned long` - retorna o MDC de `a` e `b`.
- `InversoModular`: Calcula o inverso modular.
  - Entrada: `unsigned long e, unsigned long totient` - valores para calcular o inverso modular.
  - Saída: `unsigned long` - retorna o inverso modular de `e` em relação a `totient`.
- `ExpModular`: Calcula a exponenciação modular.
  - Entrada: `unsigned long base, unsigned long exp, unsigned long mod` - valores para calcular a exponenciação modular.
  - Saída: `unsigned long` - retorna o resultado de `(base^exp) % mod`.
- `InitKeys`: Inicializa a memória para as chaves.
  - Entrada: `PrivateKeys** priv, PublicKeys** pub` - ponteiros para as chaves privadas e públicas.
  - Saída: Nenhuma.
- `DeleteKeys`: Libera a memória das chaves.
  - Entrada: `PrivateKeys* priv, PublicKeys* pub` - ponteiros para as chaves privadas e públicas.
  - Saída: Nenhuma.
- `GeraChaves`: Gera as chaves RSA.
  - Entrada: `PrivateKeys* priv, PublicKeys* pub` - ponteiros para as chaves privadas e públicas.
  - Saída: Nenhuma.
- `MostraChaves`: Mostra as chaves geradas.
  - Entrada: `const PrivateKeys* priv, const PublicKeys* pub` - ponteiros para as chaves privadas e públicas.
  - Saída: Nenhuma.
- `SalvarChaves`: Salva as chaves em um arquivo.
  - Entrada: `PrivateKeys *priv, PublicKeys *pub, FILE *fp` - ponteiros para as chaves privadas e públicas e o arquivo.
  - Saída: Nenhuma.
- `LerChavesPrivadas`: Lê as chaves privadas de um arquivo.
  - Entrada: `PrivateKeys* priv, FILE* fp` - ponteiros para as chaves privadas e o arquivo.
  - Saída: Nenhuma.
- `EncriptaMensagem`: Criptografa uma mensagem.
  - Entrada: `const unsigned char* message, unsigned long* encrypted, const PublicKeys* pub` - ponteiros para a mensagem, a mensagem criptografada e as chaves públicas.
  - Saída: `int` - retorna o tamanho da mensagem criptografada.
- `DecriptaMensagem`: Descriptografa uma mensagem criptografada.
  - Entrada: `const unsigned long* encrypted, unsigned int length, unsigned char* message, const PrivateKeys* priv` - ponteiros para a mensagem criptografada, o tamanho da mensagem, a mensagem decriptada e as chaves privadas.
  - Saída: `int` - retorna 0 se a descriptografia for bem-sucedida.
