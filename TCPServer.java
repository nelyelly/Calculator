import java.io.*;
import java.net.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TCPServer {
    private static final int PORT = 12345;
    private static final String LOG_FILE = "log_java.txt";
    private static final SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss");
    private static ExecutorService threadPool = Executors.newCachedThreadPool();

    public static void main(String[] args) {
        System.out.println("Compute Server TCP");
        System.out.println("Port : " + PORT);
        System.out.println("Waiting for clients...");

        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            while (true) {
                Socket clientSocket = serverSocket.accept();
                System.out.println("Nouveau client connecté : " + clientSocket.getInetAddress());
                
                threadPool.execute(new ClientHandler(clientSocket));
            }
        } catch (IOException e) {
            System.err.println("Erreur serveur : " + e.getMessage());
        }
    }

    static class ClientHandler implements Runnable {
        private Socket clientSocket;

        public ClientHandler(Socket socket) {
            this.clientSocket = socket;
        }

        @Override
        public void run() {
            try (
                BufferedReader in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true)
            ) {
                String line;
                double num1 = 0, num2 = 0;
                String operator = "";
                int step = 0;

                while ((line = in.readLine()) != null) {
                    line = line.trim();
                    
                    if (line.startsWith("NUMBER:")) {
                        try {
                            double value = Double.parseDouble(line.substring(7));
                            if (step == 0) {
                                num1 = value;
                                step = 1;
                            } else if (step == 1) {
                                num2 = value;
                                step = 2;
                            }
                        } catch (NumberFormatException e) {
                            sendError(out, "Format numérique invalide: " + line);
                            log("ERREUR", "Format numérique invalide: " + line, clientSocket);
                            break;
                        }
                    } else if (line.startsWith("OPERATOR:")) {
                        if (step == 2) {
                            operator = line.substring(9).trim();
                            if (!operator.matches("[+\\-*/]")) {
                                sendError(out, "Opérateur invalide: " + operator);
                                log("ERREUR", "Opérateur invalide: " + operator, clientSocket);
                                break;
                            }
                            
                            String result = calculate(num1, num2, operator);
                            out.println("RESULT:" + result);
                            log("CALCUL", num1 + " " + operator + " " + num2 + " = " + result, clientSocket);
                            
                            step = 0;
                        } else {
                            sendError(out, "Séquence invalide : attendu NUMBER avant OPERATOR");
                            log("ERREUR", "Séquence invalide", clientSocket);
                            break;
                        }
                    } else {
                        sendError(out, "Commande non reconnue: " + line);
                        log("ERREUR", "Commande non reconnue: " + line, clientSocket);
                        break;
                    }
                }
            } catch (IOException e) {
                System.err.println("Erreur avec client " + clientSocket.getInetAddress() + ": " + e.getMessage());
            } finally {
                try {
                    clientSocket.close();
                } catch (IOException e) {
                    System.err.println("Erreur fermeture socket: " + e.getMessage());
                }
            }
        }

        private String calculate(double a, double b, String op) {
            switch (op) {
                case "+": return String.valueOf(a + b);
                case "-": return String.valueOf(a - b);
                case "*": return String.valueOf(a * b);
                case "/": 
                    if (b == 0) return "ERREUR: division by zero";
                    return String.valueOf(a / b);
                default: return "ERREUR: Opérateur inconnu";
            }
        }

        private void sendError(PrintWriter out, String message) {
            out.println("ERROR:" + message);
        }

        private void log(String type, String message, Socket client) {
            String timestamp = sdf.format(new Date());
            String logEntry = String.format("[%s] [%s] [Client: %s] %s", 
                timestamp, type, client.getInetAddress(), message);
            
            try (FileWriter fw = new FileWriter(LOG_FILE, true);
                 BufferedWriter bw = new BufferedWriter(fw);
                 PrintWriter logWriter = new PrintWriter(bw)) {
                logWriter.println(logEntry);
                System.out.println(logEntry);
            } catch (IOException e) {
                System.err.println("Erreur écriture log: " + e.getMessage());
            }
        }
    }
}