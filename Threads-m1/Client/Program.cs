using System;
using System.IO.Pipes;
using System.IO;
using System.Threading;

class Client
{
    static void Main(string[] args)
    {
        Console.WriteLine("Digite 'S' para string ou 'N' para número:");
        string tipo = Console.ReadLine().ToUpper(); // Lê a entrada do usuário e converte para maiúsculas
        string pipeName = tipo == "S" ? "stringPipe" : "numberPipe"; // Define o nome do pipe com base na escolha do usuário

        Console.WriteLine("Deseja enviar múltiplas requisições simultâneas? (S/N)");
        string multiRequest = Console.ReadLine().ToUpper();

        if (multiRequest == "S")
        {
            Console.WriteLine("Digite o número de threads para enviar:");
            if (int.TryParse(Console.ReadLine(), out int threadCount) && threadCount > 0)
            {
                for (int i = 0; i < threadCount; i++)
                {
                    int threadNumber = i + 1;
                    Thread clientThread = new Thread(() => SendRequest(pipeName, threadNumber));
                    clientThread.Start();
                }
            }
            else
            {
                Console.WriteLine("Número inválido de threads.");
            }
        }
        else
        {
            // Executa a requisição única normalmente
            SendRequest(pipeName);
        }
    }

    static void SendRequest(string pipeName, int threadNumber = 0)
    {
        try
        {
            using (NamedPipeClientStream pipeClient = new NamedPipeClientStream(".", pipeName, PipeDirection.InOut))
            {
                pipeClient.Connect(); // Conecta ao servidor de pipe
                Console.WriteLine($"Thread {threadNumber}: Conectado ao servidor no pipe {pipeName}");

                using (StreamReader reader = new StreamReader(pipeClient))
                using (StreamWriter writer = new StreamWriter(pipeClient))
                {
                    // Envia a requisição personalizada com o identificador da thread (para múltiplas threads)
                    string request = threadNumber > 0 ? $"Thread-{threadNumber} request" : "Requisição única";
                    Console.WriteLine($"Thread {threadNumber}: Enviando requisição: {request}");

                    writer.WriteLine(request); // Envia a requisição para o servidor
                    writer.Flush(); // Garante que os dados sejam enviados imediatamente

                    string response = reader.ReadLine(); // Lê a resposta do servidor
                    Console.WriteLine($"Thread {threadNumber}: Resposta do servidor: {response}");
                }
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Thread {threadNumber}: Erro ao conectar ao servidor: {ex.Message}");
        }
    }
}