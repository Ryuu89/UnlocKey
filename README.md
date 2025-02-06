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

### Propósito Acadêmico da Biblioteca `rsa.h`

A biblioteca `rsa.h` foi criada com o objetivo acadêmico de fornecer uma implementação prática do algoritmo RSA, que é amplamente utilizado em criptografia. Ela serve como uma ferramenta de aprendizado para entender os conceitos fundamentais da criptografia de chave pública, incluindo a geração de chaves, criptografia e descriptografia de mensagens. Além disso, a biblioteca pode ser utilizada em projetos acadêmicos e experimentos para explorar a segurança e a eficiência do algoritmo RSA.
