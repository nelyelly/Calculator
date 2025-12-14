import java.io.*;
import java.net.*;
import java.util.Scanner;

public class TCPClient {
    private static final int SERVER_PORT = 12345;

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        
        System.out.println("Compute Client TCP");
        System.out.print("Enter server IP (or 'localhost' for local): ");
        String serverAddress = scanner.nextLine();
        
        System.out.println("Connection to server " + serverAddress + ":" + SERVER_PORT);
        
        try (
            Socket socket = new Socket(serverAddress, SERVER_PORT);
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))
        ) {
            System.out.println("Connected to server");
            
            while (true) {
                System.out.print("\nNUMBER: ");
                String input1 = scanner.nextLine();
                if (input1.equalsIgnoreCase("quit")) break;
                
                System.out.print("NUMBER: ");
                String input2 = scanner.nextLine();
                
                System.out.print("OPERATOR: ");
                String op = scanner.nextLine().trim();
                
                out.println("NUMBER:" + input1);
                out.println("NUMBER:" + input2);
                out.println("OPERATOR:" + op);
                
                String response = in.readLine();
                if (response != null) {
                    if (response.startsWith("RESULT:")) {
                        System.out.println("RESULT: " + response.substring(7));
                    } else if (response.startsWith("ERROR:")) {
                        System.out.println("ERROR: " + response.substring(6));
                    } else {
                        System.out.println("Réponse inattendue: " + response);
                    }
                } else {
                    System.out.println("Server disconnected.");
                    break;
                }
            }
            
            System.out.println("Disconnected from server.");
            
        } catch (UnknownHostException e) {
            System.err.println("Serveur inconnu: " + e.getMessage());
        } catch (ConnectException e) {
            System.err.println("Impossible de se connecter: " + e.getMessage());
            System.err.println("Vérifiez que:");
            System.err.println("1. L'adresse IP est correcte");
            System.err.println("2. Le serveur est lancé");
            System.err.println("3. Le port " + SERVER_PORT + " est ouvert");
        } catch (IOException e) {
            System.err.println("ERROR I/O: " + e.getMessage());
        } finally {
            scanner.close();
        }
    }
}