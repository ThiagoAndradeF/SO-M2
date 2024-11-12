using System;
using System.IO.Pipes;
using System.IO;
using System.Threading;

class Server
{
    static void Main(string[] args)
    {
        Console.WriteLine("Servidor iniciado...");

        // Configura a quantidade mínima e máxima de threads no pool de threads
        ThreadPool.SetMinThreads(20, 20);
        ThreadPool.SetMaxThreads(100, 100);

        // Adiciona as threads de tratamento para os pipes de string e número
        ThreadPool.QueueUserWorkItem(PipeHandler, "stringPipe");
        ThreadPool.QueueUserWorkItem(PipeHandler, "numberPipe");

        Console.ReadLine(); // Aguarda a entrada do usuário para finalizar o servidor
    }

    // Método para lidar com conexões de clientes em um loop contínuo
    private static void PipeHandler(object? pipeName)
    {
        string pipeType = (string)pipeName!; // Usa o operador '!' para garantir que não seja nulo

        while (true)
        {
            // Cria um NamedPipeServerStream que aceita conexões de clientes
            NamedPipeServerStream pipeServer = new NamedPipeServerStream(
                pipeType, 
                PipeDirection.InOut, 
                NamedPipeServerStream.MaxAllowedServerInstances, 
                PipeTransmissionMode.Byte, 
                PipeOptions.Asynchronous);

            Console.WriteLine($"Aguardando conexão no pipe: {pipeType}...");
            pipeServer.WaitForConnection(); // Espera uma conexão do cliente

            // Adiciona cada conexão de cliente em uma thread no pool de threads
            ThreadPool.QueueUserWorkItem(HandleClientConnection, (pipeServer, pipeType));
        }
    }

    // Método para tratar a conexão com o cliente
    private static void HandleClientConnection(object? state)
    {
        // Desestrutura o estado em suas partes (pipeServer e pipeType)
        var (pipeServer, pipeType) = ((NamedPipeServerStream, string))state!;

        try
        {
            // Cria leitores e escritores para a comunicação com o cliente
            using (StreamReader reader = new StreamReader(pipeServer))
            using (StreamWriter writer = new StreamWriter(pipeServer))
            {
                string request = reader.ReadLine() ?? string.Empty; // Garante que request não seja nulo
                Console.WriteLine($"Requisição recebida: {request}");

                // Verifica o tipo de pipe e processa a requisição
                if (pipeType == "stringPipe")
                {
                    writer.WriteLine($"Resposta de string: {request.ToUpper()}"); // Responde com a string em maiúsculas
                }
                else if (pipeType == "numberPipe" && int.TryParse(request, out int number))
                {
                    writer.WriteLine($"Número ao quadrado: {number * number}"); // Responde com o quadrado do número
                }
                else
                {
                    writer.WriteLine("Erro: entrada inválida"); // Responde com uma mensagem de erro se a entrada for inválida
                }

                writer.Flush(); // Garante que a resposta seja enviada imediatamente
            }
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Erro no pipe: {ex.Message}"); // Exibe mensagens de erro em caso de falha
        }
        finally
        {
            pipeServer.Close(); // Fecha o pipe
            pipeServer.Dispose(); // Libera os recursos associados ao pipe
        }
    }
}