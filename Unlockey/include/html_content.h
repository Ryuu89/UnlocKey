#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>UnlocKey - Sistema de Mensagens</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background-color: #f0f0f0; font-family: Arial, Sans-Serif; margin: 0; padding: 20px; }
    h1 { color: #333; text-align: center; }
    .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    label { display: block; margin-bottom: 5px; font-weight: bold; }
    select, input, textarea { width: 100%; padding: 8px; margin-bottom: 15px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
    button { background-color: #4CAF50; color: white; padding: 10px; border: none; border-radius: 4px; cursor: pointer; width: 100%; }
    .message { margin-top: 15px; padding: 10px; border-radius: 4px; }
    .success { background-color: #d4edda; color: #155724; }
  </style>
</head>
<body>
  <div class="container">
    <h1>UnlocKey</h1>
    <form action="/enviar" method="post">
      <label for="remetente">Seu nome:</label>
      <input type="text" id="remetente" name="remetente" required>
      
      <label for="destinatario">Destinatário:</label>
      <select id="destinatario" name="destinatario" required>
        <option value="">Selecione o destinatário</option>
        <!-- Lista de destinatários será preenchida pelo servidor -->
      </select>
      
      <label for="mensagem">Mensagem:</label>
      <textarea id="mensagem" name="mensagem" rows="4" maxlength="50" required></textarea>
      <small>Máximo 50 caracteres</small>
      
      <button type="submit">Enviar Mensagem</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

const char SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>UnlocKey - Mensagem Enviada</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="3;url=/">
  <style>
    body { background-color: #f0f0f0; font-family: Arial, Sans-Serif; margin: 0; padding: 20px; }
    h1 { color: #333; text-align: center; }
    .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
    .message { margin-top: 15px; padding: 10px; border-radius: 4px; text-align: center; }
    .success { background-color: #d4edda; color: #155724; }
  </style>
</head>
<body>
  <div class="container">
    <h1>UnlocKey</h1>
    <div class="message success">
      <p>Mensagem enviada com sucesso!</p>
      <p>Redirecionando em 3 segundos...</p>
    </div>
  </div>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H