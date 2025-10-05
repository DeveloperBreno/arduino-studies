using System.Net;
using System.Net.WebSockets;
using System.Text;

Console.WriteLine("Servidor WebSocket .NET 8 iniciado...");

HttpListener listener = new HttpListener();
listener.Prefixes.Add("http://+:8080/ws/");
listener.Start();
Console.WriteLine("Aguardando conexões...");

int imageCounter = 0;

while (true)
{
    var context = await listener.GetContextAsync();

    if (context.Request.IsWebSocketRequest)
    {
        _ = Task.Run(async () =>
        {
            var wsContext = await context.AcceptWebSocketAsync(null);
            var webSocket = wsContext.WebSocket;
            Console.WriteLine("Novo cliente conectado!");

            string clientName = "desconhecido";
            var buffer = new byte[1024 * 32];

            try
            {
                while (webSocket.State == WebSocketState.Open)
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
                        Console.WriteLine($"Conexão fechada ({clientName})");
                    }
                    else if (result.MessageType == WebSocketMessageType.Binary)
                    {
                        string clientDir = Path.Combine(AppContext.BaseDirectory, clientName);
                        Directory.CreateDirectory(clientDir);

                        string fileName = $"image_{Interlocked.Increment(ref imageCounter)}.jpg";
                        string path = Path.Combine(clientDir, fileName);
                        await File.WriteAllBytesAsync(path, ms.ToArray());

                        Console.WriteLine($"Imagem salva: {clientName}/{fileName}");
                    }
                    else if (result.MessageType == WebSocketMessageType.Text)
                    {
                        string text = Encoding.UTF8.GetString(ms.ToArray()).Trim();
                        Console.WriteLine($"Texto recebido: {text}");

                        clientName = text;
                        string clientDir = Path.Combine(AppContext.BaseDirectory, clientName);
                        Directory.CreateDirectory(clientDir);
                        Console.WriteLine($"Cliente identificado como: {clientName}");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Erro ({clientName}): {ex.Message}");
            }
        });
    }
}
