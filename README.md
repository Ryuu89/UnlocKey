# UnlocKey

Repositório para o trabalho desenvolvido na disciplina de PIC1

## Conteúdo
- [Conteúdo](#conteúdo)
- [Resumo](#resumo)
- [Descrição](#descrição)
- [Componentes/Tecnologias](#componentes)
- [Resultado](#resultado)
- [Autores](#autores)

## Resumo
O projeto integra um site para envio de mensagens criptografadas e uma caixa
física que usa biometria e NFC para autenticação e descriptografia. As chaves
públicas são geradas na caixa, enquanto as privadas ficam apenas no NFC,
garantindo segurança e privacidade.

## Descrição
O projeto consiste no desenvolvimento de um sistema integrado composto por
duas principais partes:

● Um site que funciona como uma plataforma de envio e gerenciamento de
mensagens criptografadas, semelhante a uma rede social básica.

● Uma caixa física que realiza autenticação biométrica (sensor de impressão
digital), leitura de tags NFC para decriptografia, e envio de dados para o site.
O objetivo principal é garantir a segurança e privacidade no envio e recebimento de
mensagens por meio de criptografia baseada em chaves públicas e privadas. O
sistema utiliza tecnologia NFC para armazenar as chaves privadas de maneira segura,
sem nunca salvá-las em qualquer banco de dados ou dispositivo.

### ESTRUTURA DO SISTEMA
O site é responsável por:

● Cadastro e exibição de usuários: Exibe uma lista de usuários cadastrados, com
base nos dados enviados pela caixa.

● Envio de mensagens criptografadas: Permite que os usuários enviem
mensagens que serão criptografadas usando as chaves públicas dos
destinatários.

● Armazenamento de dados: Mantém informações como:

- Nome de usuário.
- Chaves públicas associadas aos usuários.
- Mensagens criptografadas.
  
Funcionamento do Site:

1. O site recebe as chaves públicas geradas pela caixa, associando-as aos usuários
cadastrados.

2. Ao enviar uma mensagem, o site verifica o destinatário e utiliza a chave pública
associada a ele para criptografar a mensagem.

3. As mensagens criptografadas são armazenadas no banco de dados e acessíveis
apenas pelo destinatário, via caixa.

A caixa desempenha funções de autenticação, leitura de NFC e decriptografia. Suas
funcionalidades incluem:

### CADASTRO DE USUÁRIO

● Etapas do Cadastro:

1. O usuário insere sua impressão digital no sensor biométrico.
2. A caixa lê o tag NFC do usuário para obter a chave privada.
3. Com base na chave privada, a caixa gera a chave pública
correspondente.
4. O nome do usuário, junto com a chave pública e a impressão digital
cadastrada, é enviado para o banco de dados do site.

### LOGIN
● Etapas do Login:
1. O usuário autentica-se utilizando sua impressão digital previamente
cadastrada.
2. Após a autenticação, a caixa verifica a identidade do usuário e acessa as
mensagens criptografadas associadas a ele no site.

### ACESSO ÀS MENSAGENS 
● Etapas de Leitura de Mensagens:
1. Após o login, o usuário pode visualizar suas mensagens criptografadas.
2. Para decriptar uma mensagem, o usuário deve aproximar seu tag NFC,
que contém sua chave privada.
3. A caixa utiliza a chave privada para decriptografar a mensagem
localmente e exibi-la ao usuário.
4. A chave privada nunca é armazenada na caixa ou em qualquer outro
dispositivo.

## Componentes/Tecnologias

### Site

● Linguagem de Programação: C , JS 

● Banco de Dados: SQLite3 

● Frontend: HTML, CSS, 

### Caixa Física 

● Hardware: 

○ Sensor de impressão digital. 

○ Leitor NFC (compatível com tags ISO 14443).

○ Microprocessador ESP32.

● Linguagem de Programação: C++.

● Comunicação com o site: protocolo MQTT para envio e recebimento de dados.
Criptografia

● Algoritmo: RSA (Rivest–Shamir–Adleman) para criptografia assimétrica.

○ As chaves privadas (armazenadas em tags NFC) são geradas utilizando
bibliotecas como:

■ Arduino Cryptography Library

○ As chaves públicas (enviadas ao site) e a descriptografia serão
calculadas e realizadas durante o processamento feito no próprio código
do Arduino, usando algoritmos de MDC, função totiente de Euler, inverso
multiplicativo e potenciação modular.


## Resultado

Vídeo com o resultado -> [Link]

## Autores

- Alexandre -> [@AlexandreFardin](https://github.com/AlexandreFardin)
- Gabriel -> [@gabb-ggs](https://github.com/gabb-ggs)
- Lilânio -> [@Ryuu89](https://github.com/Ryuu89)
