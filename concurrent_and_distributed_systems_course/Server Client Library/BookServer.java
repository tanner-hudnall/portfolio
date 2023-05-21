import java.net.*;
import java.io.*;
import java.util.*;

public class BookServer {
    private static HashMap<String, Integer> bookInventory = new HashMap<>(); // stores book inventory by book name
    private static HashMap<Integer, String> loans = new HashMap<>();
    private static HashMap<String, ArrayList<Integer>> loanList = new HashMap<>();
    private static int loanId = 1;
    public static void main(String args[]){

        int tcpPort;
        int udpPort;
        if (args.length != 1) {
            System.out.println("ERROR: Provide 1 argument: input file containing initial inventory");
            System.exit(-1);
        }
        String fileName = args[0];
        tcpPort = 7000;
        udpPort = 8000;

        // parse the inventory file
        String inputFile = args[0];
        File file = new File(inputFile);
        try (Scanner scanner = new Scanner(file)) {
            while (scanner.hasNextLine()) {
                String line = scanner.nextLine();
                String[] parts = line.split("\"\\s+");
                String bookTitle = parts[0].replaceAll("\"", "");
                int inventoryCount = Integer.parseInt(parts[1]);

                bookInventory.put(bookTitle, inventoryCount);
            }
        } catch (FileNotFoundException e) {
            System.out.println("File not found: " + inputFile);
            return;
        }
        try {
            ServerSocket tcpSocket = new ServerSocket(tcpPort);
            DatagramSocket udpSocket = new DatagramSocket(udpPort);

            while (true) {
                // accept client connection
                System.out.println("Listening on port 7000 for tcp, 8000 for udp");
                Socket tcpClientSocket = tcpSocket.accept();
                InetAddress clientAddress = InetAddress.getByName("localhost");
                int clientPort = udpPort;

                // handle client request on a new thread
                Thread clientThread = new Thread(new ClientHandler(tcpClientSocket, clientAddress, clientPort));
                clientThread.start();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static class ClientHandler implements Runnable {
        private Socket tcpClientSocket;
        private InetAddress clientAddress;
        private int clientPort;
    
        public ClientHandler(Socket tcpClientSocket, InetAddress clientAddress, int clientPort) {
            this.tcpClientSocket = tcpClientSocket;
            this.clientAddress = clientAddress;
            this.clientPort = clientPort;
        }
    
        public void run() {
            try {
                BufferedReader inFromClient = new BufferedReader(new InputStreamReader(tcpClientSocket.getInputStream()));
                BufferedWriter outToClient = new BufferedWriter(new OutputStreamWriter(tcpClientSocket.getOutputStream()));
            
                String inputLine;
                while ((inputLine = inFromClient.readLine()) != null) {
                    if(inputLine.charAt(0) == 'b'){
                        beginLoan(inputLine, outToClient);
                    }
                    else if(inputLine.charAt(1) == 'x'){
                        break;
                    }
                    else if(inputLine.charAt(0) == 'e'){
                        endLoan(inputLine, outToClient);
                    }
                    else if(inputLine.charAt(0) == 's'){
                        setMode(inputLine, outToClient);
                    }
                    else if(inputLine.charAt(0) == 'g' && inputLine.charAt(4) == 'i'){
                        getInventory(inputLine, outToClient);
                    }
                    else{
                        getLoans(inputLine, outToClient);
                    }
                    outToClient.flush();
                }
                
                System.out.println("Client disconnected");
                tcpClientSocket.close();
            } catch (IOException e) {
                e.printStackTrace();
                }
            }
        }
    
    public static void getLoans(String cmd, BufferedWriter outToClient) throws IOException{
        outToClient.write("Server confirmed: " + cmd + "\n");
        String res = "";
        String[] parts = cmd.split(" ");
        // if(loanList.get(parts[1]) != null){
        //     ArrayList<Integer> finder = loanList.get(parts[1]);
        //     for(Integer i : finder){
        //         res = res 
        //     }
        // }
        // else{
        //     outToClient.write("No record found for "+ parts[1]+"\n");        
        // }
        
        System.out.println("Get Loans path");
    }
    public static void endLoan(String cmd, BufferedWriter outToClient) throws IOException{
        String[] parts = cmd.split(" ");
        System.out.println(loans.get(1));
        if(loans.get(parts[1]) == null){
            outToClient.write(parts[1]+ " not found, no such borrow record\n");
        }
        else{
            outToClient.write(parts[1]+" is returned");
            bookInventory.put(loans.get(parts[1]), bookInventory.get(loanList.get(parts[1])) + 1);
        }
        System.out.println("End Loan path");
    }
    public static void getInventory(String cmd, BufferedWriter outToClient) throws IOException{ //DONE
        //outToClient.write("Server confirmed: " + cmd + "\n");
        String res = "";
        for(String s : bookInventory.keySet()){
            res = res + "\""+s +"\" "+ bookInventory.get(s)+" ";
        }
        res = res + "\n";
        outToClient.write(res);
        System.out.println("Get Inventory path");
    }
    public static void setMode(String cmd, BufferedWriter outToClient) throws IOException{ //DONE
        if(cmd.charAt(9) == 't'){
            outToClient.write("The communication mode is set to TCP \n");
        }
        else{
            outToClient.write("The communication mode is set to UDP \n");
        }

        System.out.println("Set Mode path");
    }
    public static void beginLoan(String cmd, BufferedWriter outToClient) throws IOException{ // DONE
        
        String[] parts = cmd.split("\\s+");
        String command = parts[0];
        String user = parts[1];
        String book = cmd.substring(cmd.indexOf("\"") + 1, cmd.lastIndexOf("\""));
        if(bookInventory.get(book) == null){
            outToClient.write("Request Failed - We do not have this book\n");
        }
        else if(bookInventory.get(book) == 0){
            outToClient.write("Request Failed - Book not available\n");
        }
        else if(bookInventory.get(book) > 0){
            outToClient.write("Your request has been approved, "+ loanId+" "+user+" "+ book + "\n");
            bookInventory.put(book, bookInventory.get(book) - 1);
            loans.put(loanId, book);
            addLoan(user, loanId);
            loanId++;
        }

        System.out.println("Begin Loan path");
    }

    public static void addLoan(String user, int loanId){
        if(loanList.get(user) == null){
            ArrayList<Integer> list = new ArrayList<>();
            list.add(loanId);
            loanList.put(user, list);
        }
        else{
            ArrayList<Integer> list = loanList.get(user);
            list.add(loanId);
            loanList.put(user, list);
        }
    }

}
    