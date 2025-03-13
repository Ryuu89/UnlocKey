#include "WebInterface.h"
#include "rsa.h"
#include "DataTypes.h"

// Variáveis globais 
WebServer server(80);
bool novaMensagemWeb = false;
String webRemetente = "";
String webDestinatario = "";
String webMensagem = "";

// Variáveis externas
extern Usuario usuarios[MAX_USERS];
extern Mensagem mensagens[MAX_MESSAGES];
extern int numUsuarios;
extern int numMensagens;

// Declaração de funções auxiliares
extern int EncontraUsuario(Usuario *usuarios, int numUsuarios, const char *username);

void inicializarSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println(F("Erro ao montar SPIFFS"));
        return;
    }
    Serial.println(F("SPIFFS montado com sucesso"));
    
    // Listar arquivos
    Serial.println(F("Arquivos no SPIFFS:"));
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) {
        Serial.print(file.name());
        Serial.print(" (");
        Serial.print(file.size());
        Serial.println(F(" bytes)"));
        file = root.openNextFile();
    }
}

bool verificarArquivosNecessarios() {
    bool status = true;
    
    if (!SPIFFS.exists("/index.html")) {
        Serial.println(F("Erro: arquivo index.html não encontrado!"));
        status = false;
    }
    
    if (!SPIFFS.exists("/success.html")) {
        Serial.println(F("Erro: arquivo success.html não encontrado!"));
        status = false;
    }
    
    return status;
}

void inicializarServidor() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/enviar", HTTP_POST, handleEnviarMensagem);
    server.begin();
    Serial.println(F("Servidor HTTP iniciado"));
}

void handleRoot() {
    extern Usuario usuarios[];
    extern int numUsuarios;

    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(500, "text/plain", "Erro ao abrir arquivo");
        return;
    }
    
    String html = file.readString();
    file.close();
    
    // Encontrar o ponto de inserção no HTML
    int pos = html.indexOf("<option value=\"\">Selecione o destinatário</option>");
    if (pos > 0) {
        pos = html.indexOf("</select>", pos);
    
        if (pos > 0) {
            // Adicionar opções de usuários
            String userOptions = "";
            for (int i = 0; i < numUsuarios; i++) {
                userOptions += "<option value=\"" + String(usuarios[i].username) + "\">" + 
                                String(usuarios[i].username) + "</option>\n";
            }
            
            html = html.substring(0, pos) + userOptions + html.substring(pos);
        }
    }
    
    server.send(200, "text/html", html);
}

void handleEnviarMensagem() {
    if (server.method() != HTTP_POST) {
        server.sendHeader("Location", "/");
        server.send(303);
        return;
    }

    webRemetente = server.arg("remetente");
    webDestinatario = server.arg("destinatario");
    webMensagem = server.arg("mensagem");

    if (webRemetente.length() == 0 || webDestinatario.length() == 0 || webMensagem.length() == 0) {
        server.send(400, "text/html", "<html><body><h1>Erro!</h1><p>Todos os campos são obrigatórios</p><p><a href='/'>Voltar</a></p></body></html>");
        return;
    }

    novaMensagemWeb = true;
    
    File file = SPIFFS.open("/success.html", "r");
    if (!file) {
        server.send(200, "text/html", "<html><body><h1>Mensagem enviada!</h1><p>Redirecionando...</p><script>setTimeout(function(){window.location='/';},3000);</script></body></html>");
        return;
    }

    String html = file.readString();
    file.close();
    server.send(200, "text/html", html);
}

void verificarWiFi() {
    if (!wifiEnabled) {
        return;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("WiFi desconectado. Tentando reconectar..."));
        WiFi.reconnect();
        int tentativas = 0;
        
        while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
            delay(500);
            Serial.print(".");
            tentativas++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(F("\nWiFi reconectado!"));
            Serial.print("IP: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println(F("\nFalha na reconexão WiFi!"));
        }
    }
}

void ativarWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("WiFi já está ativo!"));
        return;
    }
    
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println(F("Ativando WiFi..."));
    
    // Configurar com potência reduzida
    WiFi.disconnect(true);
    delay(500);
    WiFi.mode(WIFI_OFF);
    delay(1000);
    WiFi.setTxPower(WIFI_POWER_MINUS_1dBm);
    delay(1000);
    Serial.println(F("Definindo modo WiFi..."));
    WiFi.mode(WIFI_STA);
    delay(1000);
    yield();

    Serial.println("Conectando ao WiFi: ");
    Serial.print(ssid);
    Serial.print(" ");
    WiFi.begin(ssid, password);
    delay(1000);
    yield();

    Serial.println(F("Ajustando potência..."));
    WiFi.setTxPower(WIFI_POWER_17dBm);  // Potência média
    delay(500);
    yield();
    Serial.println(F("Aguardando conexão WiFi..."));

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 10) {
        Serial.print(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        delay(1000);
        yield();
        tentativas++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("\nConectado ao WiFi!"));
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        inicializarServidor();
        
        // Inicializa mDNS
        delay(500);
        if (MDNS.begin("unlockey")) {
            Serial.println(F("Serviço mDNS iniciado em: http://unlockey.local"));
        } else {
            Serial.println(F("Falha ao iniciar mDNS, mas o WiFi ainda está funcionando."));
        }
        
        // Configura horário via NTP com timeout
        Serial.println(F("Configurando horário via NTP..."));
        configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
        
        wifiEnabled = true;
        Serial.println(F("WiFi ativado com sucesso!"));
    }
    else {
        Serial.println(F("\nFalha ao conectar ao WiFi!"));
        wifiEnabled = false;
    }
    digitalWrite(LED_BUILTIN, LOW);
}

void processarMensagemWeb() {
    extern Usuario usuarios[];
    extern int numUsuarios;
    extern Mensagem mensagens[];
    extern int numMensagens;

    if (!novaMensagemWeb) {
        return;
    }
    
    Serial.println(F("\nNova mensagem recebida via web:"));
    Serial.print("De: ");
    Serial.println(webRemetente);
    Serial.print("Para: ");
    Serial.println(webDestinatario);
    
    int idDestinatario = EncontraUsuario(usuarios, numUsuarios, webDestinatario.c_str());
    if (idDestinatario == -1) {
        Serial.println(F("Destinatário não encontrado!"));
        novaMensagemWeb = false;
        return;
    }

    if (numMensagens >= MAX_MESSAGES) {
        Serial.println(F("Sistema de mensagens cheio!"));
        novaMensagemWeb = false;
        return;
    }
    
    // Copia dados para a mensagem
    strncpy(mensagens[numMensagens].remetente, webRemetente.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[numMensagens].remetente[MAX_USERNAME_LENGTH - 1] = '\0';
    
    strncpy(mensagens[numMensagens].destinatario, webDestinatario.c_str(), MAX_USERNAME_LENGTH - 1);
    mensagens[numMensagens].destinatario[MAX_USERNAME_LENGTH - 1] = '\0';

    // Limita tamanho da mensagem
    if (webMensagem.length() > MAX_MESSAGE_LENGTH) {
        webMensagem = webMensagem.substring(0, MAX_MESSAGE_LENGTH);
    }

    mensagens[numMensagens].tamanhoMsg = webMensagem.length();
    SetDataAtual(mensagens[numMensagens].data);
    
    // Criptografa a mensagem
    EncriptaMensagem((unsigned char*)webMensagem.c_str(), 
                mensagens[numMensagens].mensagemCriptografada, 
                usuarios[idDestinatario].chavePublica);
    
    numMensagens++;
    Serial.println(F("Mensagem processada com sucesso!"));
    
    novaMensagemWeb = false;
}