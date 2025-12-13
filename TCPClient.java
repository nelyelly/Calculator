import java.io.*;
import java.net.*;
import java.util.Scanner;

public class TCPClient {
    private static final String SERVER_ADDRESS = "localhost";
    private static final int SERVER_PORT = 12345;

    public static void main(String[] args) {
        System.out.println("Compute Client TCP");
        System.out.println("Connection to server " + SERVER_ADDRESS + ":" + SERVER_PORT);
        
        try (
            Socket socket = new Socket(SERVER_ADDRESS, SERVER_PORT);
            PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
            BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            Scanner scanner = new Scanner(System.in)
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
                    System.out.println("Server disconected.");
                    break;
                }
            }
            
            System.out.println("Disconected from server.");
            
        } catch (UnknownHostException e) {
            System.err.println("Serveur inconnu: " + e.getMessage());
        } catch (ConnectException e) {
            System.err.println("Impossible connection" + e.getMessage());
        } catch (IOException e) {
            System.err.println("ERROR I/O: " + e.getMessage());
        }
    }
}