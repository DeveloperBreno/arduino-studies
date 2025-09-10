using System.Net;
using System.Net.WebSockets;
using System.Text;

Console.WriteLine("Servidor WebSocket .NET 8 iniciado...");

HttpListener listener = new HttpListener();
listener.Prefixes.Add("http://+:8080/ws/"); // Porta 8080
listener.Start();
Console.WriteLine("Aguardando conexões...");

int imageCounter = 0;

while (true)
{
    var context = await listener.GetContextAsync();

    if (context.Request.IsWebSocketRequest)
    {
        var wsContext = await context.AcceptWebSocketAsync(null);
        var webSocket = wsContext.WebSocket;
        Console.WriteLine("ESP32 conectado!");

        var buffer = new byte[1024 * 32]; // 32 KB por pacote
        while (webSocket.State == WebSocketState.Open)
        {
            try
            {
                var ms = new MemoryStream();
                WebSocketReceiveResult result;
                do
                {
                    result = await webSocket.ReceiveAsync(new ArraySegment<byte>(buffer), CancellationToken.None);
                    ms.Write(buffer, 0, result.Count);
                } while (!result.EndOfMessage);

                if (result.MessageType == WebSocketMessageType.Close)
                {
                    await webSocket.CloseAsync(WebSocketCloseStatus.NormalClosure, "", CancellationToken.None);
                    Console.WriteLine("Conexão fechada");
                }
                else if (result.MessageType == WebSocketMessageType.Binary)
                {
                    // Salvar imagem
                    string fileName = $"image_{imageCounter++}.jpg";
                    string path = Path.Combine(AppContext.BaseDirectory, fileName);
                    await File.WriteAllBytesAsync(path, ms.ToArray());
                    Console.WriteLine($"Imagem salva: {fileName}");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("Erro: " + ex.Message);
            }
        }
    }
}
