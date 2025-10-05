using System;
using System.IO;
using System.Net.WebSockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

class WebSocketClient
{
    static async Task Main(string[] args)
    {
        Console.WriteLine("[CLIENTE] Iniciando conexão...");

        using var client = new ClientWebSocket();
        var uri = new Uri("ws://localhost:8080/ws/");

        try
        {
            await client.ConnectAsync(uri, CancellationToken.None);
            Console.WriteLine("[CLIENTE] Conectado ao servidor!");

            string imagePath = "imagem.jpg"; // Caminho da imagem local

            while (client.State == WebSocketState.Open)
            {
                // Envia texto
                await SendTextAsync(client, "dotnet client");

                // Envia imagem
                if (File.Exists(imagePath))
                {
                    await SendImageAsync(client, imagePath);
                }
                else
                {
                    Console.WriteLine("[CLIENTE] Imagem não encontrada: " + imagePath);
                }

                await Task.Delay(2000); // Aguarda 2 segundos
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine("[CLIENTE] Erro: " + ex.Message);
        }
    }

    static async Task SendTextAsync(ClientWebSocket client, string message)
    {
        var buffer = Encoding.UTF8.GetBytes(message);
        await client.SendAsync(new ArraySegment<byte>(buffer), WebSocketMessageType.Text, true, CancellationToken.None);
        Console.WriteLine("[CLIENTE] Texto enviado: " + message);
    }

    static async Task SendImageAsync(ClientWebSocket client, string imagePath)
    {
        byte[] imageBytes = await File.ReadAllBytesAsync(imagePath);
        await client.SendAsync(new ArraySegment<byte>(imageBytes), WebSocketMessageType.Binary, true, CancellationToken.None);
        Console.WriteLine("[CLIENTE] Imagem enviada: " + imagePath);
    }
}
